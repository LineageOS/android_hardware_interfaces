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

#ifndef ANDROID_HARDWARE_INTERFACES_NEURALNETWORKS_UTILS_ADAPTER_PREPARED_MODEL_H
#define ANDROID_HARDWARE_INTERFACES_NEURALNETWORKS_UTILS_ADAPTER_PREPARED_MODEL_H

#include "nnapi/hal/Adapter.h"

#include <android/hardware/neuralnetworks/1.0/IExecutionCallback.h>
#include <android/hardware/neuralnetworks/1.0/types.h>
#include <android/hardware/neuralnetworks/1.2/IBurstCallback.h>
#include <android/hardware/neuralnetworks/1.2/IExecutionCallback.h>
#include <android/hardware/neuralnetworks/1.2/types.h>
#include <android/hardware/neuralnetworks/1.3/IExecutionCallback.h>
#include <android/hardware/neuralnetworks/1.3/IPreparedModel.h>
#include <android/hardware/neuralnetworks/1.3/types.h>
#include <nnapi/IPreparedModel.h>
#include <nnapi/Types.h>
#include <memory>

// See hardware/interfaces/neuralnetworks/utils/README.md for more information on HIDL interface
// lifetimes across processes and for protecting asynchronous calls across HIDL.

namespace android::hardware::neuralnetworks::adapter {

// Class that adapts nn::IPreparedModel to V1_3::IPreparedModel.
class PreparedModel final : public V1_3::IPreparedModel {
  public:
    PreparedModel(nn::SharedPreparedModel preparedModel, Executor executor, uid_t userId);

    Return<V1_0::ErrorStatus> execute(const V1_0::Request& request,
                                      const sp<V1_0::IExecutionCallback>& callback) override;
    Return<V1_0::ErrorStatus> execute_1_2(const V1_0::Request& request, V1_2::MeasureTiming measure,
                                          const sp<V1_2::IExecutionCallback>& callback) override;
    Return<V1_3::ErrorStatus> execute_1_3(const V1_3::Request& request, V1_2::MeasureTiming measure,
                                          const V1_3::OptionalTimePoint& deadline,
                                          const V1_3::OptionalTimeoutDuration& loopTimeoutDuration,
                                          const sp<V1_3::IExecutionCallback>& callback) override;
    Return<void> executeSynchronously(const V1_0::Request& request, V1_2::MeasureTiming measure,
                                      executeSynchronously_cb cb) override;
    Return<void> executeSynchronously_1_3(const V1_3::Request& request, V1_2::MeasureTiming measure,
                                          const V1_3::OptionalTimePoint& deadline,
                                          const V1_3::OptionalTimeoutDuration& loopTimeoutDuration,
                                          executeSynchronously_1_3_cb cb) override;
    Return<void> configureExecutionBurst(
            const sp<V1_2::IBurstCallback>& callback,
            const MQDescriptorSync<V1_2::FmqRequestDatum>& requestChannel,
            const MQDescriptorSync<V1_2::FmqResultDatum>& resultChannel,
            configureExecutionBurst_cb cb) override;
    Return<void> executeFenced(const V1_3::Request& request, const hidl_vec<hidl_handle>& waitFor,
                               V1_2::MeasureTiming measure, const V1_3::OptionalTimePoint& deadline,
                               const V1_3::OptionalTimeoutDuration& loopTimeoutDuration,
                               const V1_3::OptionalTimeoutDuration& duration,
                               executeFenced_cb callback) override;

    nn::SharedPreparedModel getUnderlyingPreparedModel() const;

  private:
    const nn::SharedPreparedModel kPreparedModel;
    const Executor kExecutor;
    const uid_t kUserId;
};

}  // namespace android::hardware::neuralnetworks::adapter

#endif  // ANDROID_HARDWARE_INTERFACES_NEURALNETWORKS_UTILS_ADAPTER_PREPARED_MODEL_H
