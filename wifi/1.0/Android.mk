LOCAL_PATH := $(call my-dir)
include $(CLEAR_VARS)

LOCAL_MODULE := android.hardware.wifi@1.0
LOCAL_MODULE_CLASS := SHARED_LIBRARIES

intermediates := $(local-generated-sources-dir)

HIDL := $(HOST_OUT_EXECUTABLES)/hidl-gen$(HOST_EXECUTABLE_SUFFIX)

#
# Build types.hal
#
GEN := $(intermediates)/android/hardware/wifi/1.0/types.cpp
$(GEN): $(HIDL)
$(GEN): PRIVATE_HIDL := $(HIDL)
$(GEN): PRIVATE_DEPS := $(LOCAL_PATH)/types.hal
$(GEN): PRIVATE_OUTPUT_DIR := $(intermediates)
$(GEN): PRIVATE_CUSTOM_TOOL = \
        $(PRIVATE_HIDL) -o $(PRIVATE_OUTPUT_DIR) \
        -Lc++ -randroid.hardware:hardware/interfaces \
        android.hardware.wifi@1.0::types

$(GEN): $(LOCAL_PATH)/types.hal
	$(transform-generated-source)
LOCAL_GENERATED_SOURCES += $(GEN)

#
# Build IWifi.hal
#
GEN := $(intermediates)/android/hardware/wifi/1.0/WifiAll.cpp
$(GEN): $(HIDL)
$(GEN): PRIVATE_HIDL := $(HIDL)
$(GEN): PRIVATE_DEPS := $(LOCAL_PATH)/IWifi.hal
$(GEN): PRIVATE_DEPS += $(LOCAL_PATH)/IWifiChip.hal
$(GEN): $(LOCAL_PATH)/IWifiChip.hal
$(GEN): PRIVATE_DEPS += $(LOCAL_PATH)/IWifiEventCallback.hal
$(GEN): $(LOCAL_PATH)/IWifiEventCallback.hal
$(GEN): PRIVATE_OUTPUT_DIR := $(intermediates)
$(GEN): PRIVATE_CUSTOM_TOOL = \
        $(PRIVATE_HIDL) -o $(PRIVATE_OUTPUT_DIR) \
        -Lc++ -randroid.hardware:hardware/interfaces \
        android.hardware.wifi@1.0::IWifi

$(GEN): $(LOCAL_PATH)/IWifi.hal
	$(transform-generated-source)
LOCAL_GENERATED_SOURCES += $(GEN)

#
# Build IWifiChip.hal
#
GEN := $(intermediates)/android/hardware/wifi/1.0/WifiChipAll.cpp
$(GEN): $(HIDL)
$(GEN): PRIVATE_HIDL := $(HIDL)
$(GEN): PRIVATE_DEPS := $(LOCAL_PATH)/IWifiChip.hal
$(GEN): PRIVATE_DEPS += $(LOCAL_PATH)/IWifiChipEventCallback.hal
$(GEN): $(LOCAL_PATH)/IWifiChipEventCallback.hal
$(GEN): PRIVATE_DEPS += $(LOCAL_PATH)/types.hal
$(GEN): $(LOCAL_PATH)/types.hal
$(GEN): PRIVATE_OUTPUT_DIR := $(intermediates)
$(GEN): PRIVATE_CUSTOM_TOOL = \
        $(PRIVATE_HIDL) -o $(PRIVATE_OUTPUT_DIR) \
        -Lc++ -randroid.hardware:hardware/interfaces \
        android.hardware.wifi@1.0::IWifiChip

$(GEN): $(LOCAL_PATH)/IWifiChip.hal
	$(transform-generated-source)
LOCAL_GENERATED_SOURCES += $(GEN)

#
# Build IWifiChipEventCallback.hal
#
GEN := $(intermediates)/android/hardware/wifi/1.0/WifiChipEventCallbackAll.cpp
$(GEN): $(HIDL)
$(GEN): PRIVATE_HIDL := $(HIDL)
$(GEN): PRIVATE_DEPS := $(LOCAL_PATH)/IWifiChipEventCallback.hal
$(GEN): PRIVATE_DEPS += $(LOCAL_PATH)/types.hal
$(GEN): $(LOCAL_PATH)/types.hal
$(GEN): PRIVATE_OUTPUT_DIR := $(intermediates)
$(GEN): PRIVATE_CUSTOM_TOOL = \
        $(PRIVATE_HIDL) -o $(PRIVATE_OUTPUT_DIR) \
        -Lc++ -randroid.hardware:hardware/interfaces \
        android.hardware.wifi@1.0::IWifiChipEventCallback

$(GEN): $(LOCAL_PATH)/IWifiChipEventCallback.hal
	$(transform-generated-source)
LOCAL_GENERATED_SOURCES += $(GEN)

