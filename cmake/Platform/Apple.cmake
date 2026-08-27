## Global project configuration

#
# Environment verification
#
set(OSX_DEPLOY_TARGET $ENV{MACOSX_DEPLOYMENT_TARGET})

message(STATUS "MACOSX_DEPLOYMENT_TARGET: ${OSX_DEPLOY_TARGET}")


# These mandatory variables should be set with cross-compiling
if(NOT XCOMPILE)
	# Detect current OSX version
	set(OSX_VERSION 0)
	find_program(sw_vers sw_vers)
	execute_process(COMMAND ${sw_vers} "-productVersion" OUTPUT_VARIABLE OSX_VERSION OUTPUT_STRIP_TRAILING_WHITESPACE)
	string(REGEX REPLACE "([0-9]+.[0-9]+).*" "\\1" OSX_VERSION ${OSX_VERSION})
	
	message(STATUS "OSX Version: ${OSX_VERSION}")
    
	if(NOT OSX_DEPLOY_TARGET)
		# MACOSX_DEPLOYMENT_TARGET must be set in the environment to compile properly on OSX 10.6 and earlier
		if(${OSX_VERSION} VERSION_LESS "10.7")
			message(FATAL_ERROR "MACOSX_DEPLOYMENT_TARGET environment variable not set.  Set this like so: 'export MACOSX_DEPLOYMENT_TARGET=10.6'")
		else()
			set(OSX_DEPLOY_TARGET ${OSX_VERSION})
		endif()
	endif()


	# Make sure the compiling architecture is set
	#
	# Honor an explicit -DCMAKE_OSX_ARCHITECTURES=... (e.g. to cross-build x86_64
	# under Rosetta).  Otherwise default to the host CPU so Apple Silicon Macs
	# build native arm64 binaries instead of x86_64-under-Rosetta.
	if(OSX_DEPLOY_TARGET VERSION_LESS "10.5")
		message(FATAL_ERROR "Bitfighter cannot be compiled on OSX earlier than 10.5")
	elseif(NOT CMAKE_OSX_ARCHITECTURES)
		if(OSX_DEPLOY_TARGET VERSION_LESS "10.6")
			set(CMAKE_OSX_ARCHITECTURES "i386")
		else()
			set(CMAKE_OSX_ARCHITECTURES "${CMAKE_HOST_SYSTEM_PROCESSOR}")
		endif()
	endif()
	
	message(STATUS "CMAKE_OSX_SYSROOT: ${CMAKE_OSX_SYSROOT}")
endif()

message(STATUS "Compiling for OSX architectures: ${CMAKE_OSX_ARCHITECTURES}")


#
# Linker flags
# 

# 
# Compiler specific flags
# 

# c++11
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11 -std=gnu++11")

# OSX 10.7 and greater need this to find some dependencies
if(OSX_DEPLOY_TARGET VERSION_GREATER "10.6")
	set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -stdlib=libc++")
endif()

# Enable more warnings in Debug build
if(CMAKE_COMPILER_IS_GNUCC)
	set(CMAKE_C_FLAGS_DEBUG "${CMAKE_C_FLAGS_DEBUG} -Wall")
	set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -Wall")
endif()


#
# Library searching and dependencies
#
set(BF_LIB_DIR ${CMAKE_SOURCE_DIR}/lib)
set(BF_LIB_INCLUDE_DIR ${CMAKE_SOURCE_DIR}/lib/include)

# The prebuilt frameworks/dylibs in lib/ are Intel-only.  On Apple Silicon we
# resolve the client dependencies from Homebrew instead.  An x86_64 (Rosetta)
# build keeps using the bundled libraries, so existing behaviour is unchanged.
if(CMAKE_OSX_ARCHITECTURES STREQUAL "arm64")
	set(BF_USE_HOMEBREW_LIBS TRUE)
endif()

