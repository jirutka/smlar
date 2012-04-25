SELECT set_smlar_limit(0.6);

SELECT 
	t,
	ARRAY(
		SELECT 
			epoch2timestamp(v)::date
		FROM
			generate_series(1, t) as v
	) AS v
	INTO test_date
FROM
	generate_series(1, 200) as t;


SELECT	t, smlar(v, to_tsp_array('{10,9,8,7,6,5,4,3,2,1}')::date[]) AS s FROM test_date WHERE v % to_tsp_array('{10,9,8,7,6,5,4,3,2,1}')::date[] ORDER BY s DESC, t;
SELECT	t, smlar(v, to_tsp_array('{50,49,8,7,6,5,4,33,2,1}')::date[]) AS s FROM test_date WHERE v % to_tsp_array('{50,49,8,7,6,5,4,33,2,1}')::date[] ORDER BY s DESC, t;

CREATE INDEX idx_test_date ON test_date USING gist (v _date_sml_ops);

SET enable_seqscan=off;

SELECT	t, smlar(v, to_tsp_array('{10,9,8,7,6,5,4,3,2,1}')::date[]) AS s FROM test_date WHERE v % to_tsp_array('{10,9,8,7,6,5,4,3,2,1}')::date[] ORDER BY s DESC, t;
SELECT	t, smlar(v, to_tsp_array('{50,49,8,7,6,5,4,33,2,1}')::date[]) AS s FROM test_date WHERE v % to_tsp_array('{50,49,8,7,6,5,4,33,2,1}')::date[] ORDER BY s DESC, t;

DROP INDEX idx_test_date;
CREATE INDEX idx_test_date ON test_date USING gin (v _date_sml_ops);

SELECT	t, smlar(v, to_tsp_array('{10,9,8,7,6,5,4,3,2,1}')::date[]) AS s FROM test_date WHERE v % to_tsp_array('{10,9,8,7,6,5,4,3,2,1}')::date[] ORDER BY s DESC, t;
SELECT	t, smlar(v, to_tsp_array('{50,49,8,7,6,5,4,33,2,1}')::date[]) AS s FROM test_date WHERE v % to_tsp_array('{50,49,8,7,6,5,4,33,2,1}')::date[] ORDER BY s DESC, t;

SET enable_seqscan=on;

