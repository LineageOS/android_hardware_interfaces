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

#include "CustomVibrator.h"

#include <android-base/logging.h>
#include <thread>

namespace aidl::android::hardware::tests::extension::vibrator {

ndk::ScopedAStatus CustomVibrator::getVendorCapabilities(int32_t* _aidl_return) {
    *_aidl_return = ICustomVibrator::CAP_VENDOR_DIRECTIONALITY;
    return ndk::ScopedAStatus::ok();
}

ndk::ScopedAStatus CustomVibrator::setDirectionality(Directionality directionality) {
    LOG(INFO) << "Custom vibrator set directionality";
    // do something cool in hardware
    (void)directionality;
    return ndk::ScopedAStatus::ok();
}

ndk::ScopedAStatus CustomVibrator::perform(VendorEffect effect,
                                           const std::shared_ptr<IVibratorCallback>& callback,
                                           int32_t* _aidl_return) {
    LOG(INFO) << "Custom vibrator perform";

    if (effect != VendorEffect::CRACKLE && effect != VendorEffect::WIGGLE) {
        return ndk::ScopedAStatus(AStatus_fromExceptionCode(EX_UNSUPPORTED_OPERATION));
    }

    constexpr size_t kEffectMillis = 100;

    if (callback != nullptr) {
        std::thread([=] {
            LOG(INFO) << "Starting vendor perform on another thread";
            usleep(kEffectMillis * 1000);
            LOG(INFO) << "Notifying vendor perform complete";
            callback->onComplete();
        }).detach();
    }

    *_aidl_return = kEffectMillis;
    return ndk::ScopedAStatus::ok();
}

}  // namespace aidl::android::hardware::tests::extension::vibrator
