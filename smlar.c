#include "smlar.h"

#include "fmgr.h"
#include "access/genam.h"
#include "access/heapam.h"
#include "access/htup_details.h"
#include "access/nbtree.h"
#include "catalog/indexing.h"
#include "catalog/pg_am.h"
#include "catalog/pg_amproc.h"
#include "catalog/pg_cast.h"
#include "catalog/pg_opclass.h"
#include "catalog/pg_type.h"
#include "executor/spi.h"
#include "utils/catcache.h"
#include "utils/fmgroids.h"
#include "utils/lsyscache.h"
#include "utils/memutils.h"
#include "utils/tqual.h"
#include "utils/syscache.h"
#include "utils/typcache.h"

PG_MODULE_MAGIC;

static Oid
getDefaultOpclass(Oid amoid, Oid typid) 
{
	ScanKeyData	skey;
	SysScanDesc	scan;
	HeapTuple	tuple;
	Relation	heapRel;
	Oid			opclassOid = InvalidOid;

	heapRel = heap_open(OperatorClassRelationId, AccessShareLock);	

	ScanKeyInit(&skey,
				Anum_pg_opclass_opcmethod,
				BTEqualStrategyNumber,	F_OIDEQ,
				ObjectIdGetDatum(amoid));

	scan = systable_beginscan(heapRel, 
								OpclassAmNameNspIndexId, true, 
								SnapshotNow, 1, &skey);

	while (HeapTupleIsValid((tuple = systable_getnext(scan))))
	{
		Form_pg_opclass opclass = (Form_pg_opclass)GETSTRUCT(tuple);

		if ( opclass->opcintype == typid && opclass->opcdefault )
		{
			if ( OidIsValid(opclassOid) )
				elog(ERROR, "Ambiguous opclass for type %u (access method %u)", typid, amoid); 
			opclassOid = HeapTupleGetOid(tuple);
		}
	}

	systable_endscan(scan);
	heap_close(heapRel, AccessShareLock);

	return opclassOid;
}

static Oid
getAMProc(Oid amoid, Oid typid) 
{
	Oid		opclassOid = getDefaultOpclass(amoid, typid);
	Oid		procOid = InvalidOid;
	Oid		opfamilyOid;
	ScanKeyData	skey[4];
	SysScanDesc	scan;
	HeapTuple	tuple;
	Relation	heapRel;

	if ( !OidIsValid(opclassOid) )
	{
		typid = getBaseType(typid);
		opclassOid = getDefaultOpclass(amoid, typid);
	}

	if ( !OidIsValid(opclassOid) )
	{
		CatCList	*catlist;
		int			i;

		/*
		 * Search binary-coercible type
		 */
		catlist = SearchSysCacheList(CASTSOURCETARGET, 1,
										ObjectIdGetDatum(typid),
										0, 0, 0);

		for (i = 0; i < catlist->n_members; i++)
		{
			HeapTuple   	tuple = &catlist->members[i]->tuple;
			Form_pg_cast	castForm = (Form_pg_cast)GETSTRUCT(tuple);

			if ( castForm->castfunc == InvalidOid && castForm->castcontext == COERCION_CODE_IMPLICIT )
			{
				typid = castForm->casttarget;
				opclassOid = getDefaultOpclass(amoid, typid);
				if( OidIsValid(opclassOid) )
					break;
			}
		}

		ReleaseSysCacheList(catlist);
	}
	
	if ( !OidIsValid(opclassOid) )
		return InvalidOid;

	opfamilyOid = get_opclass_family(opclassOid);

	heapRel = heap_open(AccessMethodProcedureRelationId, AccessShareLock);
	ScanKeyInit(&skey[0],
				Anum_pg_amproc_amprocfamily,
				BTEqualStrategyNumber, F_OIDEQ,
				ObjectIdGetDatum(opfamilyOid));
	ScanKeyInit(&skey[1],
				Anum_pg_amproc_amproclefttype,
				BTEqualStrategyNumber, F_OIDEQ,
				ObjectIdGetDatum(typid));
	ScanKeyInit(&skey[2],
				Anum_pg_amproc_amprocrighttype,
				BTEqualStrategyNumber, F_OIDEQ,
				ObjectIdGetDatum(typid));
#if PG_VERSION_NUM >= 90200
	ScanKeyInit(&skey[3],
				Anum_pg_amproc_amprocnum,
				BTEqualStrategyNumber, F_OIDEQ,
				Int32GetDatum(BTORDER_PROC));
#endif

	scan = systable_beginscan(heapRel, AccessMethodProcedureIndexId, true,
								SnapshotNow, 
#if PG_VERSION_NUM >= 90200
								4,
#else
								3,
#endif
								skey);
	while (HeapTupleIsValid(tuple = systable_getnext(scan)))
	{
		Form_pg_amproc amprocform = (Form_pg_amproc) GETSTRUCT(tuple);

		switch(amoid)
		{
			case BTREE_AM_OID:
			case HASH_AM_OID:
				if ( OidIsValid(procOid) )
					elog(ERROR,"Ambiguous support function for type %u (opclass %u)", typid, opfamilyOid);
				procOid = amprocform->amproc;
				break;
			default:
				elog(ERROR,"Unsupported access method");
		}
	}

	systable_endscan(scan);
	heap_close(heapRel, AccessShareLock);

	return procOid;
}

