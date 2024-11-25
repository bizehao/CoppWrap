# CMakeList.txt: CMakeProject1 的 CMake 项目，在此处包括源代码并定义
# 项目特定的逻辑。
#

function(custom_test cpp_file)
	# 将源代码添加到此项目的可执行文件。
	add_executable (${cpp_file} "${cpp_file}.cpp" "test2.cpp")

	set_property(TARGET ${cpp_file} PROPERTY CXX_STANDARD 17)

	target_link_libraries(${cpp_file} PRIVATE coro)

endfunction()

custom_test(test1)
custom_test(test2)



# TODO: 如有需要，请添加测试并安装目标。
