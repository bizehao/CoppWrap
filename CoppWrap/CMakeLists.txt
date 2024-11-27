# CMakeList.txt: CMakeProject1 的 CMake 项目，在此处包括源代码并定义
# 项目特定的逻辑。
#

# 将源代码添加到此项目的可执行文件。
add_executable (CoppWrap "CoppWrap.cpp" "CoppWrap.h")

set_property(TARGET CoppWrap PROPERTY CXX_STANDARD 17)

target_link_libraries(CoppWrap PRIVATE coro)

# TODO: 如有需要，请添加测试并安装目标。
