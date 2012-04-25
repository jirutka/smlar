#include "smlar.h"

#include "fmgr.h"
#include "utils/array.h"
#include "utils/memutils.h"

/*
 * Deconstructed array cache
 */

typedef struct ArrayCacheEntry {
	Datum			toastedArray;
	ArrayType		*da;
	SimpleArray		*sa;
	struct SmlSign	*ss;

	struct ArrayCacheEntry	*prev;
	struct ArrayCacheEntry  *next;
} ArrayCacheEntry;

#define NENTRIES	(16)

typedef struct ArrayCache {
	MemoryContext		ctx;
	int32				nentries;
	ArrayCacheEntry		*head;
	ArrayCacheEntry		*tail;
	ArrayCacheEntry*	entries[ NENTRIES ];
	StatCache			*DocStat;
} ArrayCache;

static void 
moveFirst(ArrayCache *ac, ArrayCacheEntry *entry)
{
	/*
	 * delete entry form a list
	 */
	Assert( entry != ac->head );
	if ( entry == ac->tail )
	{
		Assert( entry->next == NULL );
		ac->tail = entry->prev;
		if ( ac->tail )
			ac->tail->next = NULL;
		else
			ac->head = NULL;
	}
	else
	{
		entry->prev->next = entry->next;
		entry->next->prev = entry->prev;
	}

	/*
	 * Install into head 
	 */

	Assert( ac->head != NULL );
	Assert( ac->tail != NULL );

	entry->next = ac->head;
	entry->prev = NULL;
	ac->head->prev = entry;
	ac->head = entry;
}

#define	DATUMSIZE(d)	VARSIZE_ANY(DatumGetPointer(d))	
static int
cmpDatum(Datum a, Datum b)
{
	int32	la = DATUMSIZE(a);
	int32	lb = DATUMSIZE(b);

	if ( la == lb )
		return memcmp( DatumGetPointer(a), DatumGetPointer(b), la );

	return (la > lb) ? 1 : -1;
}

static void
fetchData(ArrayCache *ac, ArrayCacheEntry *entry, ArrayType **da, SimpleArray **sa,  struct SmlSign  **ss )
{
	ProcTypeInfo	info;

	info = findProcs( ARR_ELEMTYPE(entry->da) );

	if ( sa )
	{
		if ( entry->sa == NULL )
		{
			MemoryContext	old;

			getFmgrInfoCmp(info);

			old = MemoryContextSwitchTo( ac->ctx );
			entry->sa = Array2SimpleArrayU(info, entry->da, (getSmlType() == ST_TFIDF) ? ac : NULL);
			MemoryContextSwitchTo(old);
		}

		*sa = entry->sa;
	}

	if ( ss )
	{
		if ( entry->ss == NULL )
		{
			MemoryContext   old;

			getFmgrInfoHash(info);

			old = MemoryContextSwitchTo( ac->ctx );
			entry->ss = Array2HashedArray(info, entry->da);
			MemoryContextSwitchTo(old);
		}

		*ss = entry->ss;
	}

	if (da)
		*da = entry->da;
}

static void
cleanupData(ArrayCacheEntry *entry)
{
	if ( entry->sa )
	{
		if ( entry->sa->elems )
			pfree( entry->sa->elems );
		if ( entry->sa->df )
			pfree( entry->sa->df );
		if ( entry->sa->hash )
			pfree( entry->sa->hash );
		pfree( entry->sa );
	}
	entry->sa = NULL;

	if ( entry->ss )
		pfree(entry->ss);
	entry->ss = NULL;

	pfree( DatumGetPointer(entry->toastedArray) );
	pfree( entry->da );
}

static void
makeEntry(ArrayCache *ac, ArrayCacheEntry *entry, Datum a)
{
	ArrayType *detoastedArray;

	entry->toastedArray = (Datum)MemoryContextAlloc( ac->ctx, DATUMSIZE(a) );
	memcpy( DatumGetPointer(entry->toastedArray), DatumGetPointer(a), DATUMSIZE(a) );

	detoastedArray = (ArrayType*)PG_DETOAST_DATUM(entry->toastedArray);
	entry->da = MemoryContextAlloc( ac->ctx, VARSIZE(detoastedArray));
	memcpy( entry->da, detoastedArray, VARSIZE(detoastedArray));
}

