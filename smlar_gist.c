#include "smlar.h"

#include "fmgr.h"
#include "access/gist.h"
#include "access/skey.h"
#include "access/heaptoast.h"
#include "utils/memutils.h"

typedef struct SmlSign {
	int32	vl_len_; /* varlena header (do not touch directly!) */
	int32	flag:8,
			size:24;
	int32	maxrepeat;
	char	data[1];
} SmlSign;

#define SMLSIGNHDRSZ	(offsetof(SmlSign, data))

#define BITBYTE 8
#define SIGLENINT  61
#define SIGLEN  ( sizeof(int)*SIGLENINT )
#define SIGLENBIT (SIGLEN*BITBYTE - 1)  /* see makesign */
typedef char BITVEC[SIGLEN];
typedef char *BITVECP;
#define LOOPBYTE \
		for(i=0;i<SIGLEN;i++)

#define GETBYTE(x,i) ( *( (BITVECP)(x) + (int)( (i) / BITBYTE ) ) )
#define GETBITBYTE(x,i) ( ((char)(x)) >> i & 0x01 )
#define CLRBIT(x,i)   GETBYTE(x,i) &= ~( 0x01 << ( (i) % BITBYTE ) )
#define SETBIT(x,i)   GETBYTE(x,i) |=  ( 0x01 << ( (i) % BITBYTE ) )
#define GETBIT(x,i) ( (GETBYTE(x,i) >> ( (i) % BITBYTE )) & 0x01 )

#define HASHVAL(val) (((unsigned int)(val)) % SIGLENBIT)
#define HASH(sign, val) SETBIT((sign), HASHVAL(val))

#define ARRKEY			0x01
#define SIGNKEY			0x02
#define ALLISTRUE		0x04

#define ISARRKEY(x)		( ((SmlSign*)x)->flag & ARRKEY )
#define ISSIGNKEY(x)	( ((SmlSign*)x)->flag & SIGNKEY )
#define ISALLTRUE(x)	( ((SmlSign*)x)->flag & ALLISTRUE )

#define CALCGTSIZE(flag, len)	( SMLSIGNHDRSZ + ( ( (flag) & ARRKEY ) ? ((len)*sizeof(uint32)) : (((flag) & ALLISTRUE) ? 0 : SIGLEN) ) )
#define GETSIGN(x)				( (BITVECP)( (char*)x+SMLSIGNHDRSZ ) )
#define GETARR(x)				( (uint32*)( (char*)x+SMLSIGNHDRSZ ) )

#define GETENTRY(vec,pos) ((SmlSign *) DatumGetPointer((vec)->vector[(pos)].key))

/*
 * Fake IO
 */
PG_FUNCTION_INFO_V1(gsmlsign_in);
Datum	gsmlsign_in(PG_FUNCTION_ARGS);
Datum
gsmlsign_in(PG_FUNCTION_ARGS)
{
	elog(ERROR, "not implemented");
	PG_RETURN_DATUM(0);
}

PG_FUNCTION_INFO_V1(gsmlsign_out);
Datum	gsmlsign_out(PG_FUNCTION_ARGS);
Datum
gsmlsign_out(PG_FUNCTION_ARGS)
{
	elog(ERROR, "not implemented");
	PG_RETURN_DATUM(0);
}

/*
 * Compress/decompress
 */

/* Number of one-bits in an unsigned byte */
static const uint8 number_of_ones[256] = {
	0, 1, 1, 2, 1, 2, 2, 3, 1, 2, 2, 3, 2, 3, 3, 4,
	1, 2, 2, 3, 2, 3, 3, 4, 2, 3, 3, 4, 3, 4, 4, 5,
	1, 2, 2, 3, 2, 3, 3, 4, 2, 3, 3, 4, 3, 4, 4, 5,
	2, 3, 3, 4, 3, 4, 4, 5, 3, 4, 4, 5, 4, 5, 5, 6,
	1, 2, 2, 3, 2, 3, 3, 4, 2, 3, 3, 4, 3, 4, 4, 5,
	2, 3, 3, 4, 3, 4, 4, 5, 3, 4, 4, 5, 4, 5, 5, 6,
	2, 3, 3, 4, 3, 4, 4, 5, 3, 4, 4, 5, 4, 5, 5, 6,
	3, 4, 4, 5, 4, 5, 5, 6, 4, 5, 5, 6, 5, 6, 6, 7,
	1, 2, 2, 3, 2, 3, 3, 4, 2, 3, 3, 4, 3, 4, 4, 5,
	2, 3, 3, 4, 3, 4, 4, 5, 3, 4, 4, 5, 4, 5, 5, 6,
	2, 3, 3, 4, 3, 4, 4, 5, 3, 4, 4, 5, 4, 5, 5, 6,
	3, 4, 4, 5, 4, 5, 5, 6, 4, 5, 5, 6, 5, 6, 6, 7,
	2, 3, 3, 4, 3, 4, 4, 5, 3, 4, 4, 5, 4, 5, 5, 6,
	3, 4, 4, 5, 4, 5, 5, 6, 4, 5, 5, 6, 5, 6, 6, 7,
	3, 4, 4, 5, 4, 5, 5, 6, 4, 5, 5, 6, 5, 6, 6, 7,
	4, 5, 5, 6, 5, 6, 6, 7, 5, 6, 6, 7, 6, 7, 7, 8
};

