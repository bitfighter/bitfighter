#
# Client OBJECT build - includes resources only compiled into the client
#

BF_PLATFORM_SET_EXTRA_SOURCES()

add_library(bitfighter_client OBJECT
	${SHARED_SOURCES}
	${CLIENT_SOURCES}
	${BITFIGHTER_HEADERS}
	${OTHER_HEADERS}
)


# If certain system libs were not found, add the in-tree variants as dependencies
set(CLIENT_EXTRA_DEPS "")
if(NOT ALURE_FOUND)
	list(APPEND CLIENT_EXTRA_DEPS alure)
endif()
if(NOT TOMCRYPT_FOUND)
	list(APPEND CLIENT_EXTRA_DEPS tomcrypt)
endif()
if(NOT CLIPPER_FOUND)
	list(APPEND CLIENT_EXTRA_DEPS clipper)
endif()
if(NOT POLY2TRI_FOUND)
	list(APPEND CLIENT_EXTRA_DEPS poly2tri)
endif()
if(NOT LUAJIT_FOUND)
	list(APPEND CLIENT_EXTRA_DEPS ${LUA_LIB})
endif()


add_dependencies(bitfighter_client
	tnl
	${CLIENT_EXTRA_DEPS}
)

# Propagate usage requirements (compile definitions, include dirs) from
# alure to this OBJECT library and its consumers.
if(NOT ALURE_FOUND)
    target_link_libraries(bitfighter_client PUBLIC alure)
endif()

if(USE_GLES)
	get_property(CLIENT_DEFS TARGET bitfighter_client PROPERTY COMPILE_DEFINITIONS)
	set_target_properties(bitfighter_client
		PROPERTIES
		COMPILE_DEFINITIONS "${CLIENT_DEFS};BF_USE_GLES"
	)
endif()

target_compile_definitions(bitfighter_client
    PUBLIC
        $<$<CONFIG:Debug>:TNL_DEBUG>
)
