function(BF_SET_SEARCH_PATHS)
	# Always use SDL2 on OSX or Windows
	set(USE_SDL2 1)

	set(SDL2_SEARCH_PATHS ${CMAKE_SOURCE_DIR}/lib ${CMAKE_SOURCE_DIR}/libsdl)
	set(OGG_SEARCH_PATHS ${CMAKE_SOURCE_DIR}/lib ${CMAKE_SOURCE_DIR}/libogg)
	set(VORBIS_SEARCH_PATHS	${CMAKE_SOURCE_DIR}/lib	${CMAKE_SOURCE_DIR}/libvorbis)
	set(VORBISFILE_SEARCH_PATHS	${CMAKE_SOURCE_DIR}/lib	${CMAKE_SOURCE_DIR}/libvorbis)
	set(SPEEX_SEARCH_PATHS ${CMAKE_SOURCE_DIR}/lib ${CMAKE_SOURCE_DIR}/libspeex)
	set(MODPLUG_SEARCH_PATHS ${CMAKE_SOURCE_DIR}/lib ${CMAKE_SOURCE_DIR}/libmodplug)
	set(ALURE_SEARCH_PATHS ${CMAKE_SOURCE_DIR}/lib ${CMAKE_SOURCE_DIR}/alure)

	# OpenAL
	set(OPENAL_INCLUDE_DIR "${CMAKE_SOURCE_DIR}/openal/include")

	# libpng
	set(PNG_INCLUDE_DIR "${CMAKE_SOURCE_DIR}/libpng")
	set(PNG_PNG_INCLUDE_DIR "${CMAKE_SOURCE_DIR}/libpng")
	
	# zlib
	set(ZLIB_INCLUDE_DIR "${CMAKE_SOURCE_DIR}/zlib")
endfunction()

function(BF_COPY_RESOURCES)
	add_custom_command(TARGET test bitfighterd bitfighter POST_BUILD 
		COMMAND ${RES_COPY_CMD}
		COMMAND ${LIB_COPY_CMD}
	)
endfunction()