#
# Build IWifiEventCallback.hal
#
GEN := $(intermediates)/android/hardware/wifi/1.0/WifiEventCallbackAll.cpp
$(GEN): $(HIDL)
$(GEN): PRIVATE_HIDL := $(HIDL)
$(GEN): PRIVATE_DEPS := $(LOCAL_PATH)/IWifiEventCallback.hal
$(GEN): PRIVATE_DEPS += $(LOCAL_PATH)/types.hal
$(GEN): $(LOCAL_PATH)/types.hal
$(GEN): PRIVATE_OUTPUT_DIR := $(intermediates)
$(GEN): PRIVATE_CUSTOM_TOOL = \
        $(PRIVATE_HIDL) -o $(PRIVATE_OUTPUT_DIR) \
        -Lc++ -randroid.hardware:hardware/interfaces \
        android.hardware.wifi@1.0::IWifiEventCallback

$(GEN): $(LOCAL_PATH)/IWifiEventCallback.hal
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
LOCAL_MODULE := android.hardware.wifi@1.0-java
LOCAL_MODULE_CLASS := JAVA_LIBRARIES

intermediates := $(local-generated-sources-dir)

HIDL := $(HOST_OUT_EXECUTABLES)/hidl-gen$(HOST_EXECUTABLE_SUFFIX)

#
# Build types.hal (CommandFailureReason)
#
GEN := $(intermediates)/android/hardware/wifi/1.0/CommandFailureReason.java
$(GEN): $(HIDL)
$(GEN): PRIVATE_HIDL := $(HIDL)
$(GEN): PRIVATE_DEPS := $(LOCAL_PATH)/types.hal
$(GEN): PRIVATE_OUTPUT_DIR := $(intermediates)
$(GEN): PRIVATE_CUSTOM_TOOL = \
        $(PRIVATE_HIDL) -o $(PRIVATE_OUTPUT_DIR) \
        -Ljava -randroid.hardware:hardware/interfaces \
        android.hardware.wifi@1.0::types.CommandFailureReason

$(GEN): $(LOCAL_PATH)/types.hal
	$(transform-generated-source)
LOCAL_GENERATED_SOURCES += $(GEN)

#
# Build types.hal (FailureReason)
#
GEN := $(intermediates)/android/hardware/wifi/1.0/FailureReason.java
$(GEN): $(HIDL)
$(GEN): PRIVATE_HIDL := $(HIDL)
$(GEN): PRIVATE_DEPS := $(LOCAL_PATH)/types.hal
$(GEN): PRIVATE_OUTPUT_DIR := $(intermediates)
$(GEN): PRIVATE_CUSTOM_TOOL = \
        $(PRIVATE_HIDL) -o $(PRIVATE_OUTPUT_DIR) \
        -Ljava -randroid.hardware:hardware/interfaces \
        android.hardware.wifi@1.0::types.FailureReason

$(GEN): $(LOCAL_PATH)/types.hal
	$(transform-generated-source)
LOCAL_GENERATED_SOURCES += $(GEN)

#
# Build IWifi.hal
#
GEN := $(intermediates)/android/hardware/wifi/1.0/IWifi.java
$(GEN): $(HIDL)
$(GEN): PRIVATE_HIDL := $(HIDL)
$(GEN): PRIVATE_DEPS := $(LOCAL_PATH)/IWifi.hal
$(GEN): PRIVATE_DEPS += $(LOCAL_PATH)/IWifiChip.hal
$(GEN): $(LOCAL_PATH)/IWifiChip.hal
$(GEN): PRIVATE_DEPS += $(LOCAL_PATH)/IWifiEventCallback.hal
$(GEN): $(LOCAL_PATH)/IWifiEventCallback.hal
$(GEN): PRIVATE_OUTPUT_DIR := $(intermediates)
$(GEN): PRIVATE_CUSTOM_TOOL = \
        $(PRIVATE_HIDL) -o $(PRIVATE_OUTPUT_DIR) \
        -Ljava -randroid.hardware:hardware/interfaces \
        android.hardware.wifi@1.0::IWifi

$(GEN): $(LOCAL_PATH)/IWifi.hal
	$(transform-generated-source)
LOCAL_GENERATED_SOURCES += $(GEN)

#
# Build IWifiChip.hal
#
GEN := $(intermediates)/android/hardware/wifi/1.0/IWifiChip.java
$(GEN): $(HIDL)
$(GEN): PRIVATE_HIDL := $(HIDL)
$(GEN): PRIVATE_DEPS := $(LOCAL_PATH)/IWifiChip.hal
$(GEN): PRIVATE_DEPS += $(LOCAL_PATH)/IWifiChipEventCallback.hal
$(GEN): $(LOCAL_PATH)/IWifiChipEventCallback.hal
$(GEN): PRIVATE_DEPS += $(LOCAL_PATH)/types.hal
$(GEN): $(LOCAL_PATH)/types.hal
$(GEN): PRIVATE_OUTPUT_DIR := $(intermediates)
$(GEN): PRIVATE_CUSTOM_TOOL = \
        $(PRIVATE_HIDL) -o $(PRIVATE_OUTPUT_DIR) \
        -Ljava -randroid.hardware:hardware/interfaces \
        android.hardware.wifi@1.0::IWifiChip

