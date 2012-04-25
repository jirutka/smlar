SELECT set_smlar_limit(0.8);

DROP INDEX idx_test_interval;

SELECT smlar('{199,199,199,199,1}', '{199,1}'::interval[]);
SELECT  t, smlar(v, '{10,9,8,7,6,5,4,3,2,1}') AS s FROM test_interval WHERE v % '{10,9,8,7,6,5,4,3,2,1}' ORDER BY s DESC, t;
SELECT  t, smlar(v, '{50,49,8,7,6,5,4,33,2,1}') AS s FROM test_interval WHERE v % '{50,49,8,7,6,5,4,33,2,1}' ORDER BY s DESC, t;

CREATE INDEX idx_test_interval ON test_interval USING gist (v _interval_sml_ops);
SET enable_seqscan = off;

SELECT smlar('{199,199,199,199,1}', '{199,1}'::interval[]);
SELECT  t, smlar(v, '{10,9,8,7,6,5,4,3,2,1}') AS s FROM test_interval WHERE v % '{10,9,8,7,6,5,4,3,2,1}' ORDER BY s DESC, t;
SELECT  t, smlar(v, '{50,49,8,7,6,5,4,33,2,1}') AS s FROM test_interval WHERE v % '{50,49,8,7,6,5,4,33,2,1}' ORDER BY s DESC, t;

RESET enable_seqscan;
DROP INDEX idx_test_interval;

CREATE INDEX idx_test_interval ON test_interval USING gin (v _interval_sml_ops);
SET enable_seqscan = off;

SELECT smlar('{199,199,199,199,1}', '{199,1}'::interval[]);
SELECT  t, smlar(v, '{10,9,8,7,6,5,4,3,2,1}') AS s FROM test_interval WHERE v % '{10,9,8,7,6,5,4,3,2,1}' ORDER BY s DESC, t;
SELECT  t, smlar(v, '{50,49,8,7,6,5,4,33,2,1}') AS s FROM test_interval WHERE v % '{50,49,8,7,6,5,4,33,2,1}' ORDER BY s DESC, t;

RESET enable_seqscan;
