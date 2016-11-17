#
# Copyright (C) 2016 The Android Open Source Project
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#       http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.
#

LOCAL_PATH := $(call my-dir)

# build VTS driver for Light v2.0.
include $(CLEAR_VARS)

LOCAL_MODULE := libvts_driver_hidl_light@2.0

LOCAL_SRC_FILES := \
    Light.vts \
    types.vts \

LOCAL_SHARED_LIBRARIES += \
    android.hardware.light@2.0 \
    libbase \
    libutils \
    libcutils \
    liblog \
    libhidl \
    libhwbinder \
    libprotobuf-cpp-full \
    libvts_common \
    libvts_datatype \
    libvts_measurement \
    libvts_multidevice_proto

LOCAL_PROTOC_OPTIMIZE_TYPE := full

LOCAL_MULTILIB := both

include $(BUILD_SHARED_LIBRARY)
