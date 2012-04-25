-- complain if script is sourced in psql, rather than via CREATE EXTENSION
\echo Use "CREATE EXTENSION extension_name" to load this file. \quit

ALTER EXTENSION smlar ADD FUNCTION tsvector2textarray(tsvector);
ALTER EXTENSION smlar ADD FUNCTION array_unique(anyarray);
ALTER EXTENSION smlar ADD FUNCTION inarray(anyarray, anyelement);
ALTER EXTENSION smlar ADD FUNCTION inarray(anyarray, anyelement, float4, float4);

ALTER EXTENSION smlar ADD FUNCTION smlar(anyarray, anyarray);
ALTER EXTENSION smlar ADD FUNCTION smlar(anyarray, anyarray, bool);
ALTER EXTENSION smlar ADD FUNCTION smlar(anyarray, anyarray, text);
ALTER EXTENSION smlar ADD FUNCTION set_smlar_limit(float4);
ALTER EXTENSION smlar ADD FUNCTION show_smlar_limit();
ALTER EXTENSION smlar ADD FUNCTION smlar_op(anyarray,anyarray);
ALTER EXTENSION smlar ADD OPERATOR % (anyarray,anyarray);

ALTER EXTENSION smlar ADD FUNCTION gsmlsign_out(gsmlsign);
ALTER EXTENSION smlar ADD FUNCTION gsmlsign_in(cstring);
ALTER EXTENSION smlar ADD TYPE gsmlsign;

ALTER EXTENSION smlar ADD FUNCTION gsmlsign_consistent(gsmlsign,internal,int4);
ALTER EXTENSION smlar ADD FUNCTION gsmlsign_compress(internal);
ALTER EXTENSION smlar ADD FUNCTION gsmlsign_decompress(internal);
ALTER EXTENSION smlar ADD FUNCTION gsmlsign_penalty(internal,internal,internal);
ALTER EXTENSION smlar ADD FUNCTION gsmlsign_picksplit(internal, internal);
ALTER EXTENSION smlar ADD FUNCTION gsmlsign_union(bytea, internal);
ALTER EXTENSION smlar ADD FUNCTION gsmlsign_same(gsmlsign, gsmlsign, internal);

ALTER EXTENSION smlar ADD FUNCTION smlararrayextract(anyarray, internal);
ALTER EXTENSION smlar ADD FUNCTION smlarqueryarrayextract(anyarray, internal, internal);
ALTER EXTENSION smlar ADD FUNCTION smlararrayconsistent(internal, internal, anyarray);

ALTER EXTENSION smlar ADD OPERATOR CLASS _int2_sml_ops USING gin;
ALTER EXTENSION smlar ADD OPERATOR CLASS _int4_sml_ops USING gin;
ALTER EXTENSION smlar ADD OPERATOR CLASS _int8_sml_ops USING gin;
ALTER EXTENSION smlar ADD OPERATOR CLASS _float4_sml_ops USING gin;
ALTER EXTENSION smlar ADD OPERATOR CLASS _float8_sml_ops USING gin;
ALTER EXTENSION smlar ADD OPERATOR CLASS _money_sml_ops USING gin;
ALTER EXTENSION smlar ADD OPERATOR CLASS _oid_sml_ops USING gin;
ALTER EXTENSION smlar ADD OPERATOR CLASS _timestamp_sml_ops USING gin;
ALTER EXTENSION smlar ADD OPERATOR CLASS _timestamptz_sml_ops USING gin;
ALTER EXTENSION smlar ADD OPERATOR CLASS _time_sml_ops USING gin;
ALTER EXTENSION smlar ADD OPERATOR CLASS _timetz_sml_ops USING gin;
ALTER EXTENSION smlar ADD OPERATOR CLASS _date_sml_ops USING gin;
ALTER EXTENSION smlar ADD OPERATOR CLASS _interval_sml_ops USING gin;
ALTER EXTENSION smlar ADD OPERATOR CLASS _macaddr_sml_ops USING gin;
ALTER EXTENSION smlar ADD OPERATOR CLASS _inet_sml_ops USING gin;
ALTER EXTENSION smlar ADD OPERATOR CLASS _cidr_sml_ops USING gin;
ALTER EXTENSION smlar ADD OPERATOR CLASS _text_sml_ops USING gin;
ALTER EXTENSION smlar ADD OPERATOR CLASS _varchar_sml_ops USING gin;
ALTER EXTENSION smlar ADD OPERATOR CLASS _char_sml_ops USING gin;
ALTER EXTENSION smlar ADD OPERATOR CLASS _bytea_sml_ops USING gin;
ALTER EXTENSION smlar ADD OPERATOR CLASS _bit_sml_ops USING gin;
ALTER EXTENSION smlar ADD OPERATOR CLASS _varbit_sml_ops USING gin;
ALTER EXTENSION smlar ADD OPERATOR CLASS _numeric_sml_ops USING gin;

