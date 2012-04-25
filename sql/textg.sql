SELECT set_smlar_limit(0.3);

SET smlar.type = "tfidf";
SET smlar.stattable = "text_stat";

UPDATE test_text SET v = v || '{50,50,50}'::text[] WHERE t >=50 AND t < 60;
INSERT INTO test_text VALUES(201, '{50,50,50,50,50,50,50,50,50,50,50,50,50,50,50,50,50,50,50}'); 

CREATE TABLE text_stat (
    value   text UNIQUE,
	ndoc    int4 NOT NULL CHECK (ndoc>0)
);

INSERT INTO text_stat (
    SELECT
		q.w,
		count(*)
	FROM
		(SELECT array_to_col(v) AS w FROM test_text WHERE v IS NOT NULL)  AS q
	GROUP BY w
);

INSERT INTO text_stat VALUES (NULL, (SELECT count(*) FROM test_text));

DROP INDEX idx_test_text;

SET smlar.tf_method = "n";
SET smlar.idf_plus_one = off;
SELECT smlar('{199,199,199,199,1}', '{199,1}'::text[]);
SELECT  t, smlar(v, '{10,9,8,7,6,5,4,3,2,1}') AS s FROM test_text WHERE v % '{10,9,8,7,6,5,4,3,2,1}' ORDER BY s DESC, t;
SELECT  t, smlar(v, '{50,49,8,7,6,5,4,33,2,1}') AS s FROM test_text WHERE v % '{50,49,8,7,6,5,4,33,2,1}' ORDER BY s DESC, t;

SET smlar.tf_method = "n";
SET smlar.idf_plus_one = on;
SELECT smlar('{199,199,199,199,1}', '{199,1}'::text[]);
SELECT  t, smlar(v, '{10,9,8,7,6,5,4,3,2,1}') AS s FROM test_text WHERE v % '{10,9,8,7,6,5,4,3,2,1}' ORDER BY s DESC, t;
SELECT  t, smlar(v, '{50,49,8,7,6,5,4,33,2,1}') AS s FROM test_text WHERE v % '{50,49,8,7,6,5,4,33,2,1}' ORDER BY s DESC, t;

SET smlar.tf_method = "log";
SET smlar.idf_plus_one = off;
SELECT smlar('{199,199,199,199,1}', '{199,1}'::text[]);
SELECT  t, smlar(v, '{10,9,8,7,6,5,4,3,2,1}') AS s FROM test_text WHERE v % '{10,9,8,7,6,5,4,3,2,1}' ORDER BY s DESC, t;
SELECT  t, smlar(v, '{50,49,8,7,6,5,4,33,2,1}') AS s FROM test_text WHERE v % '{50,49,8,7,6,5,4,33,2,1}' ORDER BY s DESC, t;

SET smlar.tf_method = "log";
SET smlar.idf_plus_one = on;
SELECT smlar('{199,199,199,199,1}', '{199,1}'::text[]);
SELECT  t, smlar(v, '{10,9,8,7,6,5,4,3,2,1}') AS s FROM test_text WHERE v % '{10,9,8,7,6,5,4,3,2,1}' ORDER BY s DESC, t;
SELECT  t, smlar(v, '{50,49,8,7,6,5,4,33,2,1}') AS s FROM test_text WHERE v % '{50,49,8,7,6,5,4,33,2,1}' ORDER BY s DESC, t;

SET smlar.tf_method = "const";
SET smlar.idf_plus_one = off;
SELECT smlar('{199,199,199,199,1}', '{199,1}'::text[]);
SELECT  t, smlar(v, '{10,9,8,7,6,5,4,3,2,1}') AS s FROM test_text WHERE v % '{10,9,8,7,6,5,4,3,2,1}' ORDER BY s DESC, t;
SELECT  t, smlar(v, '{50,49,8,7,6,5,4,33,2,1}') AS s FROM test_text WHERE v % '{50,49,8,7,6,5,4,33,2,1}' ORDER BY s DESC, t;

