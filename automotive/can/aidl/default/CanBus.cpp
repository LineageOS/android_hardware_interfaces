/*
 * Copyright 2022, The Android Open Source Project
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

#include "CanBus.h"

#include <android-base/logging.h>
#include <libnetdevice/libnetdevice.h>

namespace aidl::android::hardware::automotive::can {

CanBus::CanBus(std::string_view ifname) : mIfname(ifname) {}

CanBus::~CanBus() {
    std::lock_guard<std::mutex> lck(mIsUpGuard);
    CHECK(!mIsUp) << "Interface is still up while being destroyed";
}

Result CanBus::preUp() {
    return Result::OK;
}

bool CanBus::postDown() {
    return true;
}

std::string CanBus::getIfaceName() {
    return mIfname;
}

Result CanBus::up() {
    std::lock_guard<std::mutex> lck(mIsUpGuard);

    if (mIsUp) {
        LOG(WARNING) << "Interface is already up";
        return Result::INVALID_STATE;
    }

    const auto preResult = preUp();
    if (preResult != Result::OK) return preResult;

    const auto isUp = ::android::netdevice::isUp(mIfname);
    if (!isUp.has_value()) {
        // preUp() should prepare the interface (either create or make sure it's there)
        LOG(ERROR) << "Interface " << mIfname << " didn't get prepared";
        return Result::BAD_INTERFACE_ID;
    }

    if (!*isUp && !::android::netdevice::up(mIfname)) {
        LOG(ERROR) << "Can't bring " << mIfname << " up";
        return Result::UNKNOWN_ERROR;
    }
    mDownAfterUse = !*isUp;

    mIsUp = true;
    return Result::OK;
}

Result CanBus::down() {
    std::lock_guard<std::mutex> lck(mIsUpGuard);

    if (!mIsUp) {
        LOG(WARNING) << "Interface is already down";
        return Result::INVALID_STATE;
    }
    mIsUp = false;

    Result success = Result::OK;

    if (mDownAfterUse && !::android::netdevice::down(mIfname)) {
        LOG(ERROR) << "Can't bring " << mIfname << " down";
        // don't return yet, let's try to do best-effort cleanup
        success = Result::UNKNOWN_ERROR;
    }

    if (!postDown()) success = Result::UNKNOWN_ERROR;

    return success;
}

}  // namespace aidl::android::hardware::automotive::can
