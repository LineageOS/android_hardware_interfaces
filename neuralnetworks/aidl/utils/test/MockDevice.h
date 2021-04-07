/*
 * Copyright (C) 2021 The Android Open Source Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#ifndef ANDROID_HARDWARE_INTERFACES_NEURALNETWORKS_AIDL_UTILS_TEST_MOCK_DEVICE_H
#define ANDROID_HARDWARE_INTERFACES_NEURALNETWORKS_AIDL_UTILS_TEST_MOCK_DEVICE_H

#include <aidl/android/hardware/neuralnetworks/BnDevice.h>
#include <android/binder_auto_utils.h>
#include <android/binder_interface_utils.h>
#include <gmock/gmock.h>
#include <gtest/gtest.h>

namespace aidl::android::hardware::neuralnetworks::utils {

class MockDevice final : public BnDevice {
  public:
    static std::shared_ptr<MockDevice> create();

    MOCK_METHOD(ndk::ScopedAStatus, allocate,
                (const BufferDesc& desc, const std::vector<IPreparedModelParcel>& preparedModels,
                 const std::vector<BufferRole>& inputRoles,
                 const std::vector<BufferRole>& outputRoles, DeviceBuffer* deviceBuffer),
                (override));
    MOCK_METHOD(ndk::ScopedAStatus, getCapabilities, (Capabilities * capabilities), (override));
    MOCK_METHOD(ndk::ScopedAStatus, getNumberOfCacheFilesNeeded,
                (NumberOfCacheFiles * numberOfCacheFiles), (override));
    MOCK_METHOD(ndk::ScopedAStatus, getSupportedExtensions, (std::vector<Extension> * extensions),
                (override));
    MOCK_METHOD(ndk::ScopedAStatus, getSupportedOperations,
                (const Model& model, std::vector<bool>* supportedOperations), (override));
    MOCK_METHOD(ndk::ScopedAStatus, getType, (DeviceType * deviceType), (override));
    MOCK_METHOD(ndk::ScopedAStatus, getVersionString, (std::string * version), (override));
    MOCK_METHOD(ndk::ScopedAStatus, prepareModel,
                (const Model& model, ExecutionPreference preference, Priority priority,
                 int64_t deadline, const std::vector<ndk::ScopedFileDescriptor>& modelCache,
                 const std::vector<ndk::ScopedFileDescriptor>& dataCache,
                 const std::vector<uint8_t>& token,
                 const std::shared_ptr<IPreparedModelCallback>& callback),
                (override));
    MOCK_METHOD(ndk::ScopedAStatus, prepareModelFromCache,
                (int64_t deadline, const std::vector<ndk::ScopedFileDescriptor>& modelCache,
                 const std::vector<ndk::ScopedFileDescriptor>& dataCache,
                 const std::vector<uint8_t>& token,
                 const std::shared_ptr<IPreparedModelCallback>& callback),
                (override));
};

inline std::shared_ptr<MockDevice> MockDevice::create() {
    return ndk::SharedRefBase::make<MockDevice>();
}

}  // namespace aidl::android::hardware::neuralnetworks::utils

#endif  // ANDROID_HARDWARE_INTERFACES_NEURALNETWORKS_AIDL_UTILS_TEST_MOCK_DEVICE_H
