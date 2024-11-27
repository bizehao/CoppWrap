add_library(coro INTERFACE)

target_include_directories(coro INTERFACE
    $<BUILD_INTERFACE:${CMAKE_SOURCE_DIR}/include>
    $<INSTALL_INTERFACE:include>
)

target_link_libraries(coro INTERFACE Async++ noboost_context)