LOCAL_PATH:= $(call my-dir)

include $(CLEAR_VARS)
LOCAL_MODULE := android.hardware.nfc@1.0-impl
LOCAL_MODULE_RELATIVE_PATH := hw
LOCAL_SRC_FILES := Nfc.cpp
LOCAL_SHARED_LIBRARIES := liblog libcutils libhardware libhwbinder libbase libcutils libutils libhidl android.hardware.nfc@1.0
include $(BUILD_SHARED_LIBRARY)