static int
compareint(const void *va, const void *vb)
{
	uint32	a = *((uint32 *) va);
	uint32	b = *((uint32 *) vb);

	if (a == b)
		return 0;
	return (a > b) ? 1 : -1;
}

/*
 * Removes duplicates from an array of int32. 'l' is
 * size of the input array. Returns the new size of the array.
 */
static int
uniqueint(uint32 *a, int32 l, int32 *max)
{
	uint32		*ptr,
				*res;
	int32		cnt = 0;

	*max = 1;

	if (l <= 1)
		return l;

	ptr = res = a;

	qsort((void *) a, l, sizeof(uint32), compareint);

	while (ptr - a < l)
		if (*ptr != *res)
		{
			cnt = 1;
			*(++res) = *ptr++;
		}
		else
		{
			cnt++;
			if ( cnt > *max )
				*max = cnt;
			ptr++;
		}

	if ( cnt > *max )
		*max = cnt;

	return res + 1 - a;
}

SmlSign*
Array2HashedArray(ProcTypeInfo info, ArrayType *a)
{
	SimpleArray *s = Array2SimpleArray(info, a);
	SmlSign		*sign;
	int32		len, i;
	uint32		*ptr;

	len = CALCGTSIZE( ARRKEY, s->nelems );

	getFmgrInfoHash(s->info);
	if (s->info->tupDesc)
		elog(ERROR, "GiST  doesn't support composite (weighted) type");

	sign = palloc( len );
	sign->flag = ARRKEY;
	sign->size = s->nelems;

	ptr = GETARR(sign);
	for(i=0;i<s->nelems;i++)
		ptr[i] = DatumGetUInt32(FunctionCall1Coll(&s->info->hashFunc,
												  DEFAULT_COLLATION_OID,
												  s->elems[i]));

	/*
	 * there is a collision of hash-function; len is always equal or less than
	 * s->nelems
	 */
	sign->size = uniqueint( GETARR(sign), sign->size, &sign->maxrepeat );
	len = CALCGTSIZE( ARRKEY, sign->size );
	SET_VARSIZE(sign, len);

	return sign;
}

static int
HashedElemCmp(const void *va, const void *vb)
{
	uint32	a = ((HashedElem *) va)->hash;
	uint32	b = ((HashedElem *) vb)->hash;

	if (a == b)
	{
		double	ma = ((HashedElem *) va)->idfMin;
		double  mb = ((HashedElem *) va)->idfMin;

		if (ma == mb)
			return 0;

		return ( ma > mb ) ? 1 : -1;
	}

	return (a > b) ? 1 : -1;
}

static int
uniqueHashedElem(HashedElem *a, int32 l)
{
	HashedElem	*ptr,
				*res;

	if (l <= 1)
		return l;

	ptr = res = a;

	qsort(a, l, sizeof(HashedElem), HashedElemCmp);

	while (ptr - a < l)
		if (ptr->hash != res->hash)
			*(++res) = *ptr++;
		else
		{
			res->idfMax = ptr->idfMax;
			ptr++;
		}

	return res + 1 - a;
}

static StatCache*
getHashedCache(void *cache)
{
	StatCache *stat = getStat(cache, SIGLENBIT);

	if ( stat->nhelems < 0 )
	{
		int i;
		/*
		 * Init
		 */

		if (stat->info->tupDesc)
			elog(ERROR, "GiST  doesn't support composite (weighted) type");
		getFmgrInfoHash(stat->info);
		for(i=0;i<stat->nelems;i++)
		{
			uint32	hash;

			hash = DatumGetUInt32(FunctionCall1Coll(&stat->info->hashFunc,
													DEFAULT_COLLATION_OID,
													stat->elems[i].datum));
			int		index = HASHVAL(hash);

			stat->helems[i].hash = hash;
			stat->helems[i].idfMin = stat->helems[i].idfMax = stat->elems[i].idf;	
			if ( stat->selems[index].idfMin == 0.0 )
				stat->selems[index].idfMin = stat->selems[index].idfMax = stat->elems[i].idf;
			else if ( stat->selems[index].idfMin > stat->elems[i].idf )
				stat->selems[index].idfMin = stat->elems[i].idf;
			else if ( stat->selems[index].idfMax < stat->elems[i].idf )
				stat->selems[index].idfMax = stat->elems[i].idf;
		}

		stat->nhelems = uniqueHashedElem( stat->helems, stat->nelems);
	}

	return stat;
}

