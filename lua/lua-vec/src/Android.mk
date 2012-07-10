LOCAL_PATH := $(call my-dir)

include $(CLEAR_VARS)

LOCAL_MODULE := luavec

LOCAL_C_INCLUDES := $(LOCAL_PATH)

LOCAL_CFLAGS := 

# Add your application source files here...
LOCAL_SRC_FILES := lapi.c lcode.c ldebug.c ldo.c ldump.c lfunc.c lgc.c llex.c lmem.c \
	lobject.c lopcodes.c lparser.c lstate.c lstring.c ltable.c ltm.c  \
	lundump.c lvm.c lzio.c \
	lauxlib.c lbaselib.c ldblib.c liolib.c lmathlib.c loslib.c ltablib.c \
	lstrlib.c loadlib.c linit.c lveclib.c lgcveclib.c

LOCAL_SHARED_LIBRARIES := 

LOCAL_LDLIBS := -llog 

include $(BUILD_SHARED_LIBRARY)
