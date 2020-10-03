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

#ifndef ANDROID_HARDWARE_INTERFACES_NEURALNETWORKS_1_1_CONVERSIONS_H
#define ANDROID_HARDWARE_INTERFACES_NEURALNETWORKS_1_1_CONVERSIONS_H

#include <android/hardware/neuralnetworks/1.1/types.h>
#include <nnapi/Result.h>
#include <nnapi/Types.h>
#include <nnapi/hal/CommonUtils.h>

namespace android::nn {

Result<OperationType> convert(const hal::V1_1::OperationType& operationType);
Result<Capabilities> convert(const hal::V1_1::Capabilities& capabilities);
Result<Operation> convert(const hal::V1_1::Operation& operation);
Result<Model> convert(const hal::V1_1::Model& model);
Result<ExecutionPreference> convert(const hal::V1_1::ExecutionPreference& executionPreference);

}  // namespace android::nn

namespace android::hardware::neuralnetworks::V1_1::utils {

nn::Result<OperationType> convert(const nn::OperationType& operationType);
nn::Result<Capabilities> convert(const nn::Capabilities& capabilities);
nn::Result<Operation> convert(const nn::Operation& operation);
nn::Result<Model> convert(const nn::Model& model);
nn::Result<ExecutionPreference> convert(const nn::ExecutionPreference& executionPreference);

}  // namespace android::hardware::neuralnetworks::V1_1::utils

#endif  // ANDROID_HARDWARE_INTERFACES_NEURALNETWORKS_1_1_CONVERSIONS_H
