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

#include "GeneratedTestHarness.h"
#include "TestHarness.h"

#include <android/hardware/neuralnetworks/1.0/types.h>

#include <cstring>
#include <map>
#include <vector>

namespace android {
namespace hardware {
namespace neuralnetworks {

using ::android::hardware::neuralnetworks::V1_0::RequestArgument;
using ::test_helper::for_each;
using ::test_helper::MixedTyped;

template <typename T>
void copy_back_(std::map<int, std::vector<T>>* dst, const std::vector<RequestArgument>& ra,
                char* src) {
    for_each<T>(*dst, [&ra, src](int index, std::vector<T>& m) {
        ASSERT_EQ(m.size(), ra[index].location.length / sizeof(T));
        char* begin = src + ra[index].location.offset;
        memcpy(m.data(), begin, ra[index].location.length);
    });
}

void copy_back(MixedTyped* dst, const std::vector<RequestArgument>& ra, char* src) {
    copy_back_(&dst->float32Operands, ra, src);
    copy_back_(&dst->int32Operands, ra, src);
    copy_back_(&dst->quant8AsymmOperands, ra, src);
    copy_back_(&dst->quant16SymmOperands, ra, src);
    copy_back_(&dst->float16Operands, ra, src);
    copy_back_(&dst->bool8Operands, ra, src);
    copy_back_(&dst->quant8ChannelOperands, ra, src);
    copy_back_(&dst->quant16AsymmOperands, ra, src);
    copy_back_(&dst->quant8SymmOperands, ra, src);
    static_assert(9 == MixedTyped::kNumTypes,
                  "Number of types in MixedTyped changed, but copy_back function wasn't updated");
}

}  // namespace neuralnetworks
}  // namespace hardware
}  // namespace android
