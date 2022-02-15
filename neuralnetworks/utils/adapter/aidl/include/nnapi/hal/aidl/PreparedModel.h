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

#ifndef ANDROID_HARDWARE_INTERFACES_NEURALNETWORKS_UTILS_ADAPTER_AIDL_PREPARED_MDOEL_H
#define ANDROID_HARDWARE_INTERFACES_NEURALNETWORKS_UTILS_ADAPTER_AIDL_PREPARED_MDOEL_H

#include "nnapi/hal/aidl/Adapter.h"

#include <aidl/android/hardware/neuralnetworks/BnPreparedModel.h>
#include <aidl/android/hardware/neuralnetworks/ExecutionResult.h>
#include <aidl/android/hardware/neuralnetworks/FencedExecutionResult.h>
#include <aidl/android/hardware/neuralnetworks/IBurst.h>
#include <aidl/android/hardware/neuralnetworks/IExecution.h>
#include <aidl/android/hardware/neuralnetworks/Request.h>
#include <android/binder_auto_utils.h>
#include <nnapi/IPreparedModel.h>
#include <nnapi/Types.h>

#include <memory>
#include <vector>

// See hardware/interfaces/neuralnetworks/utils/README.md for more information on AIDL interface
// lifetimes across processes and for protecting asynchronous calls across AIDL.

namespace aidl::android::hardware::neuralnetworks::adapter {

// Class that adapts nn::IPreparedModel to BnPreparedModel.
class PreparedModel : public BnPreparedModel {
  public:
    explicit PreparedModel(::android::nn::SharedPreparedModel preparedModel);

    ndk::ScopedAStatus executeSynchronously(const Request& request, bool measureTiming,
                                            int64_t deadlineNs, int64_t loopTimeoutDurationNs,
                                            ExecutionResult* executionResult) override;
    ndk::ScopedAStatus executeFenced(const Request& request,
                                     const std::vector<ndk::ScopedFileDescriptor>& waitFor,
                                     bool measureTiming, int64_t deadlineNs,
                                     int64_t loopTimeoutDurationNs, int64_t durationNs,
                                     FencedExecutionResult* executionResult) override;
    ndk::ScopedAStatus configureExecutionBurst(std::shared_ptr<IBurst>* burst) override;
    ndk::ScopedAStatus createReusableExecution(const Request& request,
                                               const ExecutionConfig& config,
                                               std::shared_ptr<IExecution>* execution) override;
    ndk::ScopedAStatus executeSynchronouslyWithConfig(const Request& request,
                                                      const ExecutionConfig& config,
                                                      int64_t deadlineNs,
                                                      ExecutionResult* executionResult) override;
    ndk::ScopedAStatus executeFencedWithConfig(
            const Request& request, const std::vector<ndk::ScopedFileDescriptor>& waitFor,
            const ExecutionConfig& config, int64_t deadlineNs, int64_t durationNs,
            FencedExecutionResult* executionResult) override;

    ::android::nn::SharedPreparedModel getUnderlyingPreparedModel() const;

  protected:
    const ::android::nn::SharedPreparedModel kPreparedModel;
};

}  // namespace aidl::android::hardware::neuralnetworks::adapter

#endif  // ANDROID_HARDWARE_INTERFACES_NEURALNETWORKS_UTILS_ADAPTER_AIDL_PREPARED_MDOEL_H
