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

#include "CommonUtils.h"

#include <android-base/logging.h>
#include <nnapi/Result.h>
#include <nnapi/SharedMemory.h>
#include <nnapi/TypeUtils.h>
#include <nnapi/Types.h>
#include <nnapi/Validation.h>

#include <algorithm>
#include <any>
#include <functional>
#include <optional>
#include <variant>
#include <vector>

namespace android::hardware::neuralnetworks::utils {
namespace {

bool hasNoPointerData(const nn::Operand& operand);
bool hasNoPointerData(const nn::Model::Subgraph& subgraph);
bool hasNoPointerData(const nn::Request::Argument& argument);

template <typename Type>
bool hasNoPointerData(const std::vector<Type>& objects) {
    return std::all_of(objects.begin(), objects.end(),
                       [](const auto& object) { return hasNoPointerData(object); });
}

bool hasNoPointerData(const nn::DataLocation& location) {
    return std::visit([](auto ptr) { return ptr == nullptr; }, location.pointer);
}

bool hasNoPointerData(const nn::Operand& operand) {
    return hasNoPointerData(operand.location);
}

bool hasNoPointerData(const nn::Model::Subgraph& subgraph) {
    return hasNoPointerData(subgraph.operands);
}

bool hasNoPointerData(const nn::Request::Argument& argument) {
    return hasNoPointerData(argument.location);
}

void copyPointersToSharedMemory(nn::Operand* operand, nn::ConstantMemoryBuilder* memoryBuilder) {
    CHECK(operand != nullptr);
    CHECK(memoryBuilder != nullptr);

    if (operand->lifetime != nn::Operand::LifeTime::POINTER) {
        return;
    }

    const void* data = std::visit([](auto ptr) { return static_cast<const void*>(ptr); },
                                  operand->location.pointer);
    CHECK(data != nullptr);
    operand->lifetime = nn::Operand::LifeTime::CONSTANT_REFERENCE;
    operand->location = memoryBuilder->append(data, operand->location.length);
}

void copyPointersToSharedMemory(nn::Model::Subgraph* subgraph,
                                nn::ConstantMemoryBuilder* memoryBuilder) {
    CHECK(subgraph != nullptr);
    std::for_each(subgraph->operands.begin(), subgraph->operands.end(),
                  [memoryBuilder](auto& operand) {
                      copyPointersToSharedMemory(&operand, memoryBuilder);
                  });
}

}  // anonymous namespace

nn::Capabilities::OperandPerformanceTable makeQuantized8PerformanceConsistentWithP(
        const nn::Capabilities::PerformanceInfo& float32Performance,
        const nn::Capabilities::PerformanceInfo& quantized8Performance) {
    // In Android P, most data types are treated as having the same performance as
    // TENSOR_QUANT8_ASYMM. This collection must be in sorted order.
    std::vector<nn::Capabilities::OperandPerformance> operandPerformances = {
            {.type = nn::OperandType::FLOAT32, .info = float32Performance},
            {.type = nn::OperandType::INT32, .info = quantized8Performance},
            {.type = nn::OperandType::UINT32, .info = quantized8Performance},
            {.type = nn::OperandType::TENSOR_FLOAT32, .info = float32Performance},
            {.type = nn::OperandType::TENSOR_INT32, .info = quantized8Performance},
            {.type = nn::OperandType::TENSOR_QUANT8_ASYMM, .info = quantized8Performance},
            {.type = nn::OperandType::OEM, .info = quantized8Performance},
            {.type = nn::OperandType::TENSOR_OEM_BYTE, .info = quantized8Performance},
    };
    return nn::Capabilities::OperandPerformanceTable::create(std::move(operandPerformances))
            .value();
}

bool hasNoPointerData(const nn::Model& model) {
    return hasNoPointerData(model.main) && hasNoPointerData(model.referenced);
}

bool hasNoPointerData(const nn::Request& request) {
    return hasNoPointerData(request.inputs) && hasNoPointerData(request.outputs);
}

nn::GeneralResult<std::reference_wrapper<const nn::Model>> flushDataFromPointerToShared(
        const nn::Model* model, std::optional<nn::Model>* maybeModelInSharedOut) {
    CHECK(model != nullptr);
    CHECK(maybeModelInSharedOut != nullptr);

    if (hasNoPointerData(*model)) {
        return *model;
    }

    // Make a copy of the model in order to make modifications. The modified model is returned to
    // the caller through `maybeModelInSharedOut` if the function succeeds.
    nn::Model modelInShared = *model;

    nn::ConstantMemoryBuilder memoryBuilder(modelInShared.pools.size());
    copyPointersToSharedMemory(&modelInShared.main, &memoryBuilder);
    std::for_each(modelInShared.referenced.begin(), modelInShared.referenced.end(),
                  [&memoryBuilder](auto& subgraph) {
                      copyPointersToSharedMemory(&subgraph, &memoryBuilder);
                  });

    if (!memoryBuilder.empty()) {
        auto memory = NN_TRY(memoryBuilder.finish());
        modelInShared.pools.push_back(std::move(memory));
    }

    *maybeModelInSharedOut = modelInShared;
    return **maybeModelInSharedOut;
}

template <>
void InputRelocationTracker::flush() const {
    // Copy from pointers to shared memory.
    uint8_t* memoryPtr = static_cast<uint8_t*>(std::get<void*>(kMapping.pointer));
    for (const auto& [data, length, offset] : kRelocationInfos) {
        std::memcpy(memoryPtr + offset, data, length);
    }
}

template <>
void OutputRelocationTracker::flush() const {
    // Copy from shared memory to pointers.
    const uint8_t* memoryPtr = static_cast<const uint8_t*>(
            std::visit([](auto ptr) { return static_cast<const void*>(ptr); }, kMapping.pointer));
    for (const auto& [data, length, offset] : kRelocationInfos) {
        std::memcpy(data, memoryPtr + offset, length);
    }
}

nn::GeneralResult<std::reference_wrapper<const nn::Request>> convertRequestFromPointerToShared(
        const nn::Request* request, uint32_t alignment, uint32_t padding,
        std::optional<nn::Request>* maybeRequestInSharedOut, RequestRelocation* relocationOut) {
    CHECK(request != nullptr);
    CHECK(maybeRequestInSharedOut != nullptr);
    CHECK(relocationOut != nullptr);

    if (hasNoPointerData(*request)) {
        return *request;
    }

    // Make a copy of the request in order to make modifications. The modified request is returned
    // to the caller through `maybeRequestInSharedOut` if the function succeeds.
    nn::Request requestInShared = *request;

    RequestRelocation relocation;

    // Change input pointers to shared memory.
    nn::MutableMemoryBuilder inputBuilder(requestInShared.pools.size());
    std::vector<InputRelocationInfo> inputRelocationInfos;
    for (auto& input : requestInShared.inputs) {
        const auto& location = input.location;
        if (input.lifetime != nn::Request::Argument::LifeTime::POINTER) {
            continue;
        }

        input.lifetime = nn::Request::Argument::LifeTime::POOL;
        const void* data = std::visit([](auto ptr) { return static_cast<const void*>(ptr); },
                                      location.pointer);
        CHECK(data != nullptr);
        input.location = inputBuilder.append(location.length, alignment, padding);
        inputRelocationInfos.push_back({data, input.location.length, input.location.offset});
    }

    // Allocate input memory.
    if (!inputBuilder.empty()) {
        auto memory = NN_TRY(inputBuilder.finish());
        requestInShared.pools.push_back(memory);
        relocation.input = NN_TRY(
                InputRelocationTracker::create(std::move(inputRelocationInfos), std::move(memory)));
    }

    // Change output pointers to shared memory.
    nn::MutableMemoryBuilder outputBuilder(requestInShared.pools.size());
    std::vector<OutputRelocationInfo> outputRelocationInfos;
    for (auto& output : requestInShared.outputs) {
        const auto& location = output.location;
        if (output.lifetime != nn::Request::Argument::LifeTime::POINTER) {
            continue;
        }

        output.lifetime = nn::Request::Argument::LifeTime::POOL;
        void* data = std::get<void*>(location.pointer);
        CHECK(data != nullptr);
        output.location = outputBuilder.append(location.length, alignment, padding);
        outputRelocationInfos.push_back({data, output.location.length, output.location.offset});
    }

    // Allocate output memory.
    if (!outputBuilder.empty()) {
        auto memory = NN_TRY(outputBuilder.finish());
        requestInShared.pools.push_back(memory);
        relocation.output = NN_TRY(OutputRelocationTracker::create(std::move(outputRelocationInfos),
                                                                   std::move(memory)));
    }

    *maybeRequestInSharedOut = requestInShared;
    *relocationOut = std::move(relocation);
    return **maybeRequestInSharedOut;
}

}  // namespace android::hardware::neuralnetworks::utils
