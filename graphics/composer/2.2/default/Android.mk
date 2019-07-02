LOCAL_PATH := $(call my-dir)

include $(CLEAR_VARS)
LOCAL_MODULE := android.hardware.graphics.composer@2.2-service
LOCAL_VENDOR_MODULE := true
LOCAL_MODULE_RELATIVE_PATH := hw
LOCAL_CFLAGS := -Wall -Werror -DLOG_TAG=\"ComposerHal\"
LOCAL_SRC_FILES := service.cpp
LOCAL_INIT_RC := android.hardware.graphics.composer@2.2-service.rc
LOCAL_HEADER_LIBRARIES := android.hardware.graphics.composer@2.2-passthrough
LOCAL_SHARED_LIBRARIES := \
        android.hardware.graphics.composer@2.1 \
        android.hardware.graphics.composer@2.2 \
        android.hardware.graphics.mapper@2.0 \
        android.hardware.graphics.mapper@3.0 \
        libbase \
        libbinder \
        libcutils \
        libfmq \
        libhardware \
        libhidlbase \
        libhidltransport \
        libhwc2on1adapter \
        libhwc2onfbadapter \
        liblog \
        libsync \
        libutils

ifdef TARGET_USES_DISPLAY_RENDER_INTENTS
LOCAL_CFLAGS += -DUSES_DISPLAY_RENDER_INTENTS
endif

include $(BUILD_EXECUTABLE)
