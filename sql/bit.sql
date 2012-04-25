SELECT set_smlar_limit(0.6);

SELECT 
	t,
	ARRAY(
		SELECT 
			v::int4::bit(10)
		FROM
			generate_series(1, t) as v
	) AS v
	INTO test_bit
FROM
	generate_series(1, 200) as t;


SELECT	t, smlar(v, '{10,9,8,7,6,5,4,3,2,1}'::int4[]::bit(10)[]) AS s FROM test_bit WHERE v % '{10,9,8,7,6,5,4,3,2,1}'::int4[]::bit(10)[] ORDER BY s DESC, t;
SELECT	t, smlar(v, '{50,49,8,7,6,5,4,33,2,1}'::int4[]::bit(10)[]) AS s FROM test_bit WHERE v % '{50,49,8,7,6,5,4,33,2,1}'::int4[]::bit(10)[] ORDER BY s DESC, t;

CREATE INDEX idx_test_bit ON test_bit USING gin (v _bit_sml_ops);

SET enable_seqscan=off;

SELECT	t, smlar(v, '{10,9,8,7,6,5,4,3,2,1}'::int4[]::bit(10)[]) AS s FROM test_bit WHERE v % '{10,9,8,7,6,5,4,3,2,1}'::int4[]::bit(10)[] ORDER BY s DESC, t;
SELECT	t, smlar(v, '{50,49,8,7,6,5,4,33,2,1}'::int4[]::bit(10)[]) AS s FROM test_bit WHERE v % '{50,49,8,7,6,5,4,33,2,1}'::int4[]::bit(10)[] ORDER BY s DESC, t;


SET enable_seqscan=on;

