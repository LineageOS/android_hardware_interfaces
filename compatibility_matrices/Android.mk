#
# Copyright (C) 2017 The Android Open Source Project
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#      http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.
#

LOCAL_PATH := $(call my-dir)

BUILD_FRAMEWORK_COMPATIBILITY_MATRIX := $(LOCAL_PATH)/compatibility_matrix.mk

my_kernel_config_data := kernel/configs

# Install all compatibility_matrix.*.xml to /system/etc/vintf

include $(CLEAR_VARS)
include $(LOCAL_PATH)/clear_vars.mk
LOCAL_MODULE := framework_compatibility_matrix.legacy.xml
LOCAL_MODULE_STEM := compatibility_matrix.legacy.xml
LOCAL_SRC_FILES := $(LOCAL_MODULE_STEM)
LOCAL_KERNEL_CONFIG_DATA_PATHS := \
    3.18.0:$(my_kernel_config_data)/o/android-3.18 \
    4.4.0:$(my_kernel_config_data)/o/android-4.4 \
    4.9.0:$(my_kernel_config_data)/o/android-4.9 \

include $(BUILD_FRAMEWORK_COMPATIBILITY_MATRIX)

include $(CLEAR_VARS)
include $(LOCAL_PATH)/clear_vars.mk
LOCAL_MODULE := framework_compatibility_matrix.1.xml
LOCAL_MODULE_STEM := compatibility_matrix.1.xml
LOCAL_SRC_FILES := $(LOCAL_MODULE_STEM)
LOCAL_KERNEL_CONFIG_DATA_PATHS := \
    3.18.0:$(my_kernel_config_data)/o/android-3.18 \
    4.4.0:$(my_kernel_config_data)/o/android-4.4 \
    4.9.0:$(my_kernel_config_data)/o/android-4.9 \

include $(BUILD_FRAMEWORK_COMPATIBILITY_MATRIX)

include $(CLEAR_VARS)
include $(LOCAL_PATH)/clear_vars.mk
LOCAL_MODULE := framework_compatibility_matrix.2.xml
LOCAL_MODULE_STEM := compatibility_matrix.2.xml
LOCAL_SRC_FILES := $(LOCAL_MODULE_STEM)
LOCAL_KERNEL_CONFIG_DATA_PATHS := \
    3.18.0:$(my_kernel_config_data)/o-mr1/android-3.18 \
    4.4.0:$(my_kernel_config_data)/o-mr1/android-4.4 \
    4.9.0:$(my_kernel_config_data)/o-mr1/android-4.9 \

include $(BUILD_FRAMEWORK_COMPATIBILITY_MATRIX)

include $(CLEAR_VARS)
include $(LOCAL_PATH)/clear_vars.mk
LOCAL_MODULE := framework_compatibility_matrix.3.xml
LOCAL_MODULE_STEM := compatibility_matrix.3.xml
LOCAL_SRC_FILES := $(LOCAL_MODULE_STEM)
LOCAL_KERNEL_CONFIG_DATA_PATHS := \
    4.4.107:$(my_kernel_config_data)/p/android-4.4 \
    4.9.84:$(my_kernel_config_data)/p/android-4.9 \
    4.14.42:$(my_kernel_config_data)/p/android-4.14 \

include $(BUILD_FRAMEWORK_COMPATIBILITY_MATRIX)

my_kernel_config_data :=

# Framework Compatibility Matrix (common to all FCM versions)

include $(CLEAR_VARS)
include $(LOCAL_PATH)/clear_vars.mk
LOCAL_MODULE := framework_compatibility_matrix.device.xml
LOCAL_MODULE_STEM := compatibility_matrix.device.xml
# define LOCAL_MODULE_CLASS for local-generated-sources-dir.
LOCAL_MODULE_CLASS := ETC

ifndef DEVICE_FRAMEWORK_COMPATIBILITY_MATRIX_FILE
LOCAL_SRC_FILES := compatibility_matrix.empty.xml
else

# DEVICE_FRAMEWORK_COMPATIBILITY_MATRIX_FILE specify an absolute path
LOCAL_GENERATED_SOURCES := $(DEVICE_FRAMEWORK_COMPATIBILITY_MATRIX_FILE)

