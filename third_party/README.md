# Vendored dependencies

`miniz/` contains the unmodified upstream read/decompression sources from
[richgel999/miniz 3.1.0](https://github.com/richgel999/miniz/tree/3.1.0), under the
included MIT license. `SOURCE.json` records the downloaded files' SHA256 values.
`miniz_export.h` is the local static-library export configuration.

The runtime links a private static target, with deflate/writer APIs disabled.
HustMCL checks archive paths, entry types, ambiguity and size limits before
extracting selected files; extraction uses caller-owned destinations rather
than miniz's filename-based directory extraction APIs. CRC checking is retained.
