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

#include "ProtectCallback.h"

#include <android-base/logging.h>
#include <android-base/scopeguard.h>
#include <android-base/thread_annotations.h>
#include <android/hidl/base/1.0/IBase.h>
#include <hidl/HidlSupport.h>
#include <nnapi/Result.h>
#include <nnapi/hal/HandleError.h>

#include <algorithm>
#include <functional>
#include <mutex>
#include <vector>

namespace android::hardware::neuralnetworks::utils {

void DeathRecipient::serviceDied(uint64_t /*cookie*/, const wp<hidl::base::V1_0::IBase>& /*who*/) {
    std::lock_guard guard(mMutex);
    std::for_each(mObjects.begin(), mObjects.end(),
                  [](IProtectedCallback* killable) { killable->notifyAsDeadObject(); });
}

void DeathRecipient::add(IProtectedCallback* killable) const {
    CHECK(killable != nullptr);
    std::lock_guard guard(mMutex);
    mObjects.push_back(killable);
}

void DeathRecipient::remove(IProtectedCallback* killable) const {
    CHECK(killable != nullptr);
    std::lock_guard guard(mMutex);
    const auto removedIter = std::remove(mObjects.begin(), mObjects.end(), killable);
    mObjects.erase(removedIter);
}

nn::GeneralResult<DeathHandler> DeathHandler::create(sp<hidl::base::V1_0::IBase> object) {
    if (object == nullptr) {
        return NN_ERROR(nn::ErrorStatus::INVALID_ARGUMENT)
               << "utils::DeathHandler::create must have non-null object";
    }
    auto deathRecipient = sp<DeathRecipient>::make();

    const auto ret = object->linkToDeath(deathRecipient, /*cookie=*/0);
    const bool success = NN_TRY(handleTransportError(ret));
    if (!success) {
        return NN_ERROR(nn::ErrorStatus::GENERAL_FAILURE) << "IBase::linkToDeath returned false";
    }

    return DeathHandler(std::move(object), std::move(deathRecipient));
}

DeathHandler::DeathHandler(sp<hidl::base::V1_0::IBase> object, sp<DeathRecipient> deathRecipient)
    : kObject(std::move(object)), kDeathRecipient(std::move(deathRecipient)) {
    CHECK(kObject != nullptr);
    CHECK(kDeathRecipient != nullptr);
}

DeathHandler::~DeathHandler() {
    if (kObject != nullptr && kDeathRecipient != nullptr) {
        const auto ret = kObject->unlinkToDeath(kDeathRecipient);
        const auto maybeSuccess = handleTransportError(ret);
        if (!maybeSuccess.has_value()) {
            LOG(ERROR) << maybeSuccess.error().message;
        } else if (!maybeSuccess.value()) {
            LOG(ERROR) << "IBase::linkToDeath returned false";
        }
    }
}

[[nodiscard]] base::ScopeGuard<DeathHandler::Cleanup> DeathHandler::protectCallback(
        IProtectedCallback* killable) const {
    CHECK(killable != nullptr);
    kDeathRecipient->add(killable);
    return base::make_scope_guard(
            [deathRecipient = kDeathRecipient, killable] { deathRecipient->remove(killable); });
}

}  // namespace android::hardware::neuralnetworks::utils
