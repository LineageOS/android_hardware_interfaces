LOCAL_PATH := $(call my-dir)

################################################################################
include $(CLEAR_VARS)
LOCAL_MODULE := android.hardware.configstore@1.1-service
LOCAL_PROPRIETARY_MODULE := true
LOCAL_MODULE_CLASS := EXECUTABLES
LOCAL_MODULE_RELATIVE_PATH := hw
LOCAL_INIT_RC := android.hardware.configstore@1.1-service.rc
LOCAL_SRC_FILES:= service.cpp

include $(LOCAL_PATH)/surfaceflinger.mk

LOCAL_SHARED_LIBRARIES := \
    android.hardware.configstore@1.0 \
    android.hardware.configstore@1.1 \
    libhidlbase \
    libhidltransport \
    libbase \
    liblog \
    libutils \

include $(BUILD_EXECUTABLE)
