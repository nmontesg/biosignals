# Extension from this repo
duckdb_extension_load(biosignals
    SOURCE_DIR ${CMAKE_CURRENT_LIST_DIR}
)

# Any extra extensions that should be built
duckdb_extension_load(httpfs
    GIT_URL https://github.com/duckdb/duckdb-httpfs
    GIT_TAG 7773e83d5151d8d38ad22f94cec5bf5ff63000ec
)
