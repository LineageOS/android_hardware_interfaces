/*
 * Copyright (C) 2022 The Android Open Source Project
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
 */
#pragma once
#include <SurroundViewService.h>
#include <android/hidl/memory/1.0/IMemory.h>
#include <fuzzer/FuzzedDataProvider.h>

namespace android::hardware::automotive::sv::V1_0::implementation::fuzzer {

using ::android::sp;
using ::android::hidl::memory::V1_0::IMemory;

constexpr size_t kMinOverlays = 2;

class SurroundViewFuzzer {
  public:
    SurroundViewFuzzer(const uint8_t* data, size_t size) : mFuzzedDataProvider(data, size) {
        mSurroundViewService = sp<SurroundViewService>::make();
    }
    ~SurroundViewFuzzer() = default;
    void process();

  private:
    void invoke2dSessionAPI();
    void invoke3dSessionAPI();
    std::pair<hidl_memory, sp<IMemory>> getMappedSharedMemory(int32_t bytesSize);
    void initSampleOverlaysData();
    void setIndexOfOverlaysMemory(const std::vector<OverlayMemoryDesc>& overlaysMemDesc,
                                  sp<IMemory> pIMemory, int32_t indexPosition, uint16_t indexValue);
    OverlaysData mOverlaysdata = {};
    size_t mNumOverlays = kMinOverlays;
    sp<IMemory> mMemory = nullptr;
    FuzzedDataProvider mFuzzedDataProvider;
    sp<SurroundViewService> mSurroundViewService = nullptr;
    bool mIs2dStreamStarted = false;
    bool mIs3dStreamStarted = false;
};
}  // namespace android::hardware::automotive::sv::V1_0::implementation::fuzzer
