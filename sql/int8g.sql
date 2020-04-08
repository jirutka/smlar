set extra_float_digits =0;
SELECT set_smlar_limit(0.3);

SET smlar.type = "tfidf";
SET smlar.stattable = "int8_stat";

UPDATE test_int8 SET v = v || '{50,50,50}'::int8[] WHERE t >=50 AND t < 60;
INSERT INTO test_int8 VALUES(201, '{50,50,50,50,50,50,50,50,50,50,50,50,50,50,50,50,50,50,50}'); 

CREATE TABLE int8_stat (
    value   int8 UNIQUE,
	ndoc    int4 NOT NULL CHECK (ndoc>0)
);

INSERT INTO int8_stat (
    SELECT
		q.w,
		count(*)
	FROM
		(SELECT array_to_col(v) AS w FROM test_int8 WHERE v IS NOT NULL)  AS q
	GROUP BY w
);

INSERT INTO int8_stat VALUES (NULL, (SELECT count(*) FROM test_int8));

DROP INDEX idx_test_int8;

SET smlar.tf_method = "n";
SET smlar.idf_plus_one = off;
SELECT smlar('{199,199,199,199,1}', '{199,1}'::int8[]);
SELECT  t, smlar(v, '{10,9,8,7,6,5,4,3,2,1}') AS s FROM test_int8 WHERE v % '{10,9,8,7,6,5,4,3,2,1}' ORDER BY s DESC, t;
SELECT  t, smlar(v, '{50,49,8,7,6,5,4,33,2,1}') AS s FROM test_int8 WHERE v % '{50,49,8,7,6,5,4,33,2,1}' ORDER BY s DESC, t;

SET smlar.tf_method = "n";
SET smlar.idf_plus_one = on;
SELECT smlar('{199,199,199,199,1}', '{199,1}'::int8[]);
SELECT  t, smlar(v, '{10,9,8,7,6,5,4,3,2,1}') AS s FROM test_int8 WHERE v % '{10,9,8,7,6,5,4,3,2,1}' ORDER BY s DESC, t;
SELECT  t, smlar(v, '{50,49,8,7,6,5,4,33,2,1}') AS s FROM test_int8 WHERE v % '{50,49,8,7,6,5,4,33,2,1}' ORDER BY s DESC, t;

SET smlar.tf_method = "log";
SET smlar.idf_plus_one = off;
SELECT smlar('{199,199,199,199,1}', '{199,1}'::int8[]);
SELECT  t, smlar(v, '{10,9,8,7,6,5,4,3,2,1}') AS s FROM test_int8 WHERE v % '{10,9,8,7,6,5,4,3,2,1}' ORDER BY s DESC, t;
SELECT  t, smlar(v, '{50,49,8,7,6,5,4,33,2,1}') AS s FROM test_int8 WHERE v % '{50,49,8,7,6,5,4,33,2,1}' ORDER BY s DESC, t;

SET smlar.tf_method = "log";
SET smlar.idf_plus_one = on;
SELECT smlar('{199,199,199,199,1}', '{199,1}'::int8[]);
SELECT  t, smlar(v, '{10,9,8,7,6,5,4,3,2,1}') AS s FROM test_int8 WHERE v % '{10,9,8,7,6,5,4,3,2,1}' ORDER BY s DESC, t;
SELECT  t, smlar(v, '{50,49,8,7,6,5,4,33,2,1}') AS s FROM test_int8 WHERE v % '{50,49,8,7,6,5,4,33,2,1}' ORDER BY s DESC, t;

SET smlar.tf_method = "const";
SET smlar.idf_plus_one = off;
SELECT smlar('{199,199,199,199,1}', '{199,1}'::int8[]);
SELECT  t, smlar(v, '{10,9,8,7,6,5,4,3,2,1}') AS s FROM test_int8 WHERE v % '{10,9,8,7,6,5,4,3,2,1}' ORDER BY s DESC, t;
SELECT  t, smlar(v, '{50,49,8,7,6,5,4,33,2,1}') AS s FROM test_int8 WHERE v % '{50,49,8,7,6,5,4,33,2,1}' ORDER BY s DESC, t;

