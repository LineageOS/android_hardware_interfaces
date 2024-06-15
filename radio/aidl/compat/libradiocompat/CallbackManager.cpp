/*
 * Copyright (C) 2021 The Android Open Source Project
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

#include <libradiocompat/CallbackManager.h>

#include <android-base/logging.h>

using namespace std::literals::chrono_literals;

namespace android::hardware::radio::compat {

/**
 * How much setter thread will wait with setting response functions after the last
 * setResponseFunctions call from the framework. Subsequent calls from the framework reset the
 * clock, so this number should be larger than the longest time between setResponseFunctions calls
 * from the framework.
 *
 * Real world measurements with Cuttlefish give <10ms delay between Modem and Data and <2ms delays
 * between all others.
 */
static constexpr auto kDelayedSetterDelay = 100ms;

CallbackManager::CallbackManager(std::shared_ptr<DriverContext> context, sp<V1_5::IRadio> hidlHal)
    : mHidlHal(hidlHal),
      mRadioResponse(sp<compat::RadioResponse>::make(context)),
      mRadioIndication(sp<compat::RadioIndication>::make(context)),
      mDelayedSetterThread(&CallbackManager::delayedSetterThread, this) {}

CallbackManager::~CallbackManager() {
    {
        std::unique_lock<std::mutex> lock(mDelayedSetterGuard);
        mDelayedSetterDeadline = std::nullopt;
        mDestroy = true;
        mDelayedSetterCv.notify_all();
    }
    mDelayedSetterThread.join();
}

RadioResponse& CallbackManager::response() const {
    return *mRadioResponse;
}

RadioIndication& CallbackManager::indication() const {
    return *mRadioIndication;
}

void CallbackManager::setResponseFunctionsDelayed() {
    std::unique_lock<std::mutex> lock(mDelayedSetterGuard);
    mDelayedSetterDeadline = std::chrono::steady_clock::now() + kDelayedSetterDelay;
    mDelayedSetterCv.notify_all();
}

void CallbackManager::delayedSetterThread() {
    while (!mDestroy) {
        std::unique_lock<std::mutex> lock(mDelayedSetterGuard);
        auto deadline = mDelayedSetterDeadline;

        // not waiting to set response functions
        if (!deadline) {
            mDelayedSetterCv.wait(lock);
            continue;
        }

        // waiting to set response functions, but not yet
        if (*deadline > std::chrono::steady_clock::now()) {
            mDelayedSetterCv.wait_until(lock, *deadline);
            continue;
        }

        mHidlHal->setResponseFunctions(mRadioResponse, mRadioIndication).assertOk();
        mDelayedSetterDeadline = std::nullopt;
    }
}

}  // namespace android::hardware::radio::compat
