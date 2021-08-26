cmake_minimum_required (VERSION 2.8)

include_directories (
	${CMAKE_CURRENT_SOURCE_DIR}
)

# 包含子项目。
add_subdirectory ("sqlparser")
add_subdirectory ("bodb")
add_subdirectory ("bodbclient")
