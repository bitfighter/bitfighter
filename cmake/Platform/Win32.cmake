## Global project configuration

# Win64
if(CMAKE_SIZEOF_VOID_P EQUAL 8)
	message(STATUS "Win64 detected")
	set(BF_LIB_DIR ${CMAKE_SOURCE_DIR}/lib/win64)
	set(WIN64 TRUE)
else()
	message(STATUS "Win32 detected")
	set(BF_LIB_DIR ${CMAKE_SOURCE_DIR}/lib)
endif()

#
# Linker flags
# 
if(MSVC)
	# Using the following NODEFAULTLIB to fix LNK4098 warning and some linker errors
	set(CMAKE_EXE_LINKER_FLAGS_DEBUG          "${CMAKE_EXE_LINKER_FLAGS_DEBUG}          /NODEFAULTLIB:libc.lib;libcmt.lib;msvcrt.lib;libcd.lib;msvcrtd.lib")
	set(CMAKE_EXE_LINKER_FLAGS_RELEASE        "${CMAKE_EXE_LINKER_FLAGS_RELEASE}        /NODEFAULTLIB:libc.lib;msvcrt.lib;libcd.lib;libcmtd.lib;msvcrtd.lib")
	set(CMAKE_EXE_LINKER_FLAGS_RELWITHDEBINFO "${CMAKE_EXE_LINKER_FLAGS_RELWITHDEBINFO} /NODEFAULTLIB:libc.lib;msvcrt.lib;libcd.lib;libcmtd.lib;msvcrtd.lib")
	set(CMAKE_EXE_LINKER_FLAGS_MINSIZEREL     "${CMAKE_EXE_LINKER_FLAGS_MINSIZEREL}     /NODEFAULTLIB:libc.lib;msvcrt.lib;libcd.lib;libcmtd.lib;msvcrtd.lib")

	# Disable incremental linking on RelWithDebInfo for sam686
	set(LinkerFlags
		CMAKE_EXE_LINKER_FLAGS_RELWITHDEBINFO
	)
	
	foreach(LinkerFlag ${LinkerFlags})
		# Handle both /INCREMENTAL:YES and /INCREMENTAL
		# .. First set everything to '/INCREMENTAL', then turn it off
		string(REPLACE "INCREMENTAL:YES" "INCREMENTAL" ${LinkerFlag} "${${LinkerFlag}}")
		string(REPLACE "INCREMENTAL" "INCREMENTAL:NO" ${LinkerFlag} "${${LinkerFlag}}")
	endforeach()
endif()

if(MINGW)
	# MinGW won't statically compile in Microsofts c/c++ library routines
	set(BF_LINK_FLAGS "-Wl,--as-needed -static-libgcc -static-libstdc++")
	
	# Only link in what is absolutely necessary
	set(CMAKE_EXE_LINKER_FLAGS ${BF_LINK_FLAGS})
endif()

if(XCOMPILE)
	# Disable LuaJIT for cross-compile (for now)
	set(USE_LUAJIT NO)
	
	# StackWalker has too much black magic for mingw
	add_definitions(-DBF_NO_STACKTRACE)
endif()


# 
# Compiler specific flags
# 
if(MSVC)
	# Using /MT avoids dynamically linking against the stupid MSVC runtime libraries
	set(CompilerFlags
		CMAKE_CXX_FLAGS
		CMAKE_CXX_FLAGS_DEBUG
		CMAKE_CXX_FLAGS_RELEASE
		CMAKE_CXX_FLAGS_RELWITHDEBINFO
		CMAKE_C_FLAGS
		CMAKE_C_FLAGS_DEBUG
		CMAKE_C_FLAGS_RELEASE
		CMAKE_C_FLAGS_RELWITHDEBINFO
	)
	foreach(CompilerFlag ${CompilerFlags})
		string(REPLACE "/MD" "/MT" ${CompilerFlag} "${${CompilerFlag}}")
	endforeach()
	
	# Enable 'Edit and Continue' debugging support
	set(CMAKE_C_FLAGS_DEBUG "${CMAKE_C_FLAGS_DEBUG} /ZI")
	set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} /ZI")
	
	# Add parallel build to Visual Studio
	set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /MP")

	# Disable deprecated warnings (fopen, vsnprintf), some alternative function may not exist on other compilers including gcc.
	set(CMAKE_C_FLAGS "${CMAKE_CXX_FLAGS} /wd4996")
	set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /wd4996")
