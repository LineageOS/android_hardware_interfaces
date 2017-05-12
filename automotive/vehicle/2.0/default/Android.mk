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

###############################################################################
# Vehicle reference implementation lib
###############################################################################
include $(CLEAR_VARS)
LOCAL_MODULE := $(vhal_v2_0)-manager-lib
LOCAL_SRC_FILES := \
    common/src/SubscriptionManager.cpp \
    common/src/VehicleHalManager.cpp \
    common/src/VehicleObjectPool.cpp \
    common/src/VehiclePropertyStore.cpp \
    common/src/VehicleUtils.cpp \

LOCAL_C_INCLUDES := \
    $(LOCAL_PATH)/common/include/vhal_v2_0

LOCAL_EXPORT_C_INCLUDE_DIRS := \
    $(LOCAL_PATH)/common/include

LOCAL_SHARED_LIBRARIES := \
    libhidlbase \
    libhidltransport \
    liblog \
    libutils \
    $(vhal_v2_0) \

include $(BUILD_STATIC_LIBRARY)


include $(CLEAR_VARS)
LOCAL_MODULE := $(vhal_v2_0)-manager-lib-shared
LOCAL_SRC_FILES := \
    common/src/SubscriptionManager.cpp \
    common/src/VehicleHalManager.cpp \
    common/src/VehicleObjectPool.cpp \
    common/src/VehiclePropertyStore.cpp \
    common/src/VehicleUtils.cpp \

LOCAL_C_INCLUDES := \
    $(LOCAL_PATH)/common/include/vhal_v2_0

LOCAL_EXPORT_C_INCLUDE_DIRS := \
    $(LOCAL_PATH)/common/include

LOCAL_SHARED_LIBRARIES := \
    libhidlbase \
    libhidltransport \
    liblog \
    libutils \
    $(vhal_v2_0) \

include $(BUILD_SHARED_LIBRARY)

###############################################################################
# Vehicle HAL Protobuf library
###############################################################################
include $(CLEAR_VARS)
LOCAL_SRC_FILES := $(call all-proto-files-under, impl/vhal_v2_0/proto)

LOCAL_PROTOC_OPTIMIZE_TYPE := nano

LOCAL_MODULE := $(vhal_v2_0)-libproto-native
LOCAL_MODULE_CLASS := STATIC_LIBRARIES

LOCAL_MODULE_TAGS := optional

LOCAL_STRIP_MODULE := keep_symbols

generated_sources_dir := $(call local-generated-sources-dir)
LOCAL_EXPORT_C_INCLUDE_DIRS := \
    $(generated_sources_dir)/proto/$(LOCAL_PATH)/impl/vhal_v2_0/proto

include $(BUILD_STATIC_LIBRARY)


###############################################################################
# Vehicle default VehicleHAL implementation
###############################################################################
include $(CLEAR_VARS)

LOCAL_MODULE:= $(vhal_v2_0)-default-impl-lib
LOCAL_SRC_FILES:= \
    impl/vhal_v2_0/EmulatedVehicleHal.cpp \
    impl/vhal_v2_0/VehicleEmulator.cpp \
    impl/vhal_v2_0/PipeComm.cpp \
    impl/vhal_v2_0/SocketComm.cpp \

LOCAL_C_INCLUDES := \
    $(LOCAL_PATH)/impl/vhal_v2_0

LOCAL_EXPORT_C_INCLUDE_DIRS := \
    $(LOCAL_PATH)/impl

LOCAL_WHOLE_STATIC_LIBRARIES := \
    $(vhal_v2_0)-manager-lib \

LOCAL_SHARED_LIBRARIES := \
    libbase \
    libhidlbase \
    libhidltransport \
    liblog \
    libprotobuf-cpp-lite \
    libutils \
    $(vhal_v2_0) \

LOCAL_STATIC_LIBRARIES := \
    libqemu_pipe \
    $(vhal_v2_0)-libproto-native \

LOCAL_CFLAGS += -Wall -Wextra -Werror

include $(BUILD_STATIC_LIBRARY)


###############################################################################
# Vehicle reference implementation unit tests
###############################################################################
include $(CLEAR_VARS)

LOCAL_MODULE:= $(vhal_v2_0)-manager-unit-tests

LOCAL_WHOLE_STATIC_LIBRARIES := \
    $(vhal_v2_0)-manager-lib \

LOCAL_SRC_FILES:= \
    tests/RecurrentTimer_test.cpp \
    tests/SubscriptionManager_test.cpp \
    tests/VehicleHalManager_test.cpp \
    tests/VehicleObjectPool_test.cpp \
    tests/VehiclePropConfigIndex_test.cpp \

LOCAL_HEADER_LIBRARIES := \
    libbase_headers

LOCAL_SHARED_LIBRARIES := \
    libhidlbase \
    libhidltransport \
    liblog \
    libutils \
    $(vhal_v2_0) \

LOCAL_CFLAGS += -Wall -Wextra -Werror
LOCAL_MODULE_TAGS := tests

include $(BUILD_NATIVE_TEST)


###############################################################################
# Vehicle HAL service
###############################################################################
include $(CLEAR_VARS)
LOCAL_MODULE := $(vhal_v2_0)-service
LOCAL_INIT_RC := $(vhal_v2_0)-service.rc
LOCAL_PROPRIETARY_MODULE := true
LOCAL_MODULE_RELATIVE_PATH := hw

LOCAL_SRC_FILES := \
    VehicleService.cpp

LOCAL_SHARED_LIBRARIES := \
    libbase \
    libhidlbase \
    libhidltransport \
    liblog \
    libprotobuf-cpp-lite \
    libutils \
    $(vhal_v2_0) \

LOCAL_STATIC_LIBRARIES := \
    $(vhal_v2_0)-manager-lib \
    $(vhal_v2_0)-default-impl-lib \
    $(vhal_v2_0)-libproto-native \
    libqemu_pipe \

LOCAL_CFLAGS += -Wall -Wextra -Werror

include $(BUILD_EXECUTABLE)