static HashedElem*
getHashedElemIdf(StatCache *stat, uint32 hash, HashedElem *StopLow)
{
	HashedElem	*StopMiddle,
				*StopHigh = stat->helems + stat->nhelems;

	if ( !StopLow )
		StopLow = stat->helems;

	while (StopLow < StopHigh) {
		StopMiddle = StopLow + ((StopHigh - StopLow) >> 1);

		if ( StopMiddle->hash == hash )
			return StopMiddle;
		else if ( StopMiddle->hash < hash )
			StopLow = StopMiddle + 1;
		else
			StopHigh = StopMiddle;
	}

	return NULL;
}

static void
fillHashVal(void *cache, SimpleArray *a)
{
	int i;

	if (a->hash)
		return;

	allocateHash(cache, a);

	if (a->info->tupDesc)
		elog(ERROR, "GiST  doesn't support composite (weighted) type");
	getFmgrInfoHash(a->info);

	for(i=0;i<a->nelems;i++)
		a->hash[i] = DatumGetUInt32(FunctionCall1Coll(&a->info->hashFunc,
													  DEFAULT_COLLATION_OID,
													  a->elems[i]));
}


static bool
hasHashedElem(SmlSign  *a, uint32 h)
{
	uint32	*StopLow = GETARR(a),
			*StopHigh = GETARR(a) + a->size,
			*StopMiddle;

	while (StopLow < StopHigh) {
		StopMiddle = StopLow + ((StopHigh - StopLow) >> 1);

		if ( *StopMiddle == h )
			return true;
		else if ( *StopMiddle < h )
			StopLow = StopMiddle + 1;
		else
			StopHigh = StopMiddle;
	}

	return false;
}

static void
makesign(BITVECP sign, SmlSign	*a)
{
	int32	i;
	uint32	*ptr = GETARR(a);

	MemSet((void *) sign, 0, sizeof(BITVEC));
	SETBIT(sign, SIGLENBIT);   /* set last unused bit */

	for (i = 0; i < a->size; i++)
		HASH(sign, ptr[i]);
}

static int32
sizebitvec(BITVECP sign)
{
	int32	size = 0,
			i;

	LOOPBYTE
		size += number_of_ones[(unsigned char) sign[i]];

	return size;
}

PG_FUNCTION_INFO_V1(gsmlsign_compress);
Datum gsmlsign_compress(PG_FUNCTION_ARGS);
Datum
gsmlsign_compress(PG_FUNCTION_ARGS)
{
	GISTENTRY  *entry = (GISTENTRY *) PG_GETARG_POINTER(0);
	GISTENTRY  *retval = entry;

	if (entry->leafkey) /* new key */
	{
		SmlSign		*sign;
		ArrayType	*a = DatumGetArrayTypeP(entry->key);

		sign = Array2HashedArray(NULL, a);

		if ( VARSIZE(sign) > TOAST_INDEX_TARGET )
		{	/* make signature due to its big size */
			SmlSign	*tmpsign;
			int		len;

			len = CALCGTSIZE( SIGNKEY, sign->size );
			tmpsign = palloc( len );
			tmpsign->flag = SIGNKEY;
			SET_VARSIZE(tmpsign, len);

			makesign(GETSIGN(tmpsign), sign);
			tmpsign->size = sizebitvec(GETSIGN(tmpsign));
			tmpsign->maxrepeat = sign->maxrepeat;
			sign = tmpsign;
		}

		retval = (GISTENTRY *) palloc(sizeof(GISTENTRY));
		gistentryinit(*retval, PointerGetDatum(sign),
						entry->rel, entry->page,
						entry->offset, false);
	}
	else if ( ISSIGNKEY(DatumGetPointer(entry->key)) &&
				!ISALLTRUE(DatumGetPointer(entry->key)) )
	{
		SmlSign	*sign = (SmlSign*)DatumGetPointer(entry->key);

		Assert( sign->size == sizebitvec(GETSIGN(sign)) );

		if ( sign->size == SIGLENBIT )
		{
			int32	len = CALCGTSIZE(SIGNKEY | ALLISTRUE, 0);
			int32	maxrepeat = sign->maxrepeat;

			sign = (SmlSign *) palloc(len);
			SET_VARSIZE(sign, len);
			sign->flag = SIGNKEY | ALLISTRUE;
			sign->size = SIGLENBIT;
			sign->maxrepeat = maxrepeat;

			retval = (GISTENTRY *) palloc(sizeof(GISTENTRY));

			gistentryinit(*retval, PointerGetDatum(sign),
							entry->rel, entry->page,
							entry->offset, false);
		}
	}

	PG_RETURN_POINTER(retval);
}

