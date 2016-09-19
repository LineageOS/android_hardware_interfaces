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
    -Lc++ -randroid.hardware:hardware/interfaces \
    android.hardware.nfc@1.0::types

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
$(GEN): PRIVATE_DEPS += $(LOCAL_PATH)/INfcClientCallback.hal
$(GEN): $(LOCAL_PATH)/INfcClientCallback.hal
$(GEN): PRIVATE_DEPS += $(LOCAL_PATH)/types.hal
$(GEN): $(LOCAL_PATH)/types.hal
$(GEN): PRIVATE_OUTPUT_DIR := $(intermediates)
$(GEN): PRIVATE_CUSTOM_TOOL = \
    $(PRIVATE_HIDL) -o $(PRIVATE_OUTPUT_DIR) \
    -Lc++ -randroid.hardware:hardware/interfaces \
    android.hardware.nfc@1.0::INfc

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
$(GEN): PRIVATE_DEPS += $(LOCAL_PATH)/types.hal
$(GEN): $(LOCAL_PATH)/types.hal
$(GEN): PRIVATE_OUTPUT_DIR := $(intermediates)
$(GEN): PRIVATE_CUSTOM_TOOL = \
    $(PRIVATE_HIDL) -o $(PRIVATE_OUTPUT_DIR) \
    -Lc++ -randroid.hardware:hardware/interfaces \
    android.hardware.nfc@1.0::INfcClientCallback

$(GEN): $(LOCAL_PATH)/INfcClientCallback.hal
	$(transform-generated-source)
LOCAL_GENERATED_SOURCES += $(GEN)

LOCAL_EXPORT_C_INCLUDE_DIRS := $(intermediates)
LOCAL_SHARED_LIBRARIES := \
  libhidl \
  libhwbinder \
  libutils \
  libcutils \

LOCAL_MULTILIB := both
include $(BUILD_SHARED_LIBRARY)

################################################################################

include $(CLEAR_VARS)
LOCAL_MODULE := android.hardware.nfc@1.0-java
LOCAL_MODULE_CLASS := JAVA_LIBRARIES

intermediates := $(local-generated-sources-dir)

HIDL := $(HOST_OUT_EXECUTABLES)/hidl-gen$(HOST_EXECUTABLE_SUFFIX)

#
# Build types.hal (nfc_data_t)
#
GEN := $(intermediates)/android/hardware/nfc/1.0/nfc_data_t.java
$(GEN): $(HIDL)
$(GEN): PRIVATE_HIDL := $(HIDL)
$(GEN): PRIVATE_DEPS := $(LOCAL_PATH)/types.hal
$(GEN): PRIVATE_OUTPUT_DIR := $(intermediates)
$(GEN): PRIVATE_CUSTOM_TOOL = \
    $(PRIVATE_HIDL) -o $(PRIVATE_OUTPUT_DIR) \
    -Ljava -randroid.hardware:hardware/interfaces \
    android.hardware.nfc@1.0::types.nfc_data_t

$(GEN): $(LOCAL_PATH)/types.hal
	$(transform-generated-source)
LOCAL_GENERATED_SOURCES += $(GEN)

#
# Build types.hal (nfc_event_t)
#
GEN := $(intermediates)/android/hardware/nfc/1.0/nfc_event_t.java
$(GEN): $(HIDL)
$(GEN): PRIVATE_HIDL := $(HIDL)
$(GEN): PRIVATE_DEPS := $(LOCAL_PATH)/types.hal
$(GEN): PRIVATE_OUTPUT_DIR := $(intermediates)
$(GEN): PRIVATE_CUSTOM_TOOL = \
    $(PRIVATE_HIDL) -o $(PRIVATE_OUTPUT_DIR) \
    -Ljava -randroid.hardware:hardware/interfaces \
    android.hardware.nfc@1.0::types.nfc_event_t

$(GEN): $(LOCAL_PATH)/types.hal
	$(transform-generated-source)
LOCAL_GENERATED_SOURCES += $(GEN)

#
# Build types.hal (nfc_status_t)
#
GEN := $(intermediates)/android/hardware/nfc/1.0/nfc_status_t.java
$(GEN): $(HIDL)
$(GEN): PRIVATE_HIDL := $(HIDL)
$(GEN): PRIVATE_DEPS := $(LOCAL_PATH)/types.hal
$(GEN): PRIVATE_OUTPUT_DIR := $(intermediates)
$(GEN): PRIVATE_CUSTOM_TOOL = \
    $(PRIVATE_HIDL) -o $(PRIVATE_OUTPUT_DIR) \
    -Ljava -randroid.hardware:hardware/interfaces \
    android.hardware.nfc@1.0::types.nfc_status_t

$(GEN): $(LOCAL_PATH)/types.hal
	$(transform-generated-source)
LOCAL_GENERATED_SOURCES += $(GEN)

#
# Build INfc.hal
#
GEN := $(intermediates)/android/hardware/nfc/1.0/INfc.java
$(GEN): $(HIDL)
$(GEN): PRIVATE_HIDL := $(HIDL)
$(GEN): PRIVATE_DEPS := $(LOCAL_PATH)/INfc.hal
$(GEN): PRIVATE_DEPS += $(LOCAL_PATH)/INfcClientCallback.hal
$(GEN): $(LOCAL_PATH)/INfcClientCallback.hal
$(GEN): PRIVATE_DEPS += $(LOCAL_PATH)/types.hal
$(GEN): $(LOCAL_PATH)/types.hal
$(GEN): PRIVATE_OUTPUT_DIR := $(intermediates)
$(GEN): PRIVATE_CUSTOM_TOOL = \
    $(PRIVATE_HIDL) -o $(PRIVATE_OUTPUT_DIR) \
    -Ljava -randroid.hardware:hardware/interfaces \
    android.hardware.nfc@1.0::INfc

$(GEN): $(LOCAL_PATH)/INfc.hal
	$(transform-generated-source)
LOCAL_GENERATED_SOURCES += $(GEN)

#
# Build INfcClientCallback.hal
#
GEN := $(intermediates)/android/hardware/nfc/1.0/INfcClientCallback.java
$(GEN): $(HIDL)
$(GEN): PRIVATE_HIDL := $(HIDL)
$(GEN): PRIVATE_DEPS := $(LOCAL_PATH)/INfcClientCallback.hal
$(GEN): PRIVATE_DEPS += $(LOCAL_PATH)/types.hal
$(GEN): $(LOCAL_PATH)/types.hal
$(GEN): PRIVATE_OUTPUT_DIR := $(intermediates)
$(GEN): PRIVATE_CUSTOM_TOOL = \
    $(PRIVATE_HIDL) -o $(PRIVATE_OUTPUT_DIR) \
    -Ljava -randroid.hardware:hardware/interfaces \
    android.hardware.nfc@1.0::INfcClientCallback

$(GEN): $(LOCAL_PATH)/INfcClientCallback.hal
	$(transform-generated-source)
LOCAL_GENERATED_SOURCES += $(GEN)
include $(BUILD_JAVA_LIBRARY)