if(BF_USE_HOMEBREW_LIBS)
	find_program(BREW_COMMAND brew)
	if(NOT BREW_COMMAND)
		message(FATAL_ERROR "Homebrew is required for a native arm64 build. Install it from https://brew.sh and run: brew install sdl2 libpng libogg libvorbis speex libmodplug openal-soft")
	endif()

	execute_process(COMMAND ${BREW_COMMAND} --prefix
		OUTPUT_VARIABLE HOMEBREW_PREFIX OUTPUT_STRIP_TRAILING_WHITESPACE
		RESULT_VARIABLE BREW_PREFIX_RESULT)
	# openal-soft is keg-only, so it isn't symlinked into the main prefix
	execute_process(COMMAND ${BREW_COMMAND} --prefix openal-soft
		OUTPUT_VARIABLE OPENAL_PREFIX OUTPUT_STRIP_TRAILING_WHITESPACE
		RESULT_VARIABLE OPENAL_PREFIX_RESULT)

	# Without these checks an uninstalled formula leaves the prefix empty and
	# paths like ${OPENAL_PREFIX}/lib/libopenal.dylib silently resolve to
	# /lib/..., which fails much later with a confusing message.
	if(NOT BREW_PREFIX_RESULT EQUAL 0 OR NOT HOMEBREW_PREFIX)
		message(FATAL_ERROR "Could not determine the Homebrew prefix from `brew --prefix`.")
	endif()
	if(NOT OPENAL_PREFIX_RESULT EQUAL 0 OR NOT OPENAL_PREFIX)
		message(FATAL_ERROR "openal-soft is not installed. Install it with: brew install openal-soft")
	endif()

	message(STATUS "Resolving client dependencies from Homebrew: ${HOMEBREW_PREFIX}")

	# Let the system find modules (PNG, etc.) search the Homebrew prefix
	list(APPEND CMAKE_PREFIX_PATH ${HOMEBREW_PREFIX} ${OPENAL_PREFIX})

	# Point our custom Find* modules at Homebrew instead of the bundled libs
	set(SDL2_SEARCH_PATHS    ${HOMEBREW_PREFIX})
	set(OGG_SEARCH_PATHS     ${HOMEBREW_PREFIX})
	set(VORBIS_SEARCH_PATHS  ${HOMEBREW_PREFIX})
	set(SPEEX_SEARCH_PATHS   ${HOMEBREW_PREFIX})
	set(MODPLUG_SEARCH_PATHS ${HOMEBREW_PREFIX})
	# ALURE has no system package on macOS; it is built in-tree against the
	# Homebrew OpenAL/Vorbis/ModPlug resolved above.
	set(ALURE_SEARCH_PATHS   ${BF_LIB_DIR} ${BF_LIB_INCLUDE_DIR}/alure)

	# openal-soft replaces the bundled (deprecated) OpenAL; specify it directly
	# since CMake's FindOpenAL otherwise prefers the deprecated system framework.
	# openal-soft installs its headers under include/AL, but the source uses the
	# flat <al.h>/<alc.h> include style, so add both the parent and AL dirs.
	set(OPENAL_INCLUDE_DIR "${OPENAL_PREFIX}/include" "${OPENAL_PREFIX}/include/AL")
	set(OPENAL_LIBRARY "${OPENAL_PREFIX}/lib/libopenal.dylib")
	# Homebrew ships vorbisfile as a separate dylib (the bundled Vorbis.framework
	# had it baked in); ALURE needs it for ogg decoding.  PNG and zlib resolve
	# through CMAKE_PREFIX_PATH / the system SDK.
	set(VORBISFILE_LIBRARIES "${HOMEBREW_PREFIX}/lib/libvorbisfile.dylib")
else()
	# Set some search paths
	set(SDL2_SEARCH_PATHS       ${BF_LIB_DIR} ${BF_LIB_INCLUDE_DIR}/libsdl)
	set(OGG_SEARCH_PATHS        ${BF_LIB_DIR} ${BF_LIB_INCLUDE_DIR}/libogg)
	set(VORBIS_SEARCH_PATHS     ${BF_LIB_DIR} ${BF_LIB_INCLUDE_DIR}/libvorbis)
	set(VORBISFILE_SEARCH_PATHS ${BF_LIB_DIR} ${BF_LIB_INCLUDE_DIR}/libvorbis)
	set(SPEEX_SEARCH_PATHS      ${BF_LIB_DIR} ${BF_LIB_INCLUDE_DIR}/libspeex)
	set(MODPLUG_SEARCH_PATHS    ${BF_LIB_DIR} ${BF_LIB_INCLUDE_DIR}/libmodplug)
	set(ALURE_SEARCH_PATHS      ${BF_LIB_DIR} ${BF_LIB_INCLUDE_DIR}/alure)

	# Directly set include dirs for some libraries
	set(OPENAL_INCLUDE_DIR "${BF_LIB_INCLUDE_DIR}/openal/include")
	set(ZLIB_INCLUDE_DIR "${BF_LIB_INCLUDE_DIR}/zlib")
	# libpng needs two for some weird reason
	set(PNG_INCLUDE_DIR "${BF_LIB_INCLUDE_DIR}/libpng")
	set(PNG_PNG_INCLUDE_DIR "${BF_LIB_INCLUDE_DIR}/libpng")

	# Directly specify some libs
	set(OPENAL_LIBRARY "${BF_LIB_DIR}/libopenal.1.dylib")
	set(PNG_LIBRARY "${BF_LIB_DIR}/libpng.framework")

	set(SPARKLE_SEARCH_PATHS ${BF_LIB_DIR})
	# OSX doesn't use vorbisfile (or it's built-in to normal vorbis, I think)
	set(VORBISFILE_LIBRARIES "")
