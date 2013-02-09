set(TNL_SOURCES
	assert.cpp
	asymmetricKey.cpp
	bitStream.cpp
	byteBuffer.cpp
	certificate.cpp
	clientPuzzle.cpp
	connectionStringTable.cpp
	dataChunker.cpp
	eventConnection.cpp
	ghostConnection.cpp
	huffmanStringProcessor.cpp
	log.cpp
	netBase.cpp
	netConnection.cpp
	netInterface.cpp
	netObject.cpp
	netStringTable.cpp
	platform.cpp
	random.cpp
	rpc.cpp
	symmetricCipher.cpp
	thread.cpp
	tnlMethodDispatch.cpp
	journal.cpp
	udp.cpp
	vector.cpp
)
add_library(tnl ${TNL_SOURCES})
target_link_libraries(tnl tomcrypt)

if(CMAKE_BUILD_TYPE STREQUAL "Debug")
	set_target_properties(tnl
		PROPERTIES
		COMPILE_DEFINITIONS TNL_DEBUG
		COMPILE_DEFINITIONS TNL_ENABLE_LOGGING
	)
endif()
