LOCAL_PATH := $(call my-dir)
include $(CLEAR_VARS)

LOCAL_MODULE := android.hardware.wifi.supplicant@1.0
LOCAL_MODULE_CLASS := SHARED_LIBRARIES

intermediates := $(local-generated-sources-dir)

HIDL := $(HOST_OUT_EXECUTABLES)/hidl-gen$(HOST_EXECUTABLE_SUFFIX)

#
# Build types.hal
#
GEN := $(intermediates)/android/hardware/wifi/supplicant/1.0/types.cpp
$(GEN): $(HIDL)
$(GEN): PRIVATE_HIDL := $(HIDL)
$(GEN): PRIVATE_DEPS := $(LOCAL_PATH)/types.hal
$(GEN): PRIVATE_OUTPUT_DIR := $(intermediates)
$(GEN): PRIVATE_CUSTOM_TOOL = \
        $(PRIVATE_HIDL) -o $(PRIVATE_OUTPUT_DIR) \
        -Lc++ -randroid.hardware:hardware/interfaces \
        android.hardware.wifi.supplicant@1.0::types

$(GEN): $(LOCAL_PATH)/types.hal
	$(transform-generated-source)
LOCAL_GENERATED_SOURCES += $(GEN)

#
# Build ISupplicant.hal
#
GEN := $(intermediates)/android/hardware/wifi/supplicant/1.0/SupplicantAll.cpp
$(GEN): $(HIDL)
$(GEN): PRIVATE_HIDL := $(HIDL)
$(GEN): PRIVATE_DEPS := $(LOCAL_PATH)/ISupplicant.hal
$(GEN): PRIVATE_DEPS += $(LOCAL_PATH)/ISupplicantCallback.hal
$(GEN): $(LOCAL_PATH)/ISupplicantCallback.hal
$(GEN): PRIVATE_DEPS += $(LOCAL_PATH)/ISupplicantIface.hal
$(GEN): $(LOCAL_PATH)/ISupplicantIface.hal
$(GEN): PRIVATE_DEPS += $(LOCAL_PATH)/types.hal
$(GEN): $(LOCAL_PATH)/types.hal
$(GEN): PRIVATE_OUTPUT_DIR := $(intermediates)
$(GEN): PRIVATE_CUSTOM_TOOL = \
        $(PRIVATE_HIDL) -o $(PRIVATE_OUTPUT_DIR) \
        -Lc++ -randroid.hardware:hardware/interfaces \
        android.hardware.wifi.supplicant@1.0::ISupplicant

$(GEN): $(LOCAL_PATH)/ISupplicant.hal
	$(transform-generated-source)
LOCAL_GENERATED_SOURCES += $(GEN)

#
# Build ISupplicantCallback.hal
#
GEN := $(intermediates)/android/hardware/wifi/supplicant/1.0/SupplicantCallbackAll.cpp
$(GEN): $(HIDL)
$(GEN): PRIVATE_HIDL := $(HIDL)
$(GEN): PRIVATE_DEPS := $(LOCAL_PATH)/ISupplicantCallback.hal
$(GEN): PRIVATE_OUTPUT_DIR := $(intermediates)
$(GEN): PRIVATE_CUSTOM_TOOL = \
        $(PRIVATE_HIDL) -o $(PRIVATE_OUTPUT_DIR) \
        -Lc++ -randroid.hardware:hardware/interfaces \
        android.hardware.wifi.supplicant@1.0::ISupplicantCallback

$(GEN): $(LOCAL_PATH)/ISupplicantCallback.hal
	$(transform-generated-source)
LOCAL_GENERATED_SOURCES += $(GEN)

