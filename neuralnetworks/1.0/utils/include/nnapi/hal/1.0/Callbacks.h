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

#ifndef ANDROID_HARDWARE_INTERFACES_NEURALNETWORKS_1_0_UTILS_CALLBACKS_H
#define ANDROID_HARDWARE_INTERFACES_NEURALNETWORKS_1_0_UTILS_CALLBACKS_H

#include <android/hardware/neuralnetworks/1.0/IExecutionCallback.h>
#include <android/hardware/neuralnetworks/1.0/IPreparedModelCallback.h>
#include <android/hardware/neuralnetworks/1.0/types.h>
#include <nnapi/IPreparedModel.h>
#include <nnapi/Result.h>
#include <nnapi/Types.h>
#include <nnapi/hal/CommonUtils.h>
#include <nnapi/hal/ProtectCallback.h>
#include <nnapi/hal/TransferValue.h>

// See hardware/interfaces/neuralnetworks/utils/README.md for more information on HIDL interface
// lifetimes across processes and for protecting asynchronous calls across HIDL.

namespace android::hardware::neuralnetworks::V1_0::utils {

// Converts the results of IDevice::getSupportedOperations* to the NN canonical format. On success,
// this function returns with the supported operations as indicated by a driver. On failure, this
// function returns with the appropriate nn::GeneralError.
nn::GeneralResult<std::vector<bool>> supportedOperationsCallback(
        ErrorStatus status, const hidl_vec<bool>& supportedOperations);

// Converts the results of IDevice::prepareModel* to the NN canonical format. On success, this
// function returns with a non-null nn::SharedPreparedModel with a feature level of
// nn::Version::ANDROID_OC_MR1. On failure, this function returns with the appropriate
// nn::GeneralError.
nn::GeneralResult<nn::SharedPreparedModel> prepareModelCallback(
        ErrorStatus status, const sp<IPreparedModel>& preparedModel);

// Converts the results of IDevice::execute* to the NN canonical format. On success, this function
// returns with an empty output shape vector and no timing information. On failure, this function
// returns with the appropriate nn::ExecutionError.
nn::ExecutionResult<std::pair<std::vector<nn::OutputShape>, nn::Timing>> executionCallback(
        ErrorStatus status);

// A HIDL callback class to receive the results of IDevice::prepareModel asynchronously.
class PreparedModelCallback final : public IPreparedModelCallback,
                                    public hal::utils::IProtectedCallback {
  public:
    using Data = nn::GeneralResult<nn::SharedPreparedModel>;

    Return<void> notify(ErrorStatus status, const sp<IPreparedModel>& preparedModel) override;

    void notifyAsDeadObject() override;

    Data get();

  private:
    hal::utils::TransferValue<Data> mData;
};

// A HIDL callback class to receive the results of IDevice::execute asynchronously.
class ExecutionCallback final : public IExecutionCallback, public hal::utils::IProtectedCallback {
  public:
    using Data = nn::ExecutionResult<std::pair<std::vector<nn::OutputShape>, nn::Timing>>;

    Return<void> notify(ErrorStatus status) override;

    void notifyAsDeadObject() override;

    Data get();

  private:
    hal::utils::TransferValue<Data> mData;
};

}  // namespace android::hardware::neuralnetworks::V1_0::utils

#endif  // ANDROID_HARDWARE_INTERFACES_NEURALNETWORKS_1_0_UTILS_CALLBACKS_H