SET smlar.tf_method = "const";
SET smlar.idf_plus_one = on;
SELECT smlar('{199,199,199,199,1}', '{199,1}'::int8[]);
SELECT  t, smlar(v, '{10,9,8,7,6,5,4,3,2,1}') AS s FROM test_int8 WHERE v % '{10,9,8,7,6,5,4,3,2,1}' ORDER BY s DESC, t;
SELECT  t, smlar(v, '{50,49,8,7,6,5,4,33,2,1}') AS s FROM test_int8 WHERE v % '{50,49,8,7,6,5,4,33,2,1}' ORDER BY s DESC, t;

CREATE INDEX idx_test_int8 ON test_int8 USING gist (v _int8_sml_ops);
SET enable_seqscan = off;

SET smlar.tf_method = "n";
SET smlar.idf_plus_one = off;
SELECT smlar('{199,199,199,199,1}', '{199,1}'::int8[]);
SELECT  t, smlar(v, '{10,9,8,7,6,5,4,3,2,1}') AS s FROM test_int8 WHERE v % '{10,9,8,7,6,5,4,3,2,1}' ORDER BY s DESC, t;
SELECT  t, smlar(v, '{50,49,8,7,6,5,4,33,2,1}') AS s FROM test_int8 WHERE v % '{50,49,8,7,6,5,4,33,2,1}' ORDER BY s DESC, t;

SET smlar.tf_method = "n";
SET smlar.idf_plus_one = on;
SELECT smlar('{199,199,199,199,1}', '{199,1}'::int8[]);
SELECT  t, smlar(v, '{10,9,8,7,6,5,4,3,2,1}') AS s FROM test_int8 WHERE v % '{10,9,8,7,6,5,4,3,2,1}' ORDER BY s DESC, t;
SELECT  t, smlar(v, '{50,49,8,7,6,5,4,33,2,1}') AS s FROM test_int8 WHERE v % '{50,49,8,7,6,5,4,33,2,1}' ORDER BY s DESC, t;

SET smlar.tf_method = "log";
SET smlar.idf_plus_one = off;
SELECT smlar('{199,199,199,199,1}', '{199,1}'::int8[]);
SELECT  t, smlar(v, '{10,9,8,7,6,5,4,3,2,1}') AS s FROM test_int8 WHERE v % '{10,9,8,7,6,5,4,3,2,1}' ORDER BY s DESC, t;
SELECT  t, smlar(v, '{50,49,8,7,6,5,4,33,2,1}') AS s FROM test_int8 WHERE v % '{50,49,8,7,6,5,4,33,2,1}' ORDER BY s DESC, t;

SET smlar.tf_method = "log";
SET smlar.idf_plus_one = on;
SELECT smlar('{199,199,199,199,1}', '{199,1}'::int8[]);
SELECT  t, smlar(v, '{10,9,8,7,6,5,4,3,2,1}') AS s FROM test_int8 WHERE v % '{10,9,8,7,6,5,4,3,2,1}' ORDER BY s DESC, t;
SELECT  t, smlar(v, '{50,49,8,7,6,5,4,33,2,1}') AS s FROM test_int8 WHERE v % '{50,49,8,7,6,5,4,33,2,1}' ORDER BY s DESC, t;

SET smlar.tf_method = "const";
SET smlar.idf_plus_one = off;
SELECT smlar('{199,199,199,199,1}', '{199,1}'::int8[]);
SELECT  t, smlar(v, '{10,9,8,7,6,5,4,3,2,1}') AS s FROM test_int8 WHERE v % '{10,9,8,7,6,5,4,3,2,1}' ORDER BY s DESC, t;
SELECT  t, smlar(v, '{50,49,8,7,6,5,4,33,2,1}') AS s FROM test_int8 WHERE v % '{50,49,8,7,6,5,4,33,2,1}' ORDER BY s DESC, t;

SET smlar.tf_method = "const";
SET smlar.idf_plus_one = on;
SELECT smlar('{199,199,199,199,1}', '{199,1}'::int8[]);
SELECT  t, smlar(v, '{10,9,8,7,6,5,4,3,2,1}') AS s FROM test_int8 WHERE v % '{10,9,8,7,6,5,4,3,2,1}' ORDER BY s DESC, t;
SELECT  t, smlar(v, '{50,49,8,7,6,5,4,33,2,1}') AS s FROM test_int8 WHERE v % '{50,49,8,7,6,5,4,33,2,1}' ORDER BY s DESC, t;

