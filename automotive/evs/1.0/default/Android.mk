LOCAL_PATH:=$(call my-dir)

include $(CLEAR_VARS)
LOCAL_MODULE := android.hardware.automotive.evs@1.0-service
LOCAL_INIT_RC := android.hardware.automotive.evs@1.0-service.rc
LOCAL_MODULE_RELATIVE_PATH := hw
LOCAL_VENDOR_MODULE := true

LOCAL_SRC_FILES := \
    service.cpp \
    EvsCamera.cpp \
    EvsEnumerator.cpp \
    EvsDisplay.cpp \

LOCAL_SHARED_LIBRARIES := \
    android.hardware.automotive.evs@1.0 \
    libui \
    libbase \
    libbinder \
    libcutils \
    libhardware \
    libhidlbase \
    libhidltransport \
    liblog \
    libutils \

LOCAL_CFLAGS := -O0 -g

include $(BUILD_EXECUTABLE)
