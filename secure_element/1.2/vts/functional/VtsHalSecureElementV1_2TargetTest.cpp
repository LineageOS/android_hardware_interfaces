/*
 * Copyright (C) 2019 The Android Open Source Project
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

#include <string>

#define LOG_TAG "secure_element_hidl_hal_test"
#include <android-base/logging.h>

#include <android/hardware/secure_element/1.0/types.h>
#include <android/hardware/secure_element/1.1/ISecureElementHalCallback.h>
#include <android/hardware/secure_element/1.2/ISecureElement.h>
#include <gtest/gtest.h>
#include <hidl/GtestPrinter.h>
#include <hidl/ServiceManagement.h>

#include <VtsHalHidlTargetCallbackBase.h>

using ::android::sp;
using ::android::hardware::hidl_string;
using ::android::hardware::Return;
using ::android::hardware::Void;
using ::android::hardware::secure_element::V1_0::SecureElementStatus;
using ::android::hardware::secure_element::V1_1::ISecureElementHalCallback;
using ::android::hardware::secure_element::V1_2::ISecureElement;

constexpr char kCallbackNameOnStateChange[] = "onStateChange";

class SecureElementCallbackArgs {
  public:
    bool state_;
    hidl_string reason_;
};

class SecureElementHalCallback
    : public ::testing::VtsHalHidlTargetCallbackBase<SecureElementCallbackArgs>,
      public ISecureElementHalCallback {
  public:
    virtual ~SecureElementHalCallback() = default;

    Return<void> onStateChange_1_1(bool state, const hidl_string& reason) override {
        SecureElementCallbackArgs args;
        args.state_ = state;
        args.reason_ = reason;
        NotifyFromCallback(kCallbackNameOnStateChange, args);
        return Void();
    };

    Return<void> onStateChange(__attribute__((unused)) bool state) override { return Void(); }
};

class SecureElementHidlTest : public ::testing::TestWithParam<std::string> {
  public:
    virtual void SetUp() override {
        LOG(INFO) << "get service with name:" << GetParam();
        se_ = ISecureElement::getService(GetParam());
        ASSERT_NE(se_, nullptr);

        se_cb_ = new SecureElementHalCallback();
        ASSERT_NE(se_cb_, nullptr);
        se_->init_1_1(se_cb_);
        auto res = se_cb_->WaitForCallback(kCallbackNameOnStateChange);
        EXPECT_TRUE(res.no_timeout);
        EXPECT_TRUE(res.args->state_);
        EXPECT_NE(res.args->reason_, "");
    }

    sp<ISecureElement> se_;
    sp<SecureElementHalCallback> se_cb_;
};

/*
 * Reset:
 * Calls reset()
 * Checks status
 * Check onStateChange is received with connected state set to true
 */
TEST_P(SecureElementHidlTest, Reset) {
    EXPECT_EQ(SecureElementStatus::SUCCESS, se_->reset());

    auto res = se_cb_->WaitForCallback(kCallbackNameOnStateChange);
    EXPECT_TRUE(res.no_timeout);
    EXPECT_TRUE(res.args->state_);
}

INSTANTIATE_TEST_SUITE_P(
        PerInstance, SecureElementHidlTest,
        testing::ValuesIn(android::hardware::getAllHalInstanceNames(ISecureElement::descriptor)),
        android::hardware::PrintInstanceNameToString);