static ProcTypeInfo *cacheProcs = NULL;
static int nCacheProcs = 0;

static ProcTypeInfo
fillProcs(Oid typid)
{
	ProcTypeInfo	info = malloc(sizeof(ProcTypeInfoData));

	if (!info)
		elog(ERROR, "Can't allocate %u memory", (uint32)sizeof(ProcTypeInfoData));

	info->typid = typid;
	info->typtype = get_typtype(typid);

	if (info->typtype == 'c')
	{
		/* composite type */
		TupleDesc		tupdesc;
		MemoryContext	oldcontext;

		tupdesc = lookup_rowtype_tupdesc(typid, -1);

		if (tupdesc->natts != 2)
			elog(ERROR,"Composite type has wrong number of fields");
		if (tupdesc->attrs[1]->atttypid != FLOAT4OID)
			elog(ERROR,"Second field of composite type is not float4");

		oldcontext = MemoryContextSwitchTo(TopMemoryContext);
		info->tupDesc = CreateTupleDescCopyConstr(tupdesc);
		MemoryContextSwitchTo(oldcontext);

		ReleaseTupleDesc(tupdesc);

		info->cmpFuncOid = getAMProc(BTREE_AM_OID, info->tupDesc->attrs[0]->atttypid);
		info->hashFuncOid = getAMProc(HASH_AM_OID, info->tupDesc->attrs[0]->atttypid);
	} 
	else 
	{
		info->tupDesc = NULL;

		/* plain type */
		info->cmpFuncOid = getAMProc(BTREE_AM_OID, typid);
		info->hashFuncOid = getAMProc(HASH_AM_OID, typid);
	}

	get_typlenbyvalalign(typid, &info->typlen, &info->typbyval, &info->typalign);
	info->hashFuncInited = info->cmpFuncInited = false;


	return info;
}

void
getFmgrInfoCmp(ProcTypeInfo info)
{
	if ( info->cmpFuncInited == false )
	{
		if ( !OidIsValid(info->cmpFuncOid) )
			elog(ERROR, "Could not find cmp function for type %u", info->typid);

		fmgr_info_cxt( info->cmpFuncOid, &info->cmpFunc, TopMemoryContext );
		info->cmpFuncInited = true;
	}
}

void
getFmgrInfoHash(ProcTypeInfo info)
{
	if ( info->hashFuncInited == false )
	{
		if ( !OidIsValid(info->hashFuncOid) )
			elog(ERROR, "Could not find hash function for type %u", info->typid);

		fmgr_info_cxt( info->hashFuncOid, &info->hashFunc, TopMemoryContext );
		info->hashFuncInited = true;
	}
}