#
# Build ISupplicantIface.hal
#
GEN := $(intermediates)/android/hardware/wifi/supplicant/1.0/SupplicantIfaceAll.cpp
$(GEN): $(HIDL)
$(GEN): PRIVATE_HIDL := $(HIDL)
$(GEN): PRIVATE_DEPS := $(LOCAL_PATH)/ISupplicantIface.hal
$(GEN): PRIVATE_DEPS += $(LOCAL_PATH)/ISupplicantIfaceCallback.hal
$(GEN): $(LOCAL_PATH)/ISupplicantIfaceCallback.hal
$(GEN): PRIVATE_DEPS += $(LOCAL_PATH)/ISupplicantNetwork.hal
$(GEN): $(LOCAL_PATH)/ISupplicantNetwork.hal
$(GEN): PRIVATE_DEPS += $(LOCAL_PATH)/types.hal
$(GEN): $(LOCAL_PATH)/types.hal
$(GEN): PRIVATE_OUTPUT_DIR := $(intermediates)
$(GEN): PRIVATE_CUSTOM_TOOL = \
        $(PRIVATE_HIDL) -o $(PRIVATE_OUTPUT_DIR) \
        -Lc++ -randroid.hardware:hardware/interfaces \
        android.hardware.wifi.supplicant@1.0::ISupplicantIface

$(GEN): $(LOCAL_PATH)/ISupplicantIface.hal
	$(transform-generated-source)
LOCAL_GENERATED_SOURCES += $(GEN)

#
# Build ISupplicantIfaceCallback.hal
#
GEN := $(intermediates)/android/hardware/wifi/supplicant/1.0/SupplicantIfaceCallbackAll.cpp
$(GEN): $(HIDL)
$(GEN): PRIVATE_HIDL := $(HIDL)
$(GEN): PRIVATE_DEPS := $(LOCAL_PATH)/ISupplicantIfaceCallback.hal
$(GEN): PRIVATE_DEPS += $(LOCAL_PATH)/types.hal
$(GEN): $(LOCAL_PATH)/types.hal
$(GEN): PRIVATE_OUTPUT_DIR := $(intermediates)
$(GEN): PRIVATE_CUSTOM_TOOL = \
        $(PRIVATE_HIDL) -o $(PRIVATE_OUTPUT_DIR) \
        -Lc++ -randroid.hardware:hardware/interfaces \
        android.hardware.wifi.supplicant@1.0::ISupplicantIfaceCallback

$(GEN): $(LOCAL_PATH)/ISupplicantIfaceCallback.hal
	$(transform-generated-source)
LOCAL_GENERATED_SOURCES += $(GEN)

#
# Build ISupplicantNetwork.hal
#
GEN := $(intermediates)/android/hardware/wifi/supplicant/1.0/SupplicantNetworkAll.cpp
$(GEN): $(HIDL)
$(GEN): PRIVATE_HIDL := $(HIDL)
$(GEN): PRIVATE_DEPS := $(LOCAL_PATH)/ISupplicantNetwork.hal
$(GEN): PRIVATE_DEPS += $(LOCAL_PATH)/ISupplicantNetworkCallback.hal
$(GEN): $(LOCAL_PATH)/ISupplicantNetworkCallback.hal
$(GEN): PRIVATE_DEPS += $(LOCAL_PATH)/types.hal
$(GEN): $(LOCAL_PATH)/types.hal
$(GEN): PRIVATE_OUTPUT_DIR := $(intermediates)
$(GEN): PRIVATE_CUSTOM_TOOL = \
        $(PRIVATE_HIDL) -o $(PRIVATE_OUTPUT_DIR) \
        -Lc++ -randroid.hardware:hardware/interfaces \
        android.hardware.wifi.supplicant@1.0::ISupplicantNetwork

$(GEN): $(LOCAL_PATH)/ISupplicantNetwork.hal
	$(transform-generated-source)
LOCAL_GENERATED_SOURCES += $(GEN)