endif()


# Sparkle auto-updater (client only).  The bundled Sparkle is 1.x and Intel-only;
# a native arm64 build would require migrating to Sparkle 2.x, so default it off
# there and build the client without the auto-updater for now.
if(BF_USE_HOMEBREW_LIBS)
	set(_use_sparkle_default OFF)
else()
	set(_use_sparkle_default ON)
endif()
option(USE_SPARKLE "Build the macOS client with the Sparkle auto-updater" ${_use_sparkle_default})

if(USE_SPARKLE)
	find_package(Sparkle)
else()
	message(STATUS "Building macOS client without the Sparkle auto-updater")
	add_definitions(-DBF_NO_SPARKLE)
endif()


# When building natively against Homebrew, the client .app links its dylibs by
# absolute /opt/homebrew path, so it only runs where Homebrew is installed at the
# same prefix.  Enable this to copy those dylibs into the bundle and re-point the
# load commands, producing a relocatable .app (see BF_PLATFORM_BUNDLE_DEPENDENCIES).
# Off by default since it slows the build and is only needed for distribution.
option(BUNDLE_DEPENDENCIES "Bundle Homebrew dylibs into the macOS .app for distribution" OFF)


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
	# Directory.mm is a shared source that uses Foundation (NSFileManager,
	# NSBundle, NSSearchPathForDirectoriesInDomains, ...), so every macOS
	# target -- including the dedicated server -- must link it.
	find_library(FOUNDATION_LIBRARY Foundation)
	set(EXTRA_LIBS dl m ${FOUNDATION_LIBRARY} PARENT_SCOPE)
endfunction()


function(BF_PLATFORM_APPEND_LIBS)
	list(APPEND CLIENT_LIBS ${SPARKLE_LIBRARIES})
	set(CLIENT_LIBS ${CLIENT_LIBS} PARENT_SCOPE)
endfunction()


function(BF_PLATFORM_ADD_DEFINITIONS)
	add_definitions(-iquote ${CMAKE_SOURCE_DIR}/zap)
endfunction()


function(BF_PLATFORM_SET_EXECUTABLE_NAME)
	set(BF_EXE_NAME "Bitfighter" PARENT_SCOPE)
endfunction()


function(BF_PLATFORM_SET_TARGET_PROPERTIES targetName)
	
	# We need this variable in both scopes
	set(OSX_BUILD_RESOURCE_DIR "${CMAKE_SOURCE_DIR}/build/osx/")
	set(OSX_BUILD_RESOURCE_DIR "${OSX_BUILD_RESOURCE_DIR}" PARENT_SCOPE)
	
	# Special flags needed because of LuaJIT on 64 bit OSX
	if(CMAKE_OSX_ARCHITECTURES STREQUAL "x86_64")
		set_target_properties(${targetName} PROPERTIES LINK_FLAGS "-pagezero_size 10000 -image_base 100000000")
	endif()
endfunction()


function(BF_PLATFORM_SET_TARGET_OTHER_PROPERTIES targetName)
	# Setup OSX Bundle; specify output to be a .app
	set_target_properties(${targetName} PROPERTIES MACOSX_BUNDLE TRUE)
	
	# Set up our bundle plist variables
	set(MACOSX_BUNDLE_NAME ${targetName})
	set(MACOSX_BUNDLE_EXECUTABLE ${targetName})
	
	# Fill out a plist template with CMake variables
	configure_file(${OSX_BUILD_RESOURCE_DIR}/Bitfighter-Info.plist.cmake ${OSX_BUILD_RESOURCE_DIR}/Bitfighter-Info.plist)
	
	# Use the custom plist
	set_target_properties(${targetName} PROPERTIES MACOSX_BUNDLE_INFO_PLIST ${OSX_BUILD_RESOURCE_DIR}/Bitfighter-Info.plist)
