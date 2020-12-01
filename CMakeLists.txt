# CMake 最低版本号要求
 cmake_minimum_required(VERSION 2.8)

 # 项目信息
 project("GoodsMatch")

 # 添加c++编译选项
 add_compile_options(-std=c++17)
 # no debug模式
#  ADD_DEFINITIONS(-DNDEBUG)

 # include路径
 include_directories(
 ${PROJECT_SOURCE_DIR}/
 ${PROJECT_SOURCE_DIR}/src/comm
 ${PROJECT_SOURCE_DIR}/src/buyer
 ${PROJECT_SOURCE_DIR}/src/seller
 ${PROJECT_SOURCE_DIR}/src/simlater
 ${PROJECT_SOURCE_DIR}/src/judger
 )

 # 设置可执行文件生成路径
 set(EXECUTABLE_OUTPUT_PATH ${PROJECT_BINARY_DIR}/../bin)

 # 添加所有源文件
 aux_source_directory(. DIR_SRCS)
 aux_source_directory(src/comm DIR_SRCS)
 aux_source_directory(src/buyer DIR_SRCS)
 aux_source_directory(src/seller DIR_SRCS)
 aux_source_directory(src/simulater DIR_SRCS)
 aux_source_directory(src/judger DIR_SRCS)

 # 指定生成目标
 add_executable("GoodsMatch" ${DIR_SRCS})

