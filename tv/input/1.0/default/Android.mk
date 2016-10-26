LOCAL_PATH := $(call my-dir)

include $(CLEAR_VARS)
LOCAL_MODULE := android.hardware.tv.input@1.0-impl
LOCAL_MODULE_RELATIVE_PATH := hw
LOCAL_SRC_FILES := \
    TvInput.cpp \

LOCAL_SHARED_LIBRARIES := \
    libbase \
    liblog \
    libhardware \
    libhidl \
    libhwbinder \
    libutils \
    android.hardware.audio.common@2.0 \
    android.hardware.tv.input@1.0 \

include $(BUILD_SHARED_LIBRARY)

include $(CLEAR_VARS)
LOCAL_MODULE_RELATIVE_PATH := hw
LOCAL_MODULE := android.hardware.tv.input@1.0-service
LOCAL_INIT_RC := android.hardware.tv.input@1.0-service.rc
LOCAL_SRC_FILES := \
    service.cpp \

LOCAL_SHARED_LIBRARIES := \
    liblog \
    libcutils \
    libdl \
    libbase \
    libutils \
    libhardware_legacy \
    libhardware \

LOCAL_SHARED_LIBRARIES += \
    libhwbinder \
    libhidl \
    android.hardware.audio.common@2.0 \
    android.hardware.tv.input@1.0 \

include $(BUILD_EXECUTABLE)

