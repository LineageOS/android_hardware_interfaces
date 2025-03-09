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

AIDL_DIR := hardware/interfaces/drm/aidl

MODULE_AIDL_FLAGS := \
	--stability=vintf \
	--version=1 \

MODULE_CRATE_NAME := android_hardware_drm

MODULE_AIDL_LANGUAGE := rust

MODULE_AIDL_PACKAGE := android/hardware/drm

MODULE_AIDL_INCLUDES := \
	-I $(AIDL_DIR) \

MODULE_AIDLS := \
    $(AIDL_DIR)/$(MODULE_AIDL_PACKAGE)/HdcpLevel.aidl   \
    $(AIDL_DIR)/$(MODULE_AIDL_PACKAGE)/HdcpLevels.aidl   \

include make/aidl.mk
