#
# Test runner executable
#
option(BITFIGHTER_COVERAGE "Add coverage information to the test executable and create 'coverage' target" NO)

set(TEST_SOURCES
	${CMAKE_SOURCE_DIR}/bitfighter_test/BitfighterTestEnvironment.cpp
	${CMAKE_SOURCE_DIR}/bitfighter_test/BitfighterTestEnvironment.h
	${CMAKE_SOURCE_DIR}/bitfighter_test/TestInit.cpp
	${CMAKE_SOURCE_DIR}/bitfighter_test/LevelFilesForTesting.cpp
 	${CMAKE_SOURCE_DIR}/bitfighter_test/TestBanList.cpp
	${CMAKE_SOURCE_DIR}/bitfighter_test/TestBarrier.cpp
	${CMAKE_SOURCE_DIR}/bitfighter_test/TestBfObject.cpp
	${CMAKE_SOURCE_DIR}/bitfighter_test/TestBugFixes.cpp    
	${CMAKE_SOURCE_DIR}/bitfighter_test/TestBitSet.cpp
	${CMAKE_SOURCE_DIR}/bitfighter_test/TestEditor.cpp
	${CMAKE_SOURCE_DIR}/bitfighter_test/TestGameStats.cpp
	${CMAKE_SOURCE_DIR}/bitfighter_test/TestGameType.cpp
	${CMAKE_SOURCE_DIR}/bitfighter_test/TestGameUserInterface.cpp
	${CMAKE_SOURCE_DIR}/bitfighter_test/TestGeomUtils.cpp
	${CMAKE_SOURCE_DIR}/bitfighter_test/TestGeomPrecision.cpp
	${CMAKE_SOURCE_DIR}/bitfighter_test/TestGeomUtilsSafety.cpp
	${CMAKE_SOURCE_DIR}/bitfighter_test/TestChatHelper.cpp
	${CMAKE_SOURCE_DIR}/bitfighter_test/TestClipper2.cpp
	${CMAKE_SOURCE_DIR}/bitfighter_test/TestColor.cpp
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
	${CMAKE_SOURCE_DIR}/bitfighter_test/TestMapTiling.cpp
	${CMAKE_SOURCE_DIR}/bitfighter_test/TestMaster.cpp
	${CMAKE_SOURCE_DIR}/bitfighter_test/TestMathUtils.cpp
	${CMAKE_SOURCE_DIR}/bitfighter_test/TestMatrix4.cpp
	${CMAKE_SOURCE_DIR}/bitfighter_test/TestNonce.cpp
	${CMAKE_SOURCE_DIR}/bitfighter_test/TestMove.cpp
	${CMAKE_SOURCE_DIR}/bitfighter_test/TestObjects.cpp
	${CMAKE_SOURCE_DIR}/bitfighter_test/TestPoint.cpp
	${CMAKE_SOURCE_DIR}/bitfighter_test/TestPointComparison.cpp
	${CMAKE_SOURCE_DIR}/bitfighter_test/TestPolylineGeometry.cpp
	${CMAKE_SOURCE_DIR}/bitfighter_test/TestRect.cpp
	${CMAKE_SOURCE_DIR}/bitfighter_test/TestRenderUtils.cpp
	${CMAKE_SOURCE_DIR}/bitfighter_test/TestRenderer.cpp
	${CMAKE_SOURCE_DIR}/bitfighter_test/TestRobot.cpp
	${CMAKE_SOURCE_DIR}/bitfighter_test/TestRobotManager.cpp
	${CMAKE_SOURCE_DIR}/bitfighter_test/TestServerGame.cpp
	${CMAKE_SOURCE_DIR}/bitfighter_test/TestSettings.cpp
	${CMAKE_SOURCE_DIR}/bitfighter_test/TestShip.cpp
	${CMAKE_SOURCE_DIR}/bitfighter_test/TestSpawnDelay.cpp
	${CMAKE_SOURCE_DIR}/bitfighter_test/TestStatistics.cpp
	${CMAKE_SOURCE_DIR}/bitfighter_test/TestStringUtils.cpp
	${CMAKE_SOURCE_DIR}/bitfighter_test/TestSymbolStrings.cpp
	${CMAKE_SOURCE_DIR}/bitfighter_test/TestTnlVector.cpp
	${CMAKE_SOURCE_DIR}/bitfighter_test/TestTnlVectorBool.cpp
	${CMAKE_SOURCE_DIR}/bitfighter_test/TestTimer.cpp
	${CMAKE_SOURCE_DIR}/bitfighter_test/TestTnlString.cpp
	${CMAKE_SOURCE_DIR}/bitfighter_test/TestUtils.cpp
	${CMAKE_SOURCE_DIR}/bitfighter_test/TestVoiceCodec.cpp
	${CMAKE_SOURCE_DIR}/bitfighter_test/main_test.cpp
)


