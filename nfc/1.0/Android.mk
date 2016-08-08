LOCAL_PATH := $(call my-dir)
include $(CLEAR_VARS)

LOCAL_MODULE := android.hardware.nfc@1.0
LOCAL_MODULE_CLASS := SHARED_LIBRARIES

intermediates := $(local-generated-sources-dir)

GEN := \
  $(intermediates)/android/hardware/nfc/1.0/types.cpp \
  $(intermediates)/android/hardware/nfc/1.0/NfcAll.cpp \
  $(intermediates)/android/hardware/nfc/1.0/NfcClientCallbackAll.cpp \

$(GEN): hidl-gen

$(GEN): PRIVATE_OUTPUT_DIR := $(intermediates)

$(GEN): PRIVATE_CUSTOM_TOOL = \
  hidl-gen -o $(PRIVATE_OUTPUT_DIR) -r android.hardware:hardware/interfaces android.hardware.nfc@1.0

$(GEN): $(LOCAL_PATH)/types.hal $(LOCAL_PATH)/INfc.hal $(LOCAL_PATH)/INfcClientCallback.hal
	$(transform-generated-source)

LOCAL_GENERATED_SOURCES += $(GEN)

LOCAL_EXPORT_C_INCLUDE_DIRS := $(intermediates)

LOCAL_SHARED_LIBRARIES := \
  libhwbinder \
  libutils \

include $(BUILD_SHARED_LIBRARY)
