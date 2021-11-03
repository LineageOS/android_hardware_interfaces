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
// This file contains pre-canonical-types utility code and includes HAL
// utilities. LegacyUtils.h is the subset of these utilities that do not touch
// HAL.

#include "AidlHalUtils.h"

#include <android-base/logging.h>
#include <nnapi/hal/aidl/Conversions.h>

#include <algorithm>
#include <iterator>
#include <type_traits>
#include <vector>

#include "AidlHalInterfaces.h"
#include "nnapi/TypeUtils.h"

namespace android::nn {

std::vector<aidl_hal::OperandPerformance> nonExtensionOperandPerformance(
        aidl_hal::PerformanceInfo perf) {
    static constexpr ndk::enum_range<aidl_hal::OperandType> kOperandTypeRange;
    std::vector<aidl_hal::OperandPerformance> ret;
    ret.reserve(std::distance(kOperandTypeRange.begin(), kOperandTypeRange.end()));
    for (aidl_hal::OperandType type : kOperandTypeRange) {
        if (type != aidl_hal::OperandType::SUBGRAPH) {
            ret.push_back(aidl_hal::OperandPerformance{type, perf});
        }
    }
    std::sort(ret.begin(), ret.end(),
              [](const aidl_hal::OperandPerformance& a, const aidl_hal::OperandPerformance& b) {
                  return a.type < b.type;
              });

    return ret;
}

void update(std::vector<aidl_hal::OperandPerformance>* operandPerformance,
            aidl_hal::OperandType type, aidl_hal::PerformanceInfo perf) {
    CHECK(operandPerformance != nullptr);
    const auto it = std::lower_bound(operandPerformance->begin(), operandPerformance->end(), type,
                                     [](const aidl_hal::OperandPerformance& perf,
                                        aidl_hal::OperandType type) { return perf.type < type; });
    CHECK(it != operandPerformance->end())
            << toString(type) << " not in operand performance vector";
    it->info = perf;
}

bool isExtensionOperandType(aidl_hal::OperandType type) {
    return isExtension(convert(type).value());
}

bool isNonExtensionScalar(aidl_hal::OperandType type) {
    return isNonExtensionScalar(convert(type).value());
}

}  // namespace android::nn
