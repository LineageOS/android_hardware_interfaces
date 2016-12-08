LOCAL_PATH := $(call my-dir)

include $(CLEAR_VARS)
LOCAL_MODULE := android.hardware.tests.libbinder
LOCAL_MODULE_CLASS := SHARED_LIBRARIES

LOCAL_SRC_FILES := android/tests/binder/IBenchmark.aidl

LOCAL_SHARED_LIBRARIES := \
  libbinder \
  libutils \

include $(BUILD_SHARED_LIBRARY)
