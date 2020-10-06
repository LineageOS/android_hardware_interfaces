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

#include "Conversions.h"

#include <android-base/logging.h>
#include <android/hardware/neuralnetworks/1.2/types.h>
#include <nnapi/OperandTypes.h>
#include <nnapi/OperationTypes.h>
#include <nnapi/Result.h>
#include <nnapi/SharedMemory.h>
#include <nnapi/TypeUtils.h>
#include <nnapi/Types.h>
#include <nnapi/hal/1.0/Conversions.h>
#include <nnapi/hal/CommonUtils.h>

#include <algorithm>
#include <functional>
#include <iterator>
#include <memory>
#include <type_traits>
#include <utility>

namespace {

template <typename Type>
constexpr std::underlying_type_t<Type> underlyingType(Type value) {
    return static_cast<std::underlying_type_t<Type>>(value);
}

}  // namespace

namespace android::nn {
namespace {

constexpr bool validOperandType(OperandType operandType) {
    switch (operandType) {
        case OperandType::FLOAT32:
        case OperandType::INT32:
        case OperandType::UINT32:
        case OperandType::TENSOR_FLOAT32:
        case OperandType::TENSOR_INT32:
        case OperandType::TENSOR_QUANT8_ASYMM:
        case OperandType::BOOL:
        case OperandType::TENSOR_QUANT16_SYMM:
        case OperandType::TENSOR_FLOAT16:
        case OperandType::TENSOR_BOOL8:
        case OperandType::FLOAT16:
        case OperandType::TENSOR_QUANT8_SYMM_PER_CHANNEL:
        case OperandType::TENSOR_QUANT16_ASYMM:
        case OperandType::TENSOR_QUANT8_SYMM:
        case OperandType::OEM:
        case OperandType::TENSOR_OEM_BYTE:
            return true;
        default:
            break;
    }
    return isExtension(operandType);
}

using hardware::hidl_handle;
using hardware::hidl_vec;

template <typename Input>
using ConvertOutput = std::decay_t<decltype(convert(std::declval<Input>()).value())>;

template <typename Type>
Result<std::vector<ConvertOutput<Type>>> convertVec(const hidl_vec<Type>& arguments) {
    std::vector<ConvertOutput<Type>> canonical;
    canonical.reserve(arguments.size());
    for (const auto& argument : arguments) {
        canonical.push_back(NN_TRY(nn::convert(argument)));
    }
    return canonical;
}

template <typename Type>
Result<std::vector<ConvertOutput<Type>>> convert(const hidl_vec<Type>& arguments) {
    return convertVec(arguments);
}

}  // anonymous namespace

Result<OperandType> convert(const hal::V1_2::OperandType& operandType) {
    return static_cast<OperandType>(operandType);
}

Result<OperationType> convert(const hal::V1_2::OperationType& operationType) {
    return static_cast<OperationType>(operationType);
}

Result<DeviceType> convert(const hal::V1_2::DeviceType& deviceType) {
    return static_cast<DeviceType>(deviceType);
}

Result<Capabilities> convert(const hal::V1_2::Capabilities& capabilities) {
    const bool validOperandTypes = std::all_of(
            capabilities.operandPerformance.begin(), capabilities.operandPerformance.end(),
            [](const hal::V1_2::Capabilities::OperandPerformance& operandPerformance) {
                const auto maybeType = convert(operandPerformance.type);
                return !maybeType.has_value() ? false : validOperandType(maybeType.value());
            });
    if (!validOperandTypes) {
        return NN_ERROR()
               << "Invalid OperandType when converting OperandPerformance in Capabilities";
    }

    const auto relaxedFloat32toFloat16PerformanceScalar =
            NN_TRY(convert(capabilities.relaxedFloat32toFloat16PerformanceScalar));
    const auto relaxedFloat32toFloat16PerformanceTensor =
            NN_TRY(convert(capabilities.relaxedFloat32toFloat16PerformanceTensor));
    auto operandPerformance = NN_TRY(convert(capabilities.operandPerformance));

    auto table =
            NN_TRY(Capabilities::OperandPerformanceTable::create(std::move(operandPerformance)));

    return Capabilities{
            .relaxedFloat32toFloat16PerformanceScalar = relaxedFloat32toFloat16PerformanceScalar,
            .relaxedFloat32toFloat16PerformanceTensor = relaxedFloat32toFloat16PerformanceTensor,
            .operandPerformance = std::move(table),
    };
}

Result<Capabilities::OperandPerformance> convert(
        const hal::V1_2::Capabilities::OperandPerformance& operandPerformance) {
    return Capabilities::OperandPerformance{
            .type = NN_TRY(convert(operandPerformance.type)),
            .info = NN_TRY(convert(operandPerformance.info)),
    };
}

Result<Operation> convert(const hal::V1_2::Operation& operation) {
    return Operation{
            .type = NN_TRY(convert(operation.type)),
            .inputs = operation.inputs,
            .outputs = operation.outputs,
    };
}

Result<Operand::SymmPerChannelQuantParams> convert(
        const hal::V1_2::SymmPerChannelQuantParams& symmPerChannelQuantParams) {
    return Operand::SymmPerChannelQuantParams{
            .scales = symmPerChannelQuantParams.scales,
            .channelDim = symmPerChannelQuantParams.channelDim,
    };
}

Result<Operand> convert(const hal::V1_2::Operand& operand) {
    return Operand{
            .type = NN_TRY(convert(operand.type)),
            .dimensions = operand.dimensions,
            .scale = operand.scale,
            .zeroPoint = operand.zeroPoint,
            .lifetime = NN_TRY(convert(operand.lifetime)),
            .location = NN_TRY(convert(operand.location)),
            .extraParams = NN_TRY(convert(operand.extraParams)),
    };
}

Result<Operand::ExtraParams> convert(const hal::V1_2::Operand::ExtraParams& extraParams) {
    using Discriminator = hal::V1_2::Operand::ExtraParams::hidl_discriminator;
    switch (extraParams.getDiscriminator()) {
        case Discriminator::none:
            return Operand::NoParams{};
        case Discriminator::channelQuant:
            return convert(extraParams.channelQuant());
        case Discriminator::extension:
            return extraParams.extension();
    }
    return NN_ERROR() << "Unrecognized Operand::ExtraParams discriminator: "
                      << underlyingType(extraParams.getDiscriminator());
}

Result<Model> convert(const hal::V1_2::Model& model) {
    auto operations = NN_TRY(convert(model.operations));

    // Verify number of consumers.
    const auto numberOfConsumers =
            hal::utils::countNumberOfConsumers(model.operands.size(), operations);
    CHECK(model.operands.size() == numberOfConsumers.size());
    for (size_t i = 0; i < model.operands.size(); ++i) {
        if (model.operands[i].numberOfConsumers != numberOfConsumers[i]) {
            return NN_ERROR() << "Invalid numberOfConsumers for operand " << i << ", expected "
                              << numberOfConsumers[i] << " but found "
                              << model.operands[i].numberOfConsumers;
        }
    }

    auto main = Model::Subgraph{
            .operands = NN_TRY(convert(model.operands)),
            .operations = std::move(operations),
            .inputIndexes = model.inputIndexes,
            .outputIndexes = model.outputIndexes,
    };

    return Model{
            .main = std::move(main),
            .operandValues = NN_TRY(convert(model.operandValues)),
            .pools = NN_TRY(convert(model.pools)),
            .relaxComputationFloat32toFloat16 = model.relaxComputationFloat32toFloat16,
            .extensionNameToPrefix = NN_TRY(convert(model.extensionNameToPrefix)),
    };
}

Result<Model::ExtensionNameAndPrefix> convert(
        const hal::V1_2::Model::ExtensionNameAndPrefix& extensionNameAndPrefix) {
    return Model::ExtensionNameAndPrefix{
            .name = extensionNameAndPrefix.name,
            .prefix = extensionNameAndPrefix.prefix,
    };
}

Result<OutputShape> convert(const hal::V1_2::OutputShape& outputShape) {
    return OutputShape{
            .dimensions = outputShape.dimensions,
            .isSufficient = outputShape.isSufficient,
    };
}

Result<MeasureTiming> convert(const hal::V1_2::MeasureTiming& measureTiming) {
    return static_cast<MeasureTiming>(measureTiming);
}

Result<Timing> convert(const hal::V1_2::Timing& timing) {
    return Timing{.timeOnDevice = timing.timeOnDevice, .timeInDriver = timing.timeInDriver};
}

Result<Extension> convert(const hal::V1_2::Extension& extension) {
    return Extension{
            .name = extension.name,
            .operandTypes = NN_TRY(convert(extension.operandTypes)),
    };
}

Result<Extension::OperandTypeInformation> convert(
        const hal::V1_2::Extension::OperandTypeInformation& operandTypeInformation) {
    return Extension::OperandTypeInformation{
            .type = operandTypeInformation.type,
            .isTensor = operandTypeInformation.isTensor,
            .byteSize = operandTypeInformation.byteSize,
    };
}

Result<NativeHandle> convert(const hidl_handle& handle) {
    auto* cloned = native_handle_clone(handle.getNativeHandle());
    return ::android::NativeHandle::create(cloned, /*ownsHandle=*/true);
}

Result<std::vector<Extension>> convert(const hidl_vec<hal::V1_2::Extension>& extensions) {
    return convertVec(extensions);
}

Result<std::vector<NativeHandle>> convert(const hidl_vec<hidl_handle>& handles) {
    return convertVec(handles);
}

Result<std::vector<OutputShape>> convert(const hidl_vec<hal::V1_2::OutputShape>& outputShapes) {
    return convertVec(outputShapes);
}

}  // namespace android::nn

