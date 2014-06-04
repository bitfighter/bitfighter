## Global project configuration

#
# Environment verification
#
set(OSX_DEPLOY_TARGET $ENV{MACOSX_DEPLOYMENT_TARGET})

message(STATUS "MACOSX_DEPLOYMENT_TARGET: ${OSX_DEPLOY_TARGET}")


# These mandatory variables should be set with cross-compiling
if(NOT XCOMPILE)
	# MACOSX_DEPLOYMENT_TARGET must be set in the environment to compile properly
	if(NOT OSX_DEPLOY_TARGET)
		message(FATAL_ERROR "MACOSX_DEPLOYMENT_TARGET environment variable not set.  Set this like so: 'export MACOSX_DEPLOYMENT_TARGET=10.6'")
	endif()


	# Make sure the compiling architecture is set
	if(OSX_DEPLOY_TARGET VERSION_LESS "10.4")
		message(FATAL_ERROR "Bitfighter cannot be compiled on OSX earlier than 10.4")
	elseif(OSX_DEPLOY_TARGET VERSION_LESS "10.6")
		if(NOT CMAKE_OSX_ARCHITECTURES)
			message(FATAL_ERROR "You must set CMAKE_OSX_ARCHITECTURES to either 'ppc' or 'i386'")
		endif()
	else()
		set(CMAKE_OSX_ARCHITECTURES "x86_64")
	endif()


	# Set the proper SDK for compiling
	if(OSX_DEPLOY_TARGET VERSION_EQUAL "10.4")
		set(CMAKE_OSX_SYSROOT "/Developer/SDKs/MacOSX10.4u.sdk/")
	else()
		set(CMAKE_OSX_SYSROOT "/Developer/SDKs/MacOSX${OSX_DEPLOY_TARGET}.sdk/")
	endif()
endif()


message(STATUS "Compiling for OSX architectures: ${CMAKE_OSX_ARCHITECTURES}")


# LuaJIT will not compile on 10.4 ppc - it requires GCC >= 4.3
# Disable LuaJIT for cross-compile (for now)
if(CMAKE_OSX_ARCHITECTURES STREQUAL "ppc" OR XCOMPILE)
	set(USE_LUAJIT NO)
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

		
if(OSX_DEPLOY_TARGET VERSION_EQUAL "10.4")
	# OSX 10.4 doesn't have execinfo.h for the StackTracer
	add_definitions(-DBF_NO_STACKTRACE)
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
	
	# Special flags needed because of LuaJIT on 64 bit OSX
	if(USE_LUAJIT AND CMAKE_OSX_ARCHITECTURES STREQUAL "x86_64")
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
	set(frameworksDir "${exeDir}/${targetName}.app/Contents/Frameworks")
	set(resourcesDir "${exeDir}/${targetName}.app/Contents/Resources")
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
	if(USE_LUAJIT AND CMAKE_OSX_ARCHITECTURES STREQUAL "x86_64")
		add_custom_command(TARGET ${targetName} POST_BUILD
			COMMAND cp -rp ${luaLibDir}libluajit.dylib ${frameworksDir}
		)
	endif()
	
	# Copy resources
	add_custom_command(TARGET ${targetName} POST_BUILD 
		COMMAND ${RES_COPY_CMD}
		COMMAND ${LIB_COPY_CMD}
	)
	
	# Thin out our installed frameworks by running 'lipo' to clean out the unwanted 
	# architectures and removing any header files
	if(NOT LIPO_COMMAND)
		set(LIPO_COMMAND lipo)
	endif()
	
	# This can happen when cross-compiling x86_64
	if(NOT CMAKE_OSX_ARCHITECTURES)
		set(CMAKE_OSX_ARCHITECTURES "x86_64")
	endif()
	
	set(THIN_FRAMEWORKS ${CMAKE_SOURCE_DIR}/build/osx/tools/thin_frameworks.sh ${LIPO_COMMAND} ${CMAKE_OSX_ARCHITECTURES} ${exeDir}/${targetName}.app)
	
	add_custom_command(TARGET ${targetName} POST_BUILD
		COMMAND ${THIN_FRAMEWORKS}
	)
endfunction()


function(BF_PLATFORM_INSTALL targetName)
	# Do nothing!
endfunction()


function(BF_PLATFORM_CREATE_PACKAGES targetName)
	add_custom_target(dmg)
	# TODO:  build DMG
	#set_target_properties(dmg PROPERTIES POST_INSTALL_SCRIPT ${CMAKE_CURRENT_BINARY_DIR}/CreateMacBundle.cmake)
endfunction()
