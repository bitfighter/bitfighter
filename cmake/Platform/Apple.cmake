## Global project configuration

#
# Environment verification
#
set(OSX_DEPLOY_TARGET $ENV{MACOSX_DEPLOYMENT_TARGET})

message(STATUS "MACOSX_DEPLOYMENT_TARGET: ${OSX_DEPLOY_TARGET}")
# MACOSX_DEPLOYMENT_TARGET must be set in the environment to compile properly
if(NOT OSX_DEPLOY_TARGET)
	message(FATAL_ERROR "MACOSX_DEPLOYMENT_TARGET environment variable not set.  Set this like so: 'export MACOSX_DEPLOYMENT_TARGET=10.6'")
endif()


#
# Linker flags
# 



# 
# Compiler specific flags
# 
if(CMAKE_COMPILER_IS_GNUCC)
	set(CMAKE_C_FLAGS_DEBUG "${CMAKE_C_FLAGS_DEBUG} -Wall")
	set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -Wall")
endif()


#
# Library searching and dependencies
#

# Set some search paths
set(SDL2_SEARCH_PATHS ${CMAKE_SOURCE_DIR}/lib ${CMAKE_SOURCE_DIR}/libsdl)
set(OGG_SEARCH_PATHS ${CMAKE_SOURCE_DIR}/lib ${CMAKE_SOURCE_DIR}/libogg)
set(VORBIS_SEARCH_PATHS	${CMAKE_SOURCE_DIR}/lib	${CMAKE_SOURCE_DIR}/libvorbis)
set(VORBISFILE_SEARCH_PATHS	${CMAKE_SOURCE_DIR}/lib	${CMAKE_SOURCE_DIR}/libvorbis)
set(SPEEX_SEARCH_PATHS ${CMAKE_SOURCE_DIR}/lib ${CMAKE_SOURCE_DIR}/libspeex)
set(MODPLUG_SEARCH_PATHS ${CMAKE_SOURCE_DIR}/lib ${CMAKE_SOURCE_DIR}/libmodplug)
set(ALURE_SEARCH_PATHS ${CMAKE_SOURCE_DIR}/lib ${CMAKE_SOURCE_DIR}/alure)

# Directly set include dirs for some libraries
set(OPENAL_INCLUDE_DIR "${CMAKE_SOURCE_DIR}/openal/include")
set(ZLIB_INCLUDE_DIR "${CMAKE_SOURCE_DIR}/zlib")
# libpng needs two for some weird reason
set(PNG_INCLUDE_DIR "${CMAKE_SOURCE_DIR}/libpng")
set(PNG_PNG_INCLUDE_DIR "${CMAKE_SOURCE_DIR}/libpng")

# Directly specify some libs
set(OPENAL_LIBRARY "${CMAKE_SOURCE_DIR}/lib/OpenAL-Soft.framework")
set(PNG_LIBRARY "${CMAKE_SOURCE_DIR}/lib/libpng.framework")

set(SPARKLE_SEARCH_PATHS ${CMAKE_SOURCE_DIR}/lib)
# OSX doesn't use vorbisfile (or it's built-in to normal vorbis, I think)
set(VORBISFILE_LIBRARIES "")


find_package(Sparkle)


## End Global project configuration


## Sub-project configuration
#
# Note that any variable adjustment from the parent CMakeLists.txt will
# need to be re-set with the PARENT_SCOPE option

function(BF_PLATFORM_SET_EXTRA_SOURCES)
	list(APPEND SHARED_SOURCES Directory.mm)
	set(SHARED_SOURCES ${SHARED_SOURCES} PARENT_SCOPE)
endfunction()


function(BF_PLATFORM_SET_EXTRA_LIBS)
	set(EXTRA_LIBS dl m PARENT_SCOPE)
endfunction()


function(BF_PLATFORM_APPEND_LIBS)
	list(APPEND CLIENT_LIBS ${SPARKLE_LIBRARIES})
	set(CLIENT_LIBS ${CLIENT_LIBS} PARENT_SCOPE)
endfunction()


function(BF_PLATFORM_ADD_DEFINITIONS)
	add_definitions(-iquote ${CMAKE_SOURCE_DIR}/zap)
endfunction()


function(BF_PLATFORM_SET_TARGET_PROPERTIES targetName)
	# Setup OSX Bundle
	
	# We need this variable in both scopes
	set(OSX_BUILD_RESOURCE_DIR "${CMAKE_SOURCE_DIR}/build/osx/")
	set(OSX_BUILD_RESOURCE_DIR "${OSX_BUILD_RESOURCE_DIR}" PARENT_SCOPE)
	
	# Specify output to be a .app
	set_target_properties(${targetName} PROPERTIES MACOSX_BUNDLE TRUE)
	
	# Use a custom plist
	set_target_properties(${targetName} PROPERTIES MACOSX_BUNDLE_INFO_PLIST ${OSX_BUILD_RESOURCE_DIR}/Bitfighter-Info.plist)
	
	# Set up our bundle plist variables
	set(MACOSX_BUNDLE_NAME "Bitfighter")
	set(MACOSX_BUNDLE_VERSION ${BITFIGHTER_BUILD_VERSION})
	set(MACOSX_BUNDLE_SHORT_VERSION_STRING ${BITFIGHTER_RELEASE})
	
	# TODO figure out deployment targets
	set(MACOSX_DEPLOYMENT_TARGET "10.6")
	

	# Special flags needed because of LuaJIT on 64 bit OSX
	if(CMAKE_SIZEOF_VOID_P EQUAL 8)
		set_target_properties(${targetName} PROPERTIES LINK_FLAGS "-pagezero_size 10000 -image_base 100000000")
	endif()
endfunction()


function(BF_PLATFORM_POST_BUILD_INSTALL_RESOURCES targetName)
	# The trailing slash is necessary to do here for proper native path translation
	file(TO_NATIVE_PATH ${CMAKE_SOURCE_DIR}/resource/ resDir)
	file(TO_NATIVE_PATH ${CMAKE_SOURCE_DIR}/lib/ libDir)
	file(TO_NATIVE_PATH ${CMAKE_SOURCE_DIR}/lua/luajit/src/ luaLibDir)
	file(TO_NATIVE_PATH ${CMAKE_SOURCE_DIR}/exe exeDir)
	
	# Create extra dirs in the .app
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
	
	add_custom_command(TARGET ${targetName} POST_BUILD 
		COMMAND ${COPY_RES_1}
		COMMAND ${COPY_RES_2}
		COMMAND ${COPY_RES_3}
		COMMAND ${COPY_RES_4}
		COMMAND ${COPY_RES_5}
	)
	
	# 64-bit OSX needs to use shared LuaJIT library
	if(CMAKE_SIZEOF_VOID_P EQUAL 8)
		add_custom_command(TARGET ${targetName} POST_BUILD
			COMMAND cp -rp ${luaLibDir}libluajit.dylib ${frameworksDir}
		)
	endif()
	
	# Copy resources
	add_custom_command(TARGET ${targetName} POST_BUILD 
		COMMAND ${RES_COPY_CMD}
		COMMAND ${LIB_COPY_CMD}
	)
	
	# TODO - run lipo on the frameworks to clean out unwanted architectures
endfunction()


function(BF_PLATFORM_INSTALL targetName)
	# Do nothing!
endfunction()


function(BF_PLATFORM_CREATE_PACKAGES targetName)
	add_custom_target(dmg)
	# TODO:  build DMG
	#set_target_properties(dmg PROPERTIES POST_INSTALL_SCRIPT ${CMAKE_CURRENT_BINARY_DIR}/CreateMacBundle.cmake)
endfunction()
