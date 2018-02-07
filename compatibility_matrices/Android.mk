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

# Clear potential input variables to BUILD_FRAMEWORK_COMPATIBILITY_MATRIX
LOCAL_ADD_VBMETA_VERSION :=
LOCAL_ASSEMBLE_VINTF_ENV_VARS :=
LOCAL_ASSEMBLE_VINTF_FLAGS :=
LOCAL_KERNEL_VERSIONS :=
LOCAL_GEN_FILE_DEPENDENCIES :=

# Install all compatibility_matrix.*.xml to /system/etc/vintf


include $(CLEAR_VARS)
LOCAL_MODULE_STEM := compatibility_matrix.legacy.xml
LOCAL_SRC_FILES := $(LOCAL_MODULE_STEM)
LOCAL_KERNEL_VERSIONS := 3.18.0 4.4.0 4.9.0
include $(BUILD_FRAMEWORK_COMPATIBILITY_MATRIX)

include $(CLEAR_VARS)
LOCAL_MODULE_STEM := compatibility_matrix.1.xml
LOCAL_SRC_FILES := $(LOCAL_MODULE_STEM)
LOCAL_KERNEL_VERSIONS := 3.18.0 4.4.0 4.9.0
include $(BUILD_FRAMEWORK_COMPATIBILITY_MATRIX)

include $(CLEAR_VARS)
LOCAL_MODULE_STEM := compatibility_matrix.2.xml
LOCAL_SRC_FILES := $(LOCAL_MODULE_STEM)
LOCAL_KERNEL_VERSIONS := 3.18.0 4.4.0 4.9.0
include $(BUILD_FRAMEWORK_COMPATIBILITY_MATRIX)

# TODO(b/72409164): STOPSHIP: update kernel version requirements

include $(CLEAR_VARS)
LOCAL_MODULE_STEM := compatibility_matrix.current.xml
LOCAL_SRC_FILES := $(LOCAL_MODULE_STEM)
LOCAL_KERNEL_VERSIONS := 4.4.0 4.9.0
include $(BUILD_FRAMEWORK_COMPATIBILITY_MATRIX)

# Framework Compatibility Matrix (common to all FCM versions)

include $(CLEAR_VARS)
LOCAL_MODULE_STEM := compatibility_matrix.empty.xml
LOCAL_SRC_FILES := $(LOCAL_MODULE_STEM)
LOCAL_ADD_VBMETA_VERSION := true
LOCAL_ASSEMBLE_VINTF_ENV_VARS := \
    POLICYVERS \
    PLATFORM_SEPOLICY_VERSION \
    PLATFORM_SEPOLICY_COMPAT_VERSIONS

include $(BUILD_FRAMEWORK_COMPATIBILITY_MATRIX)

# Framework Compatibility Matrix

include $(CLEAR_VARS)
LOCAL_MODULE := framework_compatibility_matrix.xml
LOCAL_MODULE_STEM := compatibility_matrix.xml
LOCAL_MODULE_PATH := $(TARGET_OUT)
LOCAL_REQUIRED_MODULES := \
    framework_compatibility_matrix.legacy.xml \
    framework_compatibility_matrix.1.xml \
    framework_compatibility_matrix.2.xml \
    framework_compatibility_matrix.current.xml \
    framework_compatibility_matrix.empty.xml
LOCAL_GENERATED_SOURCES := $(call module-installed-files,$(LOCAL_REQUIRED_MODULES))

ifdef BUILT_VENDOR_MANIFEST
LOCAL_GEN_FILE_DEPENDENCIES += $(BUILT_VENDOR_MANIFEST)
LOCAL_ASSEMBLE_VINTF_FLAGS += -c "$(BUILT_VENDOR_MANIFEST)"
endif

LOCAL_ASSEMBLE_VINTF_ENV_VARS := PRODUCT_ENFORCE_VINTF_MANIFEST

include $(BUILD_FRAMEWORK_COMPATIBILITY_MATRIX)
BUILT_SYSTEM_COMPATIBILITY_MATRIX := $(LOCAL_BUILT_MODULE)

BUILD_FRAMEWORK_COMPATIBILITY_MATRIX :=
