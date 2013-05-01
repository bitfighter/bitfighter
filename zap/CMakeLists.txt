# Search for our dependencies
# SDL on Linux, SDL2 on others
if("${CMAKE_SYSTEM}" MATCHES "Linux")
	find_package(SDL REQUIRED)
else()
	find_package(SDL2 REQUIRED)
	
	if(SDL2_FOUND)
		set(SDL_INCLUDE_DIR ${SDL2_INCLUDE_DIR})
		set(SDL_LIBRARY ${SDL2_LIBRARY})
	endif()
endif()
if(MSVC)
	find_package(ALURE)
else()
	set(ALURE_LIBRARIES alure)
endif()
find_package(OpenAL REQUIRED)
find_package(OpenGL)
find_package(Threads REQUIRED)
find_package(PNG)
find_package(OGG)
find_package(Vorbis)
find_package(VorbisFile)
find_package(Speex)
find_package(ModPlug)
	

set(SHARED_SOURCES
	BanList.cpp
	barrier.cpp
	BfObject.cpp
	BotNavMeshZone.cpp
	ChatCheck.cpp
	ClientInfo.cpp
	Color.cpp
	config.cpp
	controlObjectConnection.cpp
	CoreGame.cpp
	CTFGame.cpp
	dataConnection.cpp
	EditorObject.cpp
	EditorPlugin.cpp
	EngineeredItem.cpp
	EventManager.cpp
	flagItem.cpp
	game.cpp
	gameConnection.cpp
	gameLoader.cpp
	gameNetInterface.cpp
	GameSettings.cpp
	gameStats.cpp
	gameType.cpp
	gameWeapons.cpp
	Geometry.cpp
	GeomObject.cpp
	GeomUtils.cpp
	goalZone.cpp
	gridDB.cpp
	HTFGame.cpp
	HttpRequest.cpp
	IniFile.cpp
	InputCode.cpp
	item.cpp
	lineEditor.cpp
	LineItem.cpp
	LoadoutTracker.cpp
	loadoutZone.cpp
	LuaBase.cpp
	luaGameInfo.cpp
	luaLevelGenerator.cpp
	LuaScriptRunner.cpp
	main.cpp
	masterConnection.cpp
	MathUtils.cpp
	md5wrapper.cpp
	move.cpp
	moveObject.cpp
	NexusGame.cpp
	PickupItem.cpp
	playerInfo.cpp
	Point.cpp
	polygon.cpp
	projectile.cpp
	rabbitGame.cpp
	Rect.cpp
	retrieveGame.cpp
	robot.cpp
	ScreenInfo.cpp
	ServerGame.cpp
	ship.cpp
	shipItems.cpp
	SimpleLine.cpp
	SlipZone.cpp
	soccerGame.cpp
	SoundEffect.cpp
	SoundSystem.cpp
	Spawn.cpp
	speedZone.cpp
	statistics.cpp
	stringUtils.cpp
	teamInfo.cpp
	teleporter.cpp
	textItem.cpp
	Timer.cpp
	WallSegmentManager.cpp
	WeaponInfo.cpp
	Zone.cpp
	zoneControlGame.cpp
	../clipper/clipper.cpp
	../master/database.cpp
	../master/masterInterface.cpp
	../recast/RecastAlloc.cpp
	../recast/RecastMesh.cpp
	../sqlite/sqlite3.c
	../poly2tri/common/shapes.cc
	../poly2tri/sweep/advancing_front.cc
	../poly2tri/sweep/cdt.cc
	../poly2tri/sweep/sweep.cc
	../poly2tri/sweep/sweep_context.cc
)

set(CLIENT_SOURCES 
	AToBScroller.cpp
	ChatCommands.cpp
	ChatHelper.cpp
	ClientGame.cpp
	Console.cpp
	Cursor.cpp
	engineerHelper.cpp
	Event.cpp
	FontManager.cpp
	FpsRenderer.cpp
	gameObjectRender.cpp
	HelpItemManager.cpp
	HelperManager.cpp
	helperMenu.cpp
	Joystick.cpp
	JoystickRender.cpp
	LevelDatabaseDownloadThread.cpp
	LevelDatabaseUploadThread.cpp
	LoadoutIndicator.cpp
	loadoutHelper.cpp
	oglconsole.c
	OpenglUtils.cpp
	quickChatHelper.cpp
	RenderUtils.cpp
	ScissorsManager.cpp
	ScreenShooter.cpp
	ShipShape.cpp
	SlideOutWidget.cpp
	sparkManager.cpp
	TeamShuffleHelper.cpp
	TimeLeftRenderer.cpp
	UI.cpp
	UIAbstractInstructions.cpp
	UIChat.cpp
	UICredits.cpp
	UIDiagnostics.cpp
	UIEditor.cpp
	UIEditorInstructions.cpp
	UIEditorMenus.cpp
	UIErrorMessage.cpp
	UIGame.cpp
	UIGameParameters.cpp
	UIHighScores.cpp
	UIInstructions.cpp
	UIKeyDefMenu.cpp
	UILevelInfoDisplayer.cpp
	UIManager.cpp
	UIMenuItems.cpp
	UIMenus.cpp
	UIMessage.cpp
	UINameEntry.cpp
	UIQueryServers.cpp
	UITeamDefMenu.cpp
	UIYesNo.cpp
	VideoSystem.cpp
	voiceCodec.cpp
	../fontstash/stb_truetype.c
	../fontstash/fontstash.c
)

set(TEST_SOURCES
	${CMAKE_SOURCE_DIR}/bitfighter_test/main_test.cpp
)


