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
#include <nnapi/Validation.h>
#include <nnapi/hal/1.0/Conversions.h>
#include <nnapi/hal/1.1/Conversions.h>
#include <nnapi/hal/CommonUtils.h>
#include <nnapi/hal/HandleError.h>

#include <algorithm>
#include <functional>
#include <iterator>
#include <memory>
#include <type_traits>
#include <utility>

#include "Utils.h"

namespace {

template <typename Type>
constexpr std::underlying_type_t<Type> underlyingType(Type value) {
    return static_cast<std::underlying_type_t<Type>>(value);
}

using HalDuration = std::chrono::duration<uint64_t, std::micro>;

}  // namespace

namespace android::nn {
namespace {

using hardware::hidl_handle;
using hardware::hidl_vec;

template <typename Input>
using UnvalidatedConvertOutput =
        std::decay_t<decltype(unvalidatedConvert(std::declval<Input>()).value())>;

template <typename Type>
GeneralResult<std::vector<UnvalidatedConvertOutput<Type>>> unvalidatedConvert(
        const hidl_vec<Type>& arguments) {
    std::vector<UnvalidatedConvertOutput<Type>> canonical;
    canonical.reserve(arguments.size());
    for (const auto& argument : arguments) {
        canonical.push_back(NN_TRY(nn::unvalidatedConvert(argument)));
    }
    return canonical;
}

template <typename Type>
GeneralResult<UnvalidatedConvertOutput<Type>> validatedConvert(const Type& halObject) {
    auto canonical = NN_TRY(nn::unvalidatedConvert(halObject));
    NN_TRY(hal::V1_2::utils::compliantVersion(canonical));
    return canonical;
}

template <typename Type>
GeneralResult<std::vector<UnvalidatedConvertOutput<Type>>> validatedConvert(
        const hidl_vec<Type>& arguments) {
    std::vector<UnvalidatedConvertOutput<Type>> canonical;
    canonical.reserve(arguments.size());
    for (const auto& argument : arguments) {
        canonical.push_back(NN_TRY(validatedConvert(argument)));
    }
    return canonical;
}

}  // anonymous namespace

GeneralResult<OperandType> unvalidatedConvert(const hal::V1_2::OperandType& operandType) {
    return static_cast<OperandType>(operandType);
}

GeneralResult<OperationType> unvalidatedConvert(const hal::V1_2::OperationType& operationType) {
    return static_cast<OperationType>(operationType);
}

GeneralResult<DeviceType> unvalidatedConvert(const hal::V1_2::DeviceType& deviceType) {
    return static_cast<DeviceType>(deviceType);
}

GeneralResult<Capabilities> unvalidatedConvert(const hal::V1_2::Capabilities& capabilities) {
    const bool validOperandTypes = std::all_of(
            capabilities.operandPerformance.begin(), capabilities.operandPerformance.end(),
            [](const hal::V1_2::Capabilities::OperandPerformance& operandPerformance) {
                return validatedConvert(operandPerformance.type).has_value();
            });
    if (!validOperandTypes) {
        return NN_ERROR(nn::ErrorStatus::GENERAL_FAILURE)
               << "Invalid OperandType when converting OperandPerformance in Capabilities";
    }

    const auto relaxedFloat32toFloat16PerformanceScalar =
            NN_TRY(unvalidatedConvert(capabilities.relaxedFloat32toFloat16PerformanceScalar));
    const auto relaxedFloat32toFloat16PerformanceTensor =
            NN_TRY(unvalidatedConvert(capabilities.relaxedFloat32toFloat16PerformanceTensor));
    auto operandPerformance = NN_TRY(unvalidatedConvert(capabilities.operandPerformance));

    auto table = NN_TRY(hal::utils::makeGeneralFailure(
            Capabilities::OperandPerformanceTable::create(std::move(operandPerformance)),
            nn::ErrorStatus::GENERAL_FAILURE));

    return Capabilities{
            .relaxedFloat32toFloat16PerformanceScalar = relaxedFloat32toFloat16PerformanceScalar,
            .relaxedFloat32toFloat16PerformanceTensor = relaxedFloat32toFloat16PerformanceTensor,
            .operandPerformance = std::move(table),
    };
}

GeneralResult<Capabilities::OperandPerformance> unvalidatedConvert(
        const hal::V1_2::Capabilities::OperandPerformance& operandPerformance) {
    return Capabilities::OperandPerformance{
            .type = NN_TRY(unvalidatedConvert(operandPerformance.type)),
            .info = NN_TRY(unvalidatedConvert(operandPerformance.info)),
    };
}

GeneralResult<Operation> unvalidatedConvert(const hal::V1_2::Operation& operation) {
    return Operation{
            .type = NN_TRY(unvalidatedConvert(operation.type)),
            .inputs = operation.inputs,
            .outputs = operation.outputs,
    };
}

GeneralResult<Operand::SymmPerChannelQuantParams> unvalidatedConvert(
        const hal::V1_2::SymmPerChannelQuantParams& symmPerChannelQuantParams) {
    return Operand::SymmPerChannelQuantParams{
            .scales = symmPerChannelQuantParams.scales,
            .channelDim = symmPerChannelQuantParams.channelDim,
    };
}

GeneralResult<Operand> unvalidatedConvert(const hal::V1_2::Operand& operand) {
    return Operand{
            .type = NN_TRY(unvalidatedConvert(operand.type)),
            .dimensions = operand.dimensions,
            .scale = operand.scale,
            .zeroPoint = operand.zeroPoint,
            .lifetime = NN_TRY(unvalidatedConvert(operand.lifetime)),
            .location = NN_TRY(unvalidatedConvert(operand.location)),
            .extraParams = NN_TRY(unvalidatedConvert(operand.extraParams)),
    };
}

GeneralResult<Operand::ExtraParams> unvalidatedConvert(
        const hal::V1_2::Operand::ExtraParams& extraParams) {
    using Discriminator = hal::V1_2::Operand::ExtraParams::hidl_discriminator;
    switch (extraParams.getDiscriminator()) {
        case Discriminator::none:
            return Operand::NoParams{};
        case Discriminator::channelQuant:
            return unvalidatedConvert(extraParams.channelQuant());
        case Discriminator::extension:
            return extraParams.extension();
    }
    return NN_ERROR(nn::ErrorStatus::GENERAL_FAILURE)
           << "Unrecognized Operand::ExtraParams discriminator: "
           << underlyingType(extraParams.getDiscriminator());
}

GeneralResult<Model> unvalidatedConvert(const hal::V1_2::Model& model) {
    auto operations = NN_TRY(unvalidatedConvert(model.operations));

    // Verify number of consumers.
    const auto numberOfConsumers =
            NN_TRY(hal::utils::countNumberOfConsumers(model.operands.size(), operations));
    CHECK(model.operands.size() == numberOfConsumers.size());
    for (size_t i = 0; i < model.operands.size(); ++i) {
        if (model.operands[i].numberOfConsumers != numberOfConsumers[i]) {
            return NN_ERROR(nn::ErrorStatus::GENERAL_FAILURE)
                   << "Invalid numberOfConsumers for operand " << i << ", expected "
                   << numberOfConsumers[i] << " but found " << model.operands[i].numberOfConsumers;
        }
    }

    auto main = Model::Subgraph{
            .operands = NN_TRY(unvalidatedConvert(model.operands)),
            .operations = std::move(operations),
            .inputIndexes = model.inputIndexes,
            .outputIndexes = model.outputIndexes,
    };

    return Model{
            .main = std::move(main),
            .operandValues = NN_TRY(unvalidatedConvert(model.operandValues)),
            .pools = NN_TRY(unvalidatedConvert(model.pools)),
            .relaxComputationFloat32toFloat16 = model.relaxComputationFloat32toFloat16,
            .extensionNameToPrefix = NN_TRY(unvalidatedConvert(model.extensionNameToPrefix)),
    };
}

GeneralResult<Model::ExtensionNameAndPrefix> unvalidatedConvert(
        const hal::V1_2::Model::ExtensionNameAndPrefix& extensionNameAndPrefix) {
    return Model::ExtensionNameAndPrefix{
            .name = extensionNameAndPrefix.name,
            .prefix = extensionNameAndPrefix.prefix,
    };
}

GeneralResult<OutputShape> unvalidatedConvert(const hal::V1_2::OutputShape& outputShape) {
    return OutputShape{
            .dimensions = outputShape.dimensions,
            .isSufficient = outputShape.isSufficient,
    };
}

GeneralResult<MeasureTiming> unvalidatedConvert(const hal::V1_2::MeasureTiming& measureTiming) {
    return static_cast<MeasureTiming>(measureTiming);
}

GeneralResult<Timing> unvalidatedConvert(const hal::V1_2::Timing& timing) {
    constexpr uint64_t kMaxTiming = std::chrono::floor<HalDuration>(Duration::max()).count();
    constexpr auto convertTiming = [](uint64_t halTiming) -> OptionalDuration {
        constexpr uint64_t kNoTiming = std::numeric_limits<uint64_t>::max();
        if (halTiming == kNoTiming) {
            return {};
        }
        if (halTiming > kMaxTiming) {
            return Duration::max();
        }
        return HalDuration{halTiming};
    };
    return Timing{.timeOnDevice = convertTiming(timing.timeOnDevice),
                  .timeInDriver = convertTiming(timing.timeInDriver)};
}

GeneralResult<Extension> unvalidatedConvert(const hal::V1_2::Extension& extension) {
    return Extension{
            .name = extension.name,
            .operandTypes = NN_TRY(unvalidatedConvert(extension.operandTypes)),
    };
}

GeneralResult<Extension::OperandTypeInformation> unvalidatedConvert(
        const hal::V1_2::Extension::OperandTypeInformation& operandTypeInformation) {
    return Extension::OperandTypeInformation{
            .type = operandTypeInformation.type,
            .isTensor = operandTypeInformation.isTensor,
            .byteSize = operandTypeInformation.byteSize,
    };
}

GeneralResult<SharedHandle> unvalidatedConvert(const hidl_handle& hidlHandle) {
    if (hidlHandle.getNativeHandle() == nullptr) {
        return nullptr;
    }
    auto handle = NN_TRY(hal::utils::sharedHandleFromNativeHandle(hidlHandle.getNativeHandle()));
    return std::make_shared<const Handle>(std::move(handle));
}

GeneralResult<DeviceType> convert(const hal::V1_2::DeviceType& deviceType) {
    return validatedConvert(deviceType);
}

GeneralResult<Capabilities> convert(const hal::V1_2::Capabilities& capabilities) {
    return validatedConvert(capabilities);
}

GeneralResult<Model> convert(const hal::V1_2::Model& model) {
    return validatedConvert(model);
}

GeneralResult<MeasureTiming> convert(const hal::V1_2::MeasureTiming& measureTiming) {
    return validatedConvert(measureTiming);
}

GeneralResult<Timing> convert(const hal::V1_2::Timing& timing) {
    return validatedConvert(timing);
}

GeneralResult<SharedMemory> convert(const hardware::hidl_memory& memory) {
    return validatedConvert(memory);
}

GeneralResult<std::vector<Extension>> convert(const hidl_vec<hal::V1_2::Extension>& extensions) {
    return validatedConvert(extensions);
}

GeneralResult<std::vector<SharedHandle>> convert(const hidl_vec<hidl_handle>& handles) {
    return validatedConvert(handles);
}

GeneralResult<std::vector<OutputShape>> convert(
        const hidl_vec<hal::V1_2::OutputShape>& outputShapes) {
    return validatedConvert(outputShapes);
}

}  // namespace android::nn

