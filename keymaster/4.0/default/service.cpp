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

#include <android-base/logging.h>
#include <android/hardware/keymaster/4.0/IKeymasterDevice.h>
#include <hidl/HidlTransportSupport.h>

#include <AndroidKeymaster4Device.h>

using android::hardware::keymaster::V4_0::SecurityLevel;

int main() {
    ::android::hardware::configureRpcThreadpool(1, true /* willJoinThreadpool */);
    auto keymaster = ::keymaster::V4_0::ng::CreateKeymasterDevice(SecurityLevel::SOFTWARE);
    auto status = keymaster->registerAsService();
    if (status != android::OK) {
        LOG(FATAL) << "Could not register service for Keymaster 4.0 (" << status << ")";
    }

    android::hardware::joinRpcThreadpool();
    return -1;  // Should never get here.
}