if(MSVC)
	set(EXTRA_LIBS ws2_32 winmm)
else()
	set(EXTRA_LIBS dl m)
endif()
# libraries used by both client and server
set(SHARED_LIBS
	${CMAKE_THREAD_LIBS_INIT}
	${EXTRA_LIBS}
	lua-vec
	tnl
	tomcrypt
)

# libraries used only on the client
set(CLIENT_LIBS
	${SDL_LIBRARY}
	${PNG_LIBRARY}
	${OPENGL_LIBRARY}
	${OPENAL_LIBRARY}
	${ALURE_LIBRARIES}
	${MODPLUG_LIBRARIES}
	${OGG_LIBRARIES}
	${SPEEX_LIBRARIES}
	${VORBIS_LIBRARIES}
	${VORBISFILE_LIBRARIES}
)

if(SDL_FOUND OR SDL2_FOUND)
	add_executable(bitfighter
		${SHARED_SOURCES}
		${CLIENT_SOURCES}
	)

	add_dependencies(bitfighter
		alure
		lua-vec
		tnl
		tomcrypt
	)

	target_link_libraries(bitfighter
		${SHARED_LIBS}
		${CLIENT_LIBS}
	)

	include_directories(
		${OPENAL_INCLUDE_DIR}
		${OPENGL_INCLUDE_DIR}
		${PNG_INCLUDE_DIR}
		${SDL_INCLUDE_DIR}
		${SPEEX_INCLUDE_DIR}
	)

	# Where to put the executable
	set_target_properties(bitfighter PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${CMAKE_SOURCE_DIR}/exe)
	
	# Install windows libraries and resources
	if(MSVC)
		file(TO_NATIVE_PATH ${CMAKE_SOURCE_DIR}/resource/ resDir)
		file(TO_NATIVE_PATH ${CMAKE_SOURCE_DIR}/lib/ libDir)
		file(TO_NATIVE_PATH ${CMAKE_SOURCE_DIR}/exe exeDir)
	
		add_custom_command(TARGET bitfighter POST_BUILD 
			COMMAND xcopy /e /d /y ${resDir}* ${exeDir} 
			COMMAND xcopy /d /y ${libDir}*.dll ${exeDir}
		)
		
		# Work around the "Debug", "Release", etc. directories Visual Studio tries to add
		foreach(OUTPUTCONFIG ${CMAKE_CONFIGURATION_TYPES})
			string(TOUPPER ${OUTPUTCONFIG} OUTPUTCONFIG)
			set_target_properties(bitfighter 
				PROPERTIES RUNTIME_OUTPUT_DIRECTORY_${OUTPUTCONFIG} ${CMAKE_SOURCE_DIR}/exe
			)
		endforeach()
	endif()
endif()


add_executable(bitfighterd
	EXCLUDE_FROM_ALL
	${SHARED_SOURCES}
)

add_dependencies(bitfighterd
	tnl
	lua-vec
	tomcrypt
)

target_link_libraries(bitfighterd
	${SHARED_LIBS}
)

set_target_properties(bitfighterd
	PROPERTIES
	RUNTIME_OUTPUT_DIRECTORY ${CMAKE_SOURCE_DIR}/exe
)

if(CMAKE_BUILD_TYPE STREQUAL "Debug")
	set_target_properties(bitfighterd bitfighter
		PROPERTIES COMPILE_DEFINITIONS TNL_DEBUG
	)
endif()


if(WIN32)
	if(MSVC)
		# Set some special linker flags for Windows visual studio
		set_target_properties(bitfighter PROPERTIES LINK_FLAGS_DEBUG "/SUBSYSTEM:CONSOLE")
		set_target_properties(bitfighter PROPERTIES COMPILE_DEFINITIONS_DEBUG "_CONSOLE")
		set_target_properties(bitfighter PROPERTIES LINK_FLAGS_RELWITHDEBINFO "/SUBSYSTEM:CONSOLE")
		set_target_properties(bitfighter PROPERTIES COMPILE_DEFINITIONS_RELWITHDEBINFO "_CONSOLE")
		set_target_properties(bitfighter PROPERTIES LINK_FLAGS_RELEASE "/SUBSYSTEM:WINDOWS")
		set_target_properties(bitfighter PROPERTIES LINK_FLAGS_MINSIZEREL "/SUBSYSTEM:WINDOWS")
	endif()
endif()

get_property(DEDICATED_DEFS TARGET bitfighterd PROPERTY COMPILE_DEFINITIONS)
set_target_properties(bitfighterd
	PROPERTIES
	COMPILE_DEFINITIONS "${DEDICATED_DEFS};ZAP_DEDICATED"
)

include_directories(
	${CMAKE_SOURCE_DIR}/tnl
	${CMAKE_SOURCE_DIR}/boost
	${CMAKE_SOURCE_DIR}/gtest/include
)

# test runner executable
add_executable(test
	${CLIENT_SOURCES}
	${SHARED_SOURCES}
	${TEST_SOURCES}
)

target_link_libraries(test
	${SHARED_LIBS}
	${CLIENT_LIBS}
	gtest
)

add_dependencies(test
	alure
	lua-vec
	tnl
	tomcrypt
	gtest
)

if(NOT MSVC)
	add_definitions(-iquote ${CMAKE_SOURCE_DIR}/zap)
endif()

set_target_properties(test
	PROPERTIES
	RUNTIME_OUTPUT_DIRECTORY ${CMAKE_SOURCE_DIR}/exe
	COMPILE_DEFINITIONS BITFIGHTER_TEST
)
