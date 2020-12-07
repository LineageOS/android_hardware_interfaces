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
#include <nnapi/hal/HandleError.h>
#include <nnapi/hal/ProtectCallback.h>
#include <nnapi/hal/TransferValue.h>

#include <utility>

// See hardware/interfaces/neuralnetworks/utils/README.md for more information on HIDL interface
// lifetimes across processes and for protecting asynchronous calls across HIDL.

namespace android::hardware::neuralnetworks::V1_0::utils {

nn::GeneralResult<std::vector<bool>> supportedOperationsCallback(
        ErrorStatus status, const hidl_vec<bool>& supportedOperations) {
    HANDLE_HAL_STATUS(status) << "get supported operations failed with " << toString(status);
    return supportedOperations;
}

nn::GeneralResult<nn::SharedPreparedModel> prepareModelCallback(
        ErrorStatus status, const sp<IPreparedModel>& preparedModel) {
    HANDLE_HAL_STATUS(status) << "model preparation failed with " << toString(status);
    return NN_TRY(PreparedModel::create(preparedModel));
}

nn::ExecutionResult<std::pair<std::vector<nn::OutputShape>, nn::Timing>> executionCallback(
        ErrorStatus status) {
    HANDLE_HAL_STATUS(status) << "execution failed with " << toString(status);
    return {};
}

Return<void> PreparedModelCallback::notify(ErrorStatus status,
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

Return<void> ExecutionCallback::notify(ErrorStatus status) {
    mData.put(executionCallback(status));
    return Void();
}

void ExecutionCallback::notifyAsDeadObject() {
    mData.put(NN_ERROR(nn::ErrorStatus::DEAD_OBJECT) << "Dead object");
}

ExecutionCallback::Data ExecutionCallback::get() {
    return mData.take();
}

}  // namespace android::hardware::neuralnetworks::V1_0::utils
