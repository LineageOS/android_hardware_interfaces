# TODO(connoro): Remove this file once we eliminate existing usage of
# PRODUCT_STATIC_BOOT_CONTROL_HAL

LOCAL_PATH := $(call my-dir)

ifneq ($(strip $(PRODUCT_STATIC_BOOT_CONTROL_HAL)),)
include $(CLEAR_VARS)

LOCAL_MODULE := android.hardware.boot@1.0-impl-wrapper.recovery
LOCAL_MODULE_CLASS := SHARED_LIBRARIES
LOCAL_MULTILIB := first
ifeq ($(TARGET_IS_64_BIT),true)
LOCAL_MODULE_PATH := $(TARGET_RECOVERY_ROOT_OUT)/system/lib64/hw
else
LOCAL_MODULE_PATH := $(TARGET_RECOVERY_ROOT_OUT)/system/lib/hw
endif
LOCAL_SRC_FILES := BootControl.cpp
LOCAL_CFLAGS := -DBOOT_CONTROL_RECOVERY
LOCAL_SHARED_LIBRARIES := \
    libbase.recovery \
    liblog.recovery \
    libhidlbase.recovery \
    libhidltransport.recovery \
    libhardware.recovery \
    libutils.recovery \
    android.hardware.boot@1.0.recovery
LOCAL_STATIC_LIBRARIES := $(PRODUCT_STATIC_BOOT_CONTROL_HAL)
include $(BUILD_SHARED_LIBRARY)

endif
