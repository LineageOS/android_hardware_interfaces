/*
 * Copyright (C) 2020 The Android Open Source Project
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

#ifndef ANDROID_HARDWARE_INTERFACES_NEURALNETWORKS_UTILS_COMMON_TEST_MOCK_DEVICE_H
#define ANDROID_HARDWARE_INTERFACES_NEURALNETWORKS_UTILS_COMMON_TEST_MOCK_DEVICE_H

#include <gmock/gmock.h>
#include <gtest/gtest.h>
#include <nnapi/IDevice.h>

namespace android::nn {

class MockDevice final : public IDevice {
  public:
    MOCK_METHOD(const std::string&, getName, (), (const, override));
    MOCK_METHOD(const std::string&, getVersionString, (), (const, override));
    MOCK_METHOD(Version, getFeatureLevel, (), (const, override));
    MOCK_METHOD(DeviceType, getType, (), (const, override));
    MOCK_METHOD(const std::vector<Extension>&, getSupportedExtensions, (), (const, override));
    MOCK_METHOD(const Capabilities&, getCapabilities, (), (const, override));
    MOCK_METHOD((std::pair<uint32_t, uint32_t>), getNumberOfCacheFilesNeeded, (),
                (const, override));
    MOCK_METHOD(GeneralResult<void>, wait, (), (const, override));
    MOCK_METHOD(GeneralResult<std::vector<bool>>, getSupportedOperations, (const Model& model),
                (const, override));
    MOCK_METHOD(GeneralResult<SharedPreparedModel>, prepareModel,
                (const Model& model, ExecutionPreference preference, Priority priority,
                 OptionalTimePoint deadline, const std::vector<SharedHandle>& modelCache,
                 const std::vector<SharedHandle>& dataCache, const CacheToken& token),
                (const, override));
    MOCK_METHOD(GeneralResult<SharedPreparedModel>, prepareModelFromCache,
                (OptionalTimePoint deadline, const std::vector<SharedHandle>& modelCache,
                 const std::vector<SharedHandle>& dataCache, const CacheToken& token),
                (const, override));
    MOCK_METHOD(GeneralResult<SharedBuffer>, allocate,
                (const BufferDesc& desc, const std::vector<SharedPreparedModel>& preparedModels,
                 const std::vector<BufferRole>& inputRoles,
                 const std::vector<BufferRole>& outputRoles),
                (const, override));
};

}  // namespace android::nn

#endif  // ANDROID_HARDWARE_INTERFACES_NEURALNETWORKS_UTILS_COMMON_TEST_MOCK_DEVICE_H