SET smlar.tf_method = "const";
SET smlar.idf_plus_one = on;
SELECT smlar('{199,199,199,199,1}', '{199,1}'::text[]);
SELECT  t, smlar(v, '{10,9,8,7,6,5,4,3,2,1}') AS s FROM test_text WHERE v % '{10,9,8,7,6,5,4,3,2,1}' ORDER BY s DESC, t;
SELECT  t, smlar(v, '{50,49,8,7,6,5,4,33,2,1}') AS s FROM test_text WHERE v % '{50,49,8,7,6,5,4,33,2,1}' ORDER BY s DESC, t;

CREATE INDEX idx_test_text ON test_text USING gist (v _text_sml_ops);
SET enable_seqscan = off;

SET smlar.tf_method = "n";
SET smlar.idf_plus_one = off;
SELECT smlar('{199,199,199,199,1}', '{199,1}'::text[]);
SELECT  t, smlar(v, '{10,9,8,7,6,5,4,3,2,1}') AS s FROM test_text WHERE v % '{10,9,8,7,6,5,4,3,2,1}' ORDER BY s DESC, t;
SELECT  t, smlar(v, '{50,49,8,7,6,5,4,33,2,1}') AS s FROM test_text WHERE v % '{50,49,8,7,6,5,4,33,2,1}' ORDER BY s DESC, t;

SET smlar.tf_method = "n";
SET smlar.idf_plus_one = on;
SELECT smlar('{199,199,199,199,1}', '{199,1}'::text[]);
SELECT  t, smlar(v, '{10,9,8,7,6,5,4,3,2,1}') AS s FROM test_text WHERE v % '{10,9,8,7,6,5,4,3,2,1}' ORDER BY s DESC, t;
SELECT  t, smlar(v, '{50,49,8,7,6,5,4,33,2,1}') AS s FROM test_text WHERE v % '{50,49,8,7,6,5,4,33,2,1}' ORDER BY s DESC, t;

SET smlar.tf_method = "log";
SET smlar.idf_plus_one = off;
SELECT smlar('{199,199,199,199,1}', '{199,1}'::text[]);
SELECT  t, smlar(v, '{10,9,8,7,6,5,4,3,2,1}') AS s FROM test_text WHERE v % '{10,9,8,7,6,5,4,3,2,1}' ORDER BY s DESC, t;
SELECT  t, smlar(v, '{50,49,8,7,6,5,4,33,2,1}') AS s FROM test_text WHERE v % '{50,49,8,7,6,5,4,33,2,1}' ORDER BY s DESC, t;

SET smlar.tf_method = "log";
SET smlar.idf_plus_one = on;
SELECT smlar('{199,199,199,199,1}', '{199,1}'::text[]);
SELECT  t, smlar(v, '{10,9,8,7,6,5,4,3,2,1}') AS s FROM test_text WHERE v % '{10,9,8,7,6,5,4,3,2,1}' ORDER BY s DESC, t;
SELECT  t, smlar(v, '{50,49,8,7,6,5,4,33,2,1}') AS s FROM test_text WHERE v % '{50,49,8,7,6,5,4,33,2,1}' ORDER BY s DESC, t;

SET smlar.tf_method = "const";
SET smlar.idf_plus_one = off;
SELECT smlar('{199,199,199,199,1}', '{199,1}'::text[]);
SELECT  t, smlar(v, '{10,9,8,7,6,5,4,3,2,1}') AS s FROM test_text WHERE v % '{10,9,8,7,6,5,4,3,2,1}' ORDER BY s DESC, t;
SELECT  t, smlar(v, '{50,49,8,7,6,5,4,33,2,1}') AS s FROM test_text WHERE v % '{50,49,8,7,6,5,4,33,2,1}' ORDER BY s DESC, t;

SET smlar.tf_method = "const";
SET smlar.idf_plus_one = on;
SELECT smlar('{199,199,199,199,1}', '{199,1}'::text[]);
SELECT  t, smlar(v, '{10,9,8,7,6,5,4,3,2,1}') AS s FROM test_text WHERE v % '{10,9,8,7,6,5,4,3,2,1}' ORDER BY s DESC, t;
SELECT  t, smlar(v, '{50,49,8,7,6,5,4,33,2,1}') AS s FROM test_text WHERE v % '{50,49,8,7,6,5,4,33,2,1}' ORDER BY s DESC, t;

