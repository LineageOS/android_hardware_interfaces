#
# Copyright (C) 2018 The Android Open Source Project
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


LOCAL_PATH := $(call my-dir)

include $(CLEAR_VARS)
LOCAL_MODULE := android.hardware.soundtrigger@2.1-impl
LOCAL_VENDOR_MODULE := true
LOCAL_MODULE_RELATIVE_PATH := hw
LOCAL_SRC_FILES := \
    SoundTriggerHw.cpp

LOCAL_CFLAGS := -Wall -Werror

LOCAL_SHARED_LIBRARIES := \
        libhardware \
        libhidlbase \
        libhidlmemory \
        libhidltransport \
        liblog \
        libutils \
        android.hardware.soundtrigger@2.1 \
        android.hardware.soundtrigger@2.0 \
        android.hardware.soundtrigger@2.0-core \
        android.hidl.allocator@1.0 \
        android.hidl.memory@1.0

LOCAL_C_INCLUDE_DIRS := $(LOCAL_PATH)

ifeq ($(strip $(AUDIOSERVER_MULTILIB)),)
LOCAL_MULTILIB := 32
else
LOCAL_MULTILIB := $(AUDIOSERVER_MULTILIB)
endif

include $(BUILD_SHARED_LIBRARY)
