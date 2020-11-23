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
#include <nnapi/hal/1.0/Conversions.h>
#include <nnapi/hal/1.0/PreparedModel.h>
#include <nnapi/hal/1.2/Conversions.h>
#include <nnapi/hal/1.2/PreparedModel.h>
#include <nnapi/hal/CommonUtils.h>
#include <nnapi/hal/HandleError.h>
#include <nnapi/hal/ProtectCallback.h>
#include <nnapi/hal/TransferValue.h>

#include <utility>

namespace android::hardware::neuralnetworks::V1_3::utils {
namespace {

nn::GeneralResult<nn::SharedPreparedModel> convertPreparedModel(
        const sp<V1_0::IPreparedModel>& preparedModel) {
    return NN_TRY(V1_0::utils::PreparedModel::create(preparedModel));
}

nn::GeneralResult<nn::SharedPreparedModel> convertPreparedModel(
        const sp<V1_2::IPreparedModel>& preparedModel) {
    return NN_TRY(V1_2::utils::PreparedModel::create(preparedModel));
}

nn::GeneralResult<nn::SharedPreparedModel> convertPreparedModel(
        const sp<IPreparedModel>& preparedModel) {
    return NN_TRY(utils::PreparedModel::create(preparedModel));
}

nn::GeneralResult<std::pair<std::vector<nn::OutputShape>, nn::Timing>>
convertExecutionGeneralResultsHelper(const hidl_vec<V1_2::OutputShape>& outputShapes,
                                     const V1_2::Timing& timing) {
    return std::make_pair(NN_TRY(nn::convert(outputShapes)), NN_TRY(nn::convert(timing)));
}

nn::ExecutionResult<std::pair<std::vector<nn::OutputShape>, nn::Timing>>
convertExecutionGeneralResults(const hidl_vec<V1_2::OutputShape>& outputShapes,
                               const V1_2::Timing& timing) {
    return hal::utils::makeExecutionFailure(
            convertExecutionGeneralResultsHelper(outputShapes, timing));
}

}  // namespace

Return<void> PreparedModelCallback::notify(V1_0::ErrorStatus status,
                                           const sp<V1_0::IPreparedModel>& preparedModel) {
    if (status != V1_0::ErrorStatus::NONE) {
        const auto canonical = nn::convert(status).value_or(nn::ErrorStatus::GENERAL_FAILURE);
        notifyInternal(NN_ERROR(canonical) << "preparedModel failed with " << toString(status));
    } else if (preparedModel == nullptr) {
        notifyInternal(NN_ERROR(nn::ErrorStatus::GENERAL_FAILURE)
                       << "Returned preparedModel is nullptr");
    } else {
        notifyInternal(convertPreparedModel(preparedModel));
    }
    return Void();
}

Return<void> PreparedModelCallback::notify_1_2(V1_0::ErrorStatus status,
                                               const sp<V1_2::IPreparedModel>& preparedModel) {
    if (status != V1_0::ErrorStatus::NONE) {
        const auto canonical = nn::convert(status).value_or(nn::ErrorStatus::GENERAL_FAILURE);
        notifyInternal(NN_ERROR(canonical) << "preparedModel failed with " << toString(status));
    } else if (preparedModel == nullptr) {
        notifyInternal(NN_ERROR(nn::ErrorStatus::GENERAL_FAILURE)
                       << "Returned preparedModel is nullptr");
    } else {
        notifyInternal(convertPreparedModel(preparedModel));
    }
    return Void();
}

Return<void> PreparedModelCallback::notify_1_3(ErrorStatus status,
                                               const sp<IPreparedModel>& preparedModel) {
    if (status != ErrorStatus::NONE) {
        const auto canonical = nn::convert(status).value_or(nn::ErrorStatus::GENERAL_FAILURE);
        notifyInternal(NN_ERROR(canonical) << "preparedModel failed with " << toString(status));
    } else if (preparedModel == nullptr) {
        notifyInternal(NN_ERROR(nn::ErrorStatus::GENERAL_FAILURE)
                       << "Returned preparedModel is nullptr");
    } else {
        notifyInternal(convertPreparedModel(preparedModel));
    }
    return Void();
}

void PreparedModelCallback::notifyAsDeadObject() {
    notifyInternal(NN_ERROR(nn::ErrorStatus::DEAD_OBJECT) << "Dead object");
}

PreparedModelCallback::Data PreparedModelCallback::get() {
    return mData.take();
}

void PreparedModelCallback::notifyInternal(PreparedModelCallback::Data result) {
    mData.put(std::move(result));
}

// ExecutionCallback methods begin here

Return<void> ExecutionCallback::notify(V1_0::ErrorStatus status) {
    if (status != V1_0::ErrorStatus::NONE) {
        const auto canonical = nn::convert(status).value_or(nn::ErrorStatus::GENERAL_FAILURE);
        notifyInternal(NN_ERROR(canonical) << "execute failed with " << toString(status));
    } else {
        notifyInternal({});
    }
    return Void();
}

Return<void> ExecutionCallback::notify_1_2(V1_0::ErrorStatus status,
                                           const hidl_vec<V1_2::OutputShape>& outputShapes,
                                           const V1_2::Timing& timing) {
    if (status != V1_0::ErrorStatus::NONE) {
        const auto canonical = nn::convert(status).value_or(nn::ErrorStatus::GENERAL_FAILURE);
        notifyInternal(NN_ERROR(canonical) << "execute failed with " << toString(status));
    } else {
        notifyInternal(convertExecutionGeneralResults(outputShapes, timing));
    }
    return Void();
}

Return<void> ExecutionCallback::notify_1_3(ErrorStatus status,
                                           const hidl_vec<V1_2::OutputShape>& outputShapes,
                                           const V1_2::Timing& timing) {
    if (status != ErrorStatus::NONE) {
        const auto canonical = nn::convert(status).value_or(nn::ErrorStatus::GENERAL_FAILURE);
        notifyInternal(NN_ERROR(canonical) << "execute failed with " << toString(status));
    } else {
        notifyInternal(convertExecutionGeneralResults(outputShapes, timing));
    }
    return Void();
}

void ExecutionCallback::notifyAsDeadObject() {
    notifyInternal(NN_ERROR(nn::ErrorStatus::DEAD_OBJECT) << "Dead object");
}

ExecutionCallback::Data ExecutionCallback::get() {
    return mData.take();
}

void ExecutionCallback::notifyInternal(ExecutionCallback::Data result) {
    mData.put(std::move(result));
}

}  // namespace android::hardware::neuralnetworks::V1_3::utils