static int
cmpProcTypeInfo(const void *a, const void *b)
{
	ProcTypeInfo av = *(ProcTypeInfo*)a;
	ProcTypeInfo bv = *(ProcTypeInfo*)b;

	Assert( av->typid != bv->typid );

	return ( av->typid > bv->typid ) ? 1 : -1; 
}

ProcTypeInfo
findProcs(Oid typid)
{
	ProcTypeInfo	info = NULL;

	if ( nCacheProcs == 1 )
	{
		if ( cacheProcs[0]->typid == typid ) 
		{
			/*cacheProcs[0]->hashFuncInited = cacheProcs[0]->cmpFuncInited = false;*/
			return cacheProcs[0];
		}
	}
	else if ( nCacheProcs > 1 )
	{
		ProcTypeInfo	*StopMiddle;
		ProcTypeInfo	*StopLow = cacheProcs, 
						*StopHigh = cacheProcs + nCacheProcs;

		while (StopLow < StopHigh) {
			StopMiddle = StopLow + ((StopHigh - StopLow) >> 1);
			info = *StopMiddle;

			if ( info->typid == typid )
			{
				/* info->hashFuncInited = info->cmpFuncInited = false; */
				return info;
			}
			else if ( info->typid < typid )
				StopLow = StopMiddle + 1;
			else
				StopHigh = StopMiddle;
		}

		/* not found */
	} 

	info = fillProcs(typid);
	if ( nCacheProcs == 0 ) 
	{
		cacheProcs = malloc(sizeof(ProcTypeInfo));

		if (!cacheProcs)
			elog(ERROR, "Can't allocate %u memory", (uint32)sizeof(ProcTypeInfo));
		else
		{
			nCacheProcs = 1;
			cacheProcs[0] = info;
		}
	}
	else
	{
		ProcTypeInfo	*cacheProcsTmp = realloc(cacheProcs, (nCacheProcs+1) * sizeof(ProcTypeInfo));

		if (!cacheProcsTmp)
			elog(ERROR, "Can't allocate %u memory", (uint32)sizeof(ProcTypeInfo) * (nCacheProcs+1));
		else
		{
			cacheProcs = cacheProcsTmp;
			cacheProcs[ nCacheProcs ] = info;
			nCacheProcs++;
			qsort(cacheProcs, nCacheProcs, sizeof(ProcTypeInfo), cmpProcTypeInfo);
		}
	}

	/* info->hashFuncInited = info->cmpFuncInited = false; */

	return info;
}

/*
 * WARNING. Array2SimpleArray* doesn't copy Datum!
 */
SimpleArray * 
Array2SimpleArray(ProcTypeInfo info, ArrayType *a)
{
	SimpleArray	*s = palloc(sizeof(SimpleArray));

	CHECKARRVALID(a);

	if ( info == NULL )
		info = findProcs(ARR_ELEMTYPE(a));

	s->info = info;
	s->df = NULL;
	s->hash = NULL;

	deconstruct_array(a, info->typid,
						info->typlen, info->typbyval, info->typalign,
						&s->elems, NULL, &s->nelems);

	return s;
}

static Datum
deconstructCompositeType(ProcTypeInfo info, Datum in, double *weight)
{
	HeapTupleHeader	rec = DatumGetHeapTupleHeader(in);
	HeapTupleData	tuple;
	Datum			values[2];
	bool			nulls[2];

	/* Build a temporary HeapTuple control structure */
	tuple.t_len = HeapTupleHeaderGetDatumLength(rec);
	ItemPointerSetInvalid(&(tuple.t_self));
	tuple.t_tableOid = InvalidOid;
	tuple.t_data = rec;

	heap_deform_tuple(&tuple, info->tupDesc, values, nulls);
	if (nulls[0] || nulls[1])
		elog(ERROR, "Both fields in composite type could not be NULL");

	if (weight)
		*weight = DatumGetFloat4(values[1]);
	return values[0];
}

