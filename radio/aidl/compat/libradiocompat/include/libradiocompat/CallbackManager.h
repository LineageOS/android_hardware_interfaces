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
#pragma once

#include "DriverContext.h"
#include "RadioIndication.h"
#include "RadioResponse.h"

#include <android-base/logging.h>
#include <android/hardware/radio/1.6/IRadio.h>

#include <thread>

namespace android::hardware::radio::compat {

class CallbackManager {
    sp<V1_5::IRadio> mHidlHal;
    sp<RadioResponse> mRadioResponse;
    sp<RadioIndication> mRadioIndication;

    std::thread mDelayedSetterThread;
    std::mutex mDelayedSetterGuard;
    std::optional<std::chrono::time_point<std::chrono::steady_clock>> mDelayedSetterDeadline
            GUARDED_BY(mDelayedSetterGuard);
    std::condition_variable mDelayedSetterCv GUARDED_BY(mDelayedSetterGuard);
    bool mDestroy GUARDED_BY(mDelayedSetterGuard) = false;

    void setResponseFunctionsDelayed();
    void delayedSetterThread();

  public:
    CallbackManager(std::shared_ptr<DriverContext> context, sp<V1_5::IRadio> hidlHal);
    ~CallbackManager();

    RadioResponse& response() const;
    RadioIndication& indication() const;

    template <typename ResponseType, typename IndicationType>
    void setResponseFunctions(const std::shared_ptr<ResponseType>& response,
                              const std::shared_ptr<IndicationType>& indication) {
        CHECK(response);
        CHECK(indication);

        mRadioResponse->setResponseFunction(response);
        mRadioIndication->setResponseFunction(indication);
        setResponseFunctionsDelayed();
    }
};

}  // namespace android::hardware::radio::compat
