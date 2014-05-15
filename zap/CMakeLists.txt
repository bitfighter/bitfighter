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

file(GLOB BITFIGHTER_HEADERS *.h)

file(GLOB OTHER_HEADERS
	${CLIPPER_INCLUDE_DIR}/*.h* 
	${CMAKE_SOURCE_DIR}/other/*.h 
	${CMAKE_SOURCE_DIR}/fontstash/*.h 
	${CMAKE_SOURCE_DIR}/recast/*.h 
	${CMAKE_SOURCE_DIR}/sqlite/*.h
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


#
# Full client build
# 

BF_PLATFORM_ADD_DEFINITIONS()

add_executable(bitfighter
	${SHARED_SOURCES}
	${CLIENT_SOURCES}
	${BITFIGHTER_HEADERS}
	${OTHER_HEADERS}
	main.cpp
)

add_dependencies(bitfighter
	alure
	${LUA_LIB}
	tnl
	tomcrypt
)

target_link_libraries(bitfighter
	${CLIENT_LIBS}
	${SHARED_LIBS}
)

include_directories(SYSTEM
	${OPENAL_INCLUDE_DIR}
	${ALURE_INCLUDE_DIR}
	${GL_INCLUDE_DIR}
	${PNG_INCLUDE_DIR}
	${SDL_INCLUDE_DIR}
	${SPEEX_INCLUDE_DIR}
	${SPARKLE_INCLUDE_DIR}
	${LUA_INCLUDE_DIR}
	${TOMCRYPT_INCLUDE_DIR}
	${CLIPPER_INCLUDE_DIR}
	${ALURE_INCLUDE_DIR}
	${SQLITE3_INCLUDE_DIR}
)

# Where to put the executable
set_target_properties(bitfighter PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${CMAKE_SOURCE_DIR}/exe)

if(USE_GLES AND SDL2_FOUND)
	get_property(CLIENT_DEFS TARGET bitfighter PROPERTY COMPILE_DEFINITIONS)
	set_target_properties(bitfighter
		PROPERTIES
		COMPILE_DEFINITIONS "${CLIENT_DEFS};BF_USE_GLES"
	)
endif()


#
# Dedicated server build
# 
add_executable(bitfighterd
	EXCLUDE_FROM_ALL
	${SHARED_SOURCES}
	main.cpp
)

add_dependencies(bitfighterd
	tnl
	${LUA_LIB}
	tomcrypt
)

target_link_libraries(bitfighterd
	${SHARED_LIBS}
)

set_target_properties(bitfighterd
	PROPERTIES
	RUNTIME_OUTPUT_DIRECTORY ${CMAKE_SOURCE_DIR}/exe
)

get_property(DEDICATED_DEFS TARGET bitfighterd PROPERTY COMPILE_DEFINITIONS)
set_target_properties(bitfighterd
	PROPERTIES
	COMPILE_DEFINITIONS "${DEDICATED_DEFS};ZAP_DEDICATED"
)

include_directories(
	${CMAKE_SOURCE_DIR}/tnl
	${BOOST_INCLUDE_DIR}
)


#
# Test runner executable
# 
option(BITFIGHTER_COVERAGE "Add coverage information to the test executable and create 'coverage' target" NO)

set(TEST_SOURCES
	${CMAKE_SOURCE_DIR}/bitfighter_test/LevelFilesForTesting.cpp
	${CMAKE_SOURCE_DIR}/bitfighter_test/TestEditor.cpp
	${CMAKE_SOURCE_DIR}/bitfighter_test/TestGameType.cpp
	${CMAKE_SOURCE_DIR}/bitfighter_test/TestGameUserInterface.cpp
	${CMAKE_SOURCE_DIR}/bitfighter_test/TestGeomUtils.cpp
	${CMAKE_SOURCE_DIR}/bitfighter_test/TestHelpItemManager.cpp
	${CMAKE_SOURCE_DIR}/bitfighter_test/TestHttpRequest.cpp
	${CMAKE_SOURCE_DIR}/bitfighter_test/TestINISettings.cpp
	${CMAKE_SOURCE_DIR}/bitfighter_test/TestInputCode.cpp
	${CMAKE_SOURCE_DIR}/bitfighter_test/TestIntegration.cpp
	${CMAKE_SOURCE_DIR}/bitfighter_test/TestLevelLoader.cpp
	${CMAKE_SOURCE_DIR}/bitfighter_test/TestLevelMenuSelectUserInterface.cpp
	${CMAKE_SOURCE_DIR}/bitfighter_test/TestLoadoutIndicator.cpp
	${CMAKE_SOURCE_DIR}/bitfighter_test/TestLoadoutTracker.cpp
	${CMAKE_SOURCE_DIR}/bitfighter_test/TestLuaEnvironment.cpp
	${CMAKE_SOURCE_DIR}/bitfighter_test/TestMaster.cpp
	${CMAKE_SOURCE_DIR}/bitfighter_test/TestMove.cpp
	${CMAKE_SOURCE_DIR}/bitfighter_test/TestObjects.cpp
	${CMAKE_SOURCE_DIR}/bitfighter_test/TestPolylineGeometry.cpp
	${CMAKE_SOURCE_DIR}/bitfighter_test/TestRenderUtils.cpp
	${CMAKE_SOURCE_DIR}/bitfighter_test/TestRobot.cpp
	${CMAKE_SOURCE_DIR}/bitfighter_test/TestRobotManager.cpp
	${CMAKE_SOURCE_DIR}/bitfighter_test/TestServerGame.cpp
	${CMAKE_SOURCE_DIR}/bitfighter_test/TestSettings.cpp
	${CMAKE_SOURCE_DIR}/bitfighter_test/TestShip.cpp
	${CMAKE_SOURCE_DIR}/bitfighter_test/TestSpawnDelay.cpp
	${CMAKE_SOURCE_DIR}/bitfighter_test/TestStringUtils.cpp
	${CMAKE_SOURCE_DIR}/bitfighter_test/TestSymbolStrings.cpp
	${CMAKE_SOURCE_DIR}/bitfighter_test/TestUtils.cpp
	${CMAKE_SOURCE_DIR}/bitfighter_test/main_test.cpp
	${CMAKE_SOURCE_DIR}/master/master.cpp
	${CMAKE_SOURCE_DIR}/master/MasterServerConnection.cpp
	${CMAKE_SOURCE_DIR}/master/MasterServerConnection.cpp
	${CMAKE_SOURCE_DIR}/master/GameJoltConnector.cpp
)

if(EXISTS ${CMAKE_SOURCE_DIR}/bitfighter_test)
	add_executable(test EXCLUDE_FROM_ALL
		${CLIENT_SOURCES}
		${SHARED_SOURCES}
		${TEST_SOURCES}
		${BITFIGHTER_HEADERS}
	)

	target_link_libraries(test
		${SHARED_LIBS}
		${CLIENT_LIBS}
		gtest
	)

	add_dependencies(test
		alure
		${LUA_LIB}
		tnl
		tomcrypt
		gtest
	)

	set_target_properties(test
		PROPERTIES
		RUNTIME_OUTPUT_DIRECTORY ${CMAKE_SOURCE_DIR}/exe
		COMPILE_DEFINITIONS BITFIGHTER_TEST
	)
endif()

include_directories(
	${CMAKE_SOURCE_DIR}/gtest/include
	${CMAKE_SOURCE_DIR}/zap
)

# to use the coverage target, install lcov, enable BITFIGHTER_COVERAGE, and run it
# coverage data is output to the 'cov' directory in html format.
if(BITFIGHTER_COVERAGE)
   set_target_properties(test
      PROPERTIES
      LINK_FLAGS "--coverage"
      COMPILE_FLAGS "--coverage"
   )

   add_custom_target(coverage cd ${CMAKE_SOURCE_DIR}/exe && ${CMAKE_SOURCE_DIR}/exe/test
      COMMAND lcov --capture --directory ${CMAKE_SOURCE_DIR} --output-file ${CMAKE_SOURCE_DIR}/build/coverage.info
      COMMAND lcov --extract ${CMAKE_SOURCE_DIR}/build/coverage.info --output-file ${CMAKE_SOURCE_DIR}/build/coverage.info ${CMAKE_SOURCE_DIR}/zap/*
      COMMAND genhtml ${CMAKE_SOURCE_DIR}/build/coverage.info --output-directory ${CMAKE_SOURCE_DIR}/build/cov
      DEPENDS test alure ${LUA_LIB} tnl tomcrypt
      WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}/build 
   )
endif()


#
# Settings for all targets
# 

# You can only call 'set_target_properties' once for each properties-target combination, therefore
# we'll build up a list and call 'set_target_properties' at the end
set(ALL_DEBUG_DEFS
	"TNL_DEBUG"
)

BF_PLATFORM_SET_TARGET_PROPERTIES()

set_target_properties(bitfighterd bitfighter PROPERTIES COMPILE_DEFINITIONS_DEBUG "${ALL_DEBUG_DEFS}")

if(EXISTS ${CMAKE_SOURCE_DIR}/bitfighter_test)
	set_target_properties(test PROPERTIES COMPILE_DEFINITIONS_DEBUG "${ALL_DEBUG_DEFS}")
endif()

BF_PLATFORM_POST_BUILD_INSTALL_RESOURCES()

BF_PLATFORM_INSTALL()

BF_PLATFORM_CREATE_PACKAGES()
