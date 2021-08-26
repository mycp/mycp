cmake_minimum_required (VERSION 2.8)

project("bodbclient")

#源文件
file(GLOB ${PROJECT_NAME}_src "${CMAKE_CURRENT_SOURCE_DIR}/*.cpp" "${CMAKE_CURRENT_SOURCE_DIR}/CGC/*.cpp")

add_executable(${PROJECT_NAME} ${${PROJECT_NAME}_src})

# 为windows添加宏
if (CMAKE_SYSTEM_NAME MATCHES "Windows")

else()
	add_definitions(-fPIC)

endif()

#添加工程内引用
target_link_libraries(${PROJECT_NAME} sqlparser bodb)
if (CMAKE_SYSTEM_NAME MATCHES "Windows")

else()

endif()
