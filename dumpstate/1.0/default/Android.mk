LOCAL_PATH := $(call my-dir)

include $(CLEAR_VARS)
LOCAL_MODULE := android.hardware.dumpstate@1.0-impl
LOCAL_MODULE_RELATIVE_PATH := hw
LOCAL_SRC_FILES := \
    DumpstateDevice.cpp \

LOCAL_SHARED_LIBRARIES := \
    android.hardware.dumpstate@1.0 \
    libbase \
    libcutils \
    libhidlbase \
    libhidltransport \
    libhwbinder \
    liblog \
    libutils

LOCAL_STATIC_LIBRARIES := \
    libdumpstateutil

include $(BUILD_SHARED_LIBRARY)