namespace android::hardware::neuralnetworks::V1_2::utils {
namespace {

using utils::unvalidatedConvert;

nn::GeneralResult<V1_0::OperandLifeTime> unvalidatedConvert(const nn::Operand::LifeTime& lifetime) {
    return V1_0::utils::unvalidatedConvert(lifetime);
}

nn::GeneralResult<V1_0::PerformanceInfo> unvalidatedConvert(
        const nn::Capabilities::PerformanceInfo& performanceInfo) {
    return V1_0::utils::unvalidatedConvert(performanceInfo);
}

nn::GeneralResult<V1_0::DataLocation> unvalidatedConvert(const nn::DataLocation& location) {
    return V1_0::utils::unvalidatedConvert(location);
}

nn::GeneralResult<hidl_vec<uint8_t>> unvalidatedConvert(
        const nn::Model::OperandValues& operandValues) {
    return V1_0::utils::unvalidatedConvert(operandValues);
}

nn::GeneralResult<hidl_memory> unvalidatedConvert(const nn::SharedMemory& memory) {
    return V1_0::utils::unvalidatedConvert(memory);
}

template <typename Input>
using UnvalidatedConvertOutput =
        std::decay_t<decltype(unvalidatedConvert(std::declval<Input>()).value())>;

template <typename Type>
nn::GeneralResult<hidl_vec<UnvalidatedConvertOutput<Type>>> unvalidatedConvert(
        const std::vector<Type>& arguments) {
    hidl_vec<UnvalidatedConvertOutput<Type>> halObject(arguments.size());
    for (size_t i = 0; i < arguments.size(); ++i) {
        halObject[i] = NN_TRY(unvalidatedConvert(arguments[i]));
    }
    return halObject;
}

nn::GeneralResult<Operand::ExtraParams> makeExtraParams(nn::Operand::NoParams /*noParams*/) {
    return Operand::ExtraParams{};
}

nn::GeneralResult<Operand::ExtraParams> makeExtraParams(
        const nn::Operand::SymmPerChannelQuantParams& channelQuant) {
    Operand::ExtraParams ret;
    ret.channelQuant(NN_TRY(unvalidatedConvert(channelQuant)));
    return ret;
}

nn::GeneralResult<Operand::ExtraParams> makeExtraParams(
        const nn::Operand::ExtensionParams& extension) {
    Operand::ExtraParams ret;
    ret.extension(extension);
    return ret;
}

template <typename Type>
nn::GeneralResult<UnvalidatedConvertOutput<Type>> validatedConvert(const Type& canonical) {
    NN_TRY(compliantVersion(canonical));
    return unvalidatedConvert(canonical);
}

template <typename Type>
nn::GeneralResult<hidl_vec<UnvalidatedConvertOutput<Type>>> validatedConvert(
        const std::vector<Type>& arguments) {
    hidl_vec<UnvalidatedConvertOutput<Type>> halObject(arguments.size());
    for (size_t i = 0; i < arguments.size(); ++i) {
        halObject[i] = NN_TRY(validatedConvert(arguments[i]));
    }
    return halObject;
}

}  // anonymous namespace

nn::GeneralResult<OperandType> unvalidatedConvert(const nn::OperandType& operandType) {
    return static_cast<OperandType>(operandType);
}

nn::GeneralResult<OperationType> unvalidatedConvert(const nn::OperationType& operationType) {
    return static_cast<OperationType>(operationType);
}

nn::GeneralResult<DeviceType> unvalidatedConvert(const nn::DeviceType& deviceType) {
    switch (deviceType) {
        case nn::DeviceType::UNKNOWN:
            return NN_ERROR(nn::ErrorStatus::GENERAL_FAILURE) << "Invalid DeviceType UNKNOWN";
        case nn::DeviceType::OTHER:
        case nn::DeviceType::CPU:
        case nn::DeviceType::GPU:
        case nn::DeviceType::ACCELERATOR:
            return static_cast<DeviceType>(deviceType);
    }
    return NN_ERROR(nn::ErrorStatus::GENERAL_FAILURE)
           << "Invalid DeviceType " << underlyingType(deviceType);
}

nn::GeneralResult<Capabilities> unvalidatedConvert(const nn::Capabilities& capabilities) {
    std::vector<nn::Capabilities::OperandPerformance> operandPerformance;
    operandPerformance.reserve(capabilities.operandPerformance.asVector().size());
    std::copy_if(capabilities.operandPerformance.asVector().begin(),
                 capabilities.operandPerformance.asVector().end(),
                 std::back_inserter(operandPerformance),
                 [](const nn::Capabilities::OperandPerformance& operandPerformance) {
                     return compliantVersion(operandPerformance.type).has_value();
                 });

    return Capabilities{
            .relaxedFloat32toFloat16PerformanceScalar = NN_TRY(
                    unvalidatedConvert(capabilities.relaxedFloat32toFloat16PerformanceScalar)),
            .relaxedFloat32toFloat16PerformanceTensor = NN_TRY(
                    unvalidatedConvert(capabilities.relaxedFloat32toFloat16PerformanceTensor)),
            .operandPerformance = NN_TRY(unvalidatedConvert(operandPerformance)),
    };
}

nn::GeneralResult<Capabilities::OperandPerformance> unvalidatedConvert(
        const nn::Capabilities::OperandPerformance& operandPerformance) {
    return Capabilities::OperandPerformance{
            .type = NN_TRY(unvalidatedConvert(operandPerformance.type)),
            .info = NN_TRY(unvalidatedConvert(operandPerformance.info)),
    };
}

nn::GeneralResult<Operation> unvalidatedConvert(const nn::Operation& operation) {
    return Operation{
            .type = NN_TRY(unvalidatedConvert(operation.type)),
            .inputs = operation.inputs,
            .outputs = operation.outputs,
    };
}

nn::GeneralResult<SymmPerChannelQuantParams> unvalidatedConvert(
        const nn::Operand::SymmPerChannelQuantParams& symmPerChannelQuantParams) {
    return SymmPerChannelQuantParams{
            .scales = symmPerChannelQuantParams.scales,
            .channelDim = symmPerChannelQuantParams.channelDim,
    };
}

nn::GeneralResult<Operand> unvalidatedConvert(const nn::Operand& operand) {
    return Operand{
            .type = NN_TRY(unvalidatedConvert(operand.type)),
            .dimensions = operand.dimensions,
            .numberOfConsumers = 0,
            .scale = operand.scale,
            .zeroPoint = operand.zeroPoint,
            .lifetime = NN_TRY(unvalidatedConvert(operand.lifetime)),
            .location = NN_TRY(unvalidatedConvert(operand.location)),
            .extraParams = NN_TRY(unvalidatedConvert(operand.extraParams)),
    };
}

nn::GeneralResult<Operand::ExtraParams> unvalidatedConvert(
        const nn::Operand::ExtraParams& extraParams) {
    return std::visit([](const auto& x) { return makeExtraParams(x); }, extraParams);
}

nn::GeneralResult<Model> unvalidatedConvert(const nn::Model& model) {
    if (!hal::utils::hasNoPointerData(model)) {
        return NN_ERROR(nn::ErrorStatus::INVALID_ARGUMENT)
               << "Model cannot be unvalidatedConverted because it contains pointer-based memory";
    }

    auto operands = NN_TRY(unvalidatedConvert(model.main.operands));

    // Update number of consumers.
    const auto numberOfConsumers =
            NN_TRY(hal::utils::countNumberOfConsumers(operands.size(), model.main.operations));
    CHECK(operands.size() == numberOfConsumers.size());
    for (size_t i = 0; i < operands.size(); ++i) {
        operands[i].numberOfConsumers = numberOfConsumers[i];
    }

    return Model{
            .operands = std::move(operands),
            .operations = NN_TRY(unvalidatedConvert(model.main.operations)),
            .inputIndexes = model.main.inputIndexes,
            .outputIndexes = model.main.outputIndexes,
            .operandValues = NN_TRY(unvalidatedConvert(model.operandValues)),
            .pools = NN_TRY(unvalidatedConvert(model.pools)),
            .relaxComputationFloat32toFloat16 = model.relaxComputationFloat32toFloat16,
            .extensionNameToPrefix = NN_TRY(unvalidatedConvert(model.extensionNameToPrefix)),
    };
}

nn::GeneralResult<Model::ExtensionNameAndPrefix> unvalidatedConvert(
        const nn::Model::ExtensionNameAndPrefix& extensionNameAndPrefix) {
    return Model::ExtensionNameAndPrefix{
            .name = extensionNameAndPrefix.name,
            .prefix = extensionNameAndPrefix.prefix,
    };
}

nn::GeneralResult<OutputShape> unvalidatedConvert(const nn::OutputShape& outputShape) {
    return OutputShape{.dimensions = outputShape.dimensions,
                       .isSufficient = outputShape.isSufficient};
}

nn::GeneralResult<MeasureTiming> unvalidatedConvert(const nn::MeasureTiming& measureTiming) {
    return static_cast<MeasureTiming>(measureTiming);
}

nn::GeneralResult<Timing> unvalidatedConvert(const nn::Timing& timing) {
    constexpr auto convertTiming = [](nn::OptionalDuration canonicalTiming) -> uint64_t {
        constexpr uint64_t kNoTiming = std::numeric_limits<uint64_t>::max();
        if (!canonicalTiming.has_value()) {
            return kNoTiming;
        }
        return std::chrono::ceil<HalDuration>(*canonicalTiming).count();
    };
    return Timing{.timeOnDevice = convertTiming(timing.timeOnDevice),
                  .timeInDriver = convertTiming(timing.timeInDriver)};
}

nn::GeneralResult<Extension> unvalidatedConvert(const nn::Extension& extension) {
    return Extension{
            .name = extension.name,
            .operandTypes = NN_TRY(unvalidatedConvert(extension.operandTypes)),
    };
}

nn::GeneralResult<Extension::OperandTypeInformation> unvalidatedConvert(
        const nn::Extension::OperandTypeInformation& operandTypeInformation) {
    return Extension::OperandTypeInformation{
            .type = operandTypeInformation.type,
            .isTensor = operandTypeInformation.isTensor,
            .byteSize = operandTypeInformation.byteSize,
    };
}

nn::GeneralResult<hidl_handle> unvalidatedConvert(const nn::SharedHandle& handle) {
    if (handle == nullptr) {
        return {};
    }
    return hal::utils::hidlHandleFromSharedHandle(*handle);
}

nn::GeneralResult<DeviceType> convert(const nn::DeviceType& deviceType) {
    return validatedConvert(deviceType);
}

nn::GeneralResult<Capabilities> convert(const nn::Capabilities& capabilities) {
    return validatedConvert(capabilities);
}

nn::GeneralResult<Model> convert(const nn::Model& model) {
    return validatedConvert(model);
}

nn::GeneralResult<MeasureTiming> convert(const nn::MeasureTiming& measureTiming) {
    return validatedConvert(measureTiming);
}

nn::GeneralResult<Timing> convert(const nn::Timing& timing) {
    return validatedConvert(timing);
}

nn::GeneralResult<hidl_vec<Extension>> convert(const std::vector<nn::Extension>& extensions) {
    return validatedConvert(extensions);
}

nn::GeneralResult<hidl_vec<hidl_handle>> convert(const std::vector<nn::SharedHandle>& handles) {
    return validatedConvert(handles);
}

nn::GeneralResult<hidl_vec<OutputShape>> convert(const std::vector<nn::OutputShape>& outputShapes) {
    return validatedConvert(outputShapes);
}

nn::GeneralResult<V1_0::DeviceStatus> convert(const nn::DeviceStatus& deviceStatus) {
    return V1_1::utils::convert(deviceStatus);
}

nn::GeneralResult<V1_0::Request> convert(const nn::Request& request) {
    return V1_1::utils::convert(request);
}

nn::GeneralResult<V1_0::ErrorStatus> convert(const nn::ErrorStatus& status) {
    return V1_1::utils::convert(status);
}

nn::GeneralResult<V1_1::ExecutionPreference> convert(
        const nn::ExecutionPreference& executionPreference) {
    return V1_1::utils::convert(executionPreference);
}

}  // namespace android::hardware::neuralnetworks::V1_2::utils
