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

#include <libprotocan/MessageCounter.h>

#include <android-base/logging.h>

namespace android::hardware::automotive::protocan {

/** Whether to log counter state messages. */
static constexpr bool kSuperVerbose = false;

MessageCounter::MessageCounter(Signal signal) : upperBound(signal.maxValue + 1), mSignal(signal) {}

Signal::value MessageCounter::next() const {
  CHECK(mCurrent.has_value()) << "Counter not initialized. Did you call isReady?";
  return (*mCurrent + 1) % upperBound;
}

void MessageCounter::read(const can::V1_0::CanMessage& msg) {
    auto val = mSignal.get(msg);

    if (!mCurrent.has_value()) {
        LOG(VERBOSE) << "Got first counter val of " << val;
        mCurrent = val;
        return;
    }

    auto nextVal = next();
    if (nextVal == val) {
        if constexpr (kSuperVerbose) {
            LOG(VERBOSE) << "Got next counter val of " << nextVal;
        }
        mCurrent = nextVal;
    } else {
        LOG(DEBUG) << "Ignoring next counter val of " << val << ", waiting for " << nextVal;
    }
}

bool MessageCounter::isReady() const { return mCurrent.has_value(); }

void MessageCounter::increment(can::V1_0::CanMessage& msg) {
  auto newVal = next();
  mCurrent = newVal;
  mSignal.set(msg, newVal);
}

}  // namespace android::hardware::automotive::protocan
