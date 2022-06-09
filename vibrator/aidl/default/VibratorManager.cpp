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

#include "vibrator-impl/VibratorManager.h"

#include <android-base/logging.h>
#include <thread>

namespace aidl {
namespace android {
namespace hardware {
namespace vibrator {

static constexpr int32_t kDefaultVibratorId = 1;

ndk::ScopedAStatus VibratorManager::getCapabilities(int32_t* _aidl_return) {
    LOG(INFO) << "Vibrator manager reporting capabilities";
    *_aidl_return =
            IVibratorManager::CAP_SYNC | IVibratorManager::CAP_PREPARE_ON |
            IVibratorManager::CAP_PREPARE_PERFORM | IVibratorManager::CAP_PREPARE_COMPOSE |
            IVibratorManager::CAP_MIXED_TRIGGER_ON | IVibratorManager::CAP_MIXED_TRIGGER_PERFORM |
            IVibratorManager::CAP_MIXED_TRIGGER_COMPOSE | IVibratorManager::CAP_TRIGGER_CALLBACK;
    return ndk::ScopedAStatus::ok();
}

ndk::ScopedAStatus VibratorManager::getVibratorIds(std::vector<int32_t>* _aidl_return) {
    LOG(INFO) << "Vibrator manager getting vibrator ids";
    *_aidl_return = {kDefaultVibratorId};
    return ndk::ScopedAStatus::ok();
}

ndk::ScopedAStatus VibratorManager::getVibrator(int32_t vibratorId,
                                                std::shared_ptr<IVibrator>* _aidl_return) {
    LOG(INFO) << "Vibrator manager getting vibrator " << vibratorId;
    if (vibratorId == kDefaultVibratorId) {
        *_aidl_return = mDefaultVibrator;
        return ndk::ScopedAStatus::ok();
    } else {
        *_aidl_return = nullptr;
        return ndk::ScopedAStatus::fromExceptionCode(EX_ILLEGAL_ARGUMENT);
    }
}

ndk::ScopedAStatus VibratorManager::prepareSynced(const std::vector<int32_t>& vibratorIds) {
    LOG(INFO) << "Vibrator Manager prepare synced";
    if (vibratorIds.size() == 1 && vibratorIds[0] == kDefaultVibratorId) {
        return ndk::ScopedAStatus::ok();
    } else {
        return ndk::ScopedAStatus::fromExceptionCode(EX_ILLEGAL_ARGUMENT);
    }
}

ndk::ScopedAStatus VibratorManager::triggerSynced(
        const std::shared_ptr<IVibratorCallback>& callback) {
    LOG(INFO) << "Vibrator Manager trigger synced";
    std::thread([callback] {
        if (callback != nullptr) {
            LOG(INFO) << "Notifying perform complete";
            callback->onComplete();
        }
    }).detach();

    return ndk::ScopedAStatus::ok();
}

ndk::ScopedAStatus VibratorManager::cancelSynced() {
    LOG(INFO) << "Vibrator Manager cancel synced";
    return ndk::ScopedAStatus::ok();
}

}  // namespace vibrator
}  // namespace hardware
}  // namespace android
}  // namespace aidl