static int
cmpArrayElem(const void *a, const void *b, void *arg)
{
	ProcTypeInfo	info = (ProcTypeInfo)arg;

	if (info->tupDesc) 
		/* composite type */
		return DatumGetInt32( FCall2( &info->cmpFunc,
											deconstructCompositeType(info, *(Datum*)a, NULL),
											deconstructCompositeType(info, *(Datum*)b, NULL) ) );

	return DatumGetInt32( FCall2( &info->cmpFunc,
							*(Datum*)a, *(Datum*)b ) );
}

SimpleArray *
Array2SimpleArrayS(ProcTypeInfo info, ArrayType *a)
{
	SimpleArray	*s = Array2SimpleArray(info, a);

	if ( s->nelems > 1 )
	{
		getFmgrInfoCmp(s->info);

		qsort_arg(s->elems, s->nelems, sizeof(Datum), cmpArrayElem, s->info);
	}

	return s;
}

typedef struct cmpArrayElemData {
	ProcTypeInfo	info;
	bool			hasDuplicate;

} cmpArrayElemData;

static int
cmpArrayElemArg(const void *a, const void *b, void *arg)
{
    cmpArrayElemData    *data = (cmpArrayElemData*)arg;
	int res;

	if (data->info->tupDesc)
		res =  DatumGetInt32( FCall2( &data->info->cmpFunc,
											deconstructCompositeType(data->info, *(Datum*)a, NULL),
											deconstructCompositeType(data->info, *(Datum*)b, NULL) ) );
	else
		res = DatumGetInt32( FCall2( &data->info->cmpFunc,
								*(Datum*)a, *(Datum*)b ) );

	if ( res == 0 )
		data->hasDuplicate = true;

	return res;
}

/*
 * Uniquefy array and calculate TF. Although 
 * result doesn't depend on normalization, we
 * normalize TF by length array to have possiblity
 * to limit estimation for index support.
 *
 * Cache signals of needing of TF caclulation
 */

SimpleArray *
Array2SimpleArrayU(ProcTypeInfo info, ArrayType *a, void *cache)
{
	SimpleArray	*s = Array2SimpleArray(info, a);
	StatElem	*stat = NULL;

	if ( s->nelems > 0 && cache )
	{
		s->df = palloc(sizeof(double) * s->nelems);
		s->df[0] = 1.0; /* init */
	}

	if ( s->nelems > 1 )
	{
		cmpArrayElemData	data;
		int 				i;

		getFmgrInfoCmp(s->info);
		data.info = s->info;
		data.hasDuplicate = false;

		qsort_arg(s->elems, s->nelems, sizeof(Datum), cmpArrayElemArg, &data);

		if ( data.hasDuplicate )
		{
			Datum	*tmp,
					*dr,
					*data;
			int		num = s->nelems,
					cmp;

			data = tmp = dr = s->elems;

			while (tmp - data < num)
			{
				cmp = (tmp == dr) ? 0 : cmpArrayElem(tmp, dr, s->info);
				if ( cmp != 0 )
				{
					*(++dr) = *tmp++;
					if ( cache ) 
						s->df[ dr - data ] = 1.0;
				}
				else
				{
					if ( cache )
						s->df[ dr - data ] += 1.0;
					tmp++;
				}
			}

			s->nelems = dr + 1 - s->elems;

			if ( cache )
			{
				int tfm = getTFMethod();

				for(i=0;i<s->nelems;i++)
				{
					stat = fingArrayStat(cache, s->info->typid, s->elems[i], stat);
					if ( stat )
					{
						switch(tfm)
						{
							case TF_LOG:
								s->df[i] = (1.0 + log( s->df[i] ));
							case TF_N:
								s->df[i] *= stat->idf;
								break;
							case TF_CONST:
								s->df[i] = stat->idf;
								break;
							default:
								elog(ERROR,"Unknown TF method: %d", tfm);
						}	
					}
					else
					{
						s->df[i] = 0.0; /* unknown word */
					}
				}	
			}
		}
		else if ( cache )
		{
			for(i=0;i<s->nelems;i++)
			{
				stat = fingArrayStat(cache, s->info->typid, s->elems[i], stat);
				if ( stat )
					s->df[i] = stat->idf;
				else
					s->df[i] = 0.0;
			}
		}
	}
	else if (s->nelems > 0 && cache) 
	{
		stat = fingArrayStat(cache, s->info->typid, s->elems[0], stat);
		if ( stat )
			s->df[0] = stat->idf;
		else
			s->df[0] = 0.0;
	}

	return s;
}