static int
cmpEntry(const void *a, const void *b)
{
	return cmpDatum( (*(ArrayCacheEntry**)a)->toastedArray, (*(ArrayCacheEntry**)b)->toastedArray ); 
}

void*
SearchArrayCache( void *cache, MemoryContext ctx, Datum a, ArrayType **da, SimpleArray **sa,  struct SmlSign  **ss )
{
	ArrayCache		*ac;
	ArrayCacheEntry	*entry;

	if ( cache == NULL )
		cache = MemoryContextAllocZero(ctx, sizeof(ArrayCache));

	ac = (ArrayCache*) cache;
	ac->ctx = ctx;

	/*
	 * Fast check of resent used value 
	 */
	if ( ac->head && cmpDatum(ac->head->toastedArray, a) == 0 )
	{
		fetchData(ac, ac->head, da, sa, ss);
		return cache;
	}

	if ( ac->head == NULL  )
	{
		ac->entries[0] = ac->head = ac->tail = MemoryContextAllocZero(ctx, sizeof(ArrayCacheEntry));
		ac->nentries = 1;
		makeEntry(ac, ac->head, a);
		fetchData(ac, ac->head, da, sa, ss);
		return cache;
	}

	do {
		ArrayCacheEntry	**StopLow = ac->entries;
		ArrayCacheEntry	**StopHigh = ac->entries + ac->nentries;
		ArrayCacheEntry	**StopMiddle;
		int cmp;

        while (StopLow < StopHigh) {
			StopMiddle = StopLow + ((StopHigh - StopLow) >> 1);
			entry = *StopMiddle;
			cmp = cmpDatum(entry->toastedArray, a); 

			if ( cmp == 0 )
			{
				moveFirst(ac, entry);
				fetchData(ac, ac->head, da, sa, ss);
				return cache;
			}
			else if ( cmp < 0 )
				StopLow = StopMiddle + 1;
			else
				StopHigh = StopMiddle;
		}
	} while(0);

	/*
	 * Not found 
	 */

	if ( ac->nentries < NENTRIES )
	{
		entry = ac->entries[ ac->nentries ] = MemoryContextAllocZero(ctx, sizeof(ArrayCacheEntry));

		/* install first */
		entry->next = ac->head;
		entry->prev = NULL;
		ac->head->prev = entry;
		ac->head = entry;

		ac->nentries ++;

		makeEntry(ac, ac->head, a);
		fetchData(ac, ac->head, da, sa, ss);
	} 
	else
	{
		cleanupData( ac->tail );
		moveFirst(ac, ac->tail );
		makeEntry(ac, ac->head, a);
		fetchData(ac, ac->head, da, sa, ss);
	}

	qsort(ac->entries, ac->nentries, sizeof(ArrayCacheEntry*), cmpEntry);	
	return cache;
}

StatElem  *
fingArrayStat(void *cache, Oid typoid, Datum query, StatElem *low)
{
	ArrayCache		*ac;

	if ( cache == NULL )
		return NULL;

	ac = (ArrayCache*) cache;
	if ( ac->DocStat == NULL )
	{
		ac->DocStat = initStatCache(ac->ctx);
		low = NULL;
	}

	if ( typoid != ac->DocStat->info->typid )
		elog(ERROR,"Types of stat table and actual arguments are different");

	return findStat(ac->DocStat, query, low);	
}

StatCache *
getStat(void *cache, size_t n)
{
	ArrayCache      *ac;

	if ( cache == NULL )
		return NULL;

	ac = (ArrayCache*) cache;
	if ( ac->DocStat == NULL )
		ac->DocStat = initStatCache(ac->ctx);

	getHashStatCache(ac->DocStat, ac->ctx, n);

	return ac->DocStat;
}

void
allocateHash(void *cache, SimpleArray *a)
{
	ArrayCache	*ac;

	if ( cache == NULL )
		return;
	ac = (ArrayCache*) cache;

	if (a->hash)
		return;

	a->hash = MemoryContextAlloc( ac->ctx, sizeof(uint32) * a->nelems );
}
