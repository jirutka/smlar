SELECT set_smlar_limit(0.6);

SELECT 
	t,
	ARRAY(
		SELECT 
			epoch2timestamp(v)::timestamptz::timetz
		FROM
			generate_series(1, t) as v
	) AS v
	INTO test_timetz
FROM
	generate_series(1, 200) as t;


SELECT	t, smlar(v, to_tsp_array('{10,9,8,7,6,5,4,3,2,1}')::timestamptz[]::timetz[]) AS s FROM test_timetz WHERE v % to_tsp_array('{10,9,8,7,6,5,4,3,2,1}')::timestamptz[]::timetz[] ORDER BY s DESC, t;
SELECT	t, smlar(v, to_tsp_array('{50,49,8,7,6,5,4,33,2,1}')::timestamptz[]::timetz[]) AS s FROM test_timetz WHERE v % to_tsp_array('{50,49,8,7,6,5,4,33,2,1}')::timestamptz[]::timetz[] ORDER BY s DESC, t;

CREATE INDEX idx_test_timetz ON test_timetz USING gist (v _timetz_sml_ops);

SET enable_seqscan=off;

SELECT	t, smlar(v, to_tsp_array('{10,9,8,7,6,5,4,3,2,1}')::timestamptz[]::timetz[]) AS s FROM test_timetz WHERE v % to_tsp_array('{10,9,8,7,6,5,4,3,2,1}')::timestamptz[]::timetz[] ORDER BY s DESC, t;
SELECT	t, smlar(v, to_tsp_array('{50,49,8,7,6,5,4,33,2,1}')::timestamptz[]::timetz[]) AS s FROM test_timetz WHERE v % to_tsp_array('{50,49,8,7,6,5,4,33,2,1}')::timestamptz[]::timetz[] ORDER BY s DESC, t;

DROP INDEX idx_test_timetz;
CREATE INDEX idx_test_timetz ON test_timetz USING gin (v _timetz_sml_ops);

SELECT	t, smlar(v, to_tsp_array('{10,9,8,7,6,5,4,3,2,1}')::timestamptz[]::timetz[]) AS s FROM test_timetz WHERE v % to_tsp_array('{10,9,8,7,6,5,4,3,2,1}')::timestamptz[]::timetz[] ORDER BY s DESC, t;
SELECT	t, smlar(v, to_tsp_array('{50,49,8,7,6,5,4,33,2,1}')::timestamptz[]::timetz[]) AS s FROM test_timetz WHERE v % to_tsp_array('{50,49,8,7,6,5,4,33,2,1}')::timestamptz[]::timetz[] ORDER BY s DESC, t;

SET enable_seqscan=on;

