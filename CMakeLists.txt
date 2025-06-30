cmake_minimum_required(VERSION 2.10)
project(mymuduo)

# mymuduo 最终编译生成.so动态库 设置动态库的路径 放在根目录下的lib文件夹下
set(LIBRARY_OUTPUT_PATH ${PROJECT_SOURCE_DIR}/lib)

# 设置调试信息
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -g -std=c++11")

# 定义参与编译的源文件 .代表全部文件
aux_source_directory(. SRC_LIST)

# 编译生成动态库
add_library(mymuduo SHARED ${SRC_LIST})  # aux_source_directory(<dir> <variable>)




