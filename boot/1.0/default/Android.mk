LOCAL_PATH := $(call my-dir)

include $(CLEAR_VARS)
LOCAL_MODULE := android.hardware.boot@1.0-impl
LOCAL_MODULE_RELATIVE_PATH := hw
LOCAL_SRC_FILES := \
    BootControl.cpp \

LOCAL_SHARED_LIBRARIES := \
    liblog \
    libhidl \
    libhwbinder \
    libhardware \
    libutils \
    android.hardware.boot@1.0 \

include $(BUILD_SHARED_LIBRARY)

include $(CLEAR_VARS)
LOCAL_MODULE_RELATIVE_PATH := hw
LOCAL_MODULE := android.hardware.boot@1.0-service
LOCAL_INIT_RC := android.hardware.boot@1.0-service.rc
LOCAL_SRC_FILES := \
    service.cpp

LOCAL_SHARED_LIBRARIES := \
    liblog \
    libhwbinder \
    libhardware \
    libhidl \
    libutils \
    android.hardware.boot@1.0 \

include $(BUILD_EXECUTABLE)
