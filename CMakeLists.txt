cmake_minimum_required (VERSION 2.8)

project(mycp)

find_package(Boost 1.70.0 REQUIRED)
if(Boost_FOUND)
	message ("boost found : ${Boost_INCLUDE_DIRS}")
else()
	message (FATAL_ERROR "Cannot find Boost")
endif()

# 设置头文件搜索目录
include_directories (
	${CMAKE_SOURCE_DIR}/src
	${CMAKE_SOURCE_DIR}/src/ThirdParty/Bodb
	${CMAKE_BINARY_DIR}/src/ThirdParty
	${CMAKE_BINARY_DIR}/src/ThirdParty/openssl/crypto
	${CMAKE_BINARY_DIR}/src/ThirdParty/openssl/ssl
	${CMAKE_SOURCE_DIR}/src/ThirdParty/zlib-1.2.8
	${CMAKE_BINARY_DIR}/src/ThirdParty/zlib-1.2.8
	PRIVATE ${Boost_INCLUDE_DIRS}
)


# 设置链接搜索目录
link_directories(
	${CMAKE_BINARY_DIR}/lib
	${CMAKE_BINARY_DIR}/bin
	${Boost_INCLUDE_DIRS}/stage/lib
)

#list(APPEND CMAKE_PREFIX_PATH "./src/ThirdParty/zlib-1.2.8")
#find_package(zlib REQUIRED)

# 设置对C++11支持
# 检查c++编译器标志，设置c++11支持变量
include(CheckCXXCompilerFlag)
CHECK_CXX_COMPILER_FLAG("-std=c++11" COMPILER_SUPPORTS_CXX11)
CHECK_CXX_COMPILER_FLAG("-std=c++0x" COMPILER_SUPPORTS_CXX0X)
# 使用变量设置编译标志
if(COMPILER_SUPPORTS_CXX11)
	set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11")
elseif(COMPILER_SUPPORTS_CXX0X)
	set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++0x")
else()
	message(STATUS "The compiler ${CMAKE_CXX_COMPILER} has no C++11 support. Please use a different C++ compiler.")
endif()
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_STANDARD 11)
#add_compile_options(-std=c++11)
#add_definitions(-std=c++11)

# 为windows添加宏
if (CMAKE_SYSTEM_NAME MATCHES "Windows")
	# 解决 windows 重定义的问题
	add_definitions(-DWIN32_LEAN_AND_MEAN)
	# windows 支持utf-8源文件
	add_definitions(/utf-8)
else()

endif()

#设置第三方库cmake脚本所在目录
if (CMAKE_SYSTEM_NAME MATCHES "Windows")
	#set(CMAKE_MODULE_PATH "${CMAKE_SOURCE_DIR}/libs/debug/windows")
else()
	#set(CMAKE_MODULE_PATH "${CMAKE_SOURCE_DIR}/libs/debug/linux")
endif()

#设置编译目录
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)

# 编译成release的，可选
#set(CMAKE_BUILD_TYPE RELEASE) 

# 包含子项目。
add_subdirectory ("src")
add_subdirectory ("samples")