namespace android::hardware::neuralnetworks::V1_2::utils {
namespace {

using utils::convert;

nn::Result<V1_0::OperandLifeTime> convert(const nn::Operand::LifeTime& lifetime) {
    return V1_0::utils::convert(lifetime);
}

nn::Result<V1_0::PerformanceInfo> convert(
        const nn::Capabilities::PerformanceInfo& performanceInfo) {
    return V1_0::utils::convert(performanceInfo);
}

nn::Result<V1_0::DataLocation> convert(const nn::DataLocation& location) {
    return V1_0::utils::convert(location);
}

nn::Result<hidl_vec<uint8_t>> convert(const nn::Model::OperandValues& operandValues) {
    return V1_0::utils::convert(operandValues);
}

nn::Result<hidl_memory> convert(const nn::Memory& memory) {
    return V1_0::utils::convert(memory);
}

template <typename Input>
using ConvertOutput = std::decay_t<decltype(convert(std::declval<Input>()).value())>;

template <typename Type>
nn::Result<hidl_vec<ConvertOutput<Type>>> convertVec(const std::vector<Type>& arguments) {
    hidl_vec<ConvertOutput<Type>> halObject(arguments.size());
    for (size_t i = 0; i < arguments.size(); ++i) {
        halObject[i] = NN_TRY(convert(arguments[i]));
    }
    return halObject;
}

template <typename Type>
nn::Result<hidl_vec<ConvertOutput<Type>>> convert(const std::vector<Type>& arguments) {
    return convertVec(arguments);
}

nn::Result<Operand::ExtraParams> makeExtraParams(nn::Operand::NoParams /*noParams*/) {
    return Operand::ExtraParams{};
}

nn::Result<Operand::ExtraParams> makeExtraParams(
        const nn::Operand::SymmPerChannelQuantParams& channelQuant) {
    Operand::ExtraParams ret;
    ret.channelQuant(NN_TRY(convert(channelQuant)));
    return ret;
}

nn::Result<Operand::ExtraParams> makeExtraParams(const nn::Operand::ExtensionParams& extension) {
    Operand::ExtraParams ret;
    ret.extension(extension);
    return ret;
}

}  // anonymous namespace

nn::Result<OperandType> convert(const nn::OperandType& operandType) {
    return static_cast<OperandType>(operandType);
}

nn::Result<OperationType> convert(const nn::OperationType& operationType) {
    return static_cast<OperationType>(operationType);
}

nn::Result<DeviceType> convert(const nn::DeviceType& deviceType) {
    switch (deviceType) {
        case nn::DeviceType::UNKNOWN:
            return NN_ERROR() << "Invalid DeviceType UNKNOWN";
        case nn::DeviceType::OTHER:
        case nn::DeviceType::CPU:
        case nn::DeviceType::GPU:
        case nn::DeviceType::ACCELERATOR:
            return static_cast<DeviceType>(deviceType);
    }
    return NN_ERROR() << "Invalid DeviceType " << underlyingType(deviceType);
}

nn::Result<Capabilities> convert(const nn::Capabilities& capabilities) {
    std::vector<nn::Capabilities::OperandPerformance> operandPerformance;
    operandPerformance.reserve(capabilities.operandPerformance.asVector().size());
    std::copy_if(capabilities.operandPerformance.asVector().begin(),
                 capabilities.operandPerformance.asVector().end(),
                 std::back_inserter(operandPerformance),
                 [](const nn::Capabilities::OperandPerformance& operandPerformance) {
                     return nn::validOperandType(operandPerformance.type);
                 });

    return Capabilities{
            .relaxedFloat32toFloat16PerformanceScalar =
                    NN_TRY(convert(capabilities.relaxedFloat32toFloat16PerformanceScalar)),
            .relaxedFloat32toFloat16PerformanceTensor =
                    NN_TRY(convert(capabilities.relaxedFloat32toFloat16PerformanceTensor)),
            .operandPerformance = NN_TRY(convert(operandPerformance)),
    };
}

nn::Result<Capabilities::OperandPerformance> convert(
        const nn::Capabilities::OperandPerformance& operandPerformance) {
    return Capabilities::OperandPerformance{
            .type = NN_TRY(convert(operandPerformance.type)),
            .info = NN_TRY(convert(operandPerformance.info)),
    };
}

nn::Result<Operation> convert(const nn::Operation& operation) {
    return Operation{
            .type = NN_TRY(convert(operation.type)),
            .inputs = operation.inputs,
            .outputs = operation.outputs,
    };
}

nn::Result<SymmPerChannelQuantParams> convert(
        const nn::Operand::SymmPerChannelQuantParams& symmPerChannelQuantParams) {
    return SymmPerChannelQuantParams{
            .scales = symmPerChannelQuantParams.scales,
            .channelDim = symmPerChannelQuantParams.channelDim,
    };
}

nn::Result<Operand> convert(const nn::Operand& operand) {
    return Operand{
            .type = NN_TRY(convert(operand.type)),
            .dimensions = operand.dimensions,
            .numberOfConsumers = 0,
            .scale = operand.scale,
            .zeroPoint = operand.zeroPoint,
            .lifetime = NN_TRY(convert(operand.lifetime)),
            .location = NN_TRY(convert(operand.location)),
            .extraParams = NN_TRY(convert(operand.extraParams)),
    };
}

nn::Result<Operand::ExtraParams> convert(const nn::Operand::ExtraParams& extraParams) {
    return std::visit([](const auto& x) { return makeExtraParams(x); }, extraParams);
}

nn::Result<Model> convert(const nn::Model& model) {
    if (!hal::utils::hasNoPointerData(model)) {
        return NN_ERROR() << "Model cannot be converted because it contains pointer-based memory";
    }

    auto operands = NN_TRY(convert(model.main.operands));

    // Update number of consumers.
    const auto numberOfConsumers =
            hal::utils::countNumberOfConsumers(operands.size(), model.main.operations);
    CHECK(operands.size() == numberOfConsumers.size());
    for (size_t i = 0; i < operands.size(); ++i) {
        operands[i].numberOfConsumers = numberOfConsumers[i];
    }

    return Model{
            .operands = std::move(operands),
            .operations = NN_TRY(convert(model.main.operations)),
            .inputIndexes = model.main.inputIndexes,
            .outputIndexes = model.main.outputIndexes,
            .operandValues = NN_TRY(convert(model.operandValues)),
            .pools = NN_TRY(convert(model.pools)),
            .relaxComputationFloat32toFloat16 = model.relaxComputationFloat32toFloat16,
            .extensionNameToPrefix = NN_TRY(convert(model.extensionNameToPrefix)),
    };
}

nn::Result<Model::ExtensionNameAndPrefix> convert(
        const nn::Model::ExtensionNameAndPrefix& extensionNameAndPrefix) {
    return Model::ExtensionNameAndPrefix{
            .name = extensionNameAndPrefix.name,
            .prefix = extensionNameAndPrefix.prefix,
    };
}

nn::Result<OutputShape> convert(const nn::OutputShape& outputShape) {
    return OutputShape{.dimensions = outputShape.dimensions,
                       .isSufficient = outputShape.isSufficient};
}

nn::Result<MeasureTiming> convert(const nn::MeasureTiming& measureTiming) {
    return static_cast<MeasureTiming>(measureTiming);
}

nn::Result<Timing> convert(const nn::Timing& timing) {
    return Timing{.timeOnDevice = timing.timeOnDevice, .timeInDriver = timing.timeInDriver};
}

nn::Result<Extension> convert(const nn::Extension& extension) {
    return Extension{
            .name = extension.name,
            .operandTypes = NN_TRY(convert(extension.operandTypes)),
    };
}

nn::Result<Extension::OperandTypeInformation> convert(
        const nn::Extension::OperandTypeInformation& operandTypeInformation) {
    return Extension::OperandTypeInformation{
            .type = operandTypeInformation.type,
            .isTensor = operandTypeInformation.isTensor,
            .byteSize = operandTypeInformation.byteSize,
    };
}

nn::Result<hidl_handle> convert(const nn::NativeHandle& handle) {
    const auto hidlHandle = hidl_handle(handle->handle());
    // Copy memory to force the native_handle_t to be copied.
    auto copiedHandle = hidlHandle;
    return copiedHandle;
}

nn::Result<hidl_vec<Extension>> convert(const std::vector<nn::Extension>& extensions) {
    return convertVec(extensions);
}

nn::Result<hidl_vec<hidl_handle>> convert(const std::vector<nn::NativeHandle>& handles) {
    return convertVec(handles);
}

nn::Result<hidl_vec<OutputShape>> convert(const std::vector<nn::OutputShape>& outputShapes) {
    return convertVec(outputShapes);
}

}  // namespace android::hardware::neuralnetworks::V1_2::utils
