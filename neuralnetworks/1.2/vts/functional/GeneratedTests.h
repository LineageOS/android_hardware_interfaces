/*
 * Copyright (C) 2019 The Android Open Source Project
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

#include <android/hidl/memory/1.0/IMemory.h>
#include <hidlmemory/mapping.h>

#include "GeneratedTestHarness.h"
#include "MemoryUtils.h"
#include "TestHarness.h"
#include "Utils.h"
#include "VtsHalNeuralnetworks.h"

namespace android::hardware::neuralnetworks::V1_2::vts::functional {

std::vector<Request> createRequests(const std::vector<::test_helper::MixedTypedExample>& examples);

}  // namespace android::hardware::neuralnetworks::V1_2::vts::functional

namespace android::hardware::neuralnetworks::V1_2::generated_tests {

using namespace ::android::hardware::neuralnetworks::V1_2::vts::functional;

using ::android::hardware::neuralnetworks::V1_0::OperandLifeTime;
using ::android::hardware::neuralnetworks::V1_0::Request;

}  // namespace android::hardware::neuralnetworks::V1_2::generated_tests
