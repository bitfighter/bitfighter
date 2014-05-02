include("${CMAKE_SOURCE_DIR}/cmake/Platform/Shared.cmake")
list(APPEND SHARED_SOURCES Directory.mm)

set(EXTRA_LIBS dl m)

list(APPEND CLIENT_LIBS ${SPARKLE_LIBRARIES})

set(OSX_BUILD_RESOURCE_DIR "${CMAKE_SOURCE_DIR}/build/osx/")
# Specify output to be a .app
set_target_properties(bitfighter PROPERTIES MACOSX_BUNDLE TRUE)

# Use a custom plist
set_target_properties(bitfighter PROPERTIES MACOSX_BUNDLE_INFO_PLIST ${OSX_BUILD_RESOURCE_DIR}/Bitfighter-Info.plist)

BF_SET_SEARCH_PATHS()
set(OPENAL_LIBRARY "${CMAKE_SOURCE_DIR}/lib/OpenAL-Soft.framework")
set(PNG_LIBRARY "${CMAKE_SOURCE_DIR}/lib/libpng.framework")

set(SPARKLE_SEARCH_PATHS ${CMAKE_SOURCE_DIR}/lib)
find_package(Sparkle)
# OSX doesn't use vorbisfile (or it's built-in to normal vorbis, I think)
set(VORBISFILE_LIBRARIES "")

# Set up our bundle plist variables
set(MACOSX_BUNDLE_NAME "Bitfighter")
set(MACOSX_BUNDLE_VERSION ${BITFIGHTER_BUILD_VERSION})
set(MACOSX_BUNDLE_SHORT_VERSION_STRING ${BITFIGHTER_RELEASE})

# TODO figure out deployment targets
set(MACOSX_DEPLOYMENT_TARGET "10.6")

add_definitions(-iquote ${CMAKE_SOURCE_DIR}/zap)

# Special flags needed because of LuaJIT on 64 bit OSX
if(CMAKE_SIZEOF_VOID_P EQUAL 8)
	set_target_properties(bitfighterd bitfighter PROPERTIES LINK_FLAGS "-pagezero_size 10000 -image_base 100000000")
endif()

# Install windows libraries and resources
# The trailing slash is necessary to do here for proper native path translation
file(TO_NATIVE_PATH ${CMAKE_SOURCE_DIR}/resource/ resDir)
file(TO_NATIVE_PATH ${CMAKE_SOURCE_DIR}/lib/ libDir)
file(TO_NATIVE_PATH ${CMAKE_SOURCE_DIR}/lua/luajit/src/ luaLibDir)
file(TO_NATIVE_PATH ${CMAKE_SOURCE_DIR}/exe exeDir)


# OSX
# We have to do much, much more to properly build a .app
set(frameworksDir "${exeDir}/bitfighter.app/Contents/Frameworks")
set(resourcesDir "${exeDir}/bitfighter.app/Contents/Resources")
execute_process(COMMAND mkdir -p ${frameworksDir})
execute_process(COMMAND mkdir -p ${resourcesDir})

set(RES_COPY_CMD cp -rp ${resDir}* ${resourcesDir})
set(LIB_COPY_CMD rsync -av --exclude=*.h ${libDir}*.framework ${frameworksDir})

# Icon file
set(COPY_RES_1 cp -rp ${OSX_BUILD_RESOURCE_DIR}/Bitfighter.icns ${resourcesDir})
# Public key for Sparkle
set(COPY_RES_2 cp -rp ${OSX_BUILD_RESOURCE_DIR}/dsa_pub.pem ${resourcesDir})
# Joystick presets
set(COPY_RES_3 cp -rp ${exeDir}/joystick_presets.ini ${resourcesDir})
# Notifier
set(COPY_RES_4 cp -rp ${exeDir}/../notifier/bitfighter_notifier.py ${resourcesDir})
set(COPY_RES_5 cp -rp ${exeDir}/../notifier/redship18.png ${resourcesDir})

add_custom_command(TARGET bitfighterd bitfighter POST_BUILD 
	COMMAND ${COPY_RES_1}
	COMMAND ${COPY_RES_2}
	COMMAND ${COPY_RES_3}
	COMMAND ${COPY_RES_4}
	COMMAND ${COPY_RES_5}
)

# 64-bit OSX needs to use shared LuaJIT library
if(CMAKE_SIZEOF_VOID_P EQUAL 8)
	add_custom_command(TARGET test bitfighterd bitfighter POST_BUILD
		COMMAND cp -rp ${luaLibDir}libluajit.dylib ${frameworksDir}
	)
endif()

# TODO - run lipo on the frameworks
add_custom_target(dmg)
# TODO:  build DMG
#set_target_properties(dmg PROPERTIES POST_INSTALL_SCRIPT ${CMAKE_CURRENT_BINARY_DIR}/CreateMacBundle.cmake)
BF_COPY_RESOURCES()