# Enforce that DEVICE_FRAMEWORK_COMPATIBILITY_MATRIX_FILE does not specify required HALs
# by checking it against an empty manifest. But the empty manifest needs to contain
# BOARD_SEPOLICY_VERS to be compatible with DEVICE_FRAMEWORK_COMPATIBILITY_MATRIX_FILE.
my_manifest_src_file := $(LOCAL_PATH)/manifest.empty.xml
my_gen_check_manifest := $(local-generated-sources-dir)/manifest.check.xml
$(my_gen_check_manifest): PRIVATE_SRC_FILE := $(my_manifest_src_file)
$(my_gen_check_manifest): $(my_manifest_src_file) $(HOST_OUT_EXECUTABLES)/assemble_vintf
	BOARD_SEPOLICY_VERS=$(BOARD_SEPOLICY_VERS) \
	VINTF_IGNORE_TARGET_FCM_VERSION=true \
		$(HOST_OUT_EXECUTABLES)/assemble_vintf -i $(PRIVATE_SRC_FILE) -o $@

LOCAL_GEN_FILE_DEPENDENCIES += $(my_gen_check_manifest)
LOCAL_ASSEMBLE_VINTF_FLAGS += -c "$(my_gen_check_manifest)"

my_gen_check_manifest :=
my_manifest_src_file :=

endif # DEVICE_FRAMEWORK_COMPATIBILITY_MATRIX_FILE

LOCAL_ADD_VBMETA_VERSION := true
LOCAL_ASSEMBLE_VINTF_ENV_VARS := \
    POLICYVERS \
    PLATFORM_SEPOLICY_VERSION \
    PLATFORM_SEPOLICY_COMPAT_VERSIONS

LOCAL_ASSEMBLE_VINTF_ENV_VARS_OVERRIDE := PRODUCT_ENFORCE_VINTF_MANIFEST=true
LOCAL_ASSEMBLE_VINTF_ERROR_MESSAGE := \
    "Error: DEVICE_FRAMEWORK_COMPATIBILITY_MATRIX cannot contain required HALs."

include $(BUILD_FRAMEWORK_COMPATIBILITY_MATRIX)

# Framework Compatibility Matrix

include $(CLEAR_VARS)
include $(LOCAL_PATH)/clear_vars.mk
LOCAL_MODULE := framework_compatibility_matrix.xml
LOCAL_MODULE_STEM := compatibility_matrix.xml
LOCAL_MODULE_PATH := $(TARGET_OUT)
LOCAL_REQUIRED_MODULES := \
    framework_compatibility_matrix.legacy.xml \
    framework_compatibility_matrix.1.xml \
    framework_compatibility_matrix.2.xml \
    framework_compatibility_matrix.3.xml \
    framework_compatibility_matrix.device.xml
LOCAL_GENERATED_SOURCES := $(call module-installed-files,$(LOCAL_REQUIRED_MODULES))

ifdef BUILT_VENDOR_MANIFEST
LOCAL_GEN_FILE_DEPENDENCIES += $(BUILT_VENDOR_MANIFEST)
LOCAL_ASSEMBLE_VINTF_FLAGS += -c "$(BUILT_VENDOR_MANIFEST)"
endif

LOCAL_ASSEMBLE_VINTF_ENV_VARS := PRODUCT_ENFORCE_VINTF_MANIFEST

# TODO(b/65028233): Enforce no "unused HALs" for devices that does not define
# DEVICE_FRAMEWORK_COMPATIBILITY_MATRIX_FILE as well
ifeq (true,$(strip $(PRODUCT_ENFORCE_VINTF_MANIFEST)))
ifdef DEVICE_FRAMEWORK_COMPATIBILITY_MATRIX_FILE
LOCAL_ASSEMBLE_VINTF_ENV_VARS_OVERRIDE := VINTF_ENFORCE_NO_UNUSED_HALS=true
endif
endif

include $(BUILD_FRAMEWORK_COMPATIBILITY_MATRIX)
BUILT_SYSTEM_COMPATIBILITY_MATRIX := $(LOCAL_BUILT_MODULE)

BUILD_FRAMEWORK_COMPATIBILITY_MATRIX :=
