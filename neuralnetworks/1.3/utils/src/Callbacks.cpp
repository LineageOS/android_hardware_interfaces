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

#include "Callbacks.h"

#include "Conversions.h"
#include "PreparedModel.h"
#include "Utils.h"

#include <android/hardware/neuralnetworks/1.0/types.h>
#include <android/hardware/neuralnetworks/1.2/types.h>
#include <android/hardware/neuralnetworks/1.3/IExecutionCallback.h>
#include <android/hardware/neuralnetworks/1.3/IPreparedModelCallback.h>
#include <android/hardware/neuralnetworks/1.3/types.h>
#include <nnapi/IPreparedModel.h>
#include <nnapi/Result.h>
#include <nnapi/Types.h>
#include <nnapi/hal/1.0/Callbacks.h>
#include <nnapi/hal/1.0/Conversions.h>
#include <nnapi/hal/1.0/PreparedModel.h>
#include <nnapi/hal/1.2/Callbacks.h>
#include <nnapi/hal/1.2/Conversions.h>
#include <nnapi/hal/1.2/PreparedModel.h>
#include <nnapi/hal/CommonUtils.h>
#include <nnapi/hal/HandleError.h>
#include <nnapi/hal/ProtectCallback.h>
#include <nnapi/hal/TransferValue.h>

#include <utility>

// See hardware/interfaces/neuralnetworks/utils/README.md for more information on HIDL interface
// lifetimes across processes and for protecting asynchronous calls across HIDL.

namespace android::hardware::neuralnetworks::V1_3::utils {
namespace {

nn::GeneralResult<nn::SharedPreparedModel> prepareModelCallback(
        V1_0::ErrorStatus status, const sp<V1_0::IPreparedModel>& preparedModel) {
    if (const auto dynamicPreparedModel =
                V1_3::IPreparedModel::castFrom(preparedModel).withDefault(nullptr)) {
        const auto currentVersionStatus = NN_TRY(convertFromNonCanonical(status));
        return V1_3::utils::prepareModelCallback(currentVersionStatus, dynamicPreparedModel);
    }
    if (const auto dynamicPreparedModel =
                V1_2::IPreparedModel::castFrom(preparedModel).withDefault(nullptr)) {
        return V1_2::utils::prepareModelCallback(status, dynamicPreparedModel);
    }
    return V1_0::utils::prepareModelCallback(status, preparedModel);
}

nn::GeneralResult<std::pair<std::vector<nn::OutputShape>, nn::Timing>>
convertExecutionGeneralResultsHelper(const hidl_vec<V1_2::OutputShape>& outputShapes,
                                     const V1_2::Timing& timing) {
    return std::make_pair(NN_TRY(nn::convert(outputShapes)), NN_TRY(nn::convert(timing)));
}

}  // namespace

nn::GeneralResult<std::vector<bool>> supportedOperationsCallback(
        ErrorStatus status, const hidl_vec<bool>& supportedOperations) {
    HANDLE_HAL_STATUS(status) << "get supported operations failed with " << toString(status);
    return supportedOperations;
}

nn::GeneralResult<nn::SharedPreparedModel> prepareModelCallback(
        ErrorStatus status, const sp<IPreparedModel>& preparedModel) {
    HANDLE_HAL_STATUS(status) << "model preparation failed with " << toString(status);
    return NN_TRY(PreparedModel::create(preparedModel, /*executeSynchronously=*/true));
}

nn::ExecutionResult<std::pair<std::vector<nn::OutputShape>, nn::Timing>> executionCallback(
        ErrorStatus status, const hidl_vec<V1_2::OutputShape>& outputShapes,
        const V1_2::Timing& timing) {
    if (status == ErrorStatus::OUTPUT_INSUFFICIENT_SIZE) {
        auto canonicalOutputShapes =
                nn::convert(outputShapes).value_or(std::vector<nn::OutputShape>{});
        return NN_ERROR(nn::ErrorStatus::OUTPUT_INSUFFICIENT_SIZE, std::move(canonicalOutputShapes))
               << "execution failed with " << toString(status);
    }
    HANDLE_HAL_STATUS(status) << "execution failed with " << toString(status);
    return hal::utils::makeExecutionFailure(
            convertExecutionGeneralResultsHelper(outputShapes, timing));
}

Return<void> PreparedModelCallback::notify(V1_0::ErrorStatus status,
                                           const sp<V1_0::IPreparedModel>& preparedModel) {
    mData.put(prepareModelCallback(status, preparedModel));
    return Void();
}

Return<void> PreparedModelCallback::notify_1_2(V1_0::ErrorStatus status,
                                               const sp<V1_2::IPreparedModel>& preparedModel) {
    mData.put(prepareModelCallback(status, preparedModel));
    return Void();
}

Return<void> PreparedModelCallback::notify_1_3(ErrorStatus status,
                                               const sp<IPreparedModel>& preparedModel) {
    mData.put(prepareModelCallback(status, preparedModel));
    return Void();
}

void PreparedModelCallback::notifyAsDeadObject() {
    mData.put(NN_ERROR(nn::ErrorStatus::DEAD_OBJECT) << "Dead object");
}

PreparedModelCallback::Data PreparedModelCallback::get() {
    return mData.take();
}

// ExecutionCallback methods begin here

Return<void> ExecutionCallback::notify(V1_0::ErrorStatus status) {
    mData.put(V1_0::utils::executionCallback(status));
    return Void();
}

Return<void> ExecutionCallback::notify_1_2(V1_0::ErrorStatus status,
                                           const hidl_vec<V1_2::OutputShape>& outputShapes,
                                           const V1_2::Timing& timing) {
    mData.put(V1_2::utils::executionCallback(status, outputShapes, timing));
    return Void();
}

Return<void> ExecutionCallback::notify_1_3(ErrorStatus status,
                                           const hidl_vec<V1_2::OutputShape>& outputShapes,
                                           const V1_2::Timing& timing) {
    mData.put(executionCallback(status, outputShapes, timing));
    return Void();
}

void ExecutionCallback::notifyAsDeadObject() {
    mData.put(NN_ERROR(nn::ErrorStatus::DEAD_OBJECT) << "Dead object");
}

ExecutionCallback::Data ExecutionCallback::get() {
    return mData.take();
}

}  // namespace android::hardware::neuralnetworks::V1_3::utils
