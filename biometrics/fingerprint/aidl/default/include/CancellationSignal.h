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

#include <aidl/android/hardware/biometrics/common/BnCancellationSignal.h>
#include <aidl/android/hardware/biometrics/fingerprint/ISessionCallback.h>
#include <functional>
#include <future>

#include "WorkerThread.h"

namespace aidl::android::hardware::biometrics::fingerprint {

class CancellationSignal : public common::BnCancellationSignal {
  public:
    explicit CancellationSignal(std::promise<void>&& cancellationPromise);

    ndk::ScopedAStatus cancel() override;

  private:
    std::promise<void> mCancellationPromise;
};

// Returns whether the given cancellation future is ready, i.e. whether the operation corresponding
// to this future should be cancelled.
bool shouldCancel(const std::future<void>& cancellationFuture);

}  // namespace aidl::android::hardware::biometrics::fingerprint
