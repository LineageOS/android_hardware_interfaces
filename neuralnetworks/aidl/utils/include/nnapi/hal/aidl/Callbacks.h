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

#ifndef ANDROID_HARDWARE_INTERFACES_NEURALNETWORKS_AIDL_UTILS_CALLBACKS_H
#define ANDROID_HARDWARE_INTERFACES_NEURALNETWORKS_AIDL_UTILS_CALLBACKS_H

#include <aidl/android/hardware/neuralnetworks/BnPreparedModelCallback.h>
#include <aidl/android/hardware/neuralnetworks/IDevice.h>
#include <nnapi/IPreparedModel.h>
#include <nnapi/Result.h>
#include <nnapi/Types.h>
#include <nnapi/hal/CommonUtils.h>
#include <nnapi/hal/TransferValue.h>
#include <nnapi/hal/aidl/ProtectCallback.h>

// See hardware/interfaces/neuralnetworks/utils/README.md for more information on AIDL interface
// lifetimes across processes and for protecting asynchronous calls across AIDL.

namespace aidl::android::hardware::neuralnetworks::utils {

// An AIDL callback class to receive the results of IDevice::prepareModel* asynchronously.
class PreparedModelCallback final : public BnPreparedModelCallback,
                                    public hal::utils::IProtectedCallback {
  public:
    using Data = nn::GeneralResult<nn::SharedPreparedModel>;

    ndk::ScopedAStatus notify(ErrorStatus status,
                              const std::shared_ptr<IPreparedModel>& preparedModel) override;

    void notifyAsDeadObject() override;

    Data get();

  private:
    hal::utils::TransferValue<Data> mData;
};

}  // namespace aidl::android::hardware::neuralnetworks::utils

#endif  // ANDROID_HARDWARE_INTERFACES_NEURALNETWORKS_AIDL_UTILS_CALLBACKS_H
