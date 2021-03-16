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

#include "Callbacks.h"

#include "Conversions.h"
#include "PreparedModel.h"
#include "ProtectCallback.h"
#include "Utils.h"

#include <nnapi/IPreparedModel.h>
#include <nnapi/Result.h>
#include <nnapi/Types.h>

#include <utility>

// See hardware/interfaces/neuralnetworks/utils/README.md for more information on AIDL interface
// lifetimes across processes and for protecting asynchronous calls across AIDL.

namespace aidl::android::hardware::neuralnetworks::utils {
namespace {

// Converts the results of IDevice::prepareModel* to the NN canonical format. On success, this
// function returns with a non-null nn::SharedPreparedModel with a feature level of
// nn::Version::ANDROID_S. On failure, this function returns with the appropriate nn::GeneralError.
nn::GeneralResult<nn::SharedPreparedModel> prepareModelCallback(
        ErrorStatus status, const std::shared_ptr<IPreparedModel>& preparedModel) {
    HANDLE_HAL_STATUS(status) << "model preparation failed with " << toString(status);
    return NN_TRY(PreparedModel::create(preparedModel));
}

}  // namespace

ndk::ScopedAStatus PreparedModelCallback::notify(
        ErrorStatus status, const std::shared_ptr<IPreparedModel>& preparedModel) {
    mData.put(prepareModelCallback(status, preparedModel));
    return ndk::ScopedAStatus::ok();
}

void PreparedModelCallback::notifyAsDeadObject() {
    mData.put(NN_ERROR(nn::ErrorStatus::DEAD_OBJECT) << "Dead object");
}

PreparedModelCallback::Data PreparedModelCallback::get() {
    return mData.take();
}

}  // namespace aidl::android::hardware::neuralnetworks::utils
