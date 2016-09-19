#ifndef _SMLAR_H_
#define	_SMLAR_H_

#include "postgres.h"
#include "utils/array.h"
#include "access/tupdesc.h"
#include "catalog/pg_collation.h"

typedef	struct ProcTypeInfoData	*ProcTypeInfo;

typedef struct ProcTypeInfoData {
	Oid				typid;
	Oid				hashFuncOid;
	Oid				cmpFuncOid;
	int16			typlen;
	bool			typbyval;
	char			typalign;

	/* support of composite type */
	char			typtype;
	TupleDesc		tupDesc;

	/*
	 * Following member can become invalid,
	 * so fill it just before using
	 */
	bool			hashFuncInited;
	FmgrInfo		hashFunc;
	bool			cmpFuncInited;
	FmgrInfo		cmpFunc;
} ProcTypeInfoData;

ProcTypeInfo findProcs(Oid typid);
void getFmgrInfoHash(ProcTypeInfo info);
void getFmgrInfoCmp(ProcTypeInfo info);

#define NDIM 1
/* reject arrays we can't handle; but allow a NULL or empty array */
#define CHECKARRVALID(x) \
	do { \
		if (x) { \
			if (ARR_NDIM(x) != NDIM && ARR_NDIM(x) != 0) \
				ereport(ERROR, \
						(errcode(ERRCODE_ARRAY_SUBSCRIPT_ERROR), \
						 errmsg("array must be one-dimensional"))); \
			if (ARR_HASNULL(x)) \
				ereport(ERROR, \
						(errcode(ERRCODE_NULL_VALUE_NOT_ALLOWED), \
						 errmsg("array must not contain nulls"))); \
		} \
	} while(0)

#define ARRISVOID(x)  ((x) == NULL || ARRNELEMS(x) == 0)
#define ARRNELEMS(x)  ArrayGetNItems(ARR_NDIM(x), ARR_DIMS(x))


typedef struct SimpleArray {
	Datum		   *elems;
	double		   *df;  /* frequency in current doc */
	uint32		   *hash;
	int				nelems;
	ProcTypeInfo	info;
} SimpleArray;

SimpleArray	* Array2SimpleArray(ProcTypeInfo info, ArrayType *a);
SimpleArray	* Array2SimpleArrayS(ProcTypeInfo info, ArrayType *a);
SimpleArray	* Array2SimpleArrayU(ProcTypeInfo info, ArrayType *a, void *cache);
void allocateHash(void *cache, SimpleArray *a);

/*
 * GUC vars
 */
double GetSmlarLimit(void);
const char* GetSmlarTable(void);
bool GetSmlarUsePersistent(void);
double getOneAdd(void);
int getTFMethod(void);
int getSmlType(void); 
/*
 * GiST
 */

#define SmlarOverlapStrategy		1
#define SmlarSimilarityStrategy		2

struct SmlSign;
struct SmlSign* Array2HashedArray(ProcTypeInfo info, ArrayType *a);
/*
 * Cache subsystem
 */
void*	SearchArrayCache( void *cache, MemoryContext ctx, Datum a, ArrayType **da, SimpleArray **sa,  struct SmlSign  **ss );

typedef struct StatElem {
	Datum		datum;
	double		idf; /*  log(d/df) */
} StatElem;

typedef struct HashedElem {
	uint32		hash;
	double		idfMin;
	double		idfMax;
} HashedElem;

typedef struct SignedElem {
	double		idfMin;
	double		idfMax;
} SignedElem;

typedef struct StatCache {
	StatElem		*elems;
	int				nelems;
	int64_t			ndoc;
	HashedElem		*helems;
	int				nhelems;
	SignedElem		*selems;
	ProcTypeInfo	info;
} StatCache;

StatCache *initStatCache(MemoryContext ctx);
void getHashStatCache(StatCache *stat, MemoryContext ctx, size_t n);

void	resetStatCache(void);
StatElem  *findStat(StatCache *stat, Datum query, StatElem *low);
StatElem  *fingArrayStat(void *cache, Oid typoid, Datum query, StatElem *low);
StatCache *getStat(void *cache, size_t n);

/*
 * Formula's type of similarity
 */
#define ST_COSINE	1
#define ST_TFIDF	2
#define ST_OVERLAP	3
/*
 * TF methods
 */
#define TF_N		1
#define TF_LOG		2
#define	TF_CONST	3

#define FCall2(f, x1, x2)   FunctionCall2Coll((f), C_COLLATION_OID, (x1), (x2))

#endif
