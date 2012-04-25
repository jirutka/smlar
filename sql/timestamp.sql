SELECT set_smlar_limit(0.6);

SELECT 
	t,
	ARRAY(
		SELECT 
			epoch2timestamp(v)::timestamp
		FROM
			generate_series(1, t) as v
	) AS v
	INTO test_timestamp
FROM
	generate_series(1, 200) as t;


SELECT	t, smlar(v, to_tsp_array('{10,9,8,7,6,5,4,3,2,1}')) AS s FROM test_timestamp WHERE v % to_tsp_array('{10,9,8,7,6,5,4,3,2,1}') ORDER BY s DESC, t;
SELECT	t, smlar(v, to_tsp_array('{50,49,8,7,6,5,4,33,2,1}')) AS s FROM test_timestamp WHERE v % to_tsp_array('{50,49,8,7,6,5,4,33,2,1}') ORDER BY s DESC, t;

CREATE INDEX idx_test_timestamp ON test_timestamp USING gist (v _timestamp_sml_ops);

SET enable_seqscan=off;

SELECT	t, smlar(v, to_tsp_array('{10,9,8,7,6,5,4,3,2,1}')) AS s FROM test_timestamp WHERE v % to_tsp_array('{10,9,8,7,6,5,4,3,2,1}') ORDER BY s DESC, t;
SELECT	t, smlar(v, to_tsp_array('{50,49,8,7,6,5,4,33,2,1}')) AS s FROM test_timestamp WHERE v % to_tsp_array('{50,49,8,7,6,5,4,33,2,1}') ORDER BY s DESC, t;

DROP INDEX idx_test_timestamp;
CREATE INDEX idx_test_timestamp ON test_timestamp USING gin (v _timestamp_sml_ops);

SELECT	t, smlar(v, to_tsp_array('{10,9,8,7,6,5,4,3,2,1}')) AS s FROM test_timestamp WHERE v % to_tsp_array('{10,9,8,7,6,5,4,3,2,1}') ORDER BY s DESC, t;
SELECT	t, smlar(v, to_tsp_array('{50,49,8,7,6,5,4,33,2,1}')) AS s FROM test_timestamp WHERE v % to_tsp_array('{50,49,8,7,6,5,4,33,2,1}') ORDER BY s DESC, t;


SET enable_seqscan=on;

