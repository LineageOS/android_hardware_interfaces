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

#include "Power.h"
#include "PowerHintSession.h"

#include <android-base/logging.h>

namespace aidl {
namespace android {
namespace hardware {
namespace power {
namespace impl {
namespace example {

using namespace std::chrono_literals;

using ndk::ScopedAStatus;

const std::vector<Boost> BOOST_RANGE{ndk::enum_range<Boost>().begin(),
                                     ndk::enum_range<Boost>().end()};
const std::vector<Mode> MODE_RANGE{ndk::enum_range<Mode>().begin(), ndk::enum_range<Mode>().end()};

ScopedAStatus Power::setMode(Mode type, bool enabled) {
    LOG(VERBOSE) << "Power setMode: " << static_cast<int32_t>(type) << " to: " << enabled;
    return ScopedAStatus::ok();
}

ScopedAStatus Power::isModeSupported(Mode type, bool* _aidl_return) {
    LOG(INFO) << "Power isModeSupported: " << static_cast<int32_t>(type);
    *_aidl_return = type >= MODE_RANGE.front() && type <= MODE_RANGE.back();
    return ScopedAStatus::ok();
}

ScopedAStatus Power::setBoost(Boost type, int32_t durationMs) {
    LOG(VERBOSE) << "Power setBoost: " << static_cast<int32_t>(type)
                 << ", duration: " << durationMs;
    return ScopedAStatus::ok();
}

ScopedAStatus Power::isBoostSupported(Boost type, bool* _aidl_return) {
    LOG(INFO) << "Power isBoostSupported: " << static_cast<int32_t>(type);
    *_aidl_return = type >= BOOST_RANGE.front() && type <= BOOST_RANGE.back();
    return ScopedAStatus::ok();
}

ScopedAStatus Power::createHintSession(int32_t, int32_t, const std::vector<int32_t>& tids, int64_t,
                                       std::shared_ptr<IPowerHintSession>* _aidl_return) {
    if (tids.size() == 0) {
        *_aidl_return = nullptr;
        return ScopedAStatus::fromExceptionCode(EX_ILLEGAL_ARGUMENT);
    }
    std::shared_ptr<IPowerHintSession> powerHintSession =
            ndk::SharedRefBase::make<PowerHintSession>();
    mPowerHintSessions.push_back(powerHintSession);
    *_aidl_return = powerHintSession;
    return ScopedAStatus::ok();
}

ScopedAStatus Power::getHintSessionPreferredRate(int64_t* outNanoseconds) {
    *outNanoseconds = std::chrono::nanoseconds(1ms).count();
    return ScopedAStatus::ok();
}

}  // namespace example
}  // namespace impl
}  // namespace power
}  // namespace hardware
}  // namespace android
}  // namespace aidl