#
# Build ISupplicantNetworkCallback.hal
#
GEN := $(intermediates)/android/hardware/wifi/supplicant/1.0/SupplicantNetworkCallbackAll.cpp
$(GEN): $(HIDL)
$(GEN): PRIVATE_HIDL := $(HIDL)
$(GEN): PRIVATE_DEPS := $(LOCAL_PATH)/ISupplicantNetworkCallback.hal
$(GEN): PRIVATE_OUTPUT_DIR := $(intermediates)
$(GEN): PRIVATE_CUSTOM_TOOL = \
        $(PRIVATE_HIDL) -o $(PRIVATE_OUTPUT_DIR) \
        -Lc++ -randroid.hardware:hardware/interfaces \
        android.hardware.wifi.supplicant@1.0::ISupplicantNetworkCallback

$(GEN): $(LOCAL_PATH)/ISupplicantNetworkCallback.hal
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
LOCAL_MODULE := android.hardware.wifi.supplicant@1.0-java
LOCAL_MODULE_CLASS := JAVA_LIBRARIES

intermediates := $(local-generated-sources-dir)

HIDL := $(HOST_OUT_EXECUTABLES)/hidl-gen$(HOST_EXECUTABLE_SUFFIX)

#
# Build types.hal (SupplicantStatus)
#
GEN := $(intermediates)/android/hardware/wifi/supplicant/1.0/SupplicantStatus.java
$(GEN): $(HIDL)
$(GEN): PRIVATE_HIDL := $(HIDL)
$(GEN): PRIVATE_DEPS := $(LOCAL_PATH)/types.hal
$(GEN): PRIVATE_OUTPUT_DIR := $(intermediates)
$(GEN): PRIVATE_CUSTOM_TOOL = \
        $(PRIVATE_HIDL) -o $(PRIVATE_OUTPUT_DIR) \
        -Ljava -randroid.hardware:hardware/interfaces \
        android.hardware.wifi.supplicant@1.0::types.SupplicantStatus

$(GEN): $(LOCAL_PATH)/types.hal
	$(transform-generated-source)
LOCAL_GENERATED_SOURCES += $(GEN)

#
# Build types.hal (SupplicantStatusCode)
#
GEN := $(intermediates)/android/hardware/wifi/supplicant/1.0/SupplicantStatusCode.java
$(GEN): $(HIDL)
$(GEN): PRIVATE_HIDL := $(HIDL)
$(GEN): PRIVATE_DEPS := $(LOCAL_PATH)/types.hal
$(GEN): PRIVATE_OUTPUT_DIR := $(intermediates)
$(GEN): PRIVATE_CUSTOM_TOOL = \
        $(PRIVATE_HIDL) -o $(PRIVATE_OUTPUT_DIR) \
        -Ljava -randroid.hardware:hardware/interfaces \
        android.hardware.wifi.supplicant@1.0::types.SupplicantStatusCode

$(GEN): $(LOCAL_PATH)/types.hal
	$(transform-generated-source)
LOCAL_GENERATED_SOURCES += $(GEN)

#
# Build ISupplicant.hal
#
GEN := $(intermediates)/android/hardware/wifi/supplicant/1.0/ISupplicant.java
$(GEN): $(HIDL)
$(GEN): PRIVATE_HIDL := $(HIDL)
$(GEN): PRIVATE_DEPS := $(LOCAL_PATH)/ISupplicant.hal
$(GEN): PRIVATE_DEPS += $(LOCAL_PATH)/ISupplicantCallback.hal
$(GEN): $(LOCAL_PATH)/ISupplicantCallback.hal
$(GEN): PRIVATE_DEPS += $(LOCAL_PATH)/ISupplicantIface.hal
$(GEN): $(LOCAL_PATH)/ISupplicantIface.hal
$(GEN): PRIVATE_DEPS += $(LOCAL_PATH)/types.hal
$(GEN): $(LOCAL_PATH)/types.hal
$(GEN): PRIVATE_OUTPUT_DIR := $(intermediates)
$(GEN): PRIVATE_CUSTOM_TOOL = \
        $(PRIVATE_HIDL) -o $(PRIVATE_OUTPUT_DIR) \
        -Ljava -randroid.hardware:hardware/interfaces \
        android.hardware.wifi.supplicant@1.0::ISupplicant