static int
numOfIntersect(SimpleArray *a, SimpleArray *b)
{
	int				cnt = 0,
					cmp;
	Datum			*aptr = a->elems,
					*bptr = b->elems;
	ProcTypeInfo	info = a->info;

	Assert( a->info->typid == b->info->typid );

	getFmgrInfoCmp(info);

	while( aptr - a->elems < a->nelems && bptr - b->elems < b->nelems )
	{
		cmp = cmpArrayElem(aptr, bptr, info);
		if ( cmp < 0 )
			aptr++;
		else if ( cmp > 0 )
			bptr++;
		else
		{
			cnt++;
			aptr++;
			bptr++;
		}
	}

	return cnt;
}

static double
TFIDFSml(SimpleArray *a, SimpleArray *b)
{
	int				cmp;
	Datum			*aptr = a->elems,
					*bptr = b->elems;
	ProcTypeInfo	info = a->info;
	double			res = 0.0;
	double			suma = 0.0, sumb = 0.0;

	Assert( a->info->typid == b->info->typid );
	Assert( a->df );
	Assert( b->df );

	getFmgrInfoCmp(info);

	while( aptr - a->elems < a->nelems && bptr - b->elems < b->nelems )
	{
		cmp = cmpArrayElem(aptr, bptr, info);
		if ( cmp < 0 )
		{
			suma += a->df[ aptr - a->elems ] * a->df[ aptr - a->elems ];
			aptr++;
		}
		else if ( cmp > 0 )
		{
			sumb += b->df[ bptr - b->elems ] * b->df[ bptr - b->elems ];
			bptr++;
		}
		else
		{
			res += a->df[ aptr - a->elems ] * b->df[ bptr - b->elems ];
			suma += a->df[ aptr - a->elems ] * a->df[ aptr - a->elems ];
			sumb += b->df[ bptr - b->elems ] * b->df[ bptr - b->elems ];
			aptr++;
			bptr++;
		}
	}

	/*
	 * Compute last elements
	 */
	while( aptr - a->elems < a->nelems )
	{
		suma += a->df[ aptr - a->elems ] * a->df[ aptr - a->elems ];
		aptr++;
	}

	while( bptr - b->elems < b->nelems )
	{
		sumb += b->df[ bptr - b->elems ] * b->df[ bptr - b->elems ];
		bptr++;
	}

	if ( suma > 0.0 && sumb > 0.0 )
		res = res / sqrt( suma * sumb );
	else
		res = 0.0;

	return res;
}


PG_FUNCTION_INFO_V1(arraysml);
Datum	arraysml(PG_FUNCTION_ARGS);
Datum
arraysml(PG_FUNCTION_ARGS)
{
	ArrayType		*a, *b;
	SimpleArray		*sa, *sb;
	
	fcinfo->flinfo->fn_extra = SearchArrayCache( 
							fcinfo->flinfo->fn_extra,
							fcinfo->flinfo->fn_mcxt,
							PG_GETARG_DATUM(0), &a, &sa, NULL);
	fcinfo->flinfo->fn_extra = SearchArrayCache( 
							fcinfo->flinfo->fn_extra,
							fcinfo->flinfo->fn_mcxt,
							PG_GETARG_DATUM(1), &b, &sb, NULL);

	if ( ARR_ELEMTYPE(a) != ARR_ELEMTYPE(b) )
		elog(ERROR,"Arguments array are not the same type!");

	if (ARRISVOID(a) || ARRISVOID(b))
		 PG_RETURN_FLOAT4(0.0);

	switch(getSmlType())
	{
		case	ST_TFIDF:
			PG_RETURN_FLOAT4( TFIDFSml(sa, sb) );
			break;
		case 	ST_COSINE:
			{
				int				cnt;
				double			power;

				power = ((double)(sa->nelems)) * ((double)(sb->nelems));
				cnt = numOfIntersect(sa, sb);

				PG_RETURN_FLOAT4(  ((double)cnt) / sqrt( power ) );
			}
			break;
		case 	ST_OVERLAP:
			{
				float4 res = (float4)numOfIntersect(sa, sb);

				PG_RETURN_FLOAT4(res);
			}
			break;
		default:
			elog(ERROR,"Unsupported formula type of similarity");
	}

	PG_RETURN_FLOAT4(0.0); /* keep compiler quiet */ 
}

