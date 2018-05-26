/*
**
** Copyright 2017, The Android Open Source Project
**
** Licensed under the Apache License, Version 2.0 (the "License");
** you may not use this file except in compliance with the License.
** You may obtain a copy of the License at
**
**     http://www.apache.org/licenses/LICENSE-2.0
**
** Unless required by applicable law or agreed to in writing, software
** distributed under the License is distributed on an "AS IS" BASIS,
** WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
** See the License for the specific language governing permissions and
** limitations under the License.
*/

#define LOG_TAG "android.hardware.confirmationui@1.0-service"

#include <android-base/logging.h>
#include <hidl/HidlTransportSupport.h>

#include "ConfirmationUI.h"

using android::hardware::joinRpcThreadpool;

using android::hardware::confirmationui::V1_0::implementation::ConfirmationUI;

int main() {
    ::android::hardware::configureRpcThreadpool(1, true /*willJoinThreadpool*/);
    auto confirmationui = new ConfirmationUI();
    auto status = confirmationui->registerAsService();
    if (status != android::OK) {
        LOG(FATAL) << "Could not register service for ConfirmationIU 1.0 (" << status << ")";
    }

    joinRpcThreadpool();
    return -1;  // Should never get here.
}
