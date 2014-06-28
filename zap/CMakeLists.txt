#------------------------------------------------------------------------------
# Copyright Chris Eykamp
# See LICENSE.txt for full copyright information
#------------------------------------------------------------------------------

set(SHARED_SOURCES
	BanList.cpp
	barrier.cpp
	BfObject.cpp
	BotNavMeshZone.cpp
	ChatCheck.cpp
	ClientInfo.cpp
	Color.cpp
	config.cpp
	Console.cpp
	controlObjectConnection.cpp
	CoreGame.cpp
	CTFGame.cpp
	dataConnection.cpp
	DisplayManager.cpp
	EngineeredItem.cpp
	EventManager.cpp
	flagItem.cpp
	game.cpp
	gameConnection.cpp
	gameNetInterface.cpp
	GameRecorder.cpp
	GameSettings.cpp
	gameStats.cpp
	gameType.cpp
	gameWeapons.cpp
	GameManager.cpp
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
	Level.cpp
	LevelDatabase.cpp
	LevelLoadException.cpp
	LevelSource.cpp
	LineItem.cpp
	LoadoutTracker.cpp
	loadoutZone.cpp
	LuaBase.cpp
	LuaGlobals.cpp
	luaGameInfo.cpp
	luaLevelGenerator.cpp
	LuaScriptRunner.cpp
	masterConnection.cpp
	MathUtils.cpp
	Md5Utils.cpp
	move.cpp
	moveObject.cpp
	NexusGame.cpp
	PickupItem.cpp
	playerInfo.cpp
	Point.cpp
	PointObject.cpp
	polygon.cpp
	projectile.cpp
	rabbitGame.cpp
	Rect.cpp
	retrieveGame.cpp
	robot.cpp
	RobotManager.cpp
	ScreenInfo.cpp
	ServerGame.cpp
	Settings.cpp
	ship.cpp
	shipItems.cpp
	SimpleLine.cpp
	SlipZone.cpp
	soccerGame.cpp
	SoundEffect.cpp
	SoundSystem.cpp
	Spawn.cpp
	speedZone.cpp
	StackTracer.cpp
	StackWalker.cpp
	statistics.cpp
	stringUtils.cpp
	SystemFunctions.cpp
	teamInfo.cpp
	Teleporter.cpp
	TextItem.cpp
	Timer.cpp
	WallSegmentManager.cpp
	WeaponInfo.cpp
	Zone.cpp
	zoneControlGame.cpp
	${SQLITE3_FILES}
	${CMAKE_SOURCE_DIR}/master/database.cpp
	${CMAKE_SOURCE_DIR}/master/masterInterface.cpp
	${CMAKE_SOURCE_DIR}/recast/RecastAlloc.cpp
	${CMAKE_SOURCE_DIR}/recast/RecastMesh.cpp
	${CMAKE_SOURCE_DIR}/poly2tri/common/shapes.cc
	${CMAKE_SOURCE_DIR}/poly2tri/sweep/advancing_front.cc
	${CMAKE_SOURCE_DIR}/poly2tri/sweep/cdt.cc
	${CMAKE_SOURCE_DIR}/poly2tri/sweep/sweep.cc
	${CMAKE_SOURCE_DIR}/poly2tri/sweep/sweep_context.cc
)

set(CLIENT_SOURCES 
	AToBScroller.cpp
	ChatCommands.cpp
	ChatHelper.cpp
	ConnectionStatsRenderer.cpp
	ClientGame.cpp
	Cursor.cpp
	EditorAttributeMenuItemBuilder.cpp
	EditorPlugin.cpp
	EditorTeam.cpp
	EnergyGaugeRenderer.cpp
	engineerHelper.cpp
	Event.cpp
	FontManager.cpp
	FpsRenderer.cpp
	gameObjectRender.cpp
	GameRecorderPlayback.cpp
	HelpItemManager.cpp
	HelperManager.cpp
	helperMenu.cpp
	Joystick.cpp
	JoystickRender.cpp
	LevelDatabaseDownloadThread.cpp
	LevelDatabaseRateThread.cpp
	LevelDatabaseUploadThread.cpp
	lineEditor.cpp
	LoadoutIndicator.cpp
	loadoutHelper.cpp
	oglconsole.cpp
	OpenglUtils.cpp
	quickChatHelper.cpp
	RenderUtils.cpp
	ScissorsManager.cpp
	ScreenShooter.cpp
	ShipShape.cpp
	SlideOutWidget.cpp
	sparkManager.cpp
	SymbolShape.cpp
	TeamShuffleHelper.cpp
	TimeLeftRenderer.cpp
	UI.cpp
	UIAbstractInstructions.cpp
	UIChat.cpp
	UIColorPicker.cpp
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
	VideoSystem.cpp
	voiceCodec.cpp
	${CMAKE_SOURCE_DIR}/fontstash/stb_truetype.c
	${CMAKE_SOURCE_DIR}/fontstash/fontstash.c
)