endfunction()


function(BF_PLATFORM_POST_BUILD_INSTALL_RESOURCES targetName)
	# The trailing slash is necessary to do here for proper native path translation
	file(TO_NATIVE_PATH ${CMAKE_SOURCE_DIR}/resource/ resDir)
	file(TO_NATIVE_PATH ${CMAKE_SOURCE_DIR}/lib/ libDir)
	file(TO_NATIVE_PATH ${CMAKE_SOURCE_DIR}/lua/luajit/src/ luaLibDir)
	file(TO_NATIVE_PATH ${CMAKE_SOURCE_DIR}/exe exeDir)

	# Non-bundle targets (the dedicated server and the test runner) are plain
	# executables in exe/.  They locate resources relative to the binary (see
	# FolderManager::resolveDirs), so just drop resource/* alongside the binary
	# and skip the .app bundle / framework bundling, which assumes a .app layout
	# and Intel-only prebuilt frameworks.
	get_target_property(isBundle ${targetName} MACOSX_BUNDLE)
	if(NOT isBundle)
		add_custom_command(TARGET ${targetName} POST_BUILD
			COMMAND cp -rp ${resDir}* ${exeDir}
		)
		return()
	endif()

	# Create extra dirs in the .app
	set(frameworksDir "${exeDir}/${targetName}.app/Contents/Frameworks")
	set(resourcesDir "${exeDir}/${targetName}.app/Contents/Resources")
	execute_process(COMMAND mkdir -p ${frameworksDir})
	execute_process(COMMAND mkdir -p ${resourcesDir})
	
	set(RES_COPY_CMD cp -rp ${resDir}* ${resourcesDir})
	set(LIB_COPY_CMD rsync -av --exclude=*.h ${libDir}*.framework ${frameworksDir})
	set(LIB_COPY_CMD_2 rsync -av ${libDir}*.dylib ${frameworksDir})
	
	# Icon file
	set(COPY_RES_1 cp -rp ${OSX_BUILD_RESOURCE_DIR}/Bitfighter.icns ${resourcesDir})
	# Public key for Sparkle
	set(COPY_RES_2 cp -rp ${OSX_BUILD_RESOURCE_DIR}/dsa_pub.pem ${resourcesDir})
	# Notifier
	set(COPY_RES_3 cp -rp ${exeDir}/../notifier/bitfighter_notifier.py ${resourcesDir})
	set(COPY_RES_4 cp -rp ${exeDir}/../notifier/redship18.png ${resourcesDir})
	
	add_custom_command(TARGET ${targetName} POST_BUILD 
		COMMAND ${COPY_RES_1}
		COMMAND ${COPY_RES_2}
		COMMAND ${COPY_RES_3}
		COMMAND ${COPY_RES_4}
	)
	
	# Copy game resources into the bundle
	add_custom_command(TARGET ${targetName} POST_BUILD
		COMMAND ${RES_COPY_CMD}
	)

	# The prebuilt frameworks in lib/ are Intel-only.  A native (Homebrew) build
	# links Homebrew dylibs by absolute path and runs in place, so don't bundle
	# or lipo-thin them here.  Producing a self-contained, relocatable .app for
	# distribution (e.g. via dylibbundler) is a separate packaging step.
	if(NOT BF_USE_HOMEBREW_LIBS)
		# 64-bit OSX needs to use shared LuaJIT library
		if(CMAKE_OSX_ARCHITECTURES STREQUAL "x86_64")
			add_custom_command(TARGET ${targetName} POST_BUILD
				COMMAND cp -rp ${luaLibDir}libluajit.dylib ${frameworksDir}
			)
		endif()

		# Copy the bundled frameworks/dylibs into the .app
		add_custom_command(TARGET ${targetName} POST_BUILD
			COMMAND ${LIB_COPY_CMD}
			COMMAND ${LIB_COPY_CMD_2}
		)

		# Thin out our installed frameworks by running 'lipo' to clean out the
		# unwanted architectures and removing any header files
		if(NOT LIPO_COMMAND)
			set(LIPO_COMMAND lipo)
		endif()

		# This can happen when cross-compiling x86_64
		if(NOT CMAKE_OSX_ARCHITECTURES)
			set(CMAKE_OSX_ARCHITECTURES "x86_64")
		endif()

		set(THIN_FRAMEWORKS ${CMAKE_SOURCE_DIR}/build/osx/tools/thin_frameworks.sh ${LIPO_COMMAND} ${CMAKE_OSX_ARCHITECTURES} ${exeDir}/${targetName}.app)
		set(DO_RPATH_THING ${CMAKE_INSTALL_NAME_TOOL} -add_rpath "@executable_path/../Frameworks/" ${exeDir}/${targetName}.app/Contents/MacOS/${targetName})

		add_custom_command(TARGET ${targetName} POST_BUILD
			COMMAND ${THIN_FRAMEWORKS}
			COMMAND ${DO_RPATH_THING}
		)
	endif()
