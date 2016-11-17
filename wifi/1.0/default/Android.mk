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
LOCAL_MODULE := android.hardware.wifi@1.0-impl
LOCAL_MODULE_RELATIVE_PATH := hw
LOCAL_CPPFLAGS := -std=c++11 -Wall -Wno-unused-parameter -Werror -Wextra
LOCAL_SRC_FILES := \
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
    libhidl \
    libhwbinder \
    liblog \
    libnl \
    libutils \
    libwifi-system
LOCAL_WHOLE_STATIC_LIBRARIES := $(LIB_WIFI_HAL)
include $(BUILD_SHARED_LIBRARY)

include $(CLEAR_VARS)
LOCAL_MODULE := android.hardware.wifi@1.0-service
LOCAL_MODULE_RELATIVE_PATH := hw
LOCAL_CPPFLAGS := -std=c++11 -Wall -Wno-unused-parameter -Werror -Wextra
LOCAL_SRC_FILES := \
    service.cpp
LOCAL_SHARED_LIBRARIES := \
    android.hardware.wifi@1.0 \
    android.hardware.wifi@1.0-impl \
    libbase \
    libcutils \
    libhidl \
    libhwbinder \
    liblog \
    libnl \
    libutils \
    libwifi-system
LOCAL_WHOLE_STATIC_LIBRARIES := $(LIB_WIFI_HAL)
LOCAL_INIT_RC := android.hardware.wifi@1.0-service.rc
include $(BUILD_EXECUTABLE)