$(GEN): $(LOCAL_PATH)/ISupplicant.hal
	$(transform-generated-source)
LOCAL_GENERATED_SOURCES += $(GEN)

#
# Build ISupplicantCallback.hal
#
GEN := $(intermediates)/android/hardware/wifi/supplicant/1.0/ISupplicantCallback.java
$(GEN): $(HIDL)
$(GEN): PRIVATE_HIDL := $(HIDL)
$(GEN): PRIVATE_DEPS := $(LOCAL_PATH)/ISupplicantCallback.hal
$(GEN): PRIVATE_OUTPUT_DIR := $(intermediates)
$(GEN): PRIVATE_CUSTOM_TOOL = \
        $(PRIVATE_HIDL) -o $(PRIVATE_OUTPUT_DIR) \
        -Ljava -randroid.hardware:hardware/interfaces \
        android.hardware.wifi.supplicant@1.0::ISupplicantCallback

$(GEN): $(LOCAL_PATH)/ISupplicantCallback.hal
	$(transform-generated-source)
LOCAL_GENERATED_SOURCES += $(GEN)

#
# Build ISupplicantIface.hal
#
GEN := $(intermediates)/android/hardware/wifi/supplicant/1.0/ISupplicantIface.java
$(GEN): $(HIDL)
$(GEN): PRIVATE_HIDL := $(HIDL)
$(GEN): PRIVATE_DEPS := $(LOCAL_PATH)/ISupplicantIface.hal
$(GEN): PRIVATE_DEPS += $(LOCAL_PATH)/ISupplicantIfaceCallback.hal
$(GEN): $(LOCAL_PATH)/ISupplicantIfaceCallback.hal
$(GEN): PRIVATE_DEPS += $(LOCAL_PATH)/ISupplicantNetwork.hal
$(GEN): $(LOCAL_PATH)/ISupplicantNetwork.hal
$(GEN): PRIVATE_DEPS += $(LOCAL_PATH)/types.hal
$(GEN): $(LOCAL_PATH)/types.hal
$(GEN): PRIVATE_OUTPUT_DIR := $(intermediates)
$(GEN): PRIVATE_CUSTOM_TOOL = \
        $(PRIVATE_HIDL) -o $(PRIVATE_OUTPUT_DIR) \
        -Ljava -randroid.hardware:hardware/interfaces \
        android.hardware.wifi.supplicant@1.0::ISupplicantIface

$(GEN): $(LOCAL_PATH)/ISupplicantIface.hal
	$(transform-generated-source)
LOCAL_GENERATED_SOURCES += $(GEN)

#
# Build ISupplicantIfaceCallback.hal
#
GEN := $(intermediates)/android/hardware/wifi/supplicant/1.0/ISupplicantIfaceCallback.java
$(GEN): $(HIDL)
$(GEN): PRIVATE_HIDL := $(HIDL)
$(GEN): PRIVATE_DEPS := $(LOCAL_PATH)/ISupplicantIfaceCallback.hal
$(GEN): PRIVATE_DEPS += $(LOCAL_PATH)/types.hal
$(GEN): $(LOCAL_PATH)/types.hal
$(GEN): PRIVATE_OUTPUT_DIR := $(intermediates)
$(GEN): PRIVATE_CUSTOM_TOOL = \
        $(PRIVATE_HIDL) -o $(PRIVATE_OUTPUT_DIR) \
        -Ljava -randroid.hardware:hardware/interfaces \
        android.hardware.wifi.supplicant@1.0::ISupplicantIfaceCallback

$(GEN): $(LOCAL_PATH)/ISupplicantIfaceCallback.hal
	$(transform-generated-source)
LOCAL_GENERATED_SOURCES += $(GEN)

