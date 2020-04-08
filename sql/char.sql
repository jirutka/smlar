set extra_float_digits =0;
SELECT set_smlar_limit(0.6);

SELECT 
	t,
	ARRAY(
		SELECT 
			v::varchar(1)::"char"
		FROM
			generate_series(1, t) as v
	) AS v
	INTO test_char
FROM
	generate_series(1, 200) as t;


SELECT	t, smlar(v, '{3,2,1}') AS s FROM test_char WHERE v % '{3,2,1}' ORDER BY s DESC, t;

CREATE INDEX idx_test_char ON test_char USING gist (v _char_sml_ops);

SET enable_seqscan=off;

SELECT	t, smlar(v, '{3,2,1}') AS s FROM test_char WHERE v % '{3,2,1}' ORDER BY s DESC, t;

DROP INDEX idx_test_char;
CREATE INDEX idx_test_char ON test_char USING gin (v _char_sml_ops);

SELECT  t, smlar(v, '{3,2,1}') AS s FROM test_char WHERE v % '{3,2,1}' ORDER BY s DESC, t;

SET enable_seqscan=on;

