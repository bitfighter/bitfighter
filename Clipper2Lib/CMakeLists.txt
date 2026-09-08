add_library(clipper2 STATIC
	${CMAKE_CURRENT_SOURCE_DIR}/src/clipper.engine.cpp
	${CMAKE_CURRENT_SOURCE_DIR}/src/clipper.offset.cpp
	${CMAKE_CURRENT_SOURCE_DIR}/src/clipper.rectclip.cpp
	${CMAKE_CURRENT_SOURCE_DIR}/src/clipper.triangulation.cpp
)

target_include_directories(clipper2 PUBLIC
	${CMAKE_CURRENT_SOURCE_DIR}/include
)
