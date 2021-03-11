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

#ifndef ANDROID_HARDWARE_INTERFACES_NEURALNETWORKS_1_0_UTILS_TEST_MOCK_PREPARED_MODEL_H
#define ANDROID_HARDWARE_INTERFACES_NEURALNETWORKS_1_0_UTILS_TEST_MOCK_PREPARED_MODEL_H

#include <android/hardware/neuralnetworks/1.0/IPreparedModel.h>
#include <gmock/gmock.h>
#include <gtest/gtest.h>
#include <hidl/HidlSupport.h>
#include <hidl/Status.h>

namespace android::hardware::neuralnetworks::V1_0::utils {

class MockPreparedModel final : public IPreparedModel {
  public:
    static sp<MockPreparedModel> create();

    // IBase methods below.
    MOCK_METHOD(Return<void>, ping, (), (override));
    MOCK_METHOD(Return<bool>, linkToDeathRet, ());
    Return<bool> linkToDeath(const sp<hidl_death_recipient>& recipient,
                             uint64_t /*cookie*/) override;

    // V1_0 methods below.
    MOCK_METHOD(Return<V1_0::ErrorStatus>, execute,
                (const V1_0::Request& request, const sp<V1_0::IExecutionCallback>& callback),
                (override));

    // Helper methods.
    void simulateCrash();

  private:
    sp<hidl_death_recipient> mDeathRecipient;
};

inline sp<MockPreparedModel> MockPreparedModel::create() {
    auto mockPreparedModel = sp<MockPreparedModel>::make();

    // Setup default actions for each relevant call.
    const auto ret = []() -> Return<bool> { return true; };

    // Setup default actions for each relevant call.
    ON_CALL(*mockPreparedModel, linkToDeathRet()).WillByDefault(testing::Invoke(ret));

    // These EXPECT_CALL(...).Times(testing::AnyNumber()) calls are to suppress warnings on the
    // uninteresting methods calls.
    EXPECT_CALL(*mockPreparedModel, linkToDeathRet()).Times(testing::AnyNumber());

    return mockPreparedModel;
}

inline Return<bool> MockPreparedModel::linkToDeath(const sp<hidl_death_recipient>& recipient,
                                                   uint64_t /*cookie*/) {
    mDeathRecipient = recipient;
    return linkToDeathRet();
}

inline void MockPreparedModel::simulateCrash() {
    ASSERT_NE(nullptr, mDeathRecipient.get());

    // Currently, the utils::PreparedModel will not use the `cookie` or `who` arguments, so we pass
    // in 0 and nullptr for these arguments instead. Normally, they are used by the
    // hidl_death_recipient to determine which object is dead. However, the utils::PreparedModel
    // code only pairs a single death recipient with a single HIDL interface object, so these
    // arguments are redundant.
    mDeathRecipient->serviceDied(0, nullptr);
}

}  // namespace android::hardware::neuralnetworks::V1_0::utils

#endif  // ANDROID_HARDWARE_INTERFACES_NEURALNETWORKS_1_0_UTILS_TEST_MOCK_PREPARED_MODEL_H