PG_FUNCTION_INFO_V1(gsmlsign_decompress);
Datum gsmlsign_decompress(PG_FUNCTION_ARGS);
Datum
gsmlsign_decompress(PG_FUNCTION_ARGS)
{
	GISTENTRY	*entry = (GISTENTRY *) PG_GETARG_POINTER(0);
	SmlSign		*key =  (SmlSign*)DatumGetPointer(PG_DETOAST_DATUM(entry->key));

	if (key != (SmlSign *) DatumGetPointer(entry->key))
	{
		GISTENTRY  *retval = (GISTENTRY *) palloc(sizeof(GISTENTRY));

		gistentryinit(*retval, PointerGetDatum(key),
						entry->rel, entry->page,
						entry->offset, false);

		PG_RETURN_POINTER(retval);
	}

	PG_RETURN_POINTER(entry);
}

/*
 * Union method
 */
static bool
unionkey(BITVECP sbase, SmlSign *add)
{
	int32	i;

	if (ISSIGNKEY(add))
	{
		BITVECP	sadd = GETSIGN(add);

		if (ISALLTRUE(add))
			return true;

		LOOPBYTE
			sbase[i] |= sadd[i];
	}
	else
	{
		uint32	*ptr = GETARR(add);

		for (i = 0; i < add->size; i++)
			HASH(sbase, ptr[i]);
	}

	return false;
}

PG_FUNCTION_INFO_V1(gsmlsign_union);
Datum gsmlsign_union(PG_FUNCTION_ARGS);
Datum
gsmlsign_union(PG_FUNCTION_ARGS)
{
	GistEntryVector *entryvec = (GistEntryVector *) PG_GETARG_POINTER(0);
	int				*size = (int *) PG_GETARG_POINTER(1);
	BITVEC			base;
	int32		i,
				len,
				maxrepeat = 1;
	int32		flag = 0;
	SmlSign	   *result;

	MemSet((void *) base, 0, sizeof(BITVEC));
	for (i = 0; i < entryvec->n; i++)
	{
		if (GETENTRY(entryvec, i)->maxrepeat > maxrepeat)
			maxrepeat = GETENTRY(entryvec, i)->maxrepeat;
		if (unionkey(base, GETENTRY(entryvec, i)))
		{
			flag = ALLISTRUE;
			break;
		}
	}

	flag |= SIGNKEY;
	len = CALCGTSIZE(flag, 0);
	result = (SmlSign *) palloc(len);
	*size = len;
	SET_VARSIZE(result, len);
	result->flag = flag;
	result->maxrepeat = maxrepeat;

	if (!ISALLTRUE(result))
	{
		memcpy((void *) GETSIGN(result), (void *) base, sizeof(BITVEC));
		result->size = sizebitvec(GETSIGN(result));
	}
	else
		result->size = SIGLENBIT;

	PG_RETURN_POINTER(result);
}

/*
 * Same method
 */

PG_FUNCTION_INFO_V1(gsmlsign_same);
Datum gsmlsign_same(PG_FUNCTION_ARGS);
Datum
gsmlsign_same(PG_FUNCTION_ARGS)
{
	SmlSign	*a = (SmlSign*)PG_GETARG_POINTER(0);
	SmlSign *b = (SmlSign*)PG_GETARG_POINTER(1);
	bool	*result = (bool *) PG_GETARG_POINTER(2);

	if (a->size != b->size)
	{
		*result = false;
	}
	else if (ISSIGNKEY(a))
	{	/* then b also ISSIGNKEY */
		if ( ISALLTRUE(a) )
		{
			/* in this case b is all true too - other cases is catched
			   upper */
			*result = true;
		}
		else
		{
			int32	i;
			BITVECP	sa = GETSIGN(a),
					sb = GETSIGN(b);

			*result = true;

			if ( !ISALLTRUE(a) )
			{
				LOOPBYTE
				{
					if (sa[i] != sb[i])
					{
						*result = false;
						break;
					}
				}
			}
		}
	}
	else
	{
		uint32	*ptra = GETARR(a),
				*ptrb = GETARR(b);
		int32	i;

		*result = true;
		for (i = 0; i < a->size; i++)
		{
			if ( ptra[i] != ptrb[i])
			{
				*result = false;
				break;
			}
		}
	}

	PG_RETURN_POINTER(result);
}

/*
 * Penalty method
 */
static int
hemdistsign(BITVECP a, BITVECP b)
{
	int	i,
		diff,
		dist = 0;

	LOOPBYTE
	{
		diff = (unsigned char) (a[i] ^ b[i]);
		dist += number_of_ones[diff];
	}
	return dist;
}

static int
hemdist(SmlSign *a, SmlSign *b)
{
	if (ISALLTRUE(a))
	{
		if (ISALLTRUE(b))
			return 0;
		else
			return SIGLENBIT - b->size;
	}
	else if (ISALLTRUE(b))
		return SIGLENBIT - b->size;

	return hemdistsign(GETSIGN(a), GETSIGN(b));
}

