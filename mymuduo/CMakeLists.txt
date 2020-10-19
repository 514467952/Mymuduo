
#确定cmake版本和项目文件
cmake_minimum_required(VERSION 2.8)
project(mymduo)

#mymuduo最终编译so动态库，设置动态库的路径
set(LIBRARY_OUTPUT_PATH ${PROJECT_SOURCE_DIR}/lib)
# 设置调式信息 就是makefile中的后面的编译选项
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -g -std=c++11")

#定义参与编译的源代码文件
aux_source_directory(. SRC_LIST)
#编译生成动态库mymuduo
add_library(mymuduo SHARED ${SRC_LIST})