DROP INDEX idx_test_int8;
CREATE INDEX idx_test_int8 ON test_int8 USING gin (v _int8_sml_ops);
SET enable_seqscan = off;

SET smlar.tf_method = "n";
SET smlar.idf_plus_one = off;
SELECT smlar('{199,199,199,199,1}', '{199,1}'::int8[]);
SELECT  t, smlar(v, '{10,9,8,7,6,5,4,3,2,1}') AS s FROM test_int8 WHERE v % '{10,9,8,7,6,5,4,3,2,1}' ORDER BY s DESC, t;
SELECT  t, smlar(v, '{50,49,8,7,6,5,4,33,2,1}') AS s FROM test_int8 WHERE v % '{50,49,8,7,6,5,4,33,2,1}' ORDER BY s DESC, t;

SET smlar.tf_method = "n";
SET smlar.idf_plus_one = on;
SELECT smlar('{199,199,199,199,1}', '{199,1}'::int8[]);
SELECT  t, smlar(v, '{10,9,8,7,6,5,4,3,2,1}') AS s FROM test_int8 WHERE v % '{10,9,8,7,6,5,4,3,2,1}' ORDER BY s DESC, t;
SELECT  t, smlar(v, '{50,49,8,7,6,5,4,33,2,1}') AS s FROM test_int8 WHERE v % '{50,49,8,7,6,5,4,33,2,1}' ORDER BY s DESC, t;

SET smlar.tf_method = "log";
SET smlar.idf_plus_one = off;
SELECT smlar('{199,199,199,199,1}', '{199,1}'::int8[]);
SELECT  t, smlar(v, '{10,9,8,7,6,5,4,3,2,1}') AS s FROM test_int8 WHERE v % '{10,9,8,7,6,5,4,3,2,1}' ORDER BY s DESC, t;
SELECT  t, smlar(v, '{50,49,8,7,6,5,4,33,2,1}') AS s FROM test_int8 WHERE v % '{50,49,8,7,6,5,4,33,2,1}' ORDER BY s DESC, t;

SET smlar.tf_method = "log";
SET smlar.idf_plus_one = on;
SELECT smlar('{199,199,199,199,1}', '{199,1}'::int8[]);
SELECT  t, smlar(v, '{10,9,8,7,6,5,4,3,2,1}') AS s FROM test_int8 WHERE v % '{10,9,8,7,6,5,4,3,2,1}' ORDER BY s DESC, t;
SELECT  t, smlar(v, '{50,49,8,7,6,5,4,33,2,1}') AS s FROM test_int8 WHERE v % '{50,49,8,7,6,5,4,33,2,1}' ORDER BY s DESC, t;

SET smlar.tf_method = "const";
SET smlar.idf_plus_one = off;
SELECT smlar('{199,199,199,199,1}', '{199,1}'::int8[]);
SELECT  t, smlar(v, '{10,9,8,7,6,5,4,3,2,1}') AS s FROM test_int8 WHERE v % '{10,9,8,7,6,5,4,3,2,1}' ORDER BY s DESC, t;
SELECT  t, smlar(v, '{50,49,8,7,6,5,4,33,2,1}') AS s FROM test_int8 WHERE v % '{50,49,8,7,6,5,4,33,2,1}' ORDER BY s DESC, t;

SET smlar.tf_method = "const";
SET smlar.idf_plus_one = on;
SELECT smlar('{199,199,199,199,1}', '{199,1}'::int8[]);
SELECT  t, smlar(v, '{10,9,8,7,6,5,4,3,2,1}') AS s FROM test_int8 WHERE v % '{10,9,8,7,6,5,4,3,2,1}' ORDER BY s DESC, t;
SELECT  t, smlar(v, '{50,49,8,7,6,5,4,33,2,1}') AS s FROM test_int8 WHERE v % '{50,49,8,7,6,5,4,33,2,1}' ORDER BY s DESC, t;

RESET enable_seqscan;
