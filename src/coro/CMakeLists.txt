add_library(coro INTERFACE)

target_include_directories(coro INTERFACE
    $<BUILD_INTERFACE:${CMAKE_SOURCE_DIR}/include>
    $<INSTALL_INTERFACE:include>
)
#leveldb::leveldb
message("==> ${${yoga}}")
message("==> ${${Boost}}")
target_link_libraries(coro INTERFACE Async++ boost_context)