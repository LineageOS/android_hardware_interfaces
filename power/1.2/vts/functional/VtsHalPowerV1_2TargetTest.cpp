/*
 * Copyright (C) 2017 The Android Open Source Project
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

#define LOG_TAG "power_hidl_hal_test"
#include <android-base/logging.h>
#include <android/hardware/power/1.2/IPower.h>

#include <VtsHalHidlTargetTestBase.h>
#include <VtsHalHidlTargetTestEnvBase.h>

using ::android::sp;
using ::android::hardware::hidl_vec;
using ::android::hardware::Return;
using ::android::hardware::power::V1_2::IPower;
using ::android::hardware::power::V1_2::PowerHint;

// Test environment for Power HIDL HAL.
class PowerHidlEnvironment : public ::testing::VtsHalHidlTargetTestEnvBase {
   public:
    // get the test environment singleton
    static PowerHidlEnvironment* Instance() {
        static PowerHidlEnvironment* instance = new PowerHidlEnvironment;
        return instance;
    }

    virtual void registerTestServices() override { registerTestService<IPower>(); }
};

class PowerHidlTest : public ::testing::VtsHalHidlTargetTestBase {
   public:
    virtual void SetUp() override {
        power = ::testing::VtsHalHidlTargetTestBase::getService<IPower>(
            PowerHidlEnvironment::Instance()->getServiceName<IPower>());
        ASSERT_NE(power, nullptr);
    }

    sp<IPower> power;
};

// Sanity check Power::PowerHintAsync_1_2 on good and bad inputs.
TEST_F(PowerHidlTest, PowerHintAsync_1_2) {
    std::vector<PowerHint> hints;
    for (uint32_t i = static_cast<uint32_t>(PowerHint::VSYNC);
         i <= static_cast<uint32_t>(PowerHint::CAMERA_SHOT); ++i) {
        hints.emplace_back(static_cast<PowerHint>(i));
    }
    PowerHint badHint = static_cast<PowerHint>(0xFF);
    hints.emplace_back(badHint);

    Return<void> ret;
    for (auto& hint : hints) {
        ret = power->powerHintAsync_1_2(hint, 30000);
        ASSERT_TRUE(ret.isOk());

        ret = power->powerHintAsync_1_2(hint, 0);
        ASSERT_TRUE(ret.isOk());
    }

    // Turning these hints on in different orders triggers different code paths,
    // so iterate over possible orderings.
    std::vector<PowerHint> hints2 = {PowerHint::AUDIO_STREAMING, PowerHint::CAMERA_LAUNCH,
                                     PowerHint::CAMERA_STREAMING, PowerHint::CAMERA_SHOT};
    auto compareHints = [](PowerHint l, PowerHint r) {
        return static_cast<uint32_t>(l) < static_cast<uint32_t>(r);
    };
    std::sort(hints2.begin(), hints2.end(), compareHints);
    do {
        for (auto iter = hints2.begin(); iter != hints2.end(); iter++) {
            ret = power->powerHintAsync_1_2(*iter, 0);
            ASSERT_TRUE(ret.isOk());
        }
        for (auto iter = hints2.begin(); iter != hints2.end(); iter++) {
            ret = power->powerHintAsync_1_2(*iter, 30000);
            ASSERT_TRUE(ret.isOk());
        }
    } while (std::next_permutation(hints2.begin(), hints2.end(), compareHints));
}

int main(int argc, char** argv) {
    ::testing::AddGlobalTestEnvironment(PowerHidlEnvironment::Instance());
    ::testing::InitGoogleTest(&argc, argv);
    PowerHidlEnvironment::Instance()->init(&argc, argv);
    int status = RUN_ALL_TESTS();
    LOG(INFO) << "Test result = " << status;
    return status;
}
