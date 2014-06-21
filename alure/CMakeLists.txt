# Most ALURE platform-specific settings are included in config.h

if(WIN32)
	add_definitions(-D_WIN32)
endif()

if(NOT WIN32)
	set(EXTRA_LIBS dl)
endif()

set(ALURE_SOURCES
	buffer.cpp
	streamdec.cpp
	stream.cpp
	streamplay.cpp
	istream.cpp
	codec_wav.cpp
	codec_aiff.cpp
	alure.cpp
	codec_modplug.cpp
	codec_vorbisfile.cpp
)

add_library(alure ${ALURE_SOURCES})
include_directories(
	${OPENAL_INCLUDE_DIR} 
	${OGG_INCLUDE_DIR} 
	${VORBIS_INCLUDE_DIR} 
	${VORBISFILE_INCLUDE_DIR} 
	${MODPLUG_INCLUDE_DIR}
)
target_link_libraries(alure 
	${OPENAL_LIBRARY}
	${MODPLUG_LIBRARIES}
	${VORBIS_LIBRARIES}
	${EXTRA_LIBS}
)