# Variable to old extra sources that must be compiled into the exe last
# like the Windows .res file
set(EXTRA_EXECUTABLE_SOURCES
)

file(GLOB BITFIGHTER_HEADERS *.h)

file(GLOB OTHER_HEADERS
	${CLIPPER_INCLUDE_DIR}/*.hpp 
	${CMAKE_SOURCE_DIR}/other/*.h 
	${CMAKE_SOURCE_DIR}/fontstash/*.h 
	${CMAKE_SOURCE_DIR}/recast/*.h 
	${CMAKE_SOURCE_DIR}/sqlite/*.h
	${CMAKE_SOURCE_DIR}/poly2tri/*.h
	${CMAKE_SOURCE_DIR}/poly2tri/common/*.h
	${CMAKE_SOURCE_DIR}/poly2tri/sweep/*.h
)

BF_PLATFORM_SET_EXTRA_SOURCES()

BF_PLATFORM_SET_EXTRA_EXECUTABLE_SOURCES()


#
# Libraries
#

BF_PLATFORM_SET_EXTRA_LIBS()

# Libraries used by both client and server
set(SHARED_LIBS
	${CMAKE_THREAD_LIBS_INIT}
	${LUA_LIB}
	tnl
	${SQLITE3_LIBRARIES}
	${EXTRA_LIBS}
)

if(CLIPPER_LIBRARIES)
	set(SHARED_LIBS ${SHARED_LIBS} ${CLIPPER_LIBRARIES})
endif()

if(TOMCRYPT_LIBRARIES)
	set(SHARED_LIBS ${SHARED_LIBS} ${TOMCRYPT_LIBRARIES})
endif()

# libraries used only on the client
set(CLIENT_LIBS
	${SDL_LIBRARY}
	${PNG_LIBRARY}
	${GL_LIBRARY}
	${OPENAL_LIBRARY}
	${ALURE_LIBRARIES}
	${MODPLUG_LIBRARIES}
	${OGG_LIBRARIES}
	${SPEEX_LIBRARIES}
	${VORBIS_LIBRARIES}
	${VORBISFILE_LIBRARIES}
)

BF_PLATFORM_APPEND_LIBS()

BF_PLATFORM_ADD_DEFINITIONS()


#
# Targets
#

# Disable compiling of client and test suite if certain dependencies arent met
set(COMPILE_CLIENT YES)
if(NOT SDL2_FOUND AND NOT SDL_FOUND)
	set(COMPILE_CLIENT NO)
endif()

# Tests require the test sources, which may not be available (like in a slimmed source archive)
set(COMPILE_TEST_SUITE YES)
if(NOT EXISTS ${CMAKE_SOURCE_DIR}/bitfighter_test)
	set(COMPILE_TEST_SUITE NO)
endif()


# We should always be able to compile a dedicated server, it requires much
# fewer dependencies
include(bitfighterd.cmake)

if(COMPILE_CLIENT)
	include(bitfighter_client.cmake)
	include(bitfighter.cmake)
	
	# The test suite requires the client dependencies
	if(COMPILE_TEST_SUITE)
		include(bitfighter_test.cmake)
	endif()
endif()


# Build up our include directories based on the targets

# Defaults for bitfighterd
set(INCLUDE_DIRS
	${OPENAL_INCLUDE_DIR}
	${ALURE_INCLUDE_DIR}
	${LUA_INCLUDE_DIR}
	${TOMCRYPT_INCLUDE_DIR}
	${CLIPPER_INCLUDE_DIR}
	${SQLITE3_INCLUDE_DIR}
	${BOOST_INCLUDE_DIR}
	${CMAKE_SOURCE_DIR}/tnl
	${CMAKE_SOURCE_DIR}/zap
)

if(COMPILE_CLIENT)
	list(APPEND INCLUDE_DIRS
		${GL_INCLUDE_DIR}
		${PNG_INCLUDE_DIR}
		${SDL_INCLUDE_DIR}
		${SPEEX_INCLUDE_DIR}
		${SPARKLE_INCLUDE_DIR}
	)
	
	if(COMPILE_TEST_SUITE)
		list(APPEND INCLUDE_DIRS ${CMAKE_SOURCE_DIR}/gtest/include)
	endif()
endif()

# CMake 2.8.11 provides a target_include_directories, which is exactly what we want,
# but we don't use it for compatibility with older CMake 2.8
include_directories(SYSTEM
	${INCLUDE_DIRS}
)
