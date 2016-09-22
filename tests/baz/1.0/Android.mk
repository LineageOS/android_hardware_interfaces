LOCAL_PATH := $(call my-dir)
include $(CLEAR_VARS)

LOCAL_MODULE := android.hardware.tests.baz@1.0
LOCAL_MODULE_CLASS := SHARED_LIBRARIES

intermediates := $(local-generated-sources-dir)

HIDL := $(HOST_OUT_EXECUTABLES)/hidl-gen$(HOST_EXECUTABLE_SUFFIX)

#
# Build types.hal
#
GEN := $(intermediates)/android/hardware/tests/baz/1.0/types.cpp
$(GEN): $(HIDL)
$(GEN): PRIVATE_HIDL := $(HIDL)
$(GEN): PRIVATE_DEPS := $(LOCAL_PATH)/types.hal
$(GEN): PRIVATE_OUTPUT_DIR := $(intermediates)
$(GEN): PRIVATE_CUSTOM_TOOL = \
    $(PRIVATE_HIDL) -o $(PRIVATE_OUTPUT_DIR) \
    -Lc++ -randroid.hardware:hardware/interfaces \
    android.hardware.tests.baz@1.0::types

$(GEN): $(LOCAL_PATH)/types.hal
	$(transform-generated-source)
LOCAL_GENERATED_SOURCES += $(GEN)

#
# Build IBase.hal
#
GEN := $(intermediates)/android/hardware/tests/baz/1.0/BaseAll.cpp
$(GEN): $(HIDL)
$(GEN): PRIVATE_HIDL := $(HIDL)
$(GEN): PRIVATE_DEPS := $(LOCAL_PATH)/IBase.hal
$(GEN): PRIVATE_OUTPUT_DIR := $(intermediates)
$(GEN): PRIVATE_CUSTOM_TOOL = \
    $(PRIVATE_HIDL) -o $(PRIVATE_OUTPUT_DIR) \
    -Lc++ -randroid.hardware:hardware/interfaces \
    android.hardware.tests.baz@1.0::IBase

$(GEN): $(LOCAL_PATH)/IBase.hal
	$(transform-generated-source)
LOCAL_GENERATED_SOURCES += $(GEN)

#
# Build IBaz.hal
#
GEN := $(intermediates)/android/hardware/tests/baz/1.0/BazAll.cpp
$(GEN): $(HIDL)
$(GEN): PRIVATE_HIDL := $(HIDL)
$(GEN): PRIVATE_DEPS := $(LOCAL_PATH)/IBaz.hal
$(GEN): PRIVATE_DEPS += $(LOCAL_PATH)/IBase.hal
$(GEN): $(LOCAL_PATH)/IBase.hal
$(GEN): PRIVATE_DEPS += $(LOCAL_PATH)/IBazCallback.hal
$(GEN): $(LOCAL_PATH)/IBazCallback.hal
$(GEN): PRIVATE_OUTPUT_DIR := $(intermediates)
$(GEN): PRIVATE_CUSTOM_TOOL = \
    $(PRIVATE_HIDL) -o $(PRIVATE_OUTPUT_DIR) \
    -Lc++ -randroid.hardware:hardware/interfaces \
    android.hardware.tests.baz@1.0::IBaz

$(GEN): $(LOCAL_PATH)/IBaz.hal
	$(transform-generated-source)
LOCAL_GENERATED_SOURCES += $(GEN)

#
# Build IBazCallback.hal
#
GEN := $(intermediates)/android/hardware/tests/baz/1.0/BazCallbackAll.cpp
$(GEN): $(HIDL)
$(GEN): PRIVATE_HIDL := $(HIDL)
$(GEN): PRIVATE_DEPS := $(LOCAL_PATH)/IBazCallback.hal
$(GEN): PRIVATE_OUTPUT_DIR := $(intermediates)
$(GEN): PRIVATE_CUSTOM_TOOL = \
    $(PRIVATE_HIDL) -o $(PRIVATE_OUTPUT_DIR) \
    -Lc++ -randroid.hardware:hardware/interfaces \
    android.hardware.tests.baz@1.0::IBazCallback

$(GEN): $(LOCAL_PATH)/IBazCallback.hal
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
LOCAL_MODULE := android.hardware.tests.baz@1.0-java
LOCAL_MODULE_CLASS := JAVA_LIBRARIES

intermediates := $(local-generated-sources-dir)

HIDL := $(HOST_OUT_EXECUTABLES)/hidl-gen$(HOST_EXECUTABLE_SUFFIX)

#
# Build IBase.hal
#
GEN := $(intermediates)/android/hardware/tests/baz/1.0/IBase.java
$(GEN): $(HIDL)
$(GEN): PRIVATE_HIDL := $(HIDL)
$(GEN): PRIVATE_DEPS := $(LOCAL_PATH)/IBase.hal
$(GEN): PRIVATE_OUTPUT_DIR := $(intermediates)
$(GEN): PRIVATE_CUSTOM_TOOL = \
    $(PRIVATE_HIDL) -o $(PRIVATE_OUTPUT_DIR) \
    -Ljava -randroid.hardware:hardware/interfaces \
    android.hardware.tests.baz@1.0::IBase

$(GEN): $(LOCAL_PATH)/IBase.hal
	$(transform-generated-source)
LOCAL_GENERATED_SOURCES += $(GEN)

#
# Build IBaz.hal
#
GEN := $(intermediates)/android/hardware/tests/baz/1.0/IBaz.java
$(GEN): $(HIDL)
$(GEN): PRIVATE_HIDL := $(HIDL)
$(GEN): PRIVATE_DEPS := $(LOCAL_PATH)/IBaz.hal
$(GEN): PRIVATE_DEPS += $(LOCAL_PATH)/IBase.hal
$(GEN): $(LOCAL_PATH)/IBase.hal
$(GEN): PRIVATE_DEPS += $(LOCAL_PATH)/IBazCallback.hal
$(GEN): $(LOCAL_PATH)/IBazCallback.hal
$(GEN): PRIVATE_OUTPUT_DIR := $(intermediates)
$(GEN): PRIVATE_CUSTOM_TOOL = \
    $(PRIVATE_HIDL) -o $(PRIVATE_OUTPUT_DIR) \
    -Ljava -randroid.hardware:hardware/interfaces \
    android.hardware.tests.baz@1.0::IBaz

$(GEN): $(LOCAL_PATH)/IBaz.hal
	$(transform-generated-source)
LOCAL_GENERATED_SOURCES += $(GEN)

#
# Build IBazCallback.hal
#
GEN := $(intermediates)/android/hardware/tests/baz/1.0/IBazCallback.java
$(GEN): $(HIDL)
$(GEN): PRIVATE_HIDL := $(HIDL)
$(GEN): PRIVATE_DEPS := $(LOCAL_PATH)/IBazCallback.hal
$(GEN): PRIVATE_OUTPUT_DIR := $(intermediates)
$(GEN): PRIVATE_CUSTOM_TOOL = \
    $(PRIVATE_HIDL) -o $(PRIVATE_OUTPUT_DIR) \
    -Ljava -randroid.hardware:hardware/interfaces \
    android.hardware.tests.baz@1.0::IBazCallback

$(GEN): $(LOCAL_PATH)/IBazCallback.hal
	$(transform-generated-source)
LOCAL_GENERATED_SOURCES += $(GEN)
include $(BUILD_JAVA_LIBRARY)
