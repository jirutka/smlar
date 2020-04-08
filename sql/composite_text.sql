set extra_float_digits =0;
SELECT set_smlar_limit(0.6);

CREATE TYPE ctext AS (id text, w float4);
SELECT 
	t,
	ARRAY(
		SELECT 
			(ROW(v::text, v::float4))::ctext
		FROM
			generate_series(1, t) as v
	) AS v
	INTO test_composite_text
FROM
	generate_series(1, 200) as t;


SELECT	t, smlar(v, '{"(10,1)", "(9,1)", "(8,1)", "(7,1)", "(6,1)", "(5,1)", "(4,1)", "(3,1)", "(2,1)", "(1,1)"}'::ctext[]) AS s 
	FROM test_composite_text 
	WHERE v % '{"(10,1)", "(9,1)", "(8,1)", "(7,1)", "(6,1)", "(5,1)", "(4,1)", "(3,1)", "(2,1)", "(1,1)"}'::ctext[] ORDER BY s DESC, t;
SELECT	t, smlar(v, '{"(50,1)", "(49,1)", "(8,1)", "(7,1)", "(6,1)", "(5,1)", "(4,1)", "(33,1)", "(2,1)", "(1,1)"}'::ctext[]) AS s 
	FROM test_composite_text 
	WHERE v % '{"(50,1)", "(49,1)", "(8,1)", "(7,1)", "(6,1)", "(5,1)", "(4,1)", "(33,1)", "(2,1)", "(1,1)"}'::ctext[] ORDER BY s DESC, t;

SELECT	t, smlar(v, '{"(10,1)", "(9,1)", "(8,1)", "(7,1)", "(6,1)", "(5,1)", "(4,1)", "(3,1)", "(2,1)", "(1,1)"}'::ctext[]) AS s 
	FROM test_composite_text 
	WHERE smlar(v, '{"(10,1)", "(9,1)", "(8,1)", "(7,1)", "(6,1)", "(5,1)", "(4,1)", "(3,1)", "(2,1)", "(1,1)"}'::ctext[]) > 0.6 ORDER BY s DESC, t;
SELECT	t, smlar(v, '{"(50,1)", "(49,1)", "(8,1)", "(7,1)", "(6,1)", "(5,1)", "(4,1)", "(33,1)", "(2,1)", "(1,1)"}'::ctext[]) AS s 
	FROM test_composite_text 
	WHERE smlar(v, '{"(50,1)", "(49,1)", "(8,1)", "(7,1)", "(6,1)", "(5,1)", "(4,1)", "(33,1)", "(2,1)", "(1,1)"}'::ctext[]) > 0.6 ORDER BY s DESC, t;

SELECT	t, smlar(v, '{"(10,1)", "(9,1)", "(8,1)", "(7,1)", "(6,1)", "(5,1)", "(4,1)", "(3,1)", "(2,1)", "(1,1)"}'::ctext[], false) AS s 
	FROM test_composite_text 
	WHERE smlar(v, '{"(10,1)", "(9,1)", "(8,1)", "(7,1)", "(6,1)", "(5,1)", "(4,1)", "(3,1)", "(2,1)", "(1,1)"}'::ctext[], false) > 0.6 ORDER BY s DESC, t;
SELECT	t, smlar(v, '{"(50,1)", "(49,1)", "(8,1)", "(7,1)", "(6,1)", "(5,1)", "(4,1)", "(33,1)", "(2,1)", "(1,1)"}'::ctext[], false) AS s 
	FROM test_composite_text 
	WHERE smlar(v, '{"(50,1)", "(49,1)", "(8,1)", "(7,1)", "(6,1)", "(5,1)", "(4,1)", "(33,1)", "(2,1)", "(1,1)"}'::ctext[], false) > 0.6 ORDER BY s DESC, t;

SELECT	t, smlar(v, '{"(10,1)", "(9,1)", "(8,1)", "(7,1)", "(6,1)", "(5,1)", "(4,1)", "(3,1)", "(2,1)", "(1,1)"}'::ctext[], true) AS s 
	FROM test_composite_text 
	WHERE smlar(v, '{"(10,1)", "(9,1)", "(8,1)", "(7,1)", "(6,1)", "(5,1)", "(4,1)", "(3,1)", "(2,1)", "(1,1)"}'::ctext[], true) > 0.6 ORDER BY s DESC, t;
SELECT	t, smlar(v, '{"(50,1)", "(49,1)", "(8,1)", "(7,1)", "(6,1)", "(5,1)", "(4,1)", "(33,1)", "(2,1)", "(1,1)"}'::ctext[], true) AS s 
	FROM test_composite_text 
	WHERE smlar(v, '{"(50,1)", "(49,1)", "(8,1)", "(7,1)", "(6,1)", "(5,1)", "(4,1)", "(33,1)", "(2,1)", "(1,1)"}'::ctext[], true) > 0.6 ORDER BY s DESC, t;


