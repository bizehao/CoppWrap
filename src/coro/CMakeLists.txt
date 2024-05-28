add_library(coro INTERFACE)

find_package(Boost REQUIRED COMPONENTS context)
find_package(yoga CONFIG REQUIRED)

target_include_directories(coro INTERFACE
    $<BUILD_INTERFACE:${CMAKE_SOURCE_DIR}/include>
    $<INSTALL_INTERFACE:include>
)
#leveldb::leveldb
message("==> ${${yoga}}")
message("==> ${${Boost}}")
target_link_libraries(coro INTERFACE libcopp Async++ Boost::boost Boost::context)