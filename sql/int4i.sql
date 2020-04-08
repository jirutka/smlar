set extra_float_digits =0;
SELECT set_smlar_limit(5.0);
SET smlar.type='overlap';

DROP INDEX idx_test_int4;

SELECT smlar('{199,199,199,199,1}', '{199,1}'::int4[]);
SELECT  t, smlar(v, '{199,198,197,196,195,194,193,192,191,190}') AS s FROM test_int4 WHERE v % '{199,198,197,196,195,194,193,192,191,190}' ORDER BY s DESC, t;

CREATE INDEX idx_test_int4 ON test_int4 USING gin (v _int4_sml_ops);
SET enable_seqscan = off;

SELECT  t, smlar(v, '{199,198,197,196,195,194,193,192,191,190}') AS s FROM test_int4 WHERE v % '{199,198,197,196,195,194,193,192,191,190}' ORDER BY s DESC, t;

RESET enable_seqscan;
