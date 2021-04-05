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

#ifndef ANDROID_HARDWARE_INTERFACES_NEURALNETWORKS_1_1_UTILS_TEST_MOCK_DEVICE_H
#define ANDROID_HARDWARE_INTERFACES_NEURALNETWORKS_1_1_UTILS_TEST_MOCK_DEVICE_H

#include <android/hardware/neuralnetworks/1.1/IDevice.h>
#include <gmock/gmock.h>
#include <gtest/gtest.h>
#include <hidl/Status.h>

namespace android::hardware::neuralnetworks::V1_1::utils {

class MockDevice final : public IDevice {
  public:
    static sp<MockDevice> create();

    // IBase methods below.
    MOCK_METHOD(Return<void>, ping, (), (override));
    MOCK_METHOD(Return<bool>, linkToDeathRet, ());
    Return<bool> linkToDeath(const sp<hidl_death_recipient>& recipient, uint64_t /*cookie*/);

    // V1_0 methods below.
    MOCK_METHOD(Return<void>, getCapabilities, (getCapabilities_cb cb), (override));
    MOCK_METHOD(Return<void>, getSupportedOperations,
                (const V1_0::Model& model, getSupportedOperations_cb cb), (override));
    MOCK_METHOD(Return<V1_0::ErrorStatus>, prepareModel,
                (const V1_0::Model& model, const sp<V1_0::IPreparedModelCallback>& callback),
                (override));
    MOCK_METHOD(Return<V1_0::DeviceStatus>, getStatus, (), (override));

    // V1_1 methods below.
    MOCK_METHOD(Return<void>, getCapabilities_1_1, (getCapabilities_1_1_cb cb), (override));
    MOCK_METHOD(Return<void>, getSupportedOperations_1_1,
                (const V1_1::Model& model, getSupportedOperations_1_1_cb cb), (override));
    MOCK_METHOD(Return<V1_0::ErrorStatus>, prepareModel_1_1,
                (const V1_1::Model& model, V1_1::ExecutionPreference preference,
                 const sp<V1_0::IPreparedModelCallback>& callback),
                (override));

    // Helper methods.
    void simulateCrash();

  private:
    sp<hidl_death_recipient> mDeathRecipient;
};

inline sp<MockDevice> MockDevice::create() {
    auto mockDevice = sp<MockDevice>::make();

    // Setup default actions for each relevant call.
    const auto ret = []() -> Return<bool> { return true; };

    // Setup default actions for each relevant call.
    ON_CALL(*mockDevice, linkToDeathRet()).WillByDefault(testing::Invoke(ret));

    // These EXPECT_CALL(...).Times(testing::AnyNumber()) calls are to suppress warnings on the
    // uninteresting methods calls.
    EXPECT_CALL(*mockDevice, linkToDeathRet()).Times(testing::AnyNumber());

    return mockDevice;
}

inline Return<bool> MockDevice::linkToDeath(const sp<hidl_death_recipient>& recipient,
                                            uint64_t /*cookie*/) {
    mDeathRecipient = recipient;
    return linkToDeathRet();
}

inline void MockDevice::simulateCrash() {
    ASSERT_NE(nullptr, mDeathRecipient.get());

    // Currently, the utils::Device will not use the `cookie` or `who` arguments, so we pass in 0
    // and nullptr for these arguments instead. Normally, they are used by the hidl_death_recipient
    // to determine which object is dead. However, the utils::Device code only pairs a single death
    // recipient with a single HIDL interface object, so these arguments are redundant.
    mDeathRecipient->serviceDied(0, nullptr);
}

}  // namespace android::hardware::neuralnetworks::V1_1::utils

#endif  // ANDROID_HARDWARE_INTERFACES_NEURALNETWORKS_1_1_UTILS_TEST_MOCK_DEVICE_H
