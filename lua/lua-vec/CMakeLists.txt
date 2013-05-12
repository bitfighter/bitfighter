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

file(GLOB LUA_VEC_HEADERS "src/*.h")

add_library(lua-vec ${LUA_VEC_SOURCES} ${LUA_VEC_HEADERS})

if(CMAKE_BUILD_TYPE STREQUAL "Debug")
	set_target_properties(lua-vec
		PROPERTIES
		COMPILE_DEFINITIONS TNL_DEBUG
	)
endif()