ALTER EXTENSION smlar ADD OPERATOR FAMILY _int2_sml_ops USING gin;
ALTER EXTENSION smlar ADD OPERATOR FAMILY _int4_sml_ops USING gin;
ALTER EXTENSION smlar ADD OPERATOR FAMILY _int8_sml_ops USING gin;
ALTER EXTENSION smlar ADD OPERATOR FAMILY _float4_sml_ops USING gin;
ALTER EXTENSION smlar ADD OPERATOR FAMILY _float8_sml_ops USING gin;
ALTER EXTENSION smlar ADD OPERATOR FAMILY _money_sml_ops USING gin;
ALTER EXTENSION smlar ADD OPERATOR FAMILY _oid_sml_ops USING gin;
ALTER EXTENSION smlar ADD OPERATOR FAMILY _timestamp_sml_ops USING gin;
ALTER EXTENSION smlar ADD OPERATOR FAMILY _timestamptz_sml_ops USING gin;
ALTER EXTENSION smlar ADD OPERATOR FAMILY _time_sml_ops USING gin;
ALTER EXTENSION smlar ADD OPERATOR FAMILY _timetz_sml_ops USING gin;
ALTER EXTENSION smlar ADD OPERATOR FAMILY _date_sml_ops USING gin;
ALTER EXTENSION smlar ADD OPERATOR FAMILY _interval_sml_ops USING gin;
ALTER EXTENSION smlar ADD OPERATOR FAMILY _macaddr_sml_ops USING gin;
ALTER EXTENSION smlar ADD OPERATOR FAMILY _inet_sml_ops USING gin;
ALTER EXTENSION smlar ADD OPERATOR FAMILY _cidr_sml_ops USING gin;
ALTER EXTENSION smlar ADD OPERATOR FAMILY _text_sml_ops USING gin;
ALTER EXTENSION smlar ADD OPERATOR FAMILY _varchar_sml_ops USING gin;
ALTER EXTENSION smlar ADD OPERATOR FAMILY _char_sml_ops USING gin;
ALTER EXTENSION smlar ADD OPERATOR FAMILY _bytea_sml_ops USING gin;
ALTER EXTENSION smlar ADD OPERATOR FAMILY _bit_sml_ops USING gin;
ALTER EXTENSION smlar ADD OPERATOR FAMILY _varbit_sml_ops USING gin;
ALTER EXTENSION smlar ADD OPERATOR FAMILY _numeric_sml_ops USING gin;

ALTER EXTENSION smlar ADD OPERATOR CLASS _int2_sml_ops USING gist;
ALTER EXTENSION smlar ADD OPERATOR CLASS _int4_sml_ops USING gist;
ALTER EXTENSION smlar ADD OPERATOR CLASS _int8_sml_ops USING gist;
ALTER EXTENSION smlar ADD OPERATOR CLASS _float4_sml_ops USING gist;
ALTER EXTENSION smlar ADD OPERATOR CLASS _float8_sml_ops USING gist;
ALTER EXTENSION smlar ADD OPERATOR CLASS _oid_sml_ops USING gist;
ALTER EXTENSION smlar ADD OPERATOR CLASS _timestamp_sml_ops USING gist;
ALTER EXTENSION smlar ADD OPERATOR CLASS _timestamptz_sml_ops USING gist;
ALTER EXTENSION smlar ADD OPERATOR CLASS _time_sml_ops USING gist;
ALTER EXTENSION smlar ADD OPERATOR CLASS _timetz_sml_ops USING gist;
ALTER EXTENSION smlar ADD OPERATOR CLASS _date_sml_ops USING gist;
ALTER EXTENSION smlar ADD OPERATOR CLASS _interval_sml_ops USING gist;
ALTER EXTENSION smlar ADD OPERATOR CLASS _macaddr_sml_ops USING gist;
ALTER EXTENSION smlar ADD OPERATOR CLASS _inet_sml_ops USING gist;
ALTER EXTENSION smlar ADD OPERATOR CLASS _cidr_sml_ops USING gist;
ALTER EXTENSION smlar ADD OPERATOR CLASS _text_sml_ops USING gist;
ALTER EXTENSION smlar ADD OPERATOR CLASS _varchar_sml_ops USING gist;
ALTER EXTENSION smlar ADD OPERATOR CLASS _char_sml_ops USING gist;
ALTER EXTENSION smlar ADD OPERATOR CLASS _bytea_sml_ops USING gist;
ALTER EXTENSION smlar ADD OPERATOR CLASS _numeric_sml_ops USING gist;

ALTER EXTENSION smlar ADD OPERATOR FAMILY _int2_sml_ops USING gist;
ALTER EXTENSION smlar ADD OPERATOR FAMILY _int4_sml_ops USING gist;
ALTER EXTENSION smlar ADD OPERATOR FAMILY _int8_sml_ops USING gist;
ALTER EXTENSION smlar ADD OPERATOR FAMILY _float4_sml_ops USING gist;
ALTER EXTENSION smlar ADD OPERATOR FAMILY _float8_sml_ops USING gist;
ALTER EXTENSION smlar ADD OPERATOR FAMILY _oid_sml_ops USING gist;
ALTER EXTENSION smlar ADD OPERATOR FAMILY _timestamp_sml_ops USING gist;
ALTER EXTENSION smlar ADD OPERATOR FAMILY _timestamptz_sml_ops USING gist;
ALTER EXTENSION smlar ADD OPERATOR FAMILY _time_sml_ops USING gist;
ALTER EXTENSION smlar ADD OPERATOR FAMILY _timetz_sml_ops USING gist;
ALTER EXTENSION smlar ADD OPERATOR FAMILY _date_sml_ops USING gist;
ALTER EXTENSION smlar ADD OPERATOR FAMILY _interval_sml_ops USING gist;
ALTER EXTENSION smlar ADD OPERATOR FAMILY _macaddr_sml_ops USING gist;
ALTER EXTENSION smlar ADD OPERATOR FAMILY _inet_sml_ops USING gist;
ALTER EXTENSION smlar ADD OPERATOR FAMILY _cidr_sml_ops USING gist;
ALTER EXTENSION smlar ADD OPERATOR FAMILY _text_sml_ops USING gist;
ALTER EXTENSION smlar ADD OPERATOR FAMILY _varchar_sml_ops USING gist;
ALTER EXTENSION smlar ADD OPERATOR FAMILY _char_sml_ops USING gist;
ALTER EXTENSION smlar ADD OPERATOR FAMILY _bytea_sml_ops USING gist;
ALTER EXTENSION smlar ADD OPERATOR FAMILY _numeric_sml_ops USING gist;