$(GEN): $(LOCAL_PATH)/IWifiChip.hal
	$(transform-generated-source)
LOCAL_GENERATED_SOURCES += $(GEN)

#
# Build IWifiChipEventCallback.hal
#
GEN := $(intermediates)/android/hardware/wifi/1.0/IWifiChipEventCallback.java
$(GEN): $(HIDL)
$(GEN): PRIVATE_HIDL := $(HIDL)
$(GEN): PRIVATE_DEPS := $(LOCAL_PATH)/IWifiChipEventCallback.hal
$(GEN): PRIVATE_DEPS += $(LOCAL_PATH)/types.hal
$(GEN): $(LOCAL_PATH)/types.hal
$(GEN): PRIVATE_OUTPUT_DIR := $(intermediates)
$(GEN): PRIVATE_CUSTOM_TOOL = \
        $(PRIVATE_HIDL) -o $(PRIVATE_OUTPUT_DIR) \
        -Ljava -randroid.hardware:hardware/interfaces \
        android.hardware.wifi@1.0::IWifiChipEventCallback

$(GEN): $(LOCAL_PATH)/IWifiChipEventCallback.hal
	$(transform-generated-source)
LOCAL_GENERATED_SOURCES += $(GEN)

#
# Build IWifiEventCallback.hal
#
GEN := $(intermediates)/android/hardware/wifi/1.0/IWifiEventCallback.java
$(GEN): $(HIDL)
$(GEN): PRIVATE_HIDL := $(HIDL)
$(GEN): PRIVATE_DEPS := $(LOCAL_PATH)/IWifiEventCallback.hal
$(GEN): PRIVATE_DEPS += $(LOCAL_PATH)/types.hal
$(GEN): $(LOCAL_PATH)/types.hal
$(GEN): PRIVATE_OUTPUT_DIR := $(intermediates)
$(GEN): PRIVATE_CUSTOM_TOOL = \
        $(PRIVATE_HIDL) -o $(PRIVATE_OUTPUT_DIR) \
        -Ljava -randroid.hardware:hardware/interfaces \
        android.hardware.wifi@1.0::IWifiEventCallback

$(GEN): $(LOCAL_PATH)/IWifiEventCallback.hal
	$(transform-generated-source)
LOCAL_GENERATED_SOURCES += $(GEN)
include $(BUILD_JAVA_LIBRARY)


################################################################################

include $(CLEAR_VARS)
LOCAL_MODULE := android.hardware.wifi@1.0-java-static
LOCAL_MODULE_CLASS := JAVA_LIBRARIES

intermediates := $(local-generated-sources-dir)

HIDL := $(HOST_OUT_EXECUTABLES)/hidl-gen$(HOST_EXECUTABLE_SUFFIX)

#
# Build types.hal (CommandFailureReason)
#
GEN := $(intermediates)/android/hardware/wifi/1.0/CommandFailureReason.java
$(GEN): $(HIDL)
$(GEN): PRIVATE_HIDL := $(HIDL)
$(GEN): PRIVATE_DEPS := $(LOCAL_PATH)/types.hal
$(GEN): PRIVATE_OUTPUT_DIR := $(intermediates)
$(GEN): PRIVATE_CUSTOM_TOOL = \
        $(PRIVATE_HIDL) -o $(PRIVATE_OUTPUT_DIR) \
        -Ljava -randroid.hardware:hardware/interfaces \
        android.hardware.wifi@1.0::types.CommandFailureReason

$(GEN): $(LOCAL_PATH)/types.hal
	$(transform-generated-source)
LOCAL_GENERATED_SOURCES += $(GEN)

#
# Build types.hal (FailureReason)
#
GEN := $(intermediates)/android/hardware/wifi/1.0/FailureReason.java
$(GEN): $(HIDL)
$(GEN): PRIVATE_HIDL := $(HIDL)
$(GEN): PRIVATE_DEPS := $(LOCAL_PATH)/types.hal
$(GEN): PRIVATE_OUTPUT_DIR := $(intermediates)
$(GEN): PRIVATE_CUSTOM_TOOL = \
        $(PRIVATE_HIDL) -o $(PRIVATE_OUTPUT_DIR) \
        -Ljava -randroid.hardware:hardware/interfaces \
        android.hardware.wifi@1.0::types.FailureReason

