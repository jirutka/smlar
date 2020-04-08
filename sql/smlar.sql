CREATE EXTENSION smlar;
set extra_float_digits =0;

--sanity check
SELECT 
	opc.opcname, 
	t.typname, 
	opc.opcdefault  
FROM 
	pg_opclass opc, 
	pg_am am, 
	pg_type t, 
	pg_type k 
WHERE 
	opc.opcmethod = am.oid AND 
	am.amname='gist' AND 
	opc.opcintype = t.oid AND 
	opc.opckeytype = k.oid AND 
	k.typname='gsmlsign'
ORDER BY opc.opcname;

SELECT 
	opc.opcname, 
	t.typname, 
	opc.opcdefault  
FROM 
	pg_opclass opc, 
	pg_am am, 
	pg_type t
WHERE 
	opc.opcmethod = am.oid AND 
	am.amname='gin' AND 
	opc.opcintype = t.oid AND 
	opc.opcname ~ '_sml_ops$'
ORDER BY opc.opcname;

SELECT 
    trim( leading '_'  from t.typname ) || '[]' AS "Array Type",
    gin.opcname AS "GIN operator class",
    gist.opcname AS "GiST operator class"
FROM
    (
        SELECT *
        FROM
            pg_catalog.pg_opclass,
            pg_catalog.pg_am
        WHERE
            pg_opclass.opcmethod = pg_am.oid AND
            pg_am.amname = 'gin' AND
            pg_opclass.opcname ~ '_sml_ops$'
    ) AS gin
    FULL OUTER JOIN
        (
            SELECT *
            FROM
                pg_catalog.pg_opclass,
                pg_catalog.pg_am
            WHERE
                pg_opclass.opcmethod = pg_am.oid AND
                pg_am.amname = 'gist' AND
                pg_opclass.opcname ~ '_sml_ops$'
        ) AS gist
        ON (
            gist.opcname = gin.opcname AND 
            gist.opcintype = gin.opcintype 
        ),
    pg_catalog.pg_type t
WHERE
    t.oid = COALESCE(gist.opcintype, gin.opcintype) AND
    t.typarray = 0
ORDER BY
    "Array Type" ASC 
;

SELECT set_smlar_limit(0.1);
SET smlar.threshold = 0.6;

--First checks
SELECT smlar('{3,2}'::int[], '{3,2,1}');
SELECT smlar('{2,1}'::int[], '{3,2,1}');
SELECT smlar('{}'::int[], '{3,2,1}');
SELECT smlar('{12,10}'::int[], '{3,2,1}');
SELECT smlar('{123}'::int[], '{}');
SELECT smlar('{1,4,6}'::int[], '{1,4,6}');
SELECT smlar('{1,4,6}'::int[], '{6,4,1}');
SELECT smlar('{1,4,6}'::int[], '{5,4,6}');
SELECT smlar('{1,4,6}'::int[], '{5,4,6}');
SELECT smlar('{1,2}'::int[], '{2,2,2,2,2,1}');
SELECT smlar('{1,2}'::int[], '{1,2,2,2,2,2}');
SELECT smlar('{}'::int[], '{3}');
SELECT smlar('{3}'::int[], '{3}');
SELECT smlar('{2}'::int[], '{3}');


SELECT smlar('{1,4,6}'::int[], '{5,4,6}', 'N.i / (N.a + N.b)' );
SELECT smlar('{1,4,6}'::int[], '{5,4,6}', 'N.i / sqrt(N.a * N.b)' );

SELECT tsvector2textarray('qwe:12 asd:45');

SELECT array_unique('{12,12,1,4,1,16}'::int4[]);
SELECT array_unique('{12,12,1,4,1,16}'::bigint[]);
SELECT array_unique('{12,12,1,4,1,16}'::smallint[]);
SELECT array_unique('{12,12,1,4,1,16}'::text[]);
SELECT array_unique('{12,12,1,4,1,16}'::varchar[]);

SELECT inarray('{12,12,1,4,1,16}'::int[], 13::int);
SELECT inarray('{12,12,1,4,1,16}'::int[], 12::int);
SELECT inarray('{12,12,1,4,1,16}'::text[], 13::text);
SELECT inarray('{12,12,1,4,1,16}'::text[], 12::text);
SELECT inarray('{12,12,1,4,1,16}'::int[], 13::int, 0.9, 0.1);
SELECT inarray('{12,12,1,4,1,16}'::int[], 12::int, 0.9, 0.1);
SELECT inarray('{12,12,1,4,1,16}'::text[], 13::text, 0.9, 0.1);
SELECT inarray('{12,12,1,4,1,16}'::text[], 12::text, 0.9, 0.1);

--testing function
CREATE OR REPLACE FUNCTION epoch2timestamp(int4)
RETURNS timestamp AS $$
    SELECT ('1970-01-01 00:00:00'::timestamp +   ( ($1*3600*24 + $1) ::text || ' seconds' )::interval)::timestamp;
	$$ LANGUAGE SQL RETURNS NULL ON NULL INPUT IMMUTABLE;

CREATE OR REPLACE FUNCTION to_tsp_array(_int4)
RETURNS _timestamp AS $$
	SELECT ARRAY( 
		SELECT 
			epoch2timestamp( $1[n] )
		FROM
			generate_series( 1, array_upper( $1, 1) - array_lower( $1, 1 ) + 1 ) AS n
	);
	$$ LANGUAGE SQL RETURNS NULL ON NULL INPUT IMMUTABLE;

CREATE OR REPLACE FUNCTION array_to_col(anyarray)
RETURNS SETOF anyelement AS
$$
    SELECT $1[n] FROM generate_series( 1, array_upper( $1, 1) - array_lower( $1, 1 ) + 1 ) AS n;
$$ LANGUAGE SQL RETURNS NULL ON NULL INPUT IMMUTABLE;

