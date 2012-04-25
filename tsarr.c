#include "postgres.h"

#include "catalog/pg_type.h"
#include "tsearch/ts_type.h"
#include "utils/array.h"

PG_FUNCTION_INFO_V1(tsvector2textarray);
Datum       tsvector2textarray(PG_FUNCTION_ARGS);
Datum
tsvector2textarray(PG_FUNCTION_ARGS)
{
	TSVector	ts = PG_GETARG_TSVECTOR(0);
	ArrayType	*a;
	Datum		*words;
	int			i;
	WordEntry	*wptr = ARRPTR(ts);

	words = palloc( sizeof(Datum) * (ts->size+1) );

	for(i=0; i<ts->size; i++)
	{
		text		*t = palloc(VARHDRSZ + wptr->len);

		SET_VARSIZE(t, VARHDRSZ + wptr->len);
		memcpy( VARDATA(t), STRPTR(ts) + wptr->pos, wptr->len);
		words[i] = PointerGetDatum(t);
	
		wptr++;
	}

	a = construct_array( words, ts->size,
							TEXTOID, -1, false, 'i' );

	PG_FREE_IF_COPY(ts, 0);

	PG_RETURN_ARRAYTYPE_P(a);
}

