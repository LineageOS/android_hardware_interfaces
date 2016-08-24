LOCAL_PATH := $(call my-dir)

include $(CLEAR_VARS)
LOCAL_MODULE := android.hardware.tests.libbinder
LOCAL_MODULE_CLASS := SHARED_LIBRARIES

LOCAL_SRC_FILES := android/tests/binder/IBenchmark.aidl

LOCAL_SHARED_LIBRARIES := \
  libbinder \
  libutils \

LOCAL_STATIC_LIBRARIES := libtestUtil

LOCAL_COMPATIBILITY_SUITE := vts
-include test/vts/tools/build/Android.packaging_sharedlib.mk
include $(BUILD_SHARED_LIBRARY)
