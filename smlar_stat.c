#include "smlar.h"

#include "fmgr.h"
#include "catalog/pg_type.h"
#include "executor/spi.h"
#include "utils/array.h"
#include "utils/datum.h"
#include "utils/memutils.h"

static StatCache *PersistentDocStat = NULL;

static void*
cacheAlloc(MemoryContext ctx, size_t size)
{
	if ( GetSmlarUsePersistent() )
	{
		void *ptr = malloc(size);

		if (!ptr)
			ereport(ERROR,
					(errcode(ERRCODE_OUT_OF_MEMORY),
					errmsg("out of memory")));

		return ptr;
	}

	return	MemoryContextAlloc(ctx, size);
}

static void*
cacheAllocZero(MemoryContext ctx, size_t size)
{
	void *ptr = cacheAlloc(ctx, size);
	memset(ptr, 0, size);
	return ptr;
}

struct StatCache *
initStatCache(MemoryContext ctx)
{
	if (PersistentDocStat && GetSmlarUsePersistent())
		return PersistentDocStat;
	else {
		int			stat;
		char		buf[1024];
		const char	*tbl = GetSmlarTable();
		StatCache	*cache = NULL;

		if ( tbl == NULL || *tbl == '\0' )
			elog(ERROR,"smlar.stattable is not defined");

		sprintf(buf,"SELECT * FROM \"%s\" ORDER BY 1;", tbl);
		SPI_connect();
		stat = SPI_execute(buf, true, 0);

		if (stat != SPI_OK_SELECT)
			elog(ERROR, "SPI_execute() returns %d", stat);

		if ( SPI_processed == 0 )
		{
			elog(ERROR, "Stat table '%s' is empty", tbl);
		}
		else
		{
			int		i;
			double	totaldocs = 0.0;
			Oid		ndocType = SPI_gettypeid(SPI_tuptable->tupdesc, 2);

			if ( SPI_tuptable->tupdesc->natts != 2 )
				elog(ERROR,"Stat table is not (type, int4)");
			if ( !(ndocType == INT4OID || ndocType == INT8OID) )
				elog(ERROR,"Stat table is not (type, int4) nor (type, int8)");

			cache = cacheAllocZero(ctx, sizeof(StatCache));
			cache->info = findProcs( SPI_gettypeid(SPI_tuptable->tupdesc, 1) );
			if (cache->info->tupDesc)
				elog(ERROR, "TF/IDF is not supported for composite (weighted) type");
			getFmgrInfoCmp(cache->info);
			cache->elems = cacheAlloc(ctx, sizeof(StatElem) * SPI_processed);

			for(i=0; i<SPI_processed; i++)
			{
				bool	isnullvalue, isnullndoc;
				Datum	datum = SPI_getbinval(SPI_tuptable->vals[i], SPI_tuptable->tupdesc, 1, &isnullvalue);
				int64	ndoc;

				if (ndocType == INT4OID)
					ndoc = DatumGetInt32(SPI_getbinval(SPI_tuptable->vals[i], SPI_tuptable->tupdesc, 2, &isnullndoc));
				else
					ndoc = DatumGetInt64(SPI_getbinval(SPI_tuptable->vals[i], SPI_tuptable->tupdesc, 2, &isnullndoc));

				if (isnullndoc)
					elog(ERROR,"NULL value in second column of table '%s'", tbl);

				if (isnullvalue)
				{
					/* total number of docs */

					if (ndoc <= 0)
						elog(ERROR,"Total number of document should be positive");
					if ( totaldocs > 0 )
						elog(ERROR,"Total number of document is repeated");
					totaldocs = ndoc;
				}
				else
				{
					if ( i>0 && DatumGetInt32( FCall2( &cache->info->cmpFunc, cache->elems[i-1].datum, datum ) ) == 0 )
						elog(ERROR,"Values of first column of table '%s' are not unique", tbl);

					if (ndoc <= 0)
						elog(ERROR,"Number of documents with current value should be positive");

					if ( cache->info->typbyval )
						cache->elems[i].datum = datum;
					else
					{
						size_t	size = datumGetSize(datum, false, cache->info->typlen);

						cache->elems[i].datum = PointerGetDatum(cacheAlloc(ctx, size));
						memcpy(DatumGetPointer(cache->elems[i].datum), DatumGetPointer(datum), size);
					}

					cache->elems[i].idf = ndoc;
				}
			}

			if ( totaldocs <= 0)
				elog(ERROR,"Total number of document is unknown");
			cache->nelems = SPI_processed - 1;

			for(i=0;i<cache->nelems;i++)
			{
				if ( totaldocs < cache->elems[i].idf )
					elog(ERROR,"Inconsitent data in '%s': there is values with frequency > 1", tbl);
				cache->elems[i].idf = log( totaldocs / cache->elems[i].idf + getOneAdd() );
			}
		}

		SPI_finish();

		if ( GetSmlarUsePersistent() )
			PersistentDocStat = cache;

		return cache;
	}
}

void
resetStatCache(void)
{
	if ( PersistentDocStat )
	{

		if (!PersistentDocStat->info->typbyval)
		{
			int i;
			for(i=0;i<PersistentDocStat->nelems;i++)
				free( DatumGetPointer(PersistentDocStat->elems[i].datum) );
		}

		if (PersistentDocStat->helems)
			free(PersistentDocStat->helems);
		free(PersistentDocStat->elems);
		free(PersistentDocStat);
	}

	PersistentDocStat = NULL;
}

StatElem  *
findStat(StatCache *stat, Datum query, StatElem *low)
{
	StatElem	*StopLow = (low) ? low : stat->elems,
				*StopHigh = stat->elems + stat->nelems,
				*StopMiddle;
	int			cmp;

	if (stat->info->tupDesc)
		elog(ERROR, "TF/IDF is not supported for composite (weighted) type");

	while (StopLow < StopHigh)
	{
		StopMiddle = StopLow + ((StopHigh - StopLow) >> 1);
		cmp = DatumGetInt32( FCall2( &stat->info->cmpFunc, StopMiddle->datum, query ) );

		if ( cmp == 0 )
			return StopMiddle;
		else if (cmp < 0)
			StopLow = StopMiddle + 1;
		else
			StopHigh = StopMiddle;
	}

	return NULL;
}

void
getHashStatCache(StatCache *stat, MemoryContext ctx, size_t n)
{
	if ( !stat->helems )
	{
		stat->helems = cacheAlloc(ctx, (stat->nelems +1) * sizeof(HashedElem));
		stat->selems = cacheAllocZero(ctx, n * sizeof(SignedElem));
		stat->nhelems = -1;
	}
}