#
# Build ISupplicantNetwork.hal
#
GEN := $(intermediates)/android/hardware/wifi/supplicant/1.0/ISupplicantNetwork.java
$(GEN): $(HIDL)
$(GEN): PRIVATE_HIDL := $(HIDL)
$(GEN): PRIVATE_DEPS := $(LOCAL_PATH)/ISupplicantNetwork.hal
$(GEN): PRIVATE_DEPS += $(LOCAL_PATH)/ISupplicantNetworkCallback.hal
$(GEN): $(LOCAL_PATH)/ISupplicantNetworkCallback.hal
$(GEN): PRIVATE_DEPS += $(LOCAL_PATH)/types.hal
$(GEN): $(LOCAL_PATH)/types.hal
$(GEN): PRIVATE_OUTPUT_DIR := $(intermediates)
$(GEN): PRIVATE_CUSTOM_TOOL = \
        $(PRIVATE_HIDL) -o $(PRIVATE_OUTPUT_DIR) \
        -Ljava -randroid.hardware:hardware/interfaces \
        android.hardware.wifi.supplicant@1.0::ISupplicantNetwork

$(GEN): $(LOCAL_PATH)/ISupplicantNetwork.hal
	$(transform-generated-source)
LOCAL_GENERATED_SOURCES += $(GEN)

#
# Build ISupplicantNetworkCallback.hal
#
GEN := $(intermediates)/android/hardware/wifi/supplicant/1.0/ISupplicantNetworkCallback.java
$(GEN): $(HIDL)
$(GEN): PRIVATE_HIDL := $(HIDL)
$(GEN): PRIVATE_DEPS := $(LOCAL_PATH)/ISupplicantNetworkCallback.hal
$(GEN): PRIVATE_OUTPUT_DIR := $(intermediates)
$(GEN): PRIVATE_CUSTOM_TOOL = \
        $(PRIVATE_HIDL) -o $(PRIVATE_OUTPUT_DIR) \
        -Ljava -randroid.hardware:hardware/interfaces \
        android.hardware.wifi.supplicant@1.0::ISupplicantNetworkCallback

$(GEN): $(LOCAL_PATH)/ISupplicantNetworkCallback.hal
	$(transform-generated-source)
LOCAL_GENERATED_SOURCES += $(GEN)
include $(BUILD_JAVA_LIBRARY)


################################################################################

include $(CLEAR_VARS)
LOCAL_MODULE := android.hardware.wifi.supplicant@1.0-java-static
LOCAL_MODULE_CLASS := JAVA_LIBRARIES

intermediates := $(local-generated-sources-dir)

HIDL := $(HOST_OUT_EXECUTABLES)/hidl-gen$(HOST_EXECUTABLE_SUFFIX)

#
# Build types.hal (SupplicantStatus)
#
GEN := $(intermediates)/android/hardware/wifi/supplicant/1.0/SupplicantStatus.java
$(GEN): $(HIDL)
$(GEN): PRIVATE_HIDL := $(HIDL)
$(GEN): PRIVATE_DEPS := $(LOCAL_PATH)/types.hal
$(GEN): PRIVATE_OUTPUT_DIR := $(intermediates)
$(GEN): PRIVATE_CUSTOM_TOOL = \
        $(PRIVATE_HIDL) -o $(PRIVATE_OUTPUT_DIR) \
        -Ljava -randroid.hardware:hardware/interfaces \
        android.hardware.wifi.supplicant@1.0::types.SupplicantStatus

$(GEN): $(LOCAL_PATH)/types.hal
	$(transform-generated-source)
LOCAL_GENERATED_SOURCES += $(GEN)

#
# Build types.hal (SupplicantStatusCode)
#
GEN := $(intermediates)/android/hardware/wifi/supplicant/1.0/SupplicantStatusCode.java
$(GEN): $(HIDL)
$(GEN): PRIVATE_HIDL := $(HIDL)
$(GEN): PRIVATE_DEPS := $(LOCAL_PATH)/types.hal
$(GEN): PRIVATE_OUTPUT_DIR := $(intermediates)
$(GEN): PRIVATE_CUSTOM_TOOL = \
        $(PRIVATE_HIDL) -o $(PRIVATE_OUTPUT_DIR) \
        -Ljava -randroid.hardware:hardware/interfaces \
        android.hardware.wifi.supplicant@1.0::types.SupplicantStatusCode

