include_directories(
	include
)

add_library(glad STATIC
	${CMAKE_CURRENT_SOURCE_DIR}/src/glad.c
)