PG_FUNCTION_INFO_V1(gsmlsign_penalty);
Datum gsmlsign_penalty(PG_FUNCTION_ARGS);
Datum
gsmlsign_penalty(PG_FUNCTION_ARGS)
{
	GISTENTRY	*origentry = (GISTENTRY *) PG_GETARG_POINTER(0); /* always ISSIGNKEY */
	GISTENTRY	*newentry = (GISTENTRY *) PG_GETARG_POINTER(1);
	float		*penalty = (float *) PG_GETARG_POINTER(2);
	SmlSign		*origval = (SmlSign *) DatumGetPointer(origentry->key);
	SmlSign		*newval = (SmlSign *) DatumGetPointer(newentry->key);
	BITVECP		orig = GETSIGN(origval);

	*penalty = 0.0;

	if (ISARRKEY(newval))
	{
		BITVEC	sign;

		makesign(sign, newval);

		if (ISALLTRUE(origval))
			*penalty = ((float) (SIGLENBIT - sizebitvec(sign))) / (float) (SIGLENBIT + 1);
		else
			*penalty = hemdistsign(sign, orig);
	}
	else
		*penalty = hemdist(origval, newval);

	PG_RETURN_POINTER(penalty);
}

/*
 * Picksplit method
 */

typedef struct
{
	bool	allistrue;
	int32	size;
	BITVEC	sign;
} CACHESIGN;

static void
fillcache(CACHESIGN *item, SmlSign *key)
{
	item->allistrue = false;
	item->size = key->size;

	if (ISARRKEY(key))
	{
		makesign(item->sign, key);
		item->size = sizebitvec( item->sign );
	}
	else if (ISALLTRUE(key))
		item->allistrue = true;
	else
		memcpy((void *) item->sign, (void *) GETSIGN(key), sizeof(BITVEC));
}

#define WISH_F(a,b,c) (double)( -(double)(((a)-(b))*((a)-(b))*((a)-(b)))*(c) )

typedef struct
{
	OffsetNumber pos;
	int32		cost;
} SPLITCOST;

static int
comparecost(const void *va, const void *vb)
{
	SPLITCOST  *a = (SPLITCOST *) va;
	SPLITCOST  *b = (SPLITCOST *) vb;

	if (a->cost == b->cost)
		return 0;
	else
		return (a->cost > b->cost) ? 1 : -1;
}

static int
hemdistcache(CACHESIGN *a, CACHESIGN *b)
{
	if (a->allistrue)
	{
		if (b->allistrue)
			return 0;
		else
			return SIGLENBIT - b->size;
	}
	else if (b->allistrue)
		return SIGLENBIT - a->size;

	return hemdistsign(a->sign, b->sign);
}

