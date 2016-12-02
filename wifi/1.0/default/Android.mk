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

include $(CLEAR_VARS)
LOCAL_MODULE := android.hardware.wifi@1.0-service
LOCAL_MODULE_RELATIVE_PATH := hw
LOCAL_CPPFLAGS := -Wall -Wno-unused-parameter -Werror -Wextra
LOCAL_SRC_FILES := \
    hidl_struct_util.cpp \
    service.cpp \
    wifi.cpp \
    wifi_ap_iface.cpp \
    wifi_chip.cpp \
    wifi_legacy_hal.cpp \
    wifi_nan_iface.cpp \
    wifi_p2p_iface.cpp \
    wifi_rtt_controller.cpp \
    wifi_sta_iface.cpp \
    wifi_status_util.cpp
LOCAL_SHARED_LIBRARIES := \
    android.hardware.wifi@1.0 \
    libbase \
    libcutils \
    libhidlbase \
    libhidltransport \
    libhwbinder \
    liblog \
    libnl \
    libutils \
    libwifi-system
LOCAL_WHOLE_STATIC_LIBRARIES := $(LIB_WIFI_HAL)
LOCAL_INIT_RC := android.hardware.wifi@1.0-service.rc
include $(BUILD_EXECUTABLE)