add_executable(bitfighter_test
	$<TARGET_OBJECTS:bitfighter_client>
	$<TARGET_OBJECTS:master_lib>
	${TEST_SOURCES}
)

# Create GTest namespace alias so CLion recognizes this as a gtest target.
# CLion requires the GTest:: prefix for gutter test-run icons.
# We use gtest (not gtest_main) because we provide our own main() that
# transitively includes SDL.h, which is needed for SDL_main on Windows.
if(TARGET gtest AND NOT TARGET GTest::gtest)
	add_library(GTest::gtest ALIAS gtest)
endif()

target_link_libraries(bitfighter_test
	${CLIENT_LIBS}
	${SHARED_LIBS}
	GTest::gtest
)

add_dependencies(bitfighter_test
	bitfighter_client
	master_lib
	gtest
)

# Help CLion associate this target with gtest for gutter-run icon detection.
# CLion requires the gtest include directory to be visible on THIS target.
target_include_directories(bitfighter_test PRIVATE
	${CMAKE_SOURCE_DIR}/gtest/googletest/include
)

set_target_properties(bitfighter_test
	PROPERTIES
	RUNTIME_OUTPUT_DIRECTORY ${CMAKE_SOURCE_DIR}/exe
	COMPILE_DEFINITIONS BITFIGHTER_TEST
)

# Register individual tests with CTest so CLion can produce gutter-run icons.
# gtest_discover_tests with PRE_TEST discovers tests at configure time.
# We use an explicit TEST_EXECUTABLE because BF_PLATFORM_SET_TARGET_PROPERTIES
# sets DEBUG_POSTFIX _debug on Windows.
include(GoogleTest)
gtest_discover_tests(bitfighter_test
	DISCOVERY_MODE PRE_TEST
	TEST_EXECUTABLE "${CMAKE_SOURCE_DIR}/exe/bitfighter_test_debug.exe"
	WORKING_DIRECTORY "${CMAKE_SOURCE_DIR}/exe"
)

# Also write a flat test file that includes DEF_SOURCE_LINE by parsing the
# discovery JSON created by gtest_discover_tests.  This ensures CLion has
# the source-to-test mapping it needs for gutter icons regardless of how
# it queries CTest (with or without -C).
set(_BF_CTEST "${CMAKE_CURRENT_BINARY_DIR}/CTestTestfile.cmake")
set(_BF_TESTS "${CMAKE_CURRENT_BINARY_DIR}/bf_tests.cmake")
file(WRITE "${_BF_TESTS}" "")

