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

#ifndef ANDROID_HARDWARE_INTERFACES_NEURALNETWORKS_UTILS_COMMON_PROTECT_CALLBACK_H
#define ANDROID_HARDWARE_INTERFACES_NEURALNETWORKS_UTILS_COMMON_PROTECT_CALLBACK_H

#include <android-base/scopeguard.h>
#include <android-base/thread_annotations.h>
#include <android/hidl/base/1.0/IBase.h>
#include <hidl/HidlSupport.h>
#include <nnapi/Result.h>
#include <nnapi/Types.h>

#include <functional>
#include <mutex>
#include <vector>

// See hardware/interfaces/neuralnetworks/utils/README.md for more information on HIDL interface
// lifetimes across processes and for protecting asynchronous calls across HIDL.

namespace android::hardware::neuralnetworks::utils {

class IProtectedCallback {
  public:
    /**
     * Marks this object as a dead object.
     */
    virtual void notifyAsDeadObject() = 0;

    // Public virtual destructor to allow objects to be stored (and destroyed) as smart pointers.
    // E.g., std::unique_ptr<IProtectedCallback>.
    virtual ~IProtectedCallback() = default;

  protected:
    // Protect the non-destructor special member functions to prevent object slicing.
    IProtectedCallback() = default;
    IProtectedCallback(const IProtectedCallback&) = default;
    IProtectedCallback(IProtectedCallback&&) noexcept = default;
    IProtectedCallback& operator=(const IProtectedCallback&) = default;
    IProtectedCallback& operator=(IProtectedCallback&&) noexcept = default;
};

// Thread safe class
class DeathRecipient final : public hidl_death_recipient {
  public:
    void serviceDied(uint64_t cookie, const wp<hidl::base::V1_0::IBase>& who) override;
    // Precondition: `killable` must be non-null.
    void add(IProtectedCallback* killable) const;
    // Precondition: `killable` must be non-null.
    void remove(IProtectedCallback* killable) const;

  private:
    mutable std::mutex mMutex;
    mutable bool mIsDeadObject GUARDED_BY(mMutex) = false;
    mutable std::vector<IProtectedCallback*> mObjects GUARDED_BY(mMutex);
};

class DeathHandler final {
  public:
    static nn::GeneralResult<DeathHandler> create(sp<hidl::base::V1_0::IBase> object);

    DeathHandler(const DeathHandler&) = delete;
    DeathHandler(DeathHandler&&) noexcept = default;
    DeathHandler& operator=(const DeathHandler&) = delete;
    DeathHandler& operator=(DeathHandler&&) noexcept = delete;
    ~DeathHandler();

    using Cleanup = std::function<void()>;
    using Hold = base::ScopeGuard<Cleanup>;

    // Precondition: `killable` must be non-null.
    // `killable` must outlive the return value `Hold`.
    [[nodiscard]] Hold protectCallback(IProtectedCallback* killable) const;

    // Precondition: `killable` must be non-null.
    // `killable` must outlive the `DeathHandler`.
    void protectCallbackForLifetimeOfDeathHandler(IProtectedCallback* killable) const;

  private:
    DeathHandler(sp<hidl::base::V1_0::IBase> object, sp<DeathRecipient> deathRecipient);

    sp<hidl::base::V1_0::IBase> mObject;
    sp<DeathRecipient> mDeathRecipient;
};

}  // namespace android::hardware::neuralnetworks::utils

#endif  // ANDROID_HARDWARE_INTERFACES_NEURALNETWORKS_UTILS_COMMON_PROTECT_CALLBACK_H
