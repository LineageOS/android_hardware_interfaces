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

#include <android/hardware/neuralnetworks/1.0/IExecutionCallback.h>
#include <android/hardware/neuralnetworks/1.0/IPreparedModelCallback.h>
#include <android/hardware/neuralnetworks/1.0/types.h>
#include <nnapi/IPreparedModel.h>
#include <nnapi/Result.h>
#include <nnapi/Types.h>
#include <nnapi/hal/CommonUtils.h>
#include <nnapi/hal/ProtectCallback.h>
#include <nnapi/hal/TransferValue.h>

#include <utility>

namespace android::hardware::neuralnetworks::V1_0::utils {
namespace {

nn::GeneralResult<nn::SharedPreparedModel> convertPreparedModel(
        const sp<IPreparedModel>& preparedModel) {
    return NN_TRY(utils::PreparedModel::create(preparedModel));
}

}  // namespace

Return<void> PreparedModelCallback::notify(ErrorStatus status,
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

Return<void> ExecutionCallback::notify(ErrorStatus status) {
    if (status != ErrorStatus::NONE) {
        const auto canonical = nn::convert(status).value_or(nn::ErrorStatus::GENERAL_FAILURE);
        notifyInternal(NN_ERROR(canonical) << "execute failed with " << toString(status));
    } else {
        notifyInternal({});
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

}  // namespace android::hardware::neuralnetworks::V1_0::utils