# Try reading the discovery JSON for source locations
set(_BF_JSON "${CMAKE_SOURCE_DIR}/exe/cmake_test_discovery_f783cc5375.json")
set(_BF_PARSE_OK 0)
if(EXISTS "${_BF_JSON}")
	file(READ "${_BF_JSON}" _BF_JS)
	string(JSON _BF_SUITES_LEN LENGTH "${_BF_JS}" testsuites)
	if(_BF_SUITES_LEN GREATER 0)
		set(_BF_N 0)
		math(EXPR _BF_SUITES_END "${_BF_SUITES_LEN} - 1")
		foreach(_BF_SI RANGE ${_BF_SUITES_END})
			string(JSON _BF_SUITE GET "${_BF_JS}" testsuites ${_BF_SI} name)
			string(JSON _BF_TESTS_IN_SUITE LENGTH "${_BF_JS}" testsuites ${_BF_SI} testsuite)
			math(EXPR _BF_TE "${_BF_TESTS_IN_SUITE} - 1")
			foreach(_BF_TI RANGE ${_BF_TE})
				string(JSON _BF_TNAME GET "${_BF_JS}" testsuites ${_BF_SI} testsuite ${_BF_TI} name)
			string(JSON _BF_TFILE GET "${_BF_JS}" testsuites ${_BF_SI} testsuite ${_BF_TI} file)
			# Escape backslashes for CMake (\\d, \\b, etc) and convert to
			# forward slashes to avoid escape sequence interpretation
			string(REPLACE "\\" "/" _BF_TFILE "${_BF_TFILE}")
			string(JSON _BF_TLINE GET "${_BF_JS}" testsuites ${_BF_SI} testsuite ${_BF_TI} line)
				set(_BF_FULL "${_BF_SUITE}.${_BF_TNAME}")
				file(APPEND "${_BF_TESTS}"
					"add_test(${_BF_FULL} \"${CMAKE_SOURCE_DIR}/exe/bitfighter_test_debug.exe\" "
					"\"--gtest_filter=${_BF_FULL}\")\n"
					"set_tests_properties(${_BF_FULL} PROPERTIES "
					"WORKING_DIRECTORY \"${CMAKE_SOURCE_DIR}/exe\" "
					"DEF_SOURCE_LINE \"${_BF_TFILE}:${_BF_TLINE}\")\n"
				)
				math(EXPR _BF_N "${_BF_N} + 1")
			endforeach()
		endforeach()
		set(_BF_PARSE_OK 1)
	endif()
endif()

if(_BF_PARSE_OK AND _BF_N GREATER 0)
	# Write our own include file that loads bf_tests.cmake directly
	# (bypassing gtest's config-dependent include chain entirely).
	file(WRITE "${CMAKE_CURRENT_BINARY_DIR}/bitfighter_test[1]_include.cmake"
		"include(\"${_BF_TESTS}\")\n"
	)
	message(STATUS "BF tests: ${_BF_N} entries (with DEF_SOURCE_LINE from JSON)")
endif()


# to use the coverage target, install lcov, enable BITFIGHTER_COVERAGE, and run it
# coverage data is output to the 'cov' directory in html format.
if(BITFIGHTER_COVERAGE)
   set_target_properties(bitfighter_test
      PROPERTIES
      LINK_FLAGS "--coverage"
      COMPILE_FLAGS "--coverage"
   )

   add_custom_target(coverage cd ${CMAKE_SOURCE_DIR}/exe && ${CMAKE_SOURCE_DIR}/exe/bitfighter_test
      COMMAND lcov --capture --directory ${CMAKE_SOURCE_DIR} --output-file ${CMAKE_SOURCE_DIR}/build/coverage.info
      COMMAND lcov --extract ${CMAKE_SOURCE_DIR}/build/coverage.info --output-file ${CMAKE_SOURCE_DIR}/build/coverage.info ${CMAKE_SOURCE_DIR}/zap/*
      COMMAND genhtml ${CMAKE_SOURCE_DIR}/build/coverage.info --output-directory ${CMAKE_SOURCE_DIR}/build/cov
      DEPENDS bitfighter_test alure ${LUA_LIB} tnl tomcrypt
      WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}/build
   )
endif()


target_compile_definitions(bitfighter_test PRIVATE $<$<CONFIG:Debug>:TNL_DEBUG>)

BF_PLATFORM_SET_TARGET_PROPERTIES(bitfighter_test)

BF_PLATFORM_POST_BUILD_INSTALL_RESOURCES(bitfighter_test)

# BF_PLATFORM_INSTALL(bitfighter_test)

# BF_PLATFORM_CREATE_PACKAGES(bitfighter_test)