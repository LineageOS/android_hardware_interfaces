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

vhal_v2_0 = android.hardware.automotive.vehicle@2.0
vhal_v2_1 = android.hardware.automotive.vehicle@2.1

###############################################################################
# Vehicle reference implementation lib
###############################################################################
include $(CLEAR_VARS)
LOCAL_MODULE := $(vhal_v2_1)-manager-lib
LOCAL_SRC_FILES := \
    common/src/Obd2SensorStore.cpp

LOCAL_C_INCLUDES := \
    $(LOCAL_PATH)/common/include/vhal_v2_1 \
    $(LOCAL_PATH)/../../2.0/default/common/include/vhal_v2_0 \

LOCAL_EXPORT_C_INCLUDE_DIRS := \
    $(LOCAL_PATH)/common/include

LOCAL_SHARED_LIBRARIES := \
    libhidlbase \
    libhidltransport \
    libhwbinder \
    liblog \
    libutils \
    $(vhal_v2_1) \

include $(BUILD_STATIC_LIBRARY)

###############################################################################
# Vehicle default VehicleHAL implementation
###############################################################################
include $(CLEAR_VARS)

LOCAL_MODULE:= $(vhal_v2_1)-default-impl-lib
LOCAL_SRC_FILES:= \
    impl/vhal_v2_1/EmulatedVehicleHal.cpp \

LOCAL_C_INCLUDES := \
    $(LOCAL_PATH)/impl/vhal_v2_1 \
    $(LOCAL_PATH)/common/include

LOCAL_EXPORT_C_INCLUDE_DIRS := \
    $(LOCAL_PATH)/impl \
    $(LOCAL_PATH)/common/include


# LOCAL_WHOLE_STATIC_LIBRARIES := \

LOCAL_STATIC_LIBRARIES := \
    $(vhal_v2_0)-default-impl-lib \
    $(vhal_v2_0)-manager-lib \
    libqemu_pipe \
    $(vhal_v2_1)-manager-lib \
    $(vhal_v2_0)-libproto-native

LOCAL_SHARED_LIBRARIES := \
    libbase \
    libhidlbase \
    libhidltransport \
    libhwbinder \
    liblog \
    libutils \
    libprotobuf-cpp-lite \
    $(vhal_v2_0) \
    $(vhal_v2_1) \

LOCAL_CFLAGS += -Wall -Wextra -Werror

include $(BUILD_STATIC_LIBRARY)

###############################################################################
# Vehicle HAL service
###############################################################################
include $(CLEAR_VARS)
LOCAL_MODULE := $(vhal_v2_1)-service
LOCAL_INIT_RC := $(vhal_v2_1)-service.rc
LOCAL_MODULE_RELATIVE_PATH := hw
LOCAL_PROPRIETARY_MODULE := true
LOCAL_SRC_FILES := \
    service.cpp

LOCAL_WHOLE_STATIC_LIBRARIES := \
    $(vhal_v2_0)-libproto-native \

LOCAL_STATIC_LIBRARIES := \
    $(vhal_v2_0)-manager-lib \
    $(vhal_v2_0)-default-impl-lib \
    $(vhal_v2_1)-default-impl-lib \
    libqemu_pipe \
    $(vhal_v2_1)-manager-lib \

LOCAL_SHARED_LIBRARIES := \
    libbase \
    libhidlbase \
    libhidltransport \
    libhwbinder \
    liblog \
    libutils \
    libprotobuf-cpp-lite \
    $(vhal_v2_0) \
    $(vhal_v2_1) \

LOCAL_CFLAGS += -Wall -Wextra -Werror

include $(BUILD_EXECUTABLE)
