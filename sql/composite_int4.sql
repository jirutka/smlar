SELECT set_smlar_limit(0.6);

CREATE TYPE cint AS (id int, w float4);
SELECT 
	t,
	ARRAY(
		SELECT 
			(ROW(v::int4, v::float4))::cint
		FROM
			generate_series(1, t) as v
	) AS v
	INTO test_composite_int4
FROM
	generate_series(1, 200) as t;


SELECT	t, smlar(v, '{"(10,1)", "(9,1)", "(8,1)", "(7,1)", "(6,1)", "(5,1)", "(4,1)", "(3,1)", "(2,1)", "(1,1)"}'::cint[]) AS s 
	FROM test_composite_int4 
	WHERE v % '{"(10,1)", "(9,1)", "(8,1)", "(7,1)", "(6,1)", "(5,1)", "(4,1)", "(3,1)", "(2,1)", "(1,1)"}'::cint[] ORDER BY s DESC, t;
SELECT	t, smlar(v, '{"(50,1)", "(49,1)", "(8,1)", "(7,1)", "(6,1)", "(5,1)", "(4,1)", "(33,1)", "(2,1)", "(1,1)"}'::cint[] AS s 
	FROM test_composite_int4 
	WHERE v % '{"(50,1)", "(49,1)", "(8,1)", "(7,1)", "(6,1)", "(5,1)", "(4,1)", "(33,1)", "(2,1)", "(1,1)"}'::cint[]) ORDER BY s DESC, t;

SELECT	t, smlar(v, '{"(10,1)", "(9,1)", "(8,1)", "(7,1)", "(6,1)", "(5,1)", "(4,1)", "(3,1)", "(2,1)", "(1,1)"}'::cint[]) AS s 
	FROM test_composite_int4 
	WHERE smlar(v, '{"(10,1)", "(9,1)", "(8,1)", "(7,1)", "(6,1)", "(5,1)", "(4,1)", "(3,1)", "(2,1)", "(1,1)"}'::cint[]) > 0.6 ORDER BY s DESC, t;
SELECT	t, smlar(v, '{"(50,1)", "(49,1)", "(8,1)", "(7,1)", "(6,1)", "(5,1)", "(4,1)", "(33,1)", "(2,1)", "(1,1)"}'::cint[]) AS s 
	FROM test_composite_int4 
	WHERE smlar(v, '{"(50,1)", "(49,1)", "(8,1)", "(7,1)", "(6,1)", "(5,1)", "(4,1)", "(33,1)", "(2,1)", "(1,1)"}'::cint[]) > 0.6 ORDER BY s DESC, t;

SELECT	t, smlar(v, '{"(10,1)", "(9,1)", "(8,1)", "(7,1)", "(6,1)", "(5,1)", "(4,1)", "(3,1)", "(2,1)", "(1,1)"}'::cint[], false) AS s 
	FROM test_composite_int4 
	WHERE smlar(v, '{"(10,1)", "(9,1)", "(8,1)", "(7,1)", "(6,1)", "(5,1)", "(4,1)", "(3,1)", "(2,1)", "(1,1)"}'::cint[], false) > 0.6 ORDER BY s DESC, t;
SELECT	t, smlar(v, '{"(50,1)", "(49,1)", "(8,1)", "(7,1)", "(6,1)", "(5,1)", "(4,1)", "(33,1)", "(2,1)", "(1,1)"}'::cint[], false) AS s 
	FROM test_composite_int4 
	WHERE smlar(v, '{"(50,1)", "(49,1)", "(8,1)", "(7,1)", "(6,1)", "(5,1)", "(4,1)", "(33,1)", "(2,1)", "(1,1)"}'::cint[], false) > 0.6 ORDER BY s DESC, t;

SELECT	t, smlar(v, '{"(10,1)", "(9,1)", "(8,1)", "(7,1)", "(6,1)", "(5,1)", "(4,1)", "(3,1)", "(2,1)", "(1,1)"}'::cint[], true) AS s 
	FROM test_composite_int4 
	WHERE smlar(v, '{"(10,1)", "(9,1)", "(8,1)", "(7,1)", "(6,1)", "(5,1)", "(4,1)", "(3,1)", "(2,1)", "(1,1)"}'::cint[], true) > 0.6 ORDER BY s DESC, t;
SELECT	t, smlar(v, '{"(50,1)", "(49,1)", "(8,1)", "(7,1)", "(6,1)", "(5,1)", "(4,1)", "(33,1)", "(2,1)", "(1,1)"}'::cint[], true) AS s 
	FROM test_composite_int4 
	WHERE smlar(v, '{"(50,1)", "(49,1)", "(8,1)", "(7,1)", "(6,1)", "(5,1)", "(4,1)", "(33,1)", "(2,1)", "(1,1)"}'::cint[], true) > 0.6 ORDER BY s DESC, t;


