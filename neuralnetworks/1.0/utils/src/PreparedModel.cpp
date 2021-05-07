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

#include "Burst.h"
#include "Callbacks.h"
#include "Conversions.h"
#include "Execution.h"
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
        return NN_ERROR() << "V1_0::utils::PreparedModel::create must have non-null preparedModel";
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
        const nn::OptionalDuration& /*loopTimeoutDuration*/) const {
    // Ensure that request is ready for IPC.
    std::optional<nn::Request> maybeRequestInShared;
    hal::utils::RequestRelocation relocation;
    const nn::Request& requestInShared =
            NN_TRY(hal::utils::makeExecutionFailure(hal::utils::convertRequestFromPointerToShared(
                    &request, nn::kDefaultRequestMemoryAlignment, nn::kMinMemoryPadding,
                    &maybeRequestInShared, &relocation)));

    const auto hidlRequest = NN_TRY(hal::utils::makeExecutionFailure(convert(requestInShared)));

    return executeInternal(hidlRequest, relocation);
}

nn::ExecutionResult<std::pair<std::vector<nn::OutputShape>, nn::Timing>>
PreparedModel::executeInternal(const V1_0::Request& request,
                               const hal::utils::RequestRelocation& relocation) const {
    if (relocation.input) {
        relocation.input->flush();
    }

    const auto cb = sp<ExecutionCallback>::make();
    const auto scoped = kDeathHandler.protectCallback(cb.get());

    const auto ret = kPreparedModel->execute(request, cb);
    const auto status = HANDLE_TRANSPORT_FAILURE(ret);
    HANDLE_HAL_STATUS(status) << "execution failed with " << toString(status);

    auto result = NN_TRY(cb->get());
    if (relocation.output) {
        relocation.output->flush();
    }
    return result;
}

nn::GeneralResult<std::pair<nn::SyncFence, nn::ExecuteFencedInfoCallback>>
PreparedModel::executeFenced(const nn::Request& /*request*/,
                             const std::vector<nn::SyncFence>& /*waitFor*/,
                             nn::MeasureTiming /*measure*/,
                             const nn::OptionalTimePoint& /*deadline*/,
                             const nn::OptionalDuration& /*loopTimeoutDuration*/,
                             const nn::OptionalDuration& /*timeoutDurationAfterFence*/) const {
    return NN_ERROR(nn::ErrorStatus::GENERAL_FAILURE)
           << "IPreparedModel::executeFenced is not supported on 1.0 HAL service";
}

nn::GeneralResult<nn::SharedExecution> PreparedModel::createReusableExecution(
        const nn::Request& request, nn::MeasureTiming /*measure*/,
        const nn::OptionalDuration& /*loopTimeoutDuration*/) const {
    // Ensure that request is ready for IPC.
    std::optional<nn::Request> maybeRequestInShared;
    hal::utils::RequestRelocation relocation;
    const nn::Request& requestInShared = NN_TRY(hal::utils::convertRequestFromPointerToShared(
            &request, nn::kDefaultRequestMemoryAlignment, nn::kMinMemoryPadding,
            &maybeRequestInShared, &relocation));

    auto hidlRequest = NN_TRY(convert(requestInShared));
    return Execution::create(shared_from_this(), std::move(hidlRequest), std::move(relocation));
}

nn::GeneralResult<nn::SharedBurst> PreparedModel::configureExecutionBurst() const {
    return Burst::create(shared_from_this());
}

std::any PreparedModel::getUnderlyingResource() const {
    sp<V1_0::IPreparedModel> resource = kPreparedModel;
    return resource;
}

}  // namespace android::hardware::neuralnetworks::V1_0::utils
