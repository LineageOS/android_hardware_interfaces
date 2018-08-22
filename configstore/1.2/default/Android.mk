LOCAL_PATH := $(call my-dir)

################################################################################
include $(CLEAR_VARS)
LOCAL_MODULE := android.hardware.configstore@1.2-service
# seccomp is not required for coverage build.
ifneq ($(NATIVE_COVERAGE),true)
LOCAL_REQUIRED_MODULES_arm64 := configstore.policy
endif
LOCAL_VENDOR_MODULE := true
LOCAL_MODULE_CLASS := EXECUTABLES
LOCAL_MODULE_RELATIVE_PATH := hw
LOCAL_INIT_RC := android.hardware.configstore@1.2-service.rc
LOCAL_SRC_FILES:= service.cpp

include $(LOCAL_PATH)/surfaceflinger.mk

LOCAL_SHARED_LIBRARIES := \
    libhidlbase \
    libhidltransport \
    libbase \
    libhwminijail \
    liblog \
    libutils \
    android.hardware.configstore@1.0 \
    android.hardware.configstore@1.1 \
    android.hardware.configstore@1.2

include $(BUILD_EXECUTABLE)

# seccomp filter for configstore
ifeq ($(TARGET_ARCH), $(filter $(TARGET_ARCH), arm64))
include $(CLEAR_VARS)
LOCAL_MODULE := configstore.policy
LOCAL_MODULE_CLASS := ETC
LOCAL_MODULE_PATH := $(TARGET_OUT_VENDOR)/etc/seccomp_policy
LOCAL_SRC_FILES := seccomp_policy/configstore-$(TARGET_ARCH).policy
include $(BUILD_PREBUILT)
endif
