add_library(poly2tri STATIC
	${CMAKE_CURRENT_SOURCE_DIR}/common/shapes.cc
	${CMAKE_CURRENT_SOURCE_DIR}/sweep/advancing_front.cc
	${CMAKE_CURRENT_SOURCE_DIR}/sweep/cdt.cc
	${CMAKE_CURRENT_SOURCE_DIR}/sweep/sweep.cc
	${CMAKE_CURRENT_SOURCE_DIR}/sweep/sweep_context.cc
)
