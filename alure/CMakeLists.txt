# ALURE 2.x in-tree build
# Simplified CMakeLists for static linking into Bitfighter.

cmake_minimum_required(VERSION 3.10)

# --- Core sources (always required) ---
set(ALURE_SOURCES
    auxeffectslot.cpp
    buffer.cpp
    context.cpp
    device.cpp
    devicemanager.cpp
    effect.cpp
    source.cpp
    sourcegroup.cpp
)

# --- Decoder sources ---

# Wave: bundled, always available
set(HAVE_WAVE 1)
list(APPEND ALURE_SOURCES decoders/wave.cpp)

# MINIMP3: bundled, always available
set(HAVE_MINIMP3 1)
list(APPEND ALURE_SOURCES decoders/mp3.cpp)

# FLAC: bundled via dr_flac.h single-header library
set(HAVE_FLAC 1)
list(APPEND ALURE_SOURCES decoders/flac.cpp)

# Vorbis: optional system dependency
if(ALURE_ENABLE_VORBIS AND NOT ALURE_ENABLE_VORBIS)
    # User explicitly disabled
else()
    find_package(Vorbis QUIET)
    if(VORBIS_FOUND)
        set(HAVE_VORBISFILE 1)
        list(APPEND ALURE_SOURCES decoders/vorbisfile.cpp)
        include_directories(${VORBIS_INCLUDE_DIR})
        include_directories(${OGG_INCLUDE_DIR})
    endif()
endif()

# Opus: optional system dependency
find_package(Opus QUIET)
if(OPUS_FOUND)
    set(HAVE_OPUSFILE 1)
    list(APPEND ALURE_SOURCES decoders/opusfile.cpp)
    include_directories(${OPUS_INCLUDE_DIR})
endif()

# SndFile: optional system dependency
find_package(SndFile QUIET)
if(SNDFILE_FOUND)
    set(HAVE_LIBSNDFILE 1)
    list(APPEND ALURE_SOURCES decoders/sndfile.cpp)
    include_directories(${SNDFILE_INCLUDE_DIR})
endif()

# --- Generate config.h ---
configure_file(
    ${CMAKE_CURRENT_SOURCE_DIR}/config.h.in
    ${CMAKE_CURRENT_BINARY_DIR}/config.h
)

# --- Build the static library ---
add_library(alure STATIC ${ALURE_SOURCES})

target_include_directories(alure
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/include/AL
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/include
        ${CMAKE_CURRENT_SOURCE_DIR}/include/AL
        ${CMAKE_CURRENT_SOURCE_DIR}
        ${CMAKE_CURRENT_SOURCE_DIR}/decoders
        ${CMAKE_CURRENT_BINARY_DIR}
        ${OPENAL_INCLUDE_DIR}
)

target_compile_definitions(alure
    PUBLIC
        ALURE_STATIC_LIB
    PRIVATE
        NOMINMAX
)

target_link_libraries(alure
    PUBLIC
        ${OPENAL_LIBRARY}
    PRIVATE
        Threads::Threads
        ${VORBIS_LIBRARIES}
        ${OPUS_LIBRARIES}
        ${SNDFILE_LIBRARIES}
)

# Suppress MSVC warnings from ALURE code
if(MSVC)
    target_compile_options(alure PRIVATE /wd4996)
endif()