endif()

if(MINGW)
	set(CMAKE_RC_COMPILER_INIT windres)
	enable_language(RC)
	set(CMAKE_RC_COMPILE_OBJECT "<CMAKE_RC_COMPILER> -i <SOURCE> -o <OBJECT>")
endif()


#
# Library searching and dependencies
#

# Set some search paths
set(SDL2_SEARCH_PATHS       ${BF_LIB_DIR} ${CMAKE_SOURCE_DIR}/libsdl)
set(OGG_SEARCH_PATHS        ${BF_LIB_DIR} ${CMAKE_SOURCE_DIR}/libogg)
set(VORBIS_SEARCH_PATHS	    ${BF_LIB_DIR} ${CMAKE_SOURCE_DIR}/libvorbis)
set(VORBISFILE_SEARCH_PATHS	${BF_LIB_DIR} ${CMAKE_SOURCE_DIR}/libvorbis)
set(SPEEX_SEARCH_PATHS      ${BF_LIB_DIR} ${CMAKE_SOURCE_DIR}/libspeex)
set(MODPLUG_SEARCH_PATHS    ${BF_LIB_DIR} ${CMAKE_SOURCE_DIR}/libmodplug)

# Directly set include dirs for some libraries
set(OPENAL_INCLUDE_DIR "${CMAKE_SOURCE_DIR}/openal/include")
set(ZLIB_INCLUDE_DIR "${CMAKE_SOURCE_DIR}/zlib")
# libpng needs two for some weird reason
set(PNG_INCLUDE_DIR "${CMAKE_SOURCE_DIR}/libpng")
set(PNG_PNG_INCLUDE_DIR "${CMAKE_SOURCE_DIR}/libpng")
set(PHYSFS_INCLUDE_DIR ${BF_LIB_DIR}/include/physfs)

# Directly specify some libs (because of deficiences in CMake modules?)
set(OPENAL_LIBRARY "${BF_LIB_DIR}/OpenAL32.lib")
set(ZLIB_LIBRARY   "${BF_LIB_DIR}/zlib.lib")
set(PNG_LIBRARY    "${BF_LIB_DIR}/libpng14.lib")
set(PHYSFS_LIBRARY "${BF_LIB_DIR}/physfs.lib")
	
find_package(VorbisFile)


## End Global project configuration


## Sub-project configuration
#
# Note that any variable adjustment from the parent CMakeLists.txt will
# need to be re-set with the PARENT_SCOPE option

function(BF_PLATFORM_SET_EXTRA_SOURCES)
	if(NOT XCOMPILE)
		# Add icon resource in Visual Studio.  This must be added into the final
		# executable
		list(APPEND EXTRA_SOURCES ZAP.rc)
		set(EXTRA_SOURCES ${EXTRA_SOURCES} PARENT_SCOPE)
	endif()
endfunction()


function(BF_PLATFORM_SET_EXTRA_LIBS)
	set(EXTRA_LIBS ws2_32 winmm PARENT_SCOPE)
endfunction()


function(BF_PLATFORM_APPEND_LIBS)
	# Do nothing!
endfunction()


function(BF_PLATFORM_ADD_DEFINITIONS)
	if(NOT MSVC)
		add_definitions(-iquote ${CMAKE_SOURCE_DIR}/zap)
	endif()
endfunction()


function(BF_PLATFORM_SET_EXECUTABLE_NAME)
	# Do nothing!
endfunction()


