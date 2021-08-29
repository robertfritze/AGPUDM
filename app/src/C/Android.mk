LOCAL_PATH := $(call my-dir)
INCLUDE_PATH := $(LOCAL_PATH)/include

# To allow building native part of application with OpenCL,
# OpenCL header files and statically linked library are required.
# They are used from Intel OpenCL SDK installation directories,
# that are determined below:

#APP_ALLOW_MISSING_DEPS=true


# Setting LOCAL_CFLAGS with -I is not good in comparison to LOCAL_C_INCLUDES
# according to NDK documentation, but this only variant that works correctly


include $(CLEAR_VARS)
LOCAL_CFLAGS     += -I$(INCLUDE_PATH) -O3 -std=c99
LOCAL_MODULE     := rwlock_wp
LOCAL_SRC_FILES  := source/rwlock_wp.c
include $(BUILD_SHARED_LIBRARY)

include $(CLEAR_VARS)
LOCAL_CFLAGS     += -I$(INCLUDE_PATH) -O3 -std=c99
LOCAL_MODULE     := dbscan_c
LOCAL_SRC_FILES  := source/dbscan_c.c
LOCAL_SHARED_LIBRARIES = OpenCL rwlock_wp
include $(BUILD_SHARED_LIBRARY)

include $(CLEAR_VARS)
LOCAL_CFLAGS     += -I$(INCLUDE_PATH) -O3 -std=c99
LOCAL_MODULE     := kmeans_c
LOCAL_SRC_FILES  := source/kmeans_c.c
LOCAL_SHARED_LIBRARIES = OpenCL rwlock_wp
include $(BUILD_SHARED_LIBRARY)


include $(CLEAR_VARS)
LOCAL_CFLAGS     += -I$(INCLUDE_PATH) -O3 -std=c99
LOCAL_MODULE     := OpenCL
LOCAL_SRC_FILES  := source/OpenCL.c
include $(BUILD_SHARED_LIBRARY)


include $(CLEAR_VARS)
LOCAL_MODULE     := oclwrapper
LOCAL_SRC_FILES  := source/oclwrapper.c
LOCAL_CFLAGS     += -I$(INCLUDE_PATH) -O3 -std=c99
LOCAL_SHARED_LIBRARIES := OpenCL
include $(BUILD_SHARED_LIBRARY)


