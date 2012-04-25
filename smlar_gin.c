#include "smlar.h"

#include "fmgr.h"
#include "access/gin.h"
#include "access/skey.h"
#include "access/tuptoaster.h"

PG_FUNCTION_INFO_V1(smlararrayextract);
Datum smlararrayextract(PG_FUNCTION_ARGS);
Datum
smlararrayextract(PG_FUNCTION_ARGS)
{
	ArrayType	*array;
	int32      *nentries = (int32 *) PG_GETARG_POINTER(1);
	SimpleArray	*sa;

	/*
	 * we should guarantee that array will not be destroyed during all
	 * operation
	 */
	array = PG_GETARG_ARRAYTYPE_P_COPY(0);

	CHECKARRVALID(array);

	sa = Array2SimpleArrayU(NULL, array, NULL);

	*nentries = sa->nelems;

	if (sa->nelems == 0 && PG_NARGS() == 3)
	{
		switch (PG_GETARG_UINT16(2))    /* StrategyNumber */
		{
			case	SmlarOverlapStrategy:
			case	SmlarSimilarityStrategy:
				*nentries = -1; /* nobody can be found */
				break;
			default:
				break;
		}
	}

	PG_RETURN_POINTER( sa->elems );
}

PG_FUNCTION_INFO_V1(smlarqueryarrayextract);
Datum smlarqueryarrayextract(PG_FUNCTION_ARGS);
Datum
smlarqueryarrayextract(PG_FUNCTION_ARGS)
{
	PG_RETURN_DATUM(DirectFunctionCall3(smlararrayextract,
										PG_GETARG_DATUM(0),
										PG_GETARG_DATUM(1),
										PG_GETARG_DATUM(2)));
}

PG_FUNCTION_INFO_V1(smlararrayconsistent);
Datum smlararrayconsistent(PG_FUNCTION_ARGS);
Datum
smlararrayconsistent(PG_FUNCTION_ARGS)
{
	bool       		*check = (bool *) PG_GETARG_POINTER(0);
	StrategyNumber 	strategy = PG_GETARG_UINT16(1);
	SimpleArray		*sa;
	bool			res = false;
	int				i,
					cnt = 0;
	bool       		*recheck = (bool *) PG_GETARG_POINTER(5);

	*recheck = true;

	switch (strategy)
	{
		case SmlarOverlapStrategy:
			/* at least one element in check[] is true, so result = true */
			res = true;
			*recheck = false;
			break;
		case SmlarSimilarityStrategy:

			fcinfo->flinfo->fn_extra = SearchArrayCache(
												fcinfo->flinfo->fn_extra,
												fcinfo->flinfo->fn_mcxt,
												PG_GETARG_DATUM(2), NULL, &sa, NULL );

			for(i=0; i<sa->nelems; i++)
				cnt += check[i];

			/*
			 * cnt is a lower limit of elements's number in indexed array;
			 */

			switch(getSmlType())
			{
				case	ST_TFIDF:
						{
							double 	weight = 0.0, /* exact weight of union */
									saSum = 0.0,  /* exact length of query */
									siSum = 0.0;  /* lower limit of length of indexed value */ 

							if ( getTFMethod() != TF_CONST )
								elog(ERROR,"GIN supports only smlar.tf_method = \"const\"" );

							Assert(sa->df);

							for(i=0; i<sa->nelems; i++)
							{
								/*
								 * With smlar.tf_method = "const"   sa->df[i] is 
								 * equal to its idf, so lookup of StatElem is not needed
								 */
								if ( check[i] )
								{
									weight += sa->df[i] * sa->df[i];
									siSum += sa->df[i] * sa->df[i];
								}
								saSum += sa->df[i] * sa->df[i];
							}

							if ( saSum > 0.0 && siSum > 0.0 && weight / sqrt(saSum * siSum ) > GetSmlarLimit() )
								res = true;
						}
						break;
				case 	ST_COSINE:
						{
							double			power;

							power = sqrt( ((double)(sa->nelems)) * ((double)(cnt)) );

							if (  ((double)cnt) / power >= GetSmlarLimit()  )
								res = true;
						}
						break;
				case 	ST_OVERLAP:
						if (cnt >= GetSmlarLimit())
							res = true;
						break;
				default:
					elog(ERROR,"GIN doesn't support current formula type of similarity");
			}
			break;
		default:
			elog(ERROR, "smlararrayconsistent: unknown strategy number: %d", strategy);
	}

	PG_RETURN_BOOL(res);
}
