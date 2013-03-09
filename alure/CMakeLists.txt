set(EXECUTABLE_OUTPUT_PATH ${CMAKE_CURRENT_SOURCE_DIR}/exe)
set(LIBRARY_OUTPUT_PATH ${CMAKE_CURRENT_SOURCE_DIR}/lib)
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
target_link_libraries(alure vorbisfile modplug dl)
