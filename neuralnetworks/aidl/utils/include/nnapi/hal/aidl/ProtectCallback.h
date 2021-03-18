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

#ifndef ANDROID_HARDWARE_INTERFACES_NEURALNETWORKS_AIDL_UTILS_PROTECT_CALLBACK_H
#define ANDROID_HARDWARE_INTERFACES_NEURALNETWORKS_AIDL_UTILS_PROTECT_CALLBACK_H

#include <android-base/scopeguard.h>
#include <android-base/thread_annotations.h>
#include <android/binder_interface_utils.h>
#include <nnapi/Result.h>
#include <nnapi/Types.h>
#include <nnapi/hal/CommonUtils.h>
#include <nnapi/hal/ProtectCallback.h>

#include <functional>
#include <mutex>
#include <vector>

// See hardware/interfaces/neuralnetworks/utils/README.md for more information on AIDL interface
// lifetimes across processes and for protecting asynchronous calls across AIDL.

namespace aidl::android::hardware::neuralnetworks::utils {

// Thread safe class
class DeathMonitor final {
  public:
    static void serviceDied(void* cookie);
    void serviceDied();
    // Precondition: `killable` must be non-null.
    void add(hal::utils::IProtectedCallback* killable) const;
    // Precondition: `killable` must be non-null.
    void remove(hal::utils::IProtectedCallback* killable) const;

  private:
    mutable std::mutex mMutex;
    mutable std::vector<hal::utils::IProtectedCallback*> mObjects GUARDED_BY(mMutex);
};

class DeathHandler final {
  public:
    static nn::GeneralResult<DeathHandler> create(std::shared_ptr<ndk::ICInterface> object);

    DeathHandler(const DeathHandler&) = delete;
    DeathHandler(DeathHandler&&) noexcept = default;
    DeathHandler& operator=(const DeathHandler&) = delete;
    DeathHandler& operator=(DeathHandler&&) noexcept = delete;
    ~DeathHandler();

    using Cleanup = std::function<void()>;
    // Precondition: `killable` must be non-null.
    [[nodiscard]] ::android::base::ScopeGuard<Cleanup> protectCallback(
            hal::utils::IProtectedCallback* killable) const;

    std::shared_ptr<DeathMonitor> getDeathMonitor() const { return kDeathMonitor; }

  private:
    DeathHandler(std::shared_ptr<ndk::ICInterface> object,
                 ndk::ScopedAIBinder_DeathRecipient deathRecipient,
                 std::shared_ptr<DeathMonitor> deathMonitor);

    std::shared_ptr<ndk::ICInterface> kObject;
    ndk::ScopedAIBinder_DeathRecipient kDeathRecipient;
    std::shared_ptr<DeathMonitor> kDeathMonitor;
};

}  // namespace aidl::android::hardware::neuralnetworks::utils

#endif  // ANDROID_HARDWARE_INTERFACES_NEURALNETWORKS_AIDL_UTILS_PROTECT_CALLBACK_H
