# TNL Makefile
# From Bitfigher (http://bitfighter.org)

LOCAL_PATH := $(call my-dir)

include $(CLEAR_VARS)

LOCAL_MODULE := tnl

LOCAL_C_INCLUDES := $(LOCAL_PATH)

# using -DTNL_DEBUG hangs on start-up
LOCAL_CFLAGS := -DTNL_ENABLE_LOGGING

# Add your application source files here...
LOCAL_SRC_FILES := assert.cpp \
	asymmetricKey.cpp \
	bitStream.cpp \
	byteBuffer.cpp \
	certificate.cpp \
	clientPuzzle.cpp \
	connectionStringTable.cpp \
	dataChunker.cpp \
	eventConnection.cpp \
	ghostConnection.cpp \
	huffmanStringProcessor.cpp \
	log.cpp \
	netBase.cpp \
	netConnection.cpp \
	netInterface.cpp \
	netObject.cpp \
	netStringTable.cpp \
	platform.cpp \
	random.cpp \
	rpc.cpp \
	symmetricCipher.cpp \
	thread.cpp \
	tnlMethodDispatch.cpp \
	journal.cpp \
	udp.cpp \
	vector.cpp \

LOCAL_SHARED_LIBRARIES := tomcrypt

LOCAL_LDLIBS := -llog

include $(BUILD_SHARED_LIBRARY)