PG_FUNCTION_INFO_V1(gsmlsign_picksplit);
Datum gsmlsign_picksplit(PG_FUNCTION_ARGS);
Datum
gsmlsign_picksplit(PG_FUNCTION_ARGS)
{
	GistEntryVector *entryvec = (GistEntryVector *) PG_GETARG_POINTER(0);
	GIST_SPLITVEC *v = (GIST_SPLITVEC *) PG_GETARG_POINTER(1);
	OffsetNumber k,
				j;
	SmlSign		*datum_l,
				*datum_r;
	BITVECP		union_l,
				union_r;
	int32		size_alpha,
				size_beta;
	int32		size_waste,
				waste = -1;
	int32		nbytes;
	OffsetNumber seed_1 = 0,
				seed_2 = 0;
	OffsetNumber *left,
				*right;
	OffsetNumber maxoff;
	BITVECP		ptr;
	int			i;
	CACHESIGN  *cache;
	SPLITCOST  *costvector;

	maxoff = entryvec->n - 2;
	nbytes = (maxoff + 2) * sizeof(OffsetNumber);
	v->spl_left = (OffsetNumber *) palloc(nbytes);
	v->spl_right = (OffsetNumber *) palloc(nbytes);

	cache = (CACHESIGN *) palloc(sizeof(CACHESIGN) * (maxoff + 2));
	fillcache(&cache[FirstOffsetNumber], GETENTRY(entryvec, FirstOffsetNumber));

	for (k = FirstOffsetNumber; k < maxoff; k = OffsetNumberNext(k))
	{
		for (j = OffsetNumberNext(k); j <= maxoff; j = OffsetNumberNext(j))
		{
			if (k == FirstOffsetNumber)
				fillcache(&cache[j], GETENTRY(entryvec, j));

			size_waste = hemdistcache(&(cache[j]), &(cache[k]));
			if (size_waste > waste)
			{
				waste = size_waste;
				seed_1 = k;
				seed_2 = j;
			}
		}
	}

	left = v->spl_left;
	v->spl_nleft = 0;
	right = v->spl_right;
	v->spl_nright = 0;

	if (seed_1 == 0 || seed_2 == 0)
	{
		seed_1 = 1;
		seed_2 = 2;
	}

	/* form initial .. */
	if (cache[seed_1].allistrue)
	{
		datum_l = (SmlSign *) palloc(CALCGTSIZE(SIGNKEY | ALLISTRUE, 0));
		SET_VARSIZE(datum_l, CALCGTSIZE(SIGNKEY | ALLISTRUE, 0));
		datum_l->flag = SIGNKEY | ALLISTRUE;
		datum_l->size = SIGLENBIT;
	}
	else
	{
		datum_l = (SmlSign *) palloc(CALCGTSIZE(SIGNKEY, 0));
		SET_VARSIZE(datum_l, CALCGTSIZE(SIGNKEY, 0));
		datum_l->flag = SIGNKEY;
		memcpy((void *) GETSIGN(datum_l), (void *) cache[seed_1].sign, sizeof(BITVEC));
		datum_l->size = cache[seed_1].size;
	}
	if (cache[seed_2].allistrue)
	{
		datum_r = (SmlSign *) palloc(CALCGTSIZE(SIGNKEY | ALLISTRUE, 0));
		SET_VARSIZE(datum_r, CALCGTSIZE(SIGNKEY | ALLISTRUE, 0));
		datum_r->flag = SIGNKEY | ALLISTRUE;
		datum_r->size = SIGLENBIT;
	}
	else
	{
		datum_r = (SmlSign *) palloc(CALCGTSIZE(SIGNKEY, 0));
		SET_VARSIZE(datum_r, CALCGTSIZE(SIGNKEY, 0));
		datum_r->flag = SIGNKEY;
		memcpy((void *) GETSIGN(datum_r), (void *) cache[seed_2].sign, sizeof(BITVEC));
		datum_r->size = cache[seed_2].size;
	}

	union_l = GETSIGN(datum_l);
	union_r = GETSIGN(datum_r);
	maxoff = OffsetNumberNext(maxoff);
	fillcache(&cache[maxoff], GETENTRY(entryvec, maxoff));
	/* sort before ... */
	costvector = (SPLITCOST *) palloc(sizeof(SPLITCOST) * maxoff);
	for (j = FirstOffsetNumber; j <= maxoff; j = OffsetNumberNext(j))
	{
		costvector[j - 1].pos = j;
		size_alpha = hemdistcache(&(cache[seed_1]), &(cache[j]));
		size_beta = hemdistcache(&(cache[seed_2]), &(cache[j]));
		costvector[j - 1].cost = Abs(size_alpha - size_beta);
	}
	qsort((void *) costvector, maxoff, sizeof(SPLITCOST), comparecost);

	datum_l->maxrepeat = datum_r->maxrepeat = 1;

	for (k = 0; k < maxoff; k++)
	{
		j = costvector[k].pos;
		if (j == seed_1)
		{
			*left++ = j;
			v->spl_nleft++;
			continue;
		}
		else if (j == seed_2)
		{
			*right++ = j;
			v->spl_nright++;
			continue;
		}

		if (ISALLTRUE(datum_l) || cache[j].allistrue)
		{
			if (ISALLTRUE(datum_l) && cache[j].allistrue)
				size_alpha = 0;
			else
				size_alpha = SIGLENBIT - (
									(cache[j].allistrue) ? datum_l->size : cache[j].size
							);
		}
		else
			size_alpha = hemdistsign(cache[j].sign, GETSIGN(datum_l));

		if (ISALLTRUE(datum_r) || cache[j].allistrue)
		{
			if (ISALLTRUE(datum_r) && cache[j].allistrue)
				size_beta = 0;
			else
				size_beta = SIGLENBIT - (
									(cache[j].allistrue) ? datum_r->size : cache[j].size
							);
		}
		else
			size_beta = hemdistsign(cache[j].sign, GETSIGN(datum_r));

		if (size_alpha < size_beta + WISH_F(v->spl_nleft, v->spl_nright, 0.1))
		{
			if (ISALLTRUE(datum_l) || cache[j].allistrue)
			{
				if (!ISALLTRUE(datum_l))
					MemSet((void *) GETSIGN(datum_l), 0xff, sizeof(BITVEC));
				datum_l->size = SIGLENBIT;
			}
			else
			{
				ptr = cache[j].sign;
				LOOPBYTE
					union_l[i] |= ptr[i];
				datum_l->size = sizebitvec(union_l);
			}
			*left++ = j;
			v->spl_nleft++;
		}
		else
		{
			if (ISALLTRUE(datum_r) || cache[j].allistrue)
			{
				if (!ISALLTRUE(datum_r))
					MemSet((void *) GETSIGN(datum_r), 0xff, sizeof(BITVEC));
				datum_r->size = SIGLENBIT;
			}
			else
			{
				ptr = cache[j].sign;
				LOOPBYTE
					union_r[i] |= ptr[i];
				datum_r->size = sizebitvec(union_r);
			}
			*right++ = j;
			v->spl_nright++;
		}
	}
	*right = *left = FirstOffsetNumber;
	v->spl_ldatum = PointerGetDatum(datum_l);
	v->spl_rdatum = PointerGetDatum(datum_r);

	Assert( datum_l->size = sizebitvec(GETSIGN(datum_l)) );
	Assert( datum_r->size = sizebitvec(GETSIGN(datum_r)) );

	PG_RETURN_POINTER(v);
}

