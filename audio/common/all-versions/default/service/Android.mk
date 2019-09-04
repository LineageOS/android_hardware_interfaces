#
# Copyright (C) 2016 The Android Open Source Project
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

#
# Service
#

include $(CLEAR_VARS)
LOCAL_MODULE := android.hardware.audio@2.0-service
LOCAL_INIT_RC := android.hardware.audio@2.0-service.rc
LOCAL_MODULE_RELATIVE_PATH := hw
LOCAL_PROPRIETARY_MODULE := true
LOCAL_SRC_FILES := \
    service.cpp

LOCAL_CFLAGS := -Wall -Werror

LOCAL_SHARED_LIBRARIES := \
    libcutils \
    libbinder \
    libhidlbase \
    libhidltransport \
    liblog \
    libutils \
    libhardware \
    libhwbinder \
    android.hardware.audio@2.0 \
    android.hardware.audio@4.0 \
    android.hardware.audio@5.0 \
    android.hardware.audio.common@2.0 \
    android.hardware.audio.common@4.0 \
    android.hardware.audio.common@5.0 \
    android.hardware.audio.effect@2.0 \
    android.hardware.audio.effect@4.0 \
    android.hardware.audio.effect@5.0 \
    android.hardware.bluetooth.a2dp@1.0 \
    android.hardware.bluetooth.audio@2.0 \
    android.hardware.soundtrigger@2.0 \
    android.hardware.soundtrigger@2.1 \
    android.hardware.soundtrigger@2.2

# Can not switch to Android.bp until AUDIOSERVER_MULTILIB
# is deprecated as build config variable are not supported
ifeq ($(strip $(AUDIOSERVER_MULTILIB)),)
LOCAL_MULTILIB := 32
else
LOCAL_MULTILIB := $(AUDIOSERVER_MULTILIB)
endif

include $(BUILD_EXECUTABLE)
