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

include $(BUILD_FRAMEWORK_COMPATIBILITY_MATRIX)

my_system_matrix_deps := \
    framework_compatibility_matrix.legacy.xml \
    framework_compatibility_matrix.1.xml \
    framework_compatibility_matrix.2.xml \
    framework_compatibility_matrix.3.xml \
    framework_compatibility_matrix.current.xml \
    framework_compatibility_matrix.device.xml

# Phony target that installs all framework compatibility matrix files
include $(CLEAR_VARS)
LOCAL_MODULE := framework_compatibility_matrix.xml
LOCAL_REQUIRED_MODULES := $(my_system_matrix_deps)
include $(BUILD_PHONY_PACKAGE)

# Final Framework Compatibility Matrix for OTA
include $(CLEAR_VARS)
include $(LOCAL_PATH)/clear_vars.mk
LOCAL_MODULE := verified_assembled_system_matrix.xml
LOCAL_MODULE_PATH := $(PRODUCT_OUT)
LOCAL_REQUIRED_MODULES := $(my_system_matrix_deps)
LOCAL_GENERATED_SOURCES := $(call module-installed-files,$(LOCAL_REQUIRED_MODULES))
LOCAL_ADD_VBMETA_VERSION_OVERRIDE := true

ifdef BUILT_VENDOR_MANIFEST
LOCAL_GEN_FILE_DEPENDENCIES += $(BUILT_VENDOR_MANIFEST)
LOCAL_ASSEMBLE_VINTF_FLAGS += -c "$(BUILT_VENDOR_MANIFEST)"
endif

ifneq ($(PRODUCT_OTA_ENFORCE_VINTF_KERNEL_REQUIREMENTS),true)
LOCAL_ASSEMBLE_VINTF_FLAGS += --no-kernel-requirements
endif

include $(BUILD_FRAMEWORK_COMPATIBILITY_MATRIX)
BUILT_SYSTEM_MATRIX := $(LOCAL_BUILT_MODULE)

my_system_matrix_deps :=
BUILD_FRAMEWORK_COMPATIBILITY_MATRIX :=
