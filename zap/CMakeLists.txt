#------------------------------------------------------------------------------
# Copyright Chris Eykamp
# See LICENSE.txt for full copyright information
#------------------------------------------------------------------------------

file(STRINGS version.h versionLine REGEX "define[ ]+ZAP_GAME_RELEASE")
string(REGEX REPLACE "#define[ ]+ZAP_GAME_RELEASE[ ]+\"([^\"]+)\".*" "\\1" BF_VERSION ${versionLine})
message(STATUS "Bitfighter version: ${BF_VERSION}")

file(STRINGS version.h buildVersionStringDefine REGEX "define[ ]+BUILD_VERSION")
string(REGEX REPLACE "#define[ ]+BUILD_VERSION[ ]+([^ ]+).*" "\\1" buildVersionString ${buildVersionStringDefine})
file(STRINGS version.h buildVersionLine REGEX "${buildVersionString}")
string(REGEX REPLACE "#define[ ]+${buildVersionString}[ ]+([0-9]+).*" "\\1" BF_BUILD_VERSION ${buildVersionLine})
message(STATUS "Bitfighter build version: ${BF_BUILD_VERSION}")

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
	gameLoader.cpp
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
	LevelDatabase.cpp
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
	md5wrapper.cpp
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
	${CMAKE_SOURCE_DIR}/recast/RecastAlloc.cpp
	${CMAKE_SOURCE_DIR}/recast/RecastMesh.cpp
)

# Extra sources that are needed by the final executable.  The existence
# of these probably suggests some problem with our code
set(EXTRA_SOURCES
	${CMAKE_SOURCE_DIR}/master/database.cpp
	${CMAKE_SOURCE_DIR}/master/masterInterface.cpp
)

# Client-only classes
set(CLIENT_SOURCES 
	AppIntegrator.cpp
	AToBScroller.cpp
	ChatCommands.cpp
	ChatHelper.cpp
	ConnectionStatsRenderer.cpp
	ClientGame.cpp
	Cursor.cpp
	EditorAttributeMenuItemBuilder.cpp
	EditorPlugin.cpp
	EditorTeam.cpp
	engineerHelper.cpp
	Event.cpp
	FontManager.cpp
	FpsRenderer.cpp
	gameObjectRender.cpp
	GameRecorderPlayback.cpp
    GaugeRenderer.cpp
	GL2Renderer.cpp
	GL2RingBuffer.cpp
	GLRenderer.cpp
    GLLegacyRenderer.cpp
	HelpItemManager.cpp
	HelperManager.cpp
	helperMenu.cpp
	Joystick.cpp
	JoystickRender.cpp
    LevelDatabaseCommentThread.cpp
	LevelDatabaseDownloadThread.cpp
	LevelDatabaseRateThread.cpp
	LevelDatabaseUploadThread.cpp
	lineEditor.cpp
	LoadoutIndicator.cpp
	loadoutHelper.cpp
	Matrix4.cpp
	oglconsole.cpp
	quickChatHelper.cpp
    Renderer.cpp
	RenderUtils.cpp
	ScissorsManager.cpp
	ScreenShooter.cpp
	Shader.cpp
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
	${CMAKE_SOURCE_DIR}/fontstash/fontstash.cpp
)

file(GLOB BITFIGHTER_HEADERS *.h)

file(GLOB OTHER_HEADERS
	${CLIPPER_INCLUDE_DIR}/*.hpp 
	${CMAKE_SOURCE_DIR}/other/*.h 
	${CMAKE_SOURCE_DIR}/fontstash/*.h 
	${CMAKE_SOURCE_DIR}/recast/*.h 
	${CMAKE_SOURCE_DIR}/poly2tri/*.h
	${CMAKE_SOURCE_DIR}/poly2tri/common/*.h
	${CMAKE_SOURCE_DIR}/poly2tri/sweep/*.h
)

BF_PLATFORM_SET_EXTRA_SOURCES()


#
# Libraries
#

BF_PLATFORM_SET_EXTRA_LIBS()

# Libraries used by both client and server
set(SHARED_LIBS
	${CMAKE_THREAD_LIBS_INIT}
	${LUA_LIB}
	tnl
	${TOMCRYPT_LIBRARIES}
	${SQLITE3_LIBRARIES}
	${CLIPPER_LIBRARIES}
	${POLY2TRI_LIBRARIES}
	${EXTRA_LIBS}
)


# libraries used only on the client
set(CLIENT_LIBS
	${SDL_LIBRARY}	
	${PNG_LIBRARY}
	${GL_LIBRARY}
	${GL_LOADER_LIBRARY}
	${OPENAL_LIBRARY}
	${ALURE_LIBRARIES}
	${MODPLUG_LIBRARIES}
	${OGG_LIBRARIES}
	${SPEEX_LIBRARIES}
	${VORBIS_LIBRARIES}
	${VORBISFILE_LIBRARIES}
	${DISCORD_LIBRARY}
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
	${POLY2TRI_INCLUDE_DIR}
	${SQLITE3_INCLUDE_DIR}
	${BOOST_INCLUDE_DIR}
	${CMAKE_SOURCE_DIR}/tnl
	${CMAKE_SOURCE_DIR}/zap
)

if(COMPILE_CLIENT)
	list(APPEND INCLUDE_DIRS
		${GL_INCLUDE_DIR}
		${GL_LOADER_INCLUDE_DIR}
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
