# 要求最低Cmake版本
cmake_minimum_required(VERSION 3.15)
# 项目名称
project(RGS LANGUAGES CXX)
# C++标准
set(CMAKE_CXX_STANDARD 17)

#设置多个变量

set(HEADERS

)

set(SOURCES
   "src/Main.cpp"
   "src/Application.cpp"
)

set(TARGET RGS)
#添加运行程序 打开变量
#${}替换宏
add_executable(${TARGET} ${HEADERS} ${SOURCES})