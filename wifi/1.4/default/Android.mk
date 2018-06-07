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

###
### android.hardware.wifi static library
###
include $(CLEAR_VARS)
LOCAL_MODULE := android.hardware.wifi@1.0-service-lib
LOCAL_MODULE_RELATIVE_PATH := hw
LOCAL_PROPRIETARY_MODULE := true
LOCAL_CPPFLAGS := -Wall -Werror -Wextra
ifdef WIFI_HAL_INTERFACE_COMBINATIONS
LOCAL_CPPFLAGS += -DWIFI_HAL_INTERFACE_COMBINATIONS="$(WIFI_HAL_INTERFACE_COMBINATIONS)"
endif
ifdef WIFI_HIDL_FEATURE_AWARE
LOCAL_CPPFLAGS += -DWIFI_HIDL_FEATURE_AWARE
endif
ifdef WIFI_HIDL_FEATURE_DUAL_INTERFACE
LOCAL_CPPFLAGS += -DWIFI_HIDL_FEATURE_DUAL_INTERFACE
endif
ifdef WIFI_HIDL_FEATURE_DISABLE_AP
LOCAL_CPPFLAGS += -DWIFI_HIDL_FEATURE_DISABLE_AP
endif
ifdef WIFI_HIDL_FEATURE_DISABLE_AP_MAC_RANDOMIZATION
LOCAL_CPPFLAGS += -DWIFI_HIDL_FEATURE_DISABLE_AP_MAC_RANDOMIZATION
endif
ifdef WIFI_AVOID_IFACE_RESET_MAC_CHANGE
LOCAL_CPPFLAGS += -DWIFI_AVOID_IFACE_RESET_MAC_CHANGE
endif
ifdef QC_WIFI_HIDL_FEATURE_DUAL_AP
LOCAL_CPPFLAGS += -DQC_WIFI_HIDL_FEATURE_DUAL_AP
endif
# Allow implicit fallthroughs in wifi_legacy_hal.cpp until they are fixed.
LOCAL_CFLAGS += -Wno-error=implicit-fallthrough
LOCAL_SRC_FILES := \
    hidl_struct_util.cpp \
    hidl_sync_util.cpp \
    ringbuffer.cpp \
    wifi.cpp \
    wifi_ap_iface.cpp \
    wifi_chip.cpp \
    wifi_feature_flags.cpp \
    wifi_iface_util.cpp \
    wifi_legacy_hal.cpp \
    wifi_legacy_hal_stubs.cpp \
    wifi_mode_controller.cpp \
    wifi_nan_iface.cpp \
    wifi_p2p_iface.cpp \
    wifi_rtt_controller.cpp \
    wifi_sta_iface.cpp \
    wifi_status_util.cpp
LOCAL_SHARED_LIBRARIES := \
    libbase \
    libcutils \
    libhidlbase \
    liblog \
    libnl \
    libutils \
    libwifi-hal \
    libwifi-system-iface \
    android.hardware.wifi@1.0 \
    android.hardware.wifi@1.1 \
    android.hardware.wifi@1.2 \
    android.hardware.wifi@1.3 \
    android.hardware.wifi@1.4
LOCAL_EXPORT_C_INCLUDE_DIRS := $(LOCAL_PATH)
include $(BUILD_STATIC_LIBRARY)

###
### android.hardware.wifi daemon
###
include $(CLEAR_VARS)
LOCAL_MODULE := android.hardware.wifi@1.0-service
LOCAL_VINTF_FRAGMENTS := android.hardware.wifi@1.0-service.xml
LOCAL_MODULE_RELATIVE_PATH := hw
LOCAL_PROPRIETARY_MODULE := true
LOCAL_CPPFLAGS := -Wall -Werror -Wextra
LOCAL_SRC_FILES := \
    service.cpp
LOCAL_SHARED_LIBRARIES := \
    libbase \
    libcutils \
    libhidlbase \
    liblog \
    libnl \
    libutils \
    libwifi-hal \
    libwifi-system-iface \
    android.hardware.wifi@1.0 \
    android.hardware.wifi@1.1 \
    android.hardware.wifi@1.2 \
    android.hardware.wifi@1.3 \
    android.hardware.wifi@1.4
LOCAL_STATIC_LIBRARIES := \
    android.hardware.wifi@1.0-service-lib
LOCAL_INIT_RC := android.hardware.wifi@1.0-service.rc
include $(BUILD_EXECUTABLE)

###
### android.hardware.wifi daemon
###
include $(CLEAR_VARS)
LOCAL_MODULE := android.hardware.wifi@1.0-service-lazy
LOCAL_VINTF_FRAGMENTS := android.hardware.wifi@1.0-service.xml
LOCAL_OVERRIDES_MODULES := android.hardware.wifi@1.0-service
LOCAL_CFLAGS := -DLAZY_SERVICE
LOCAL_MODULE_RELATIVE_PATH := hw
LOCAL_PROPRIETARY_MODULE := true
LOCAL_CPPFLAGS := -Wall -Werror -Wextra
LOCAL_SRC_FILES := \
    service.cpp
LOCAL_SHARED_LIBRARIES := \
    libbase \
    libcutils \
    libhidlbase \
    liblog \
    libnl \
    libutils \
    libwifi-hal \
    libwifi-system-iface \
    android.hardware.wifi@1.0 \
    android.hardware.wifi@1.1 \
    android.hardware.wifi@1.2 \
    android.hardware.wifi@1.3 \
    android.hardware.wifi@1.4
LOCAL_STATIC_LIBRARIES := \
    android.hardware.wifi@1.0-service-lib
LOCAL_INIT_RC := android.hardware.wifi@1.0-service-lazy.rc
include $(BUILD_EXECUTABLE)

###
### android.hardware.wifi unit tests.
###
include $(CLEAR_VARS)
LOCAL_MODULE := android.hardware.wifi@1.0-service-tests
LOCAL_PROPRIETARY_MODULE := true
LOCAL_CPPFLAGS := -Wall -Werror -Wextra
LOCAL_SRC_FILES := \
    tests/hidl_struct_util_unit_tests.cpp \
    tests/main.cpp \
    tests/mock_interface_tool.cpp \
    tests/mock_wifi_feature_flags.cpp \
    tests/mock_wifi_iface_util.cpp \
    tests/mock_wifi_legacy_hal.cpp \
    tests/mock_wifi_mode_controller.cpp \
    tests/ringbuffer_unit_tests.cpp \
    tests/wifi_nan_iface_unit_tests.cpp \
    tests/wifi_chip_unit_tests.cpp \
    tests/wifi_iface_util_unit_tests.cpp
LOCAL_STATIC_LIBRARIES := \
    libgmock \
    libgtest \
    android.hardware.wifi@1.0 \
    android.hardware.wifi@1.1 \
    android.hardware.wifi@1.2 \
    android.hardware.wifi@1.3 \
    android.hardware.wifi@1.4 \
    android.hardware.wifi@1.0-service-lib
LOCAL_SHARED_LIBRARIES := \
    libbase \
    libcutils \
    libhidlbase \
    liblog \
    libnl \
    libutils \
    libwifi-hal \
    libwifi-system-iface
include $(BUILD_NATIVE_TEST)
