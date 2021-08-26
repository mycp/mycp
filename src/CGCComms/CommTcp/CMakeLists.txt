cmake_minimum_required (VERSION 2.8)

project("CommTcp")

#源文件
file(GLOB ${PROJECT_NAME}_src "${CMAKE_CURRENT_SOURCE_DIR}/*.cpp")

#添加动态库
add_library(${PROJECT_NAME} SHARED ${${PROJECT_NAME}_src})

# 为windows添加宏
target_link_libraries(${PROJECT_NAME} crypto ssl)
if (CMAKE_SYSTEM_NAME MATCHES "Windows")
	target_link_libraries(${PROJECT_NAME} crypt32)

else()
	add_definitions(-fPIC)

endif()