PG_FUNCTION_INFO_V1(arraysmlw);
Datum	arraysmlw(PG_FUNCTION_ARGS);
Datum
arraysmlw(PG_FUNCTION_ARGS)
{
	ArrayType		*a, *b;
	SimpleArray		*sa, *sb;
	bool			useIntersect = PG_GETARG_BOOL(2);
	double  		numerator = 0.0;
	double  		denominatorA = 0.0,
					denominatorB = 0.0,
					tmpA, tmpB;
	int				cmp;
	ProcTypeInfo	info;
	int				ai = 0, bi = 0;

	
	fcinfo->flinfo->fn_extra = SearchArrayCache( 
							fcinfo->flinfo->fn_extra,
							fcinfo->flinfo->fn_mcxt,
							PG_GETARG_DATUM(0), &a, &sa, NULL);
	fcinfo->flinfo->fn_extra = SearchArrayCache( 
							fcinfo->flinfo->fn_extra,
							fcinfo->flinfo->fn_mcxt,
							PG_GETARG_DATUM(1), &b, &sb, NULL);

	if ( ARR_ELEMTYPE(a) != ARR_ELEMTYPE(b) )
		elog(ERROR,"Arguments array are not the same type!");

	if (ARRISVOID(a) || ARRISVOID(b))
		 PG_RETURN_FLOAT4(0.0);

	info = sa->info;
	if (info->tupDesc == NULL)
		elog(ERROR, "Only weigthed (composite) types should be used");
	getFmgrInfoCmp(info);

	while(ai < sa->nelems && bi < sb->nelems)
	{
		Datum	ad = deconstructCompositeType(info, sa->elems[ai], &tmpA),
				bd = deconstructCompositeType(info, sb->elems[bi], &tmpB);

		cmp = DatumGetInt32(FCall2(&info->cmpFunc, ad, bd));

		if ( cmp < 0 ) {
			if (useIntersect == false)
				denominatorA += tmpA * tmpA;
			ai++;
		} else if ( cmp > 0 ) {
			if (useIntersect == false)
				denominatorB += tmpB * tmpB;
			bi++;
		} else {
			denominatorA += tmpA * tmpA;
			denominatorB += tmpB * tmpB;
			numerator += tmpA * tmpB;
			ai++;
			bi++;
		}
	}

	if (useIntersect == false) {
		while(ai < sa->nelems) {
			deconstructCompositeType(info, sa->elems[ai], &tmpA);
			denominatorA += tmpA * tmpA;
			ai++;
		}

		while(bi < sb->nelems) {
			deconstructCompositeType(info, sb->elems[bi], &tmpB);
			denominatorB += tmpB * tmpB;
			bi++;
		}
	}

	if (numerator != 0.0) {
		numerator = numerator / sqrt( denominatorA * denominatorB );
	}

	PG_RETURN_FLOAT4(numerator);
}

