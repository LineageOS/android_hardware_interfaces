LOCAL_PATH := $(call my-dir)
include $(CLEAR_VARS)

LOCAL_MODULE := android.hardware.nfc@1.0
LOCAL_MODULE_CLASS := SHARED_LIBRARIES

intermediates := $(local-generated-sources-dir)

HIDL := $(HOST_OUT_EXECUTABLES)/hidl-gen$(HOST_EXECUTABLE_SUFFIX)

#
# Build types.hal
#
GEN := $(intermediates)/android/hardware/nfc/1.0/types.cpp
$(GEN): $(HIDL)
$(GEN): PRIVATE_HIDL := $(HIDL)
$(GEN): PRIVATE_DEPS := $(LOCAL_PATH)/types.hal
$(GEN): PRIVATE_OUTPUT_DIR := $(intermediates)
$(GEN): PRIVATE_CUSTOM_TOOL = \
    $(PRIVATE_HIDL) -o $(PRIVATE_OUTPUT_DIR) \
    -Lc++ -randroid.hardware:hardware/interfaces\
    android.hardware.nfc@1.0::$(patsubst %.hal,%,$(notdir $(PRIVATE_DEPS)))

$(GEN): $(LOCAL_PATH)/types.hal
	$(transform-generated-source)
LOCAL_GENERATED_SOURCES += $(GEN)

#
# Build INfc.hal
#
GEN := $(intermediates)/android/hardware/nfc/1.0/NfcAll.cpp
$(GEN): $(HIDL)
$(GEN): PRIVATE_HIDL := $(HIDL)
$(GEN): PRIVATE_DEPS := $(LOCAL_PATH)/INfc.hal
$(GEN): PRIVATE_OUTPUT_DIR := $(intermediates)
$(GEN): PRIVATE_CUSTOM_TOOL = \
    $(PRIVATE_HIDL) -o $(PRIVATE_OUTPUT_DIR) \
    -Lc++ -randroid.hardware:hardware/interfaces\
    android.hardware.nfc@1.0::$(patsubst %.hal,%,$(notdir $(PRIVATE_DEPS)))

$(GEN): $(LOCAL_PATH)/INfc.hal
	$(transform-generated-source)
LOCAL_GENERATED_SOURCES += $(GEN)

#
# Build INfcClientCallback.hal
#
GEN := $(intermediates)/android/hardware/nfc/1.0/NfcClientCallbackAll.cpp
$(GEN): $(HIDL)
$(GEN): PRIVATE_HIDL := $(HIDL)
$(GEN): PRIVATE_DEPS := $(LOCAL_PATH)/INfcClientCallback.hal
$(GEN): PRIVATE_OUTPUT_DIR := $(intermediates)
$(GEN): PRIVATE_CUSTOM_TOOL = \
    $(PRIVATE_HIDL) -o $(PRIVATE_OUTPUT_DIR) \
    -Lc++ -randroid.hardware:hardware/interfaces\
    android.hardware.nfc@1.0::$(patsubst %.hal,%,$(notdir $(PRIVATE_DEPS)))

$(GEN): $(LOCAL_PATH)/INfcClientCallback.hal
	$(transform-generated-source)
LOCAL_GENERATED_SOURCES += $(GEN)

LOCAL_EXPORT_C_INCLUDE_DIRS := $(intermediates)
LOCAL_SHARED_LIBRARIES := \
  libhidl \
  libhwbinder \
  libutils \

include $(BUILD_SHARED_LIBRARY)
