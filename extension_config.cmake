# Extension from this repo
duckdb_extension_load(biosignals
    SOURCE_DIR ${CMAKE_CURRENT_LIST_DIR}
)

# Any extra extensions that should be built
duckdb_extension_load(httpfs
    GIT_URL https://github.com/duckdb/duckdb-httpfs
    GIT_TAG 53c5b032f6c368cfcc1a1ac3819118e86d3286a6
)