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

#pragma once

#include <aidl/android/hardware/vibrator/BnVibratorManager.h>
#include <android-base/thread_annotations.h>

#include "vibrator-impl/Vibrator.h"

namespace aidl {
namespace android {
namespace hardware {
namespace vibrator {

class VibratorManager : public BnVibratorManager {
  public:
    VibratorManager(std::shared_ptr<Vibrator> vibrator) : mDefaultVibrator(std::move(vibrator)) {};

    ndk::ScopedAStatus getCapabilities(int32_t* _aidl_return) override;
    ndk::ScopedAStatus getVibratorIds(std::vector<int32_t>* _aidl_return) override;
    ndk::ScopedAStatus getVibrator(int32_t vibratorId,
                                   std::shared_ptr<IVibrator>* _aidl_return) override;
    ndk::ScopedAStatus prepareSynced(const std::vector<int32_t>& vibratorIds) override;
    ndk::ScopedAStatus triggerSynced(const std::shared_ptr<IVibratorCallback>& callback) override;
    ndk::ScopedAStatus cancelSynced() override;
    ndk::ScopedAStatus startSession(const std::vector<int32_t>& vibratorIds,
                                    const VibrationSessionConfig& config,
                                    const std::shared_ptr<IVibratorCallback>& callback,
                                    std::shared_ptr<IVibrationSession>* _aidl_return) override;
    ndk::ScopedAStatus clearSessions() override;

    void abortSession();
    void closeSession(int32_t delayMs);

  private:
    std::shared_ptr<Vibrator> mDefaultVibrator;
    mutable std::mutex mMutex;
    int32_t mCapabilities GUARDED_BY(mMutex) = 0;
    bool mIsPreparing GUARDED_BY(mMutex) = false;
    bool mIsClosingSession GUARDED_BY(mMutex) = false;
    std::shared_ptr<IVibrationSession> mSession GUARDED_BY(mMutex) = nullptr;
    std::shared_ptr<IVibratorCallback> mSessionCallback GUARDED_BY(mMutex) = nullptr;

    void clearSession(const std::shared_ptr<IVibrationSession>& session);
};

}  // namespace vibrator
}  // namespace hardware
}  // namespace android
}  // namespace aidl