PG_FUNCTION_INFO_V1(arraysml_op);
Datum   arraysml_op(PG_FUNCTION_ARGS);
Datum
arraysml_op(PG_FUNCTION_ARGS)
{
	ArrayType		*a, *b;
	SimpleArray		*sa, *sb;
	double			power = 0.0;
	
	fcinfo->flinfo->fn_extra = SearchArrayCache( 
							fcinfo->flinfo->fn_extra,
							fcinfo->flinfo->fn_mcxt,
							PG_GETARG_DATUM(0), &a, &sa, NULL);
	fcinfo->flinfo->fn_extra = SearchArrayCache( 
							fcinfo->flinfo->fn_extra,
							fcinfo->flinfo->fn_mcxt,
							PG_GETARG_DATUM(1), &b, &sb, NULL);

	if ( ARR_ELEMTYPE(a) != ARR_ELEMTYPE(b) )
		elog(ERROR,"Arguments array are not the same type!");

	if (ARRISVOID(a) || ARRISVOID(b))
		 PG_RETURN_BOOL(false);

	switch(getSmlType())
	{
		case	ST_TFIDF:
			power = TFIDFSml(sa, sb);
			break;
		case 	ST_COSINE:
			{
				int				cnt;

				power = sqrt( ((double)(sa->nelems)) * ((double)(sb->nelems)) );

				if (  ((double)Min(sa->nelems, sb->nelems)) / power < GetSmlarLimit()  )
					PG_RETURN_BOOL(false);

				cnt = numOfIntersect(sa, sb);
				power = ((double)cnt) / power;
			}
			break;
		case 	ST_OVERLAP:
			power = (double)numOfIntersect(sa, sb);
			break;
		default:
			elog(ERROR,"Unsupported formula type of similarity");
	}

	PG_RETURN_BOOL(power >= GetSmlarLimit());
}

#define	QBSIZE		8192
static char cachedFormula[QBSIZE];
static int	cachedLen  = 0;
static void	*cachedPlan = NULL;

PG_FUNCTION_INFO_V1(arraysml_func);
Datum   arraysml_func(PG_FUNCTION_ARGS);
Datum
arraysml_func(PG_FUNCTION_ARGS)
{
	ArrayType		*a, *b;
	SimpleArray		*sa, *sb;
	int				cnt;
	float4			result = -1.0;
	Oid				arg[] = {INT4OID, INT4OID, INT4OID};
	Datum			pars[3];
	bool			isnull;
	void			*plan;
	int				stat;
	text			*formula = PG_GETARG_TEXT_P(2); 
	
	fcinfo->flinfo->fn_extra = SearchArrayCache( 
							fcinfo->flinfo->fn_extra,
							fcinfo->flinfo->fn_mcxt,
							PG_GETARG_DATUM(0), &a, &sa, NULL);
	fcinfo->flinfo->fn_extra = SearchArrayCache( 
							fcinfo->flinfo->fn_extra,
							fcinfo->flinfo->fn_mcxt,
							PG_GETARG_DATUM(1), &b, &sb, NULL);

	if ( ARR_ELEMTYPE(a) != ARR_ELEMTYPE(b) )
		elog(ERROR,"Arguments array are not the same type!");

	if (ARRISVOID(a) || ARRISVOID(b))
		 PG_RETURN_BOOL(false);

	cnt = numOfIntersect(sa, sb);

	if ( VARSIZE(formula) - VARHDRSZ > QBSIZE - 1024 )
		elog(ERROR,"Formula is too long");

	SPI_connect();

	
	if ( cachedPlan == NULL || cachedLen != VARSIZE(formula) - VARHDRSZ ||
				memcmp( cachedFormula, VARDATA(formula), VARSIZE(formula) - VARHDRSZ ) != 0 )
	{
		char			*ptr, buf[QBSIZE];

		*cachedFormula = '\0';
		if ( cachedPlan )
			SPI_freeplan(cachedPlan);
		cachedPlan = NULL;
		cachedLen = 0;

		ptr = stpcpy( buf, "SELECT (" );
		memcpy( ptr, VARDATA(formula), VARSIZE(formula) - VARHDRSZ );
		ptr += VARSIZE(formula) - VARHDRSZ;
		ptr = stpcpy( ptr, ")::float4 FROM");
		ptr = stpcpy( ptr, " (SELECT $1 ::float8 AS i, $2 ::float8 AS a, $3 ::float8 AS b) AS N;");
		*ptr = '\0';

		plan = SPI_prepare(buf, 3, arg);
		if (!plan)
			elog(ERROR, "SPI_prepare() failed");

		cachedPlan = SPI_saveplan(plan);
		if (!cachedPlan)
			elog(ERROR, "SPI_saveplan() failed");

		SPI_freeplan(plan);
		cachedLen = VARSIZE(formula) - VARHDRSZ;
		memcpy( cachedFormula, VARDATA(formula), VARSIZE(formula) - VARHDRSZ );	
	}

	plan = cachedPlan;


	pars[0] = Int32GetDatum( cnt );
	pars[1] = Int32GetDatum( sa->nelems );
	pars[2] = Int32GetDatum( sb->nelems );

	stat = SPI_execute_plan(plan, pars, NULL, true, 3);
	if (stat < 0)
		elog(ERROR, "SPI_execute_plan() returns %d", stat);

	if ( SPI_processed > 0)
		result = DatumGetFloat4(SPI_getbinval(SPI_tuptable->vals[0], SPI_tuptable->tupdesc, 1, &isnull));

	SPI_finish();

	PG_RETURN_FLOAT4(result);
}