static double
getIdfMaxLimit(SmlSign *key)
{
	switch( getTFMethod() )
	{
		case TF_CONST:
			return 1.0;
			break;
		case TF_N:
			return (double)(key->maxrepeat);
			break;
		case TF_LOG:
			return 1.0 + log( (double)(key->maxrepeat) );
			break;
		default:
			elog(ERROR,"Unknown TF method: %d", getTFMethod());
	}

	return 0.0;
}

/*
 * Consistent function
 */
PG_FUNCTION_INFO_V1(gsmlsign_consistent);
Datum gsmlsign_consistent(PG_FUNCTION_ARGS);
Datum
gsmlsign_consistent(PG_FUNCTION_ARGS)
{
	GISTENTRY		*entry = (GISTENTRY *) PG_GETARG_POINTER(0);
	StrategyNumber	strategy = (StrategyNumber) PG_GETARG_UINT16(2);
	bool			*recheck = (bool *) PG_GETARG_POINTER(4);
	ArrayType		*a;
	SmlSign			*key = (SmlSign*)DatumGetPointer(entry->key);
	int				res = false;
	SmlSign			*query;
	SimpleArray		*s;
	int32			i;

	fcinfo->flinfo->fn_extra = SearchArrayCache(
									fcinfo->flinfo->fn_extra,
									fcinfo->flinfo->fn_mcxt,
									PG_GETARG_DATUM(1), &a, &s, &query);

	*recheck = true;

	if ( ARRISVOID(a) )
		PG_RETURN_BOOL(res);

	if ( strategy == SmlarOverlapStrategy )
	{
		if (ISALLTRUE(key))
		{
			res = true;
		}
		else if (ISARRKEY(key))
		{
			uint32	*kptr = GETARR(key),
					*qptr = GETARR(query);

			while( kptr - GETARR(key) < key->size && qptr - GETARR(query) < query->size )
			{
				if ( *kptr < *qptr )
					kptr++;
				else if ( *kptr > *qptr )
					qptr++;
				else
				{
					res = true;
					break;
				}
			}
			*recheck = false;
		}
		else
		{
			BITVECP sign = GETSIGN(key);

			fillHashVal(fcinfo->flinfo->fn_extra, s);

			for(i=0; i<s->nelems; i++)
			{
				if ( GETBIT(sign, HASHVAL(s->hash[i])) )
				{
					res = true;
					break;
				}
			}
		}
	}
	/*
	 *  SmlarSimilarityStrategy
	 */
	else if (ISALLTRUE(key))
	{
		if ( GIST_LEAF(entry) )
		{
			/*
			 * With TF/IDF similarity we cannot say anything useful
			 */
			if ( query->size < SIGLENBIT && getSmlType() != ST_TFIDF )
			{
				double power = ((double)(query->size)) * ((double)(SIGLENBIT));

				if ( ((double)(query->size)) / sqrt(power) >= GetSmlarLimit() ) 
					res = true;
			}
			else
			{
				res = true;	
			}
		}
		else
			res = true;
	}
	else if (ISARRKEY(key))
	{
		uint32	*kptr = GETARR(key),
				*qptr = GETARR(query);

		Assert( GIST_LEAF(entry) );

		switch(getSmlType())
		{
			case ST_TFIDF:
				{
					StatCache	*stat = getHashedCache(fcinfo->flinfo->fn_extra);
					double		sumU = 0.0,
								sumQ = 0.0,
								sumK = 0.0;
					double		maxKTF = getIdfMaxLimit(key);
					HashedElem  *h;

					Assert( s->df );
					fillHashVal(fcinfo->flinfo->fn_extra, s);
					if ( stat->info != s->info )
						elog(ERROR,"Statistic and actual argument have different type");

					for(i=0;i<s->nelems;i++)
					{
						sumQ += s->df[i] * s->df[i];

						h = getHashedElemIdf(stat, s->hash[i], NULL);
						if ( h && hasHashedElem(key, s->hash[i]) )
						{
							sumK += h->idfMin * h->idfMin;
							sumU += h->idfMax * maxKTF * s->df[i];
						} 
					}

					if ( sumK > 0.0 && sumQ > 0.0 && sumU / sqrt( sumK * sumQ ) >= GetSmlarLimit() )
					{
						/* 
						 * More precisely calculate sumK
						 */
						h = NULL;
						sumK = 0.0;

						for(i=0;i<key->size;i++)
						{
							h = getHashedElemIdf(stat, GETARR(key)[i], h);					
							if (h)
								sumK += h->idfMin * h->idfMin;
						}

						if ( sumK > 0.0 && sumQ > 0.0 && sumU / sqrt( sumK * sumQ ) >= GetSmlarLimit() )
							res = true;
					}
				}
				break;
			case ST_COSINE:
				{
					double			power;
					power = sqrt( ((double)(key->size)) * ((double)(s->nelems)) );

					if (  ((double)Min(key->size, s->nelems)) / power >= GetSmlarLimit() )
					{
						int  cnt = 0;

						while( kptr - GETARR(key) < key->size && qptr - GETARR(query) < query->size )
						{
							if ( *kptr < *qptr )
								kptr++;
							else if ( *kptr > *qptr )
								qptr++;
							else
							{
								cnt++;
								kptr++;
								qptr++;
							}
						}

						if ( ((double)cnt) / power >= GetSmlarLimit() )
							res = true;
					}
				}
				break;
			default:
				elog(ERROR,"GiST doesn't support current formula type of similarity");
		}
	}
	else
	{	/* signature */
		BITVECP sign = GETSIGN(key);
		int32	count = 0;

		fillHashVal(fcinfo->flinfo->fn_extra, s);

		if ( GIST_LEAF(entry) )
		{
			switch(getSmlType())
			{
				case ST_TFIDF:
					{
						StatCache	*stat = getHashedCache(fcinfo->flinfo->fn_extra);
						double		sumU = 0.0,
									sumQ = 0.0,
									sumK = 0.0;
						double		maxKTF = getIdfMaxLimit(key);

						Assert( s->df );
						if ( stat->info != s->info )
							elog(ERROR,"Statistic and actual argument have different type");

						for(i=0;i<s->nelems;i++)
						{
							int32		hbit = HASHVAL(s->hash[i]);

							sumQ += s->df[i] * s->df[i];
							if ( GETBIT(sign, hbit) )
							{
								sumK += stat->selems[ hbit ].idfMin * stat->selems[ hbit ].idfMin;
								sumU += stat->selems[ hbit ].idfMax * maxKTF * s->df[i];
							}
						}

						if ( sumK > 0.0 && sumQ > 0.0 && sumU / sqrt( sumK * sumQ ) >= GetSmlarLimit() )
						{
							/* 
							 * More precisely calculate sumK
							 */
							sumK = 0.0;

							for(i=0;i<SIGLENBIT;i++)
								if ( GETBIT(sign,i) )
									sumK += stat->selems[ i ].idfMin * stat->selems[ i ].idfMin;

							if ( sumK > 0.0 && sumQ > 0.0 && sumU / sqrt( sumK * sumQ ) >= GetSmlarLimit() )
								res = true;
						}
					}
					break;
				case ST_COSINE:
					{
						double	power;

						power = sqrt( ((double)(key->size)) * ((double)(s->nelems)) );

						for(i=0; i<s->nelems; i++)
							count += GETBIT(sign, HASHVAL(s->hash[i]));

						if ( ((double)count) / power >= GetSmlarLimit() )
							res = true;
					}
					break;
				default:
					elog(ERROR,"GiST doesn't support current formula type of similarity");
			}
		}
		else /* non-leaf page */
		{
			switch(getSmlType())
			{
				case ST_TFIDF:
					{
						StatCache	*stat = getHashedCache(fcinfo->flinfo->fn_extra);
						double		sumU = 0.0,
									sumQ = 0.0,
									minK = -1.0;
						double		maxKTF = getIdfMaxLimit(key);

						Assert( s->df );
						if ( stat->info != s->info )
							elog(ERROR,"Statistic and actual argument have different type");

						for(i=0;i<s->nelems;i++)
						{
							int32		hbit = HASHVAL(s->hash[i]);

							sumQ += s->df[i] * s->df[i];
							if ( GETBIT(sign, hbit) )
							{
								sumU += stat->selems[ hbit ].idfMax * maxKTF * s->df[i];
								if ( minK > stat->selems[ hbit ].idfMin  || minK < 0.0 )
									minK = stat->selems[ hbit ].idfMin;
							}
						}

						if ( sumQ > 0.0 && minK > 0.0 && sumU / sqrt( sumQ * minK ) >= GetSmlarLimit() )
							res = true;
					}
					break;
				case ST_COSINE:
					{
						for(i=0; i<s->nelems; i++)
							count += GETBIT(sign, HASHVAL(s->hash[i]));

						if ( s->nelems == count  || sqrt(((double)count) / ((double)(s->nelems))) >= GetSmlarLimit() )
							res = true;
					}
					break;
				default:
					elog(ERROR,"GiST doesn't support current formula type of similarity");
			}
		}
	}

#if 0
	{
		static int nnres = 0;
		static int nres = 0;
		if ( GIST_LEAF(entry) ) {
			if ( res )
				nres++;
			else
				nnres++;
			elog(NOTICE,"%s nn:%d n:%d", (ISARRKEY(key)) ? "ARR" : ( (ISALLTRUE(key)) ? "TRUE" : "SIGN" ), nnres, nres  );
		}
	}
#endif

	PG_RETURN_BOOL(res);
}
