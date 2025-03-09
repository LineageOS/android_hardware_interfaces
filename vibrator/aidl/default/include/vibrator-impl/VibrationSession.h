/*
 * Copyright (C) 2024 The Android Open Source Project
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

#include <aidl/android/hardware/vibrator/BnVibrationSession.h>
#include <aidl/android/hardware/vibrator/IVibrator.h>
#include <aidl/android/hardware/vibrator/IVibratorCallback.h>
#include <android-base/thread_annotations.h>

#include "vibrator-impl/VibratorManager.h"

namespace aidl {
namespace android {
namespace hardware {
namespace vibrator {

class VibrationSession : public BnVibrationSession {
  public:
    VibrationSession(std::shared_ptr<VibratorManager> manager) : mManager(std::move(manager)) {};

    ndk::ScopedAStatus close() override;
    ndk::ScopedAStatus abort() override;

  private:
    mutable std::mutex mMutex;
    std::shared_ptr<VibratorManager> mManager;
};

}  // namespace vibrator
}  // namespace hardware
}  // namespace android
}  // namespace aidl
