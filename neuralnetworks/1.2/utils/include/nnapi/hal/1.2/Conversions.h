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

#ifndef ANDROID_HARDWARE_INTERFACES_NEURALNETWORKS_1_2_CONVERSIONS_H
#define ANDROID_HARDWARE_INTERFACES_NEURALNETWORKS_1_2_CONVERSIONS_H

#include <android/hardware/neuralnetworks/1.2/types.h>
#include <nnapi/Result.h>
#include <nnapi/Types.h>
#include <nnapi/hal/CommonUtils.h>

namespace android::nn {

GeneralResult<OperandType> convert(const hal::V1_2::OperandType& operandType);
GeneralResult<OperationType> convert(const hal::V1_2::OperationType& operationType);
GeneralResult<DeviceType> convert(const hal::V1_2::DeviceType& deviceType);
GeneralResult<Capabilities> convert(const hal::V1_2::Capabilities& capabilities);
GeneralResult<Capabilities::OperandPerformance> convert(
        const hal::V1_2::Capabilities::OperandPerformance& operandPerformance);
GeneralResult<Operation> convert(const hal::V1_2::Operation& operation);
GeneralResult<Operand::SymmPerChannelQuantParams> convert(
        const hal::V1_2::SymmPerChannelQuantParams& symmPerChannelQuantParams);
GeneralResult<Operand> convert(const hal::V1_2::Operand& operand);
GeneralResult<Operand::ExtraParams> convert(const hal::V1_2::Operand::ExtraParams& extraParams);
GeneralResult<Model> convert(const hal::V1_2::Model& model);
GeneralResult<Model::ExtensionNameAndPrefix> convert(
        const hal::V1_2::Model::ExtensionNameAndPrefix& extensionNameAndPrefix);
GeneralResult<OutputShape> convert(const hal::V1_2::OutputShape& outputShape);
GeneralResult<MeasureTiming> convert(const hal::V1_2::MeasureTiming& measureTiming);
GeneralResult<Timing> convert(const hal::V1_2::Timing& timing);
GeneralResult<Extension> convert(const hal::V1_2::Extension& extension);
GeneralResult<Extension::OperandTypeInformation> convert(
        const hal::V1_2::Extension::OperandTypeInformation& operandTypeInformation);
GeneralResult<NativeHandle> convert(const hardware::hidl_handle& handle);

GeneralResult<std::vector<Extension>> convert(
        const hardware::hidl_vec<hal::V1_2::Extension>& extensions);
GeneralResult<std::vector<NativeHandle>> convert(
        const hardware::hidl_vec<hardware::hidl_handle>& handles);
GeneralResult<std::vector<OutputShape>> convert(
        const hardware::hidl_vec<hal::V1_2::OutputShape>& outputShapes);

}  // namespace android::nn

namespace android::hardware::neuralnetworks::V1_2::utils {

nn::GeneralResult<OperandType> convert(const nn::OperandType& operandType);
nn::GeneralResult<OperationType> convert(const nn::OperationType& operationType);
nn::GeneralResult<DeviceType> convert(const nn::DeviceType& deviceType);
nn::GeneralResult<Capabilities> convert(const nn::Capabilities& capabilities);
nn::GeneralResult<Capabilities::OperandPerformance> convert(
        const nn::Capabilities::OperandPerformance& operandPerformance);
nn::GeneralResult<Operation> convert(const nn::Operation& operation);
nn::GeneralResult<SymmPerChannelQuantParams> convert(
        const nn::Operand::SymmPerChannelQuantParams& symmPerChannelQuantParams);
nn::GeneralResult<Operand> convert(const nn::Operand& operand);
nn::GeneralResult<Operand::ExtraParams> convert(const nn::Operand::ExtraParams& extraParams);
nn::GeneralResult<Model> convert(const nn::Model& model);
nn::GeneralResult<Model::ExtensionNameAndPrefix> convert(
        const nn::Model::ExtensionNameAndPrefix& extensionNameAndPrefix);
nn::GeneralResult<OutputShape> convert(const nn::OutputShape& outputShape);
nn::GeneralResult<MeasureTiming> convert(const nn::MeasureTiming& measureTiming);
nn::GeneralResult<Timing> convert(const nn::Timing& timing);
nn::GeneralResult<Extension> convert(const nn::Extension& extension);
nn::GeneralResult<Extension::OperandTypeInformation> convert(
        const nn::Extension::OperandTypeInformation& operandTypeInformation);
nn::GeneralResult<hidl_handle> convert(const nn::NativeHandle& handle);

nn::GeneralResult<hidl_vec<Extension>> convert(const std::vector<nn::Extension>& extensions);
nn::GeneralResult<hidl_vec<hidl_handle>> convert(const std::vector<nn::NativeHandle>& handles);
nn::GeneralResult<hidl_vec<OutputShape>> convert(const std::vector<nn::OutputShape>& outputShapes);

}  // namespace android::hardware::neuralnetworks::V1_2::utils

#endif  // ANDROID_HARDWARE_INTERFACES_NEURALNETWORKS_1_2_CONVERSIONS_H
