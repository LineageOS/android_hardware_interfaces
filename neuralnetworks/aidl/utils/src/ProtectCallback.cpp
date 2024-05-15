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

#include "ProtectCallback.h"

#include <android-base/logging.h>
#include <android-base/scopeguard.h>
#include <android-base/thread_annotations.h>
#include <android/binder_auto_utils.h>
#include <android/binder_interface_utils.h>
#include <nnapi/Result.h>

#include <algorithm>
#include <functional>
#include <map>
#include <memory>
#include <mutex>
#include <vector>

#include "Utils.h"

namespace aidl::android::hardware::neuralnetworks::utils {

namespace {

// Only dereference the cookie if it's valid (if it's in this set)
// Only used with ndk
std::mutex sCookiesMutex;
uintptr_t sCookieKeyCounter GUARDED_BY(sCookiesMutex) = 0;
std::map<uintptr_t, std::weak_ptr<DeathMonitor>> sCookies GUARDED_BY(sCookiesMutex);

}  // namespace

void DeathMonitor::serviceDied() {
    std::lock_guard guard(mMutex);
    std::for_each(mObjects.begin(), mObjects.end(),
                  [](IProtectedCallback* killable) { killable->notifyAsDeadObject(); });
}

void DeathMonitor::serviceDied(void* cookie) {
    std::shared_ptr<DeathMonitor> monitor;
    {
        std::lock_guard<std::mutex> guard(sCookiesMutex);
        if (auto it = sCookies.find(reinterpret_cast<uintptr_t>(cookie)); it != sCookies.end()) {
            monitor = it->second.lock();
            sCookies.erase(it);
        } else {
            LOG(INFO)
                    << "Service died, but cookie is no longer valid so there is nothing to notify.";
            return;
        }
    }
    if (monitor) {
        LOG(INFO) << "Notifying DeathMonitor from serviceDied.";
        monitor->serviceDied();
    } else {
        LOG(INFO) << "Tried to notify DeathMonitor from serviceDied but could not promote.";
    }
}

void DeathMonitor::add(IProtectedCallback* killable) const {
    CHECK(killable != nullptr);
    std::lock_guard guard(mMutex);
    mObjects.push_back(killable);
}

void DeathMonitor::remove(IProtectedCallback* killable) const {
    CHECK(killable != nullptr);
    std::lock_guard guard(mMutex);
    const auto removedIter = std::remove(mObjects.begin(), mObjects.end(), killable);
    mObjects.erase(removedIter);
}

DeathMonitor::~DeathMonitor() {
    // lock must be taken so object is not used in OnBinderDied"
    std::lock_guard<std::mutex> guard(sCookiesMutex);
    sCookies.erase(kCookieKey);
}

nn::GeneralResult<DeathHandler> DeathHandler::create(std::shared_ptr<ndk::ICInterface> object) {
    if (object == nullptr) {
        return NN_ERROR(nn::ErrorStatus::INVALID_ARGUMENT)
               << "utils::DeathHandler::create must have non-null object";
    }

    std::shared_ptr<DeathMonitor> deathMonitor;
    {
        std::lock_guard<std::mutex> guard(sCookiesMutex);
        deathMonitor = std::make_shared<DeathMonitor>(sCookieKeyCounter++);
        sCookies[deathMonitor->getCookieKey()] = deathMonitor;
    }

    auto deathRecipient = ndk::ScopedAIBinder_DeathRecipient(
            AIBinder_DeathRecipient_new(DeathMonitor::serviceDied));

    // If passed a local binder, AIBinder_linkToDeath will do nothing and return
    // STATUS_INVALID_OPERATION. We ignore this case because we only use local binders in tests
    // where this is not an error.
    if (object->isRemote()) {
        const auto ret = ndk::ScopedAStatus::fromStatus(
                AIBinder_linkToDeath(object->asBinder().get(), deathRecipient.get(),
                                     reinterpret_cast<void*>(deathMonitor->getCookieKey())));
        HANDLE_ASTATUS(ret) << "AIBinder_linkToDeath failed";
    }

    return DeathHandler(std::move(object), std::move(deathRecipient), std::move(deathMonitor));
}

DeathHandler::DeathHandler(std::shared_ptr<ndk::ICInterface> object,
                           ndk::ScopedAIBinder_DeathRecipient deathRecipient,
                           std::shared_ptr<DeathMonitor> deathMonitor)
    : kObject(std::move(object)),
      kDeathRecipient(std::move(deathRecipient)),
      kDeathMonitor(std::move(deathMonitor)) {
    CHECK(kObject != nullptr);
    CHECK(kDeathRecipient.get() != nullptr);
    CHECK(kDeathMonitor != nullptr);
}

DeathHandler::~DeathHandler() {
    if (kObject != nullptr && kDeathRecipient.get() != nullptr && kDeathMonitor != nullptr) {
        const auto ret = ndk::ScopedAStatus::fromStatus(
                AIBinder_unlinkToDeath(kObject->asBinder().get(), kDeathRecipient.get(),
                                       reinterpret_cast<void*>(kDeathMonitor->getCookieKey())));
        const auto maybeSuccess = handleTransportError(ret);
        if (!maybeSuccess.ok()) {
            LOG(ERROR) << maybeSuccess.error().message;
        }
    }
}

[[nodiscard]] ::android::base::ScopeGuard<DeathHandler::Cleanup> DeathHandler::protectCallback(
        IProtectedCallback* killable) const {
    CHECK(killable != nullptr);
    kDeathMonitor->add(killable);
    return ::android::base::make_scope_guard(
            [deathMonitor = kDeathMonitor, killable] { deathMonitor->remove(killable); });
}

}  // namespace aidl::android::hardware::neuralnetworks::utils
