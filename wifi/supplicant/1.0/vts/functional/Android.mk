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
#
LOCAL_PATH := $(call my-dir)

include $(CLEAR_VARS)
LOCAL_MODULE := VtsHalWifiSupplicantV1_0TargetTest
LOCAL_CPPFLAGS := -Wall -Werror -Wextra
LOCAL_SRC_FILES := \
    VtsHalWifiSupplicantV1_0TargetTest.cpp \
    supplicant_hidl_test.cpp \
    supplicant_hidl_test_utils.cpp \
    supplicant_p2p_iface_hidl_test.cpp \
    supplicant_sta_iface_hidl_test.cpp \
    supplicant_sta_network_hidl_test.cpp
LOCAL_SHARED_LIBRARIES := \
    android.hardware.wifi.supplicant@1.0 \
    android.hardware.wifi@1.0 \
    libbase \
    libcutils \
    libhidlbase \
    libhidltransport \
    liblog \
    libutils \
    libwifi-system \
    libwifi-system-iface
LOCAL_STATIC_LIBRARIES := \
    libgmock \
    VtsHalHidlTargetTestBase \
    VtsHalWifiV1_0TargetTestUtil
include $(BUILD_NATIVE_TEST)

