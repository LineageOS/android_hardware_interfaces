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
#include <android-base/logging.h>
#include <hidl/HidlTransportSupport.h>

#include "BroadcastRadio.h"
#include "VirtualRadio.h"

using android::sp;
using android::hardware::configureRpcThreadpool;
using android::hardware::joinRpcThreadpool;
using android::hardware::broadcastradio::V2_0::implementation::BroadcastRadio;
using android::hardware::broadcastradio::V2_0::implementation::gAmFmRadio;
using android::hardware::broadcastradio::V2_0::implementation::gDabRadio;

int main() {
    android::base::SetDefaultTag("BcRadioDef");
    android::base::SetMinimumLogSeverity(android::base::VERBOSE);
    configureRpcThreadpool(4, true);

    sp<BroadcastRadio> broadcastRadio(new BroadcastRadio(gAmFmRadio));
    auto amFmStatus = broadcastRadio->registerAsService("amfm");
    CHECK_EQ(amFmStatus, android::OK)
        << "Failed to register Broadcast Radio AM/FM HAL implementation";

    sp<BroadcastRadio> dabRadio(new BroadcastRadio(gDabRadio));
    auto dabStatus = dabRadio->registerAsService("dab");
    CHECK_EQ(dabStatus, android::OK) << "Failed to register Broadcast Radio DAB HAL implementation";

    joinRpcThreadpool();
    return 1;  // joinRpcThreadpool shouldn't exit
}
