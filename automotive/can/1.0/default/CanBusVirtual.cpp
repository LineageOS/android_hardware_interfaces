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

#include "CanBusVirtual.h"

#include <android-base/logging.h>
#include <libnetdevice/libnetdevice.h>

namespace android::hardware::automotive::can::V1_0::implementation {

CanBusVirtual::CanBusVirtual(const std::string& ifname) : CanBus(ifname) {}

ICanController::Result CanBusVirtual::preUp() {
    if (netdevice::exists(mIfname)) return ICanController::Result::OK;

    LOG(DEBUG) << "Virtual interface " << mIfname << " doesn't exist, creating...";
    mWasCreated = true;
    if (!netdevice::add(mIfname, "vcan")) {
        LOG(ERROR) << "Can't create vcan interface " << mIfname;
        return ICanController::Result::UNKNOWN_ERROR;
    }

    return ICanController::Result::OK;
}

bool CanBusVirtual::postDown() {
    if (mWasCreated) {
        mWasCreated = false;
        if (!netdevice::del(mIfname)) {
            LOG(ERROR) << "Couldn't remove vcan interface " << mIfname;
            return false;
        }
    }
    return true;
}

}  // namespace android::hardware::automotive::can::V1_0::implementation
