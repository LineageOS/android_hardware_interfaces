LOCAL_PATH := $(call my-dir)

include $(CLEAR_VARS)
LOCAL_MODULE := android.hardware.light@2.0-impl
LOCAL_MODULE_RELATIVE_PATH := hw
LOCAL_SRC_FILES := \
    Light.cpp \

LOCAL_SHARED_LIBRARIES := \
    libhidl \
    libhwbinder \
    libutils \
    liblog \
    libcutils \
    libhardware \
    libbase \
    libcutils \
    android.hardware.light@2.0 \

include $(BUILD_SHARED_LIBRARY)
