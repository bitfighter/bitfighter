#
# Full client build
# 

add_executable(bitfighter
	main.cpp
)

add_dependencies(bitfighter
	bitfighter_client
)

target_link_libraries(bitfighter
	${BF_CLIENT_LIBRARY_BEFORE_FLAGS}
	bitfighter_client
	${BF_CLIENT_LIBRARY_AFTER_FLAGS}
)

# Where to put the executable
set_target_properties(bitfighter PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${CMAKE_SOURCE_DIR}/exe)

set_target_properties(bitfighter PROPERTIES COMPILE_DEFINITIONS_DEBUG "TNL_DEBUG")

BF_PLATFORM_SET_TARGET_PROPERTIES(bitfighter)

BF_PLATFORM_POST_BUILD_INSTALL_RESOURCES(bitfighter)

BF_PLATFORM_INSTALL(bitfighter)

BF_PLATFORM_CREATE_PACKAGES(bitfighter)
