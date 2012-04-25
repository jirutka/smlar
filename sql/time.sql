SELECT set_smlar_limit(0.6);

SELECT 
	t,
	ARRAY(
		SELECT 
			epoch2timestamp(v)::time
		FROM
			generate_series(1, t) as v
	) AS v
	INTO test_time
FROM
	generate_series(1, 200) as t;


SELECT	t, smlar(v, to_tsp_array('{10,9,8,7,6,5,4,3,2,1}')::time[]) AS s FROM test_time WHERE v % to_tsp_array('{10,9,8,7,6,5,4,3,2,1}')::time[] ORDER BY s DESC, t;
SELECT	t, smlar(v, to_tsp_array('{50,49,8,7,6,5,4,33,2,1}')::time[]) AS s FROM test_time WHERE v % to_tsp_array('{50,49,8,7,6,5,4,33,2,1}')::time[] ORDER BY s DESC, t;

CREATE INDEX idx_test_time ON test_time USING gist (v _time_sml_ops);

SET enable_seqscan=off;

SELECT	t, smlar(v, to_tsp_array('{10,9,8,7,6,5,4,3,2,1}')::time[]) AS s FROM test_time WHERE v % to_tsp_array('{10,9,8,7,6,5,4,3,2,1}')::time[] ORDER BY s DESC, t;
SELECT	t, smlar(v, to_tsp_array('{50,49,8,7,6,5,4,33,2,1}')::time[]) AS s FROM test_time WHERE v % to_tsp_array('{50,49,8,7,6,5,4,33,2,1}')::time[] ORDER BY s DESC, t;

DROP INDEX idx_test_time;
CREATE INDEX idx_test_time ON test_time USING gin (v _time_sml_ops);

SELECT	t, smlar(v, to_tsp_array('{10,9,8,7,6,5,4,3,2,1}')::time[]) AS s FROM test_time WHERE v % to_tsp_array('{10,9,8,7,6,5,4,3,2,1}')::time[] ORDER BY s DESC, t;
SELECT	t, smlar(v, to_tsp_array('{50,49,8,7,6,5,4,33,2,1}')::time[]) AS s FROM test_time WHERE v % to_tsp_array('{50,49,8,7,6,5,4,33,2,1}')::time[] ORDER BY s DESC, t;

SET enable_seqscan=on;

