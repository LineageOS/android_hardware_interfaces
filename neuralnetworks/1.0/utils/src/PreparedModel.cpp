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

#include "PreparedModel.h"

#include "Callbacks.h"
#include "Conversions.h"
#include "Utils.h"

#include <android/hardware/neuralnetworks/1.0/IPreparedModel.h>
#include <android/hardware/neuralnetworks/1.0/types.h>
#include <nnapi/IPreparedModel.h>
#include <nnapi/Result.h>
#include <nnapi/Types.h>
#include <nnapi/hal/CommonUtils.h>
#include <nnapi/hal/HandleError.h>
#include <nnapi/hal/ProtectCallback.h>

#include <memory>
#include <tuple>
#include <utility>
#include <vector>

// See hardware/interfaces/neuralnetworks/utils/README.md for more information on HIDL interface
// lifetimes across processes and for protecting asynchronous calls across HIDL.

namespace android::hardware::neuralnetworks::V1_0::utils {

nn::GeneralResult<std::shared_ptr<const PreparedModel>> PreparedModel::create(
        sp<V1_0::IPreparedModel> preparedModel) {
    if (preparedModel == nullptr) {
        return NN_ERROR(nn::ErrorStatus::INVALID_ARGUMENT)
               << "V1_0::utils::PreparedModel::create must have non-null preparedModel";
    }

    auto deathHandler = NN_TRY(hal::utils::DeathHandler::create(preparedModel));
    return std::make_shared<const PreparedModel>(PrivateConstructorTag{}, std::move(preparedModel),
                                                 std::move(deathHandler));
}

PreparedModel::PreparedModel(PrivateConstructorTag /*tag*/, sp<V1_0::IPreparedModel> preparedModel,
                             hal::utils::DeathHandler deathHandler)
    : kPreparedModel(std::move(preparedModel)), kDeathHandler(std::move(deathHandler)) {}

nn::ExecutionResult<std::pair<std::vector<nn::OutputShape>, nn::Timing>> PreparedModel::execute(
        const nn::Request& request, nn::MeasureTiming /*measure*/,
        const nn::OptionalTimePoint& /*deadline*/,
        const nn::OptionalTimeoutDuration& /*loopTimeoutDuration*/) const {
    // Ensure that request is ready for IPC.
    std::optional<nn::Request> maybeRequestInShared;
    const nn::Request& requestInShared = NN_TRY(hal::utils::makeExecutionFailure(
            hal::utils::flushDataFromPointerToShared(&request, &maybeRequestInShared)));

    const auto hidlRequest = NN_TRY(hal::utils::makeExecutionFailure(convert(requestInShared)));

    const auto cb = sp<ExecutionCallback>::make();
    const auto scoped = kDeathHandler.protectCallback(cb.get());

    const auto ret = kPreparedModel->execute(hidlRequest, cb);
    const auto status = HANDLE_TRANSPORT_FAILURE(ret);
    if (status != ErrorStatus::NONE) {
        const auto canonical = nn::convert(status).value_or(nn::ErrorStatus::GENERAL_FAILURE);
        return NN_ERROR(canonical) << "execute failed with " << toString(status);
    }

    auto result = NN_TRY(cb->get());
    NN_TRY(hal::utils::makeExecutionFailure(
            hal::utils::unflushDataFromSharedToPointer(request, maybeRequestInShared)));

    return result;
}

nn::GeneralResult<std::pair<nn::SyncFence, nn::ExecuteFencedInfoCallback>>
PreparedModel::executeFenced(
        const nn::Request& /*request*/, const std::vector<nn::SyncFence>& /*waitFor*/,
        nn::MeasureTiming /*measure*/, const nn::OptionalTimePoint& /*deadline*/,
        const nn::OptionalTimeoutDuration& /*loopTimeoutDuration*/,
        const nn::OptionalTimeoutDuration& /*timeoutDurationAfterFence*/) const {
    return NN_ERROR(nn::ErrorStatus::GENERAL_FAILURE)
           << "IPreparedModel::executeFenced is not supported on 1.0 HAL service";
}

std::any PreparedModel::getUnderlyingResource() const {
    sp<V1_0::IPreparedModel> resource = kPreparedModel;
    return resource;
}

}  // namespace android::hardware::neuralnetworks::V1_0::utils
