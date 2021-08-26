cmake_minimum_required (VERSION 2.8)

project("sqlparser")

#源文件
file(GLOB ${PROJECT_NAME}_src "${CMAKE_CURRENT_SOURCE_DIR}/*.cpp")

#添加静态库
add_library(${PROJECT_NAME} STATIC ${${PROJECT_NAME}_src})

# 为windows添加宏
if (CMAKE_SYSTEM_NAME MATCHES "Windows")

else()
	add_definitions(-fPIC)

endif()
