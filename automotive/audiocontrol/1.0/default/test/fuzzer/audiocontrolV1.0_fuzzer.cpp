/******************************************************************************
 *
 * Copyright (C) 2021 The Android Open Source Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at:
 *
 * http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 *
 *****************************************************************************
 * Originally developed and contributed by Ittiam Systems Pvt. Ltd, Bangalore
 */
#include <AudioControl.h>
#include <fuzzer/FuzzedDataProvider.h>

using ::android::sp;
using ::android::hardware::automotive::audiocontrol::V1_0::ContextNumber;
using ::android::hardware::automotive::audiocontrol::V1_0::implementation::AudioControl;

namespace android::hardware::automotive::audiocontrol::V1_0::implementation::fuzzer {
extern "C" int LLVMFuzzerTestOneInput(const uint8_t* data, size_t size) {
    if (size < 1) {
        return 0;
    }
    if (sp<AudioControl> audioControl = new AudioControl(); audioControl != nullptr) {
        FuzzedDataProvider fdp = FuzzedDataProvider(data, size);
        ContextNumber contextNumber = static_cast<ContextNumber>(fdp.ConsumeIntegral<uint32_t>());
        audioControl->getBusForContext(contextNumber);
        audioControl->setBalanceTowardRight(fdp.ConsumeFloatingPoint<float>());
        audioControl->setFadeTowardFront(fdp.ConsumeFloatingPoint<float>());
    }
    return 0;
}
}  // namespace android::hardware::automotive::audiocontrol::V1_0::implementation::fuzzer