DROP INDEX idx_test_text;
CREATE INDEX idx_test_text ON test_text USING gin (v _text_sml_ops);
SET enable_seqscan = off;

SET smlar.tf_method = "n";
SET smlar.idf_plus_one = off;
SELECT smlar('{199,199,199,199,1}', '{199,1}'::text[]);
SELECT  t, smlar(v, '{10,9,8,7,6,5,4,3,2,1}') AS s FROM test_text WHERE v % '{10,9,8,7,6,5,4,3,2,1}' ORDER BY s DESC, t;
SELECT  t, smlar(v, '{50,49,8,7,6,5,4,33,2,1}') AS s FROM test_text WHERE v % '{50,49,8,7,6,5,4,33,2,1}' ORDER BY s DESC, t;

SET smlar.tf_method = "n";
SET smlar.idf_plus_one = on;
SELECT smlar('{199,199,199,199,1}', '{199,1}'::text[]);
SELECT  t, smlar(v, '{10,9,8,7,6,5,4,3,2,1}') AS s FROM test_text WHERE v % '{10,9,8,7,6,5,4,3,2,1}' ORDER BY s DESC, t;
SELECT  t, smlar(v, '{50,49,8,7,6,5,4,33,2,1}') AS s FROM test_text WHERE v % '{50,49,8,7,6,5,4,33,2,1}' ORDER BY s DESC, t;

SET smlar.tf_method = "log";
SET smlar.idf_plus_one = off;
SELECT smlar('{199,199,199,199,1}', '{199,1}'::text[]);
SELECT  t, smlar(v, '{10,9,8,7,6,5,4,3,2,1}') AS s FROM test_text WHERE v % '{10,9,8,7,6,5,4,3,2,1}' ORDER BY s DESC, t;
SELECT  t, smlar(v, '{50,49,8,7,6,5,4,33,2,1}') AS s FROM test_text WHERE v % '{50,49,8,7,6,5,4,33,2,1}' ORDER BY s DESC, t;

SET smlar.tf_method = "log";
SET smlar.idf_plus_one = on;
SELECT smlar('{199,199,199,199,1}', '{199,1}'::text[]);
SELECT  t, smlar(v, '{10,9,8,7,6,5,4,3,2,1}') AS s FROM test_text WHERE v % '{10,9,8,7,6,5,4,3,2,1}' ORDER BY s DESC, t;
SELECT  t, smlar(v, '{50,49,8,7,6,5,4,33,2,1}') AS s FROM test_text WHERE v % '{50,49,8,7,6,5,4,33,2,1}' ORDER BY s DESC, t;

SET smlar.tf_method = "const";
SET smlar.idf_plus_one = off;
SELECT smlar('{199,199,199,199,1}', '{199,1}'::text[]);
SELECT  t, smlar(v, '{10,9,8,7,6,5,4,3,2,1}') AS s FROM test_text WHERE v % '{10,9,8,7,6,5,4,3,2,1}' ORDER BY s DESC, t;
SELECT  t, smlar(v, '{50,49,8,7,6,5,4,33,2,1}') AS s FROM test_text WHERE v % '{50,49,8,7,6,5,4,33,2,1}' ORDER BY s DESC, t;

SET smlar.tf_method = "const";
SET smlar.idf_plus_one = on;
SELECT smlar('{199,199,199,199,1}', '{199,1}'::text[]);
SELECT  t, smlar(v, '{10,9,8,7,6,5,4,3,2,1}') AS s FROM test_text WHERE v % '{10,9,8,7,6,5,4,3,2,1}' ORDER BY s DESC, t;
SELECT  t, smlar(v, '{50,49,8,7,6,5,4,33,2,1}') AS s FROM test_text WHERE v % '{50,49,8,7,6,5,4,33,2,1}' ORDER BY s DESC, t;

RESET enable_seqscan;
