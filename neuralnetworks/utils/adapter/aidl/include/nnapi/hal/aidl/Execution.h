/*
 * Copyright (C) 2022 The Android Open Source Project
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

#ifndef ANDROID_HARDWARE_INTERFACES_NEURALNETWORKS_UTILS_ADAPTER_AIDL_EXECUTION_H
#define ANDROID_HARDWARE_INTERFACES_NEURALNETWORKS_UTILS_ADAPTER_AIDL_EXECUTION_H

#include "nnapi/hal/aidl/Adapter.h"

#include <aidl/android/hardware/neuralnetworks/BnExecution.h>
#include <aidl/android/hardware/neuralnetworks/ExecutionResult.h>
#include <aidl/android/hardware/neuralnetworks/FencedExecutionResult.h>
#include <aidl/android/hardware/neuralnetworks/IExecution.h>
#include <aidl/android/hardware/neuralnetworks/Request.h>
#include <android/binder_auto_utils.h>
#include <nnapi/IExecution.h>
#include <nnapi/Types.h>

#include <memory>
#include <vector>

// See hardware/interfaces/neuralnetworks/utils/README.md for more information on AIDL interface
// lifetimes across processes and for protecting asynchronous calls across AIDL.

namespace aidl::android::hardware::neuralnetworks::adapter {

// Class that adapts nn::IExecution to BnExecution.
class Execution : public BnExecution {
  public:
    explicit Execution(::android::nn::SharedExecution execution);

    ndk::ScopedAStatus executeSynchronously(int64_t deadlineNs,
                                            ExecutionResult* executionResult) override;
    ndk::ScopedAStatus executeFenced(const std::vector<ndk::ScopedFileDescriptor>& waitFor,
                                     int64_t deadlineNs, int64_t durationNs,
                                     FencedExecutionResult* fencedExecutionResult) override;

  protected:
    const ::android::nn::SharedExecution kExecution;
};

}  // namespace aidl::android::hardware::neuralnetworks::adapter

#endif  // ANDROID_HARDWARE_INTERFACES_NEURALNETWORKS_UTILS_ADAPTER_AIDL_EXECUTION_H
