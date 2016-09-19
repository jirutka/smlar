#include "smlar.h"

#include "fmgr.h"
#include "utils/guc.h"

#include "smlar.h"

/*
 *  Smlar limit
 */
static double smlar_limit = 0.6;

/*
 * Smlar table stores table-wide statistic
 */
static char * smlar_table = "";

static bool smlar_use_malloc = false;

/*
 * State of GUC initialization
 */
static bool smlar_guc_inited = false;

static void
SmlarTableAssign(const char *newval, void *extra)
{
	resetStatCache();
}

static bool smlar_logadd = false;

static void
SmlarLogAssign(bool newval, void *extra)
{
	resetStatCache();
}

static int smlar_smltype = ST_COSINE;

static const struct config_enum_entry SmlarTypeOptions[] = {
    {"cosine", ST_COSINE, false},
	{"tfidf", ST_TFIDF, false},
	{"overlap", ST_OVERLAP, false},
	{NULL, 0, false}
};

static int	smlar_tf_method = TF_N;
static const struct config_enum_entry SmlarTFOptions[] = {
    {"n", TF_N, false},
	{"log", TF_LOG, false},
	{"const", TF_CONST, false},
	{NULL, 0, false}
};

static void
initSmlarGUC()
{
	if (smlar_guc_inited)
		return;

	DefineCustomRealVariable(
			"smlar.threshold",
			"Lower threshold of array's similarity",
			"Array's with similarity lower than threshold are not similar by % operation",
			&smlar_limit,
			0.6, 
			0.0,
			1e10,
			PGC_USERSET,
			0,
			NULL,
			NULL,
			NULL
	);

	DefineCustomStringVariable(
			"smlar.stattable",
			"Name of table stored set-wide statistic",
			"Named table stores global frequencies of array's elements",
			&smlar_table,
			"",
			PGC_USERSET,
			GUC_IS_NAME,
			NULL,
			SmlarTableAssign,
			NULL
	);

	DefineCustomEnumVariable(
			"smlar.type",
			"Type of similarity formula",
			"Type of similarity formula: cosine(default), tfidf, overlap",
			&smlar_smltype,
			smlar_smltype,
			SmlarTypeOptions,
			PGC_SUSET,
			0,
			NULL,
			NULL,
			NULL
	);

	DefineCustomBoolVariable(
			"smlar.persistent_cache",
			"Usage of persistent cache of global stat",
			"Cache of global stat is stored in transaction-independent memory",
			&smlar_use_malloc,
			false,
			PGC_USERSET,
			0,
			NULL,
			NULL,
			NULL
	);

	DefineCustomBoolVariable(
			"smlar.idf_plus_one",
			"Calculate idf by log(1+d/df)",
			"Calculate idf by log(1+d/df)",
			&smlar_logadd,
			false,
			PGC_USERSET,
			0,
			NULL,
			SmlarLogAssign,
			NULL
	);

	DefineCustomEnumVariable(
			"smlar.tf_method",
			"Method of TF caclulation",
			"TF method: n => number of entries, log => 1+log(n), const => constant value",
			&smlar_tf_method,
			smlar_tf_method,
			SmlarTFOptions,
			PGC_SUSET,
			0,
			NULL,
			NULL,
			NULL
	);

	smlar_guc_inited = true;	
}

double 
getOneAdd(void)
{
	if (!smlar_guc_inited)
		initSmlarGUC();

	return (smlar_logadd) ? 1.0 : 0.0;
}

int
getTFMethod(void)
{
	if (!smlar_guc_inited)
		initSmlarGUC();

	return smlar_tf_method;
}

double
GetSmlarLimit(void)
{
	if (!smlar_guc_inited)
		initSmlarGUC();

	return smlar_limit;
}

const char*
GetSmlarTable(void)
{
	if (!smlar_guc_inited)
		initSmlarGUC();

	return smlar_table;
}

int
getSmlType(void)
{
	if (!smlar_guc_inited)
		initSmlarGUC();

	return smlar_smltype;
}

bool
GetSmlarUsePersistent(void)
{
	if (!smlar_guc_inited)
		initSmlarGUC();

	return smlar_use_malloc;
}

PG_FUNCTION_INFO_V1(set_smlar_limit);
Datum       set_smlar_limit(PG_FUNCTION_ARGS);
Datum
set_smlar_limit(PG_FUNCTION_ARGS)
{
	float4      nlimit = PG_GETARG_FLOAT4(0);
	char		buf[32];

	/* init smlar guc */
	initSmlarGUC();

	sprintf(buf,"%f", nlimit);
	set_config_option("smlar.threshold", buf,
						PGC_USERSET, PGC_S_SESSION ,GUC_ACTION_SET, true, 0
#if PG_VERSION_NUM >= 90500
						,false
#endif
						);
	PG_RETURN_FLOAT4((float4)GetSmlarLimit());
}

PG_FUNCTION_INFO_V1(show_smlar_limit);
Datum       show_smlar_limit(PG_FUNCTION_ARGS);
Datum
show_smlar_limit(PG_FUNCTION_ARGS)
{
	PG_RETURN_FLOAT4((float4)GetSmlarLimit());
}