function(BF_PLATFORM_SET_TARGET_PROPERTIES targetName)
	if(MSVC)
		# Work around the "Debug", "Release", etc. directories Visual Studio tries to add
		foreach(OUTPUTCONFIG ${CMAKE_CONFIGURATION_TYPES})
			string(TOUPPER ${OUTPUTCONFIG} OUTPUTCONFIG)
			set_target_properties(${targetName}
				PROPERTIES RUNTIME_OUTPUT_DIRECTORY_${OUTPUTCONFIG} ${CMAKE_SOURCE_DIR}/exe
			)
		endforeach()
		
		# Separate output name "bitfighter_debug.exe" for debug build, to avoid conflicts with debug/release build
		set_target_properties(${targetName} PROPERTIES DEBUG_POSTFIX "_debug")

		# Set some linker flags to use console mode in debug build, etc..
		# Always use SUBSYSTEM:CONSOLE; hiding the console window is controlled in zap/main.cpp near the bottom of main()
		# Allows console to stay visible if ran from typing in command window.
		set_target_properties(${targetName} PROPERTIES LINK_FLAGS_DEBUG "/SUBSYSTEM:CONSOLE")
		set_target_properties(${targetName} PROPERTIES LINK_FLAGS_RELWITHDEBINFO "/SUBSYSTEM:CONSOLE")
		set_target_properties(${targetName} PROPERTIES LINK_FLAGS_RELEASE "/SUBSYSTEM:CONSOLE")
		set_target_properties(${targetName} PROPERTIES LINK_FLAGS_MINSIZEREL "/SUBSYSTEM:CONSOLE")
		
		# Set more compiler flags for console on appropriate targets
		list(APPEND ALL_DEBUG_DEFS "_CONSOLE")
	endif()
endfunction()


function(BF_PLATFORM_SET_TARGET_OTHER_PROPERTIES targetName)
	# Do nothing
endfunction()


function(BF_PLATFORM_POST_BUILD_INSTALL_RESOURCES targetName)
	# The trailing slash is necessary to do here for proper native path translation
	file(TO_NATIVE_PATH ${CMAKE_SOURCE_DIR}/resource/ resDir)
	file(TO_NATIVE_PATH ${BF_LIB_DIR}/ libDir)
	file(TO_NATIVE_PATH ${CMAKE_SOURCE_DIR}/lua/luajit/src/ luaLibDir)
	file(TO_NATIVE_PATH ${CMAKE_SOURCE_DIR}/exe exeDir)
	
	# Set copy command
	if(MSYS OR CYGWIN OR XCOMPILE)
		set(RES_COPY_CMD cp -r ${resDir}* ${exeDir})
		set(LIB_COPY_CMD cp ${libDir}*.dll ${exeDir})
	else()
		set(RES_COPY_CMD xcopy /e /d /y ${resDir}* ${exeDir})
		set(LIB_COPY_CMD xcopy /d /y ${libDir}*.dll ${exeDir})
	endif()
	
	# Copy resources
	add_custom_command(TARGET ${targetName} POST_BUILD 
		COMMAND ${RES_COPY_CMD}
		COMMAND ${LIB_COPY_CMD}
	)
endfunction()


