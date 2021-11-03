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

#define LOG_TAG "ValidateHal"

#include "AidlValidateHal.h"

#include <android-base/logging.h>
#include <nnapi/hal/aidl/Conversions.h>

#include <algorithm>
#include <memory>
#include <set>
#include <utility>
#include <vector>

#include "AidlHalUtils.h"
#include "nnapi/TypeUtils.h"

namespace android {
namespace nn {

bool validateMemoryDesc(
        const aidl_hal::BufferDesc& desc,
        const std::vector<std::shared_ptr<aidl_hal::IPreparedModel>>& preparedModels,
        const std::vector<aidl_hal::BufferRole>& inputRoles,
        const std::vector<aidl_hal::BufferRole>& outputRoles,
        std::function<const aidl_hal::Model*(const std::shared_ptr<aidl_hal::IPreparedModel>&)>
                getModel,
        std::set<AidlHalPreparedModelRole>* preparedModelRoles,
        aidl_hal::Operand* combinedOperand) {
    NN_RET_CHECK(!preparedModels.empty());
    NN_RET_CHECK(!inputRoles.empty() || !outputRoles.empty());

    std::set<AidlHalPreparedModelRole> roles;
    std::vector<aidl_hal::Operand> operands;
    operands.reserve(inputRoles.size() + outputRoles.size());
    for (const auto& role : inputRoles) {
        NN_RET_CHECK_GE(role.modelIndex, 0);
        NN_RET_CHECK_LT(static_cast<size_t>(role.modelIndex), preparedModels.size());
        const auto& preparedModel = preparedModels[role.modelIndex];
        NN_RET_CHECK(preparedModel != nullptr);
        const auto* model = getModel(preparedModel);
        NN_RET_CHECK(model != nullptr);
        const auto& inputIndexes = model->main.inputIndexes;
        NN_RET_CHECK_GE(role.ioIndex, 0);
        NN_RET_CHECK_LT(static_cast<size_t>(role.ioIndex), inputIndexes.size());
        NN_RET_CHECK_GT(role.probability, 0.0f);
        NN_RET_CHECK_LE(role.probability, 1.0f);
        const auto [it, success] = roles.emplace(preparedModel.get(), IOType::INPUT, role.ioIndex);
        NN_RET_CHECK(success);
        operands.push_back(model->main.operands[inputIndexes[role.ioIndex]]);
    }
    for (const auto& role : outputRoles) {
        NN_RET_CHECK_GE(role.modelIndex, 0);
        NN_RET_CHECK_LT(static_cast<size_t>(role.modelIndex), preparedModels.size());
        const auto& preparedModel = preparedModels[role.modelIndex];
        NN_RET_CHECK(preparedModel != nullptr);
        const auto* model = getModel(preparedModel);
        NN_RET_CHECK(model != nullptr);
        const auto& outputIndexes = model->main.outputIndexes;
        NN_RET_CHECK_GE(role.ioIndex, 0);
        NN_RET_CHECK_LT(static_cast<size_t>(role.ioIndex), outputIndexes.size());
        NN_RET_CHECK_GT(role.probability, 0.0f);
        NN_RET_CHECK_LE(role.probability, 1.0f);
        const auto [it, success] = roles.emplace(preparedModel.get(), IOType::OUTPUT, role.ioIndex);
        NN_RET_CHECK(success);
        operands.push_back(model->main.operands[outputIndexes[role.ioIndex]]);
    }

    CHECK(!operands.empty());
    const auto opType = operands[0].type;
    const auto canonicalOperandType = convert(opType);
    NN_RET_CHECK(canonicalOperandType.has_value()) << canonicalOperandType.error().message;
    const bool isExtensionOperand = isExtension(canonicalOperandType.value());

    auto maybeDimensions = toUnsigned(desc.dimensions);
    NN_RET_CHECK(maybeDimensions.has_value()) << maybeDimensions.error().message;
    std::vector<uint32_t> dimensions = std::move(maybeDimensions).value();

    for (const auto& operand : operands) {
        NN_RET_CHECK(operand.type == operands[0].type)
                << toString(operand.type) << " vs " << toString(operands[0].type);
        NN_RET_CHECK_EQ(operand.scale, operands[0].scale);
        NN_RET_CHECK_EQ(operand.zeroPoint, operands[0].zeroPoint);
        // NOTE: validateMemoryDesc cannot validate extra parameters for extension operand type.
        if (!isExtensionOperand) {
            const auto& lhsExtraParams = operand.extraParams;
            const auto& rhsExtraParams = operands[0].extraParams;
            NN_RET_CHECK(lhsExtraParams == rhsExtraParams)
                    << (lhsExtraParams.has_value() ? lhsExtraParams.value().toString()
                                                   : "std::nullopt")
                    << " vs "
                    << (rhsExtraParams.has_value() ? rhsExtraParams.value().toString()
                                                   : "std::nullopt");
        }
        const auto maybeRhsDimensions = toUnsigned(operand.dimensions);
        NN_RET_CHECK(maybeRhsDimensions.has_value()) << maybeRhsDimensions.error().message;
        const auto combined = combineDimensions(dimensions, maybeRhsDimensions.value());
        NN_RET_CHECK(combined.has_value());
        dimensions = combined.value();
    }

    // NOTE: validateMemoryDesc cannot validate scalar dimensions with extension operand type.
    if (!isExtensionOperand) {
        NN_RET_CHECK(!isNonExtensionScalar(opType) || dimensions.empty())
                << "invalid dimensions with scalar operand type.";
    }

    if (preparedModelRoles != nullptr) {
        *preparedModelRoles = std::move(roles);
    }
    if (combinedOperand != nullptr) {
        *combinedOperand = operands[0];
        // No need to check that values fit int32_t here, since the original values are obtained
        // from int32_t.
        combinedOperand->dimensions = aidl_hal::utils::toSigned(dimensions).value();
    }
    return true;
}

}  // namespace nn
}  // namespace android
