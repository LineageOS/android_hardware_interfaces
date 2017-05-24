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

LOCAL_PATH := $(call my-dir)

include $(CLEAR_VARS)

LOCAL_CLANG := true
LOCAL_MODULE := VtsHalAudioV2_0TargetTest
LOCAL_CPPFLAGS := -O0 -g -Wall -Wextra -Werror
LOCAL_SRC_FILES := \
    AudioPrimaryHidlHalTest.cpp \
    ValidateAudioConfiguration.cpp \
    utility/ValidateXml.cpp

LOCAL_C_INCLUDES := external/libxml2/include

LOCAL_STATIC_LIBRARIES := VtsHalHidlTargetTestBase
LOCAL_SHARED_LIBRARIES := \
    libbase \
    liblog \
    libhidlbase \
    libhidltransport \
    libutils \
    libcutils \
    libxml2 \
    libicuuc \
    android.hardware.audio@2.0 \
    android.hardware.audio.common@2.0 \

include $(BUILD_NATIVE_TEST)
