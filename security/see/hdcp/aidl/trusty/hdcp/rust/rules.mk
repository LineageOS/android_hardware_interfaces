# Copyright (C) 2024 The Android Open Source Project
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

LOCAL_DIR := $(GET_LOCAL_DIR)

MODULE := $(LOCAL_DIR)

AIDL_DIR := hardware/interfaces/security/see/hdcp/aidl
DRM_AIDL_DIR := hardware/interfaces/drm/aidl

MODULE_AIDL_FLAGS := \
	--mockall \
	--version=1 \

MODULE_CRATE_NAME := android_hardware_security_see_hdcp

MODULE_AIDL_LANGUAGE := rust

MODULE_AIDL_PACKAGE := android/hardware/security/see/hdcp

MODULE_AIDL_INCLUDES := \
	-I $(AIDL_DIR) \
	-I $(DRM_AIDL_DIR) \

MODULE_AIDLS := \
    $(AIDL_DIR)/$(MODULE_AIDL_PACKAGE)/IHdcpAuthControl.aidl   \

MODULE_AIDL_RUST_DEPS := \
	android_hardware_drm

MODULE_LIBRARY_DEPS := \
	hardware/interfaces/security/see/hdcp/aidl/trusty/drm/rust \
	$(call FIND_CRATE,mockall) \

include make/aidl.mk