$(GEN): $(LOCAL_PATH)/types.hal
	$(transform-generated-source)
LOCAL_GENERATED_SOURCES += $(GEN)

#
# Build ISupplicant.hal
#
GEN := $(intermediates)/android/hardware/wifi/supplicant/1.0/ISupplicant.java
$(GEN): $(HIDL)
$(GEN): PRIVATE_HIDL := $(HIDL)
$(GEN): PRIVATE_DEPS := $(LOCAL_PATH)/ISupplicant.hal
$(GEN): PRIVATE_DEPS += $(LOCAL_PATH)/ISupplicantCallback.hal
$(GEN): $(LOCAL_PATH)/ISupplicantCallback.hal
$(GEN): PRIVATE_DEPS += $(LOCAL_PATH)/ISupplicantIface.hal
$(GEN): $(LOCAL_PATH)/ISupplicantIface.hal
$(GEN): PRIVATE_DEPS += $(LOCAL_PATH)/types.hal
$(GEN): $(LOCAL_PATH)/types.hal
$(GEN): PRIVATE_OUTPUT_DIR := $(intermediates)
$(GEN): PRIVATE_CUSTOM_TOOL = \
        $(PRIVATE_HIDL) -o $(PRIVATE_OUTPUT_DIR) \
        -Ljava -randroid.hardware:hardware/interfaces \
        android.hardware.wifi.supplicant@1.0::ISupplicant

$(GEN): $(LOCAL_PATH)/ISupplicant.hal
	$(transform-generated-source)
LOCAL_GENERATED_SOURCES += $(GEN)

#
# Build ISupplicantCallback.hal
#
GEN := $(intermediates)/android/hardware/wifi/supplicant/1.0/ISupplicantCallback.java
$(GEN): $(HIDL)
$(GEN): PRIVATE_HIDL := $(HIDL)
$(GEN): PRIVATE_DEPS := $(LOCAL_PATH)/ISupplicantCallback.hal
$(GEN): PRIVATE_OUTPUT_DIR := $(intermediates)
$(GEN): PRIVATE_CUSTOM_TOOL = \
        $(PRIVATE_HIDL) -o $(PRIVATE_OUTPUT_DIR) \
        -Ljava -randroid.hardware:hardware/interfaces \
        android.hardware.wifi.supplicant@1.0::ISupplicantCallback

$(GEN): $(LOCAL_PATH)/ISupplicantCallback.hal
	$(transform-generated-source)
LOCAL_GENERATED_SOURCES += $(GEN)

#
# Build ISupplicantIface.hal
#
GEN := $(intermediates)/android/hardware/wifi/supplicant/1.0/ISupplicantIface.java
$(GEN): $(HIDL)
$(GEN): PRIVATE_HIDL := $(HIDL)
$(GEN): PRIVATE_DEPS := $(LOCAL_PATH)/ISupplicantIface.hal
$(GEN): PRIVATE_DEPS += $(LOCAL_PATH)/ISupplicantIfaceCallback.hal
$(GEN): $(LOCAL_PATH)/ISupplicantIfaceCallback.hal
$(GEN): PRIVATE_DEPS += $(LOCAL_PATH)/ISupplicantNetwork.hal
$(GEN): $(LOCAL_PATH)/ISupplicantNetwork.hal
$(GEN): PRIVATE_DEPS += $(LOCAL_PATH)/types.hal
$(GEN): $(LOCAL_PATH)/types.hal
$(GEN): PRIVATE_OUTPUT_DIR := $(intermediates)
$(GEN): PRIVATE_CUSTOM_TOOL = \
        $(PRIVATE_HIDL) -o $(PRIVATE_OUTPUT_DIR) \
        -Ljava -randroid.hardware:hardware/interfaces \
        android.hardware.wifi.supplicant@1.0::ISupplicantIface

