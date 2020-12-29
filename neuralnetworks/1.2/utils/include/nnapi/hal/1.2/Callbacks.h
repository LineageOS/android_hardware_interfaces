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

#ifndef ANDROID_HARDWARE_INTERFACES_NEURALNETWORKS_1_2_UTILS_CALLBACKS_H
#define ANDROID_HARDWARE_INTERFACES_NEURALNETWORKS_1_2_UTILS_CALLBACKS_H

#include <android/hardware/neuralnetworks/1.0/IExecutionCallback.h>
#include <android/hardware/neuralnetworks/1.0/IPreparedModelCallback.h>
#include <android/hardware/neuralnetworks/1.0/types.h>
#include <android/hardware/neuralnetworks/1.2/IExecutionCallback.h>
#include <android/hardware/neuralnetworks/1.2/IPreparedModelCallback.h>
#include <android/hardware/neuralnetworks/1.2/types.h>
#include <nnapi/IPreparedModel.h>
#include <nnapi/Result.h>
#include <nnapi/Types.h>
#include <nnapi/hal/1.0/Callbacks.h>
#include <nnapi/hal/CommonUtils.h>
#include <nnapi/hal/ProtectCallback.h>
#include <nnapi/hal/TransferValue.h>

// See hardware/interfaces/neuralnetworks/utils/README.md for more information on HIDL interface
// lifetimes across processes and for protecting asynchronous calls across HIDL.

namespace android::hardware::neuralnetworks::V1_2::utils {

// Converts the results of IDevice::prepareModel* to the NN canonical format. On success, this
// function returns with a non-null nn::SharedPreparedModel with a feature level of
// nn::Version::ANDROID_Q. On failure, this function returns with the appropriate nn::GeneralError.
nn::GeneralResult<nn::SharedPreparedModel> prepareModelCallback(
        V1_0::ErrorStatus status, const sp<IPreparedModel>& preparedModel);

// Converts the results of IDevice::execute* to the NN canonical format. On success, this function
// returns with the output shapes and the timing information. On failure, this function returns with
// the appropriate nn::ExecutionError.
nn::ExecutionResult<std::pair<std::vector<nn::OutputShape>, nn::Timing>> executionCallback(
        V1_0::ErrorStatus status, const hidl_vec<OutputShape>& outputShapes, const Timing& timing);

// A HIDL callback class to receive the results of IDevice::prepareModel* asynchronously.
class PreparedModelCallback final : public IPreparedModelCallback,
                                    public hal::utils::IProtectedCallback {
  public:
    using Data = nn::GeneralResult<nn::SharedPreparedModel>;

    Return<void> notify(V1_0::ErrorStatus status,
                        const sp<V1_0::IPreparedModel>& preparedModel) override;
    Return<void> notify_1_2(V1_0::ErrorStatus status,
                            const sp<IPreparedModel>& preparedModel) override;

    void notifyAsDeadObject() override;

    Data get();

  private:
    hal::utils::TransferValue<Data> mData;
};

// A HIDL callback class to receive the results of IDevice::execute_1_2 asynchronously.
class ExecutionCallback final : public IExecutionCallback, public hal::utils::IProtectedCallback {
  public:
    using Data = nn::ExecutionResult<std::pair<std::vector<nn::OutputShape>, nn::Timing>>;

    Return<void> notify(V1_0::ErrorStatus status) override;
    Return<void> notify_1_2(V1_0::ErrorStatus status, const hidl_vec<OutputShape>& outputShapes,
                            const Timing& timing) override;

    void notifyAsDeadObject() override;

    Data get();

  private:
    hal::utils::TransferValue<Data> mData;
};

}  // namespace android::hardware::neuralnetworks::V1_2::utils

#endif  // ANDROID_HARDWARE_INTERFACES_NEURALNETWORKS_1_2_UTILS_CALLBACKS_H