$(GEN): $(LOCAL_PATH)/types.hal
	$(transform-generated-source)
LOCAL_GENERATED_SOURCES += $(GEN)

#
# Build IWifi.hal
#
GEN := $(intermediates)/android/hardware/wifi/1.0/IWifi.java
$(GEN): $(HIDL)
$(GEN): PRIVATE_HIDL := $(HIDL)
$(GEN): PRIVATE_DEPS := $(LOCAL_PATH)/IWifi.hal
$(GEN): PRIVATE_DEPS += $(LOCAL_PATH)/IWifiChip.hal
$(GEN): $(LOCAL_PATH)/IWifiChip.hal
$(GEN): PRIVATE_DEPS += $(LOCAL_PATH)/IWifiEventCallback.hal
$(GEN): $(LOCAL_PATH)/IWifiEventCallback.hal
$(GEN): PRIVATE_OUTPUT_DIR := $(intermediates)
$(GEN): PRIVATE_CUSTOM_TOOL = \
        $(PRIVATE_HIDL) -o $(PRIVATE_OUTPUT_DIR) \
        -Ljava -randroid.hardware:hardware/interfaces \
        android.hardware.wifi@1.0::IWifi

$(GEN): $(LOCAL_PATH)/IWifi.hal
	$(transform-generated-source)
LOCAL_GENERATED_SOURCES += $(GEN)

#
# Build IWifiChip.hal
#
GEN := $(intermediates)/android/hardware/wifi/1.0/IWifiChip.java
$(GEN): $(HIDL)
$(GEN): PRIVATE_HIDL := $(HIDL)
$(GEN): PRIVATE_DEPS := $(LOCAL_PATH)/IWifiChip.hal
$(GEN): PRIVATE_DEPS += $(LOCAL_PATH)/IWifiChipEventCallback.hal
$(GEN): $(LOCAL_PATH)/IWifiChipEventCallback.hal
$(GEN): PRIVATE_DEPS += $(LOCAL_PATH)/types.hal
$(GEN): $(LOCAL_PATH)/types.hal
$(GEN): PRIVATE_OUTPUT_DIR := $(intermediates)
$(GEN): PRIVATE_CUSTOM_TOOL = \
        $(PRIVATE_HIDL) -o $(PRIVATE_OUTPUT_DIR) \
        -Ljava -randroid.hardware:hardware/interfaces \
        android.hardware.wifi@1.0::IWifiChip

$(GEN): $(LOCAL_PATH)/IWifiChip.hal
	$(transform-generated-source)
LOCAL_GENERATED_SOURCES += $(GEN)

#
# Build IWifiChipEventCallback.hal
#
GEN := $(intermediates)/android/hardware/wifi/1.0/IWifiChipEventCallback.java
$(GEN): $(HIDL)
$(GEN): PRIVATE_HIDL := $(HIDL)
$(GEN): PRIVATE_DEPS := $(LOCAL_PATH)/IWifiChipEventCallback.hal
$(GEN): PRIVATE_DEPS += $(LOCAL_PATH)/types.hal
$(GEN): $(LOCAL_PATH)/types.hal
$(GEN): PRIVATE_OUTPUT_DIR := $(intermediates)
$(GEN): PRIVATE_CUSTOM_TOOL = \
        $(PRIVATE_HIDL) -o $(PRIVATE_OUTPUT_DIR) \
        -Ljava -randroid.hardware:hardware/interfaces \
        android.hardware.wifi@1.0::IWifiChipEventCallback

$(GEN): $(LOCAL_PATH)/IWifiChipEventCallback.hal
	$(transform-generated-source)
LOCAL_GENERATED_SOURCES += $(GEN)

#
# Build IWifiEventCallback.hal
#
GEN := $(intermediates)/android/hardware/wifi/1.0/IWifiEventCallback.java
$(GEN): $(HIDL)
$(GEN): PRIVATE_HIDL := $(HIDL)
$(GEN): PRIVATE_DEPS := $(LOCAL_PATH)/IWifiEventCallback.hal
$(GEN): PRIVATE_DEPS += $(LOCAL_PATH)/types.hal
$(GEN): $(LOCAL_PATH)/types.hal
$(GEN): PRIVATE_OUTPUT_DIR := $(intermediates)
$(GEN): PRIVATE_CUSTOM_TOOL = \
        $(PRIVATE_HIDL) -o $(PRIVATE_OUTPUT_DIR) \
        -Ljava -randroid.hardware:hardware/interfaces \
        android.hardware.wifi@1.0::IWifiEventCallback

$(GEN): $(LOCAL_PATH)/IWifiEventCallback.hal
	$(transform-generated-source)
LOCAL_GENERATED_SOURCES += $(GEN)
include $(BUILD_STATIC_JAVA_LIBRARY)



include $(call all-makefiles-under,$(LOCAL_PATH))