function(BF_PLATFORM_INSTALL targetName)
	# Binaries
	install(TARGETS ${targetName} RUNTIME DESTINATION ./)
	install(PROGRAMS ${CMAKE_SOURCE_DIR}/notifier/pyinstaller/dist/bitfighter_notifier.exe DESTINATION ./)
	
	# Libraries
	file(GLOB BF_INSTALL_LIBS ${BF_LIB_DIR}/*.dll)
	# Except libcurl which will be put into the notifier directory
	#list(REMOVE_ITEM BF_INSTALL_LIBS "${BF_LIB_DIR}/libcurl.dll")
	install(FILES ${BF_INSTALL_LIBS} DESTINATION ./)
	
	# Resources
	install(DIRECTORY ${CMAKE_SOURCE_DIR}/resource/ DESTINATION ./)
	install(FILES ${CMAKE_SOURCE_DIR}/exe/joystick_presets.ini DESTINATION ./)
	install(FILES ${CMAKE_SOURCE_DIR}/zap/bitfighter_win_icon_green.ico DESTINATION ./)
	
	# Doc
	install(FILES ${CMAKE_SOURCE_DIR}/doc/readme.txt DESTINATION ./)
	install(FILES ${CMAKE_SOURCE_DIR}/LICENSE.txt DESTINATION ./)
	install(FILES ${CMAKE_SOURCE_DIR}/COPYING.txt DESTINATION ./)
	
	# Updater
	install(FILES ${CMAKE_SOURCE_DIR}/exe/updater/bfup.exe DESTINATION updater)
	install(FILES ${CMAKE_SOURCE_DIR}/exe/updater/bfup.xml DESTINATION updater)
	install(FILES ${CMAKE_SOURCE_DIR}/exe/updater/libcurl.dll DESTINATION updater)
	
	# Other
	install(FILES ${CMAKE_SOURCE_DIR}/build/windows/installer/twoplayers.bat DESTINATION ./)
endfunction()


function(BF_PLATFORM_CREATE_PACKAGES targetName)
	set(CPACK_PACKAGE_NAME "Bitfighter")
	set(CPACK_PACKAGE_DESCRIPTION_SUMMARY "A 2-D multi-player space combat game")
	set(CPACK_PACKAGE_VENDOR "Bitfighter Industries")
	set(CPACK_PACKAGE_DESCRIPTION_FILE "${CMAKE_SOURCE_DIR}/README.txt")
	set(CPACK_RESOURCE_FILE_LICENSE "${CMAKE_SOURCE_DIR}/LICENSE.txt")
	set(CPACK_PACKAGE_VERSION_MAJOR ${BF_VERSION})
	set(CPACK_PACKAGE_INSTALL_DIRECTORY ${CPACK_PACKAGE_NAME})
	set(CPACK_CREATE_DESKTOP_LINKS ${targetName})
	# This sets up start menu and desktop shortcuts
	set(CPACK_PACKAGE_EXECUTABLES "bitfighter;Bitfighter" "bitfighter_notifier;Bitfighter Notifier")
	
	set(BF_PACKAGE_RESOURCE_DIR ${CMAKE_SOURCE_DIR}/build/windows/installer)
	
	if(WIN64)
		# We use WiX for x64 MSI
		set(CPACK_GENERATOR WIX)
		set(CPACK_PACKAGE_FILE_NAME "Bitfighter-${BF_VERSION}-x64-installer")
		
		# Keep this the same so MSI installers can update/repair across versions
		set(CPACK_WIX_UPGRADE_GUID "5E1F1E55-11FE-1E55-BAAD-00B17F164732")
		set(CPACK_WIX_UI_DIALOG ${BF_PACKAGE_RESOURCE_DIR}/wix_welcome_banner.bmp)
		set(CPACK_WIX_UI_BANNER ${BF_PACKAGE_RESOURCE_DIR}/wix_header_banner.bmp)
		set(CPACK_WIX_PROGRAM_MENU_FOLDER ${CPACK_PACKAGE_NAME})
		
		# Wix requires some version, but can't handle bitfighter versions because of the letters
		set(CPACK_PACKAGE_VERSION_MAJOR 1)
	else()
		# NSIS setup
		set(CPACK_GENERATOR NSIS) # TODO add ZIP for portable install?
		set(CPACK_PACKAGE_FILE_NAME "Bitfighter-${BF_VERSION}-win32-installer")
		set(CPACK_NSIS_COMPRESSOR "/SOLID lzma")
		set(CPACK_NSIS_HELP_LINK "http://bitfighter.org/")
		set(CPACK_NSIS_URL_INFO_ABOUT "http://bitfighter.org/")
		
		# Desktop shortcut handling for install/uninstall
		set(CPACK_NSIS_EXTRA_INSTALL_COMMANDS "CreateShortCut \\\"$DESKTOP\\\\Bitfighter.lnk\\\" \\\"$INSTDIR\\\\bitfighter.exe\\\"")
		set(CPACK_NSIS_EXTRA_UNINSTALL_COMMANDS "Delete \\\"$DESKTOP\\\\Bitfighter.lnk\\\"")

		# Any extra start menu shortcuts
		set(CPACK_NSIS_MENU_LINKS 
			"http://bitfighter.org/" "Bitfighter Home Page"
			"http://bitfighter.org/forums/" "Bitfighter Forums")
		
		# Branding
		# Four backslashes because NSIS can't resolve the last portion of a UNIX path.  Fun!
		set(CPACK_PACKAGE_ICON "${BF_PACKAGE_RESOURCE_DIR}\\\\nsis_header_banner.bmp")
		set(WELCOME_BANNER ${BF_PACKAGE_RESOURCE_DIR}\\\\nsis_welcome_banner.bmp) 
		set(CPACK_NSIS_INSTALLER_MUI_ICON_CODE "BrandingText \\\"${CPACK_PACKAGE_NAME} ${BF_VERSION}\\\"
			!define MUI_WELCOMEFINISHPAGE_BITMAP \\\"${WELCOME_BANNER}\\\"")
		
		# Need this otherwise NSIS thinks executables are in the 'bin' sub-folder
		set(CPACK_NSIS_EXECUTABLES_DIRECTORY ".")
		
		set(CPACK_NSIS_MUI_FINISHPAGE_RUN "bitfighter.exe")
	endif()
	
	include(CPack)
endfunction()
