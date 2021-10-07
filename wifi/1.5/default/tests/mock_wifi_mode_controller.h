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

#ifndef MOCK_WIFI_MODE_CONTROLLER_H_
#define MOCK_WIFI_MODE_CONTROLLER_H_

#include <gmock/gmock.h>

#include "wifi_mode_controller.h"

namespace android {
namespace hardware {
namespace wifi {
namespace V1_5 {
namespace implementation {
namespace mode_controller {

class MockWifiModeController : public WifiModeController {
   public:
    MockWifiModeController();
    MOCK_METHOD0(initialize, bool());
    MOCK_METHOD1(changeFirmwareMode, bool(IfaceType));
    MOCK_METHOD1(isFirmwareModeChangeNeeded, bool(IfaceType));
    MOCK_METHOD0(deinitialize, bool());
};
}  // namespace mode_controller
}  // namespace implementation
}  // namespace V1_5
}  // namespace wifi
}  // namespace hardware
}  // namespace android

#endif  // MOCK_WIFI_MODE_CONTROLLER_H_
