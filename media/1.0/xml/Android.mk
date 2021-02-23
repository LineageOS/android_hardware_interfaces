LOCAL_PATH := $(call my-dir)

#######################################
# media_profiles_V1_0.dtd

include $(CLEAR_VARS)

LOCAL_MODULE := media_profiles_V1_0.dtd
LOCAL_LICENSE_KINDS := SPDX-license-identifier-Apache-2.0
LOCAL_LICENSE_CONDITIONS := notice
LOCAL_NOTICE_FILE := $(LOCAL_PATH)/../../../NOTICE
LOCAL_SRC_FILES := media_profiles.dtd
LOCAL_MODULE_CLASS := ETC
LOCAL_MODULE_PATH := $(TARGET_OUT_ETC)

include $(BUILD_PREBUILT)