$(GEN): $(LOCAL_PATH)/ISupplicantIface.hal
	$(transform-generated-source)
LOCAL_GENERATED_SOURCES += $(GEN)

#
# Build ISupplicantIfaceCallback.hal
#
GEN := $(intermediates)/android/hardware/wifi/supplicant/1.0/ISupplicantIfaceCallback.java
$(GEN): $(HIDL)
$(GEN): PRIVATE_HIDL := $(HIDL)
$(GEN): PRIVATE_DEPS := $(LOCAL_PATH)/ISupplicantIfaceCallback.hal
$(GEN): PRIVATE_DEPS += $(LOCAL_PATH)/types.hal
$(GEN): $(LOCAL_PATH)/types.hal
$(GEN): PRIVATE_OUTPUT_DIR := $(intermediates)
$(GEN): PRIVATE_CUSTOM_TOOL = \
        $(PRIVATE_HIDL) -o $(PRIVATE_OUTPUT_DIR) \
        -Ljava -randroid.hardware:hardware/interfaces \
        android.hardware.wifi.supplicant@1.0::ISupplicantIfaceCallback

$(GEN): $(LOCAL_PATH)/ISupplicantIfaceCallback.hal
	$(transform-generated-source)
LOCAL_GENERATED_SOURCES += $(GEN)

#
# Build ISupplicantNetwork.hal
#
GEN := $(intermediates)/android/hardware/wifi/supplicant/1.0/ISupplicantNetwork.java
$(GEN): $(HIDL)
$(GEN): PRIVATE_HIDL := $(HIDL)
$(GEN): PRIVATE_DEPS := $(LOCAL_PATH)/ISupplicantNetwork.hal
$(GEN): PRIVATE_DEPS += $(LOCAL_PATH)/ISupplicantNetworkCallback.hal
$(GEN): $(LOCAL_PATH)/ISupplicantNetworkCallback.hal
$(GEN): PRIVATE_DEPS += $(LOCAL_PATH)/types.hal
$(GEN): $(LOCAL_PATH)/types.hal
$(GEN): PRIVATE_OUTPUT_DIR := $(intermediates)
$(GEN): PRIVATE_CUSTOM_TOOL = \
        $(PRIVATE_HIDL) -o $(PRIVATE_OUTPUT_DIR) \
        -Ljava -randroid.hardware:hardware/interfaces \
        android.hardware.wifi.supplicant@1.0::ISupplicantNetwork

$(GEN): $(LOCAL_PATH)/ISupplicantNetwork.hal
	$(transform-generated-source)
LOCAL_GENERATED_SOURCES += $(GEN)

#
# Build ISupplicantNetworkCallback.hal
#
GEN := $(intermediates)/android/hardware/wifi/supplicant/1.0/ISupplicantNetworkCallback.java
$(GEN): $(HIDL)
$(GEN): PRIVATE_HIDL := $(HIDL)
$(GEN): PRIVATE_DEPS := $(LOCAL_PATH)/ISupplicantNetworkCallback.hal
$(GEN): PRIVATE_OUTPUT_DIR := $(intermediates)
$(GEN): PRIVATE_CUSTOM_TOOL = \
        $(PRIVATE_HIDL) -o $(PRIVATE_OUTPUT_DIR) \
        -Ljava -randroid.hardware:hardware/interfaces \
        android.hardware.wifi.supplicant@1.0::ISupplicantNetworkCallback

$(GEN): $(LOCAL_PATH)/ISupplicantNetworkCallback.hal
	$(transform-generated-source)
LOCAL_GENERATED_SOURCES += $(GEN)
include $(BUILD_STATIC_JAVA_LIBRARY)



include $(call all-makefiles-under,$(LOCAL_PATH))
