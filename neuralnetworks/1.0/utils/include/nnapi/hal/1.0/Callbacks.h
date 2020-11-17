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

namespace android::hardware::neuralnetworks::V1_0::utils {

class PreparedModelCallback final : public IPreparedModelCallback,
                                    public hal::utils::IProtectedCallback {
  public:
    using Data = nn::GeneralResult<nn::SharedPreparedModel>;

    Return<void> notify(ErrorStatus status, const sp<IPreparedModel>& preparedModel) override;

    void notifyAsDeadObject() override;

    Data get();

  private:
    void notifyInternal(Data result);

    hal::utils::TransferValue<Data> mData;
};

class ExecutionCallback final : public IExecutionCallback, public hal::utils::IProtectedCallback {
  public:
    using Data = nn::ExecutionResult<std::pair<std::vector<nn::OutputShape>, nn::Timing>>;

    Return<void> notify(ErrorStatus status) override;

    void notifyAsDeadObject() override;

    Data get();

  private:
    void notifyInternal(Data result);

    hal::utils::TransferValue<Data> mData;
};

}  // namespace android::hardware::neuralnetworks::V1_0::utils

#endif  // ANDROID_HARDWARE_INTERFACES_NEURALNETWORKS_1_0_UTILS_CALLBACKS_H
