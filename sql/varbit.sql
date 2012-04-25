SELECT set_smlar_limit(0.6);

SELECT 
	t,
	ARRAY(
		SELECT 
			v::bit(10)::varbit
		FROM
			generate_series(1, t) as v
	) AS v
	INTO test_varbit
FROM
	generate_series(1, 200) as t;


SELECT	t, smlar(v, '{10,9,8,7,6,5,4,3,2,1}'::int4[]::bit(10)[]::varbit[]) AS s FROM test_varbit WHERE v % '{10,9,8,7,6,5,4,3,2,1}'::int4[]::bit(10)[]::varbit[] ORDER BY s DESC, t;
SELECT	t, smlar(v, '{50,49,8,7,6,5,4,33,2,1}'::int4[]::bit(10)[]::varbit[]) AS s FROM test_varbit WHERE v % '{50,49,8,7,6,5,4,33,2,1}'::int4[]::bit(10)[]::varbit[] ORDER BY s DESC, t;

CREATE INDEX idx_test_varbit ON test_varbit USING gin (v _varbit_sml_ops);

SET enable_seqscan=off;

SELECT	t, smlar(v, '{10,9,8,7,6,5,4,3,2,1}'::int4[]::bit(10)[]::varbit[]) AS s FROM test_varbit WHERE v % '{10,9,8,7,6,5,4,3,2,1}'::int4[]::bit(10)[]::varbit[] ORDER BY s DESC, t;
SELECT	t, smlar(v, '{50,49,8,7,6,5,4,33,2,1}'::int4[]::bit(10)[]::varbit[]) AS s FROM test_varbit WHERE v % '{50,49,8,7,6,5,4,33,2,1}'::int4[]::bit(10)[]::varbit[] ORDER BY s DESC, t;

SET enable_seqscan=on;

