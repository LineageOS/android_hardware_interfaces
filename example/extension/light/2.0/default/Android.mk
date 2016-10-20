LOCAL_PATH := $(call my-dir)

include $(CLEAR_VARS)
LOCAL_MODULE := android.hardware.example.extension.light@2.0-service
LOCAL_INIT_RC := android.hardware.example.extension.light@2.0-service.rc
LOCAL_MODULE_RELATIVE_PATH := hw
LOCAL_SRC_FILES := \
    Light.cpp \

LOCAL_SHARED_LIBRARIES := \
    libhidl \
    libhwbinder \
    libutils \
    android.hardware.light@2.0 \
    android.hardware.example.extension.light@2.0 \

include $(BUILD_SHARED_LIBRARY)
