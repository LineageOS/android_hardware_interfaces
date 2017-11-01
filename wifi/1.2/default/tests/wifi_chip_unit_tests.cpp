/*
 * Copyright (C) 2017, The Android Open Source Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include <android-base/logging.h>
#include <android-base/macros.h>
#include <gmock/gmock.h>

#undef NAN  // This is weird, NAN is defined in bionic/libc/include/math.h:38
#include "wifi_chip.h"

#include "mock_wifi_feature_flags.h"
#include "mock_wifi_legacy_hal.h"
#include "mock_wifi_mode_controller.h"

using testing::NiceMock;
using testing::Return;
using testing::Test;

namespace {}  // namespace

namespace android {
namespace hardware {
namespace wifi {
namespace V1_2 {
namespace implementation {

class WifiChipTest : public Test {
   protected:
    void setupV1IfaceCombination() {
        EXPECT_CALL(*feature_flags_, isAwareSupported())
            .WillRepeatedly(testing::Return(false));
    }

    void setupV2IfaceCombination() {
        EXPECT_CALL(*feature_flags_, isAwareSupported())
            .WillRepeatedly(testing::Return(true));
    }

    sp<WifiChip> chip_;

    ChipId chip_id_ = 5;
    std::shared_ptr<NiceMock<legacy_hal::MockWifiLegacyHal>> legacy_hal_{
        new NiceMock<legacy_hal::MockWifiLegacyHal>};
    std::shared_ptr<NiceMock<mode_controller::MockWifiModeController>>
        mode_controller_{new NiceMock<mode_controller::MockWifiModeController>};
    std::shared_ptr<NiceMock<feature_flags::MockWifiFeatureFlags>>
        feature_flags_{new NiceMock<feature_flags::MockWifiFeatureFlags>};

   public:
    void SetUp() override {
        chip_ = new WifiChip(chip_id_, legacy_hal_, mode_controller_,
                             feature_flags_);
    }
};

}  // namespace implementation
}  // namespace V1_2
}  // namespace wifi
}  // namespace hardware
}  // namespace android
