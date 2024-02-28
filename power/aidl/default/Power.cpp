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
#include <fmq/AidlMessageQueue.h>
#include <fmq/EventFlag.h>
#include <thread>

namespace aidl {
namespace android {
namespace hardware {
namespace power {
namespace impl {
namespace example {

using namespace std::chrono_literals;
using ::aidl::android::hardware::common::fmq::MQDescriptor;
using ::aidl::android::hardware::common::fmq::SynchronizedReadWrite;
using ::aidl::android::hardware::power::ChannelMessage;
using ::android::AidlMessageQueue;

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

ndk::ScopedAStatus Power::createHintSessionWithConfig(
        int32_t tgid, int32_t uid, const std::vector<int32_t>& threadIds, int64_t durationNanos,
        SessionTag, SessionConfig* config, std::shared_ptr<IPowerHintSession>* _aidl_return) {
    auto out = createHintSession(tgid, uid, threadIds, durationNanos, _aidl_return);
    static_cast<PowerHintSession*>(_aidl_return->get())->getSessionConfig(config);
    return out;
}

ndk::ScopedAStatus Power::getSessionChannel(int32_t, int32_t, ChannelConfig* _aidl_return) {
    static AidlMessageQueue<ChannelMessage, SynchronizedReadWrite> stubQueue{20, true};
    static std::thread stubThread([&] {
        ChannelMessage data;
        // This loop will only run while there is data waiting
        // to be processed, and blocks on a futex all other times
        while (stubQueue.readBlocking(&data, 1, 0)) {
        }
    });
    _aidl_return->channelDescriptor = stubQueue.dupeDesc();
    _aidl_return->readFlagBitmask = 0x01;
    _aidl_return->writeFlagBitmask = 0x02;
    _aidl_return->eventFlagDescriptor = std::nullopt;
    return ndk::ScopedAStatus::ok();
}

ndk::ScopedAStatus Power::closeSessionChannel(int32_t, int32_t) {
    return ndk::ScopedAStatus::ok();
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
