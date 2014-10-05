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
)


add_executable(test EXCLUDE_FROM_ALL
	$<TARGET_OBJECTS:bitfighter_client>
	$<TARGET_OBJECTS:master_lib>
	${TEST_SOURCES}
)

target_link_libraries(test
	${CLIENT_LIBS}
	${SHARED_LIBS}
	gtest
)

add_dependencies(test
	bitfighter_client
	master_lib
	gtest
)

set_target_properties(test
	PROPERTIES
	RUNTIME_OUTPUT_DIRECTORY ${CMAKE_SOURCE_DIR}/exe
	COMPILE_DEFINITIONS BITFIGHTER_TEST
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


set_target_properties(test PROPERTIES COMPILE_DEFINITIONS_DEBUG "TNL_DEBUG")

BF_PLATFORM_SET_TARGET_PROPERTIES(test)

BF_PLATFORM_POST_BUILD_INSTALL_RESOURCES(test)

# BF_PLATFORM_INSTALL(test)

# BF_PLATFORM_CREATE_PACKAGES(test)