PG_FUNCTION_INFO_V1(array_unique);
Datum	array_unique(PG_FUNCTION_ARGS);
Datum
array_unique(PG_FUNCTION_ARGS)
{
	ArrayType		*a = PG_GETARG_ARRAYTYPE_P(0);
	ArrayType		*res;
	SimpleArray		*sa;

	sa = Array2SimpleArrayU(NULL, a, NULL);

	res = construct_array(	sa->elems, 
							sa->nelems,
							sa->info->typid,
							sa->info->typlen,
							sa->info->typbyval,
							sa->info->typalign);

	pfree(sa->elems);
	pfree(sa);
	PG_FREE_IF_COPY(a, 0);

	PG_RETURN_ARRAYTYPE_P(res);
}

PG_FUNCTION_INFO_V1(inarray);
Datum   inarray(PG_FUNCTION_ARGS);
Datum
inarray(PG_FUNCTION_ARGS)
{
	ArrayType		*a;
	SimpleArray		*sa;
	Datum			query = PG_GETARG_DATUM(1); 
	Oid				queryTypeOid;
	Datum			*StopLow,
					*StopHigh,
					*StopMiddle;
	int				cmp;

	fcinfo->flinfo->fn_extra = SearchArrayCache( 
							fcinfo->flinfo->fn_extra,
							fcinfo->flinfo->fn_mcxt,
							PG_GETARG_DATUM(0), &a, &sa, NULL);

	queryTypeOid = get_fn_expr_argtype(fcinfo->flinfo, 1);

	if ( queryTypeOid == InvalidOid )
		elog(ERROR,"inarray: could not determine actual argument type");

	if ( queryTypeOid != sa->info->typid )
		elog(ERROR,"inarray: Type of array's element and type of argument are not the same");

	getFmgrInfoCmp(sa->info);
	StopLow = sa->elems;
	StopHigh = sa->elems + sa->nelems;

	while (StopLow < StopHigh) 
	{
		StopMiddle = StopLow + ((StopHigh - StopLow) >> 1);
		cmp = cmpArrayElem(StopMiddle, &query, sa->info);

		if ( cmp == 0 )
		{
			/* found */
			if ( PG_NARGS() >= 3 )
				PG_RETURN_DATUM(PG_GETARG_DATUM(2));
			PG_RETURN_FLOAT4(1.0);
		}
		else if (cmp < 0)
			StopLow = StopMiddle + 1;
		else
			StopHigh = StopMiddle;
	}

	if ( PG_NARGS() >= 4 )
		PG_RETURN_DATUM(PG_GETARG_DATUM(3));
	PG_RETURN_FLOAT4(0.0);
}