endfunction()


function(BF_PLATFORM_INSTALL targetName)
	set(CMAKE_INSTALL_PREFIX "/Applications")
	
	# This will install the .app.  The .app should have already been built
	# in the post-build section
	install(TARGETS ${targetName} BUNDLE DESTINATION ./)

endfunction()


function(BF_PLATFORM_BUNDLE_DEPENDENCIES targetName)
	# Only meaningful for native Homebrew builds, whose binaries link dylibs by
	# absolute /opt/homebrew path.  Opt-in via BUNDLE_DEPENDENCIES (distribution).
	if(NOT BUNDLE_DEPENDENCIES OR NOT BF_USE_HOMEBREW_LIBS)
		return()
	endif()

	find_program(DYLIBBUNDLER_COMMAND dylibbundler)
	if(NOT DYLIBBUNDLER_COMMAND)
		message(FATAL_ERROR "BUNDLE_DEPENDENCIES requires dylibbundler.  Install it with: brew install dylibbundler")
	endif()

	file(TO_NATIVE_PATH ${CMAKE_SOURCE_DIR}/exe exeDir)
	set(appDir "${exeDir}/${targetName}.app")

	# dylibbundler walks the binary's (transitive) Homebrew dependencies, copies
	# them into Contents/Frameworks, and rewrites each load command to
	# @executable_path/../Frameworks so the bundle resolves them relative to
	# itself.  -ns skips dylibbundler's own ad-hoc signing; we re-sign the whole
	# bundle afterwards, since rewriting the binary invalidates the linker's
	# signature and arm64 requires a valid (here ad-hoc) one to launch.
	add_custom_command(TARGET ${targetName} POST_BUILD
		COMMAND ${DYLIBBUNDLER_COMMAND} -of -b -cd -ns
			-x ${appDir}/Contents/MacOS/${targetName}
			-d ${appDir}/Contents/Frameworks/
			-p @executable_path/../Frameworks/
		COMMAND codesign --force --deep --sign - ${appDir}
		COMMENT "Bundling Homebrew dylibs into ${targetName}.app and ad-hoc signing"
		VERBATIM
	)
endfunction()


function(BF_PLATFORM_CREATE_PACKAGES targetName)
	set(CPACK_GENERATOR "DragNDrop")
	set(CPACK_SYSTEM_NAME "OSX")
	
	if(CMAKE_OSX_ARCHITECTURES STREQUAL "i386")
		set(DMG_ARCH "32bit-Intel")
	elseif(CMAKE_OSX_ARCHITECTURES STREQUAL "arm64")
		set(DMG_ARCH "arm64")
	else()
		set(DMG_ARCH "64bit-Intel")
	endif()
	
	set(CPACK_PACKAGE_FILE_NAME "Bitfighter-${BF_VERSION}-OSX-${DMG_ARCH}")
	set(CPACK_DMG_FORMAT "UDBZ")
	set(CPACK_DMG_VOLUME_NAME "Bitfighter ${BF_VERSION}")
	set(CPACK_DMG_DS_STORE "${OSX_BUILD_RESOURCE_DIR}/bitfighter.dsstore")
	set(CPACK_DMG_BACKGROUND_IMAGE "${OSX_BUILD_RESOURCE_DIR}/bf_dmg_background.png")
	
	#set(CPACK_PACKAGE_ICON "${ICONS_DIR}/DMG.icns")
	
	include(CPack)
endfunction()
