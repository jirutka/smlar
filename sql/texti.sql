set extra_float_digits =0;
SELECT set_smlar_limit(5.0);
SET smlar.type='overlap';

DROP INDEX idx_test_text;

SELECT smlar('{199,199,199,199,1}', '{199,1}'::text[]);
SELECT  t, smlar(v, '{199,198,197,196,195,194,193,192,191,190}') AS s FROM test_text WHERE v % '{199,198,197,196,195,194,193,192,191,190}' ORDER BY s DESC, t;

CREATE INDEX idx_test_text ON test_text USING gin (v _text_sml_ops);
SET enable_seqscan = off;

SELECT  t, smlar(v, '{199,198,197,196,195,194,193,192,191,190}') AS s FROM test_text WHERE v % '{199,198,197,196,195,194,193,192,191,190}' ORDER BY s DESC, t;

RESET enable_seqscan;
