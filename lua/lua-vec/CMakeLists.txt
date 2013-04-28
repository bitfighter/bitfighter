set(LUA_VEC_SOURCES
	src/lapi.c
	src/lcode.c
	src/ldebug.c
	src/ldo.c
	src/ldump.c
	src/lfunc.c
	src/lgc.c
	src/llex.c
	src/lmem.c
	src/lobject.c
	src/lopcodes.c
	src/lparser.c
	src/lstate.c
	src/lstring.c
	src/ltable.c
	src/ltm.c
	src/lundump.c
	src/lvm.c
	src/lzio.c
	src/lauxlib.c
	src/lbaselib.c
	src/ldblib.c
	src/liolib.c
	src/lmathlib.c
	src/loslib.c
	src/ltablib.c
	src/lstrlib.c
	src/loadlib.c
	src/linit.c
	src/lveclib.c
	src/lgcveclib.c
)
add_library(lua-vec ${LUA_VEC_SOURCES})

if(CMAKE_BUILD_TYPE STREQUAL "Debug")
	set_target_properties(lua-vec
		PROPERTIES
		COMPILE_DEFINITIONS TNL_DEBUG
	)
endif()