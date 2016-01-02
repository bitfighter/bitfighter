# Most ALURE platform-specific settings are included in config.h.in

set(ALURE_SOURCES
	buffer.cpp
	streamdec.cpp
	stream.cpp
	streamplay.cpp
	istream.cpp
	codec_wav.cpp
	codec_aiff.cpp
	alure.cpp
)

set(MUSIC_LIBS "")

# Add in music libraries
if(NOT NO_MUSIC)
	add_definitions(-DHAS_VORBISFILE=1)
	add_definitions(-DHAS_MODPLUG=1)
	
	list(APPEND ALURE_SOURCES 
		codec_modplug.cpp
		codec_vorbisfile.cpp
	)
	
	set(MUSIC_LIBS 
		${MODPLUG_LIBRARIES}
		${VORBIS_LIBRARIES}
	)

	# Dynamic loading of mpg123 for people who may want to drop in a decoder
	if(NOT ALURE_DISABLE_MP3)
		add_definitions(-DHAS_MPG123=1)
		add_definitions(-DDYNLOAD_MPG123=1)
		list(APPEND ALURE_SOURCES codec_mpg123.cpp)
	endif()
else()
	# Hack to make Alure not link music libs
	add_definitions(-DDYNLOAD=1)
endif()

if(WIN32)
	add_definitions(-D_WIN32)
endif()

if(NOT WIN32)
	set(EXTRA_LIBS dl)
endif()


add_library(alure STATIC ${ALURE_SOURCES})
include_directories(
	${OPENAL_INCLUDE_DIR} 
	${OGG_INCLUDE_DIR} 
	${VORBIS_INCLUDE_DIR} 
	${VORBISFILE_INCLUDE_DIR} 
	${MODPLUG_INCLUDE_DIR}
)
target_link_libraries(alure 
	${OPENAL_LIBRARY}
	${EXTRA_LIBS}
)
