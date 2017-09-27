/*
 * Copyright (C) 2016 The Android Open Source Project
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

#ifndef ANDROID_HARDWARE_TESTS_MEMORY_V1_0_MEMORY_TEST_H
#define ANDROID_HARDWARE_TESTS_MEMORY_V1_0_MEMORY_TEST_H

#include <android/hardware/tests/memory/1.0/IMemoryTest.h>
#include <hidl/MQDescriptor.h>
#include <hidl/Status.h>
#include <hidlmemory/HidlMemoryToken.h>

namespace android {
namespace hardware {
namespace tests {
namespace memory {
namespace V1_0 {
namespace implementation {

using ::android::sp;
using ::android::hardware::hidl_array;
using ::android::hardware::hidl_memory;
using ::android::hardware::hidl_string;
using ::android::hardware::hidl_vec;
using ::android::hardware::HidlMemoryToken;
using ::android::hardware::Return;
using ::android::hardware::Void;
using ::android::hardware::tests::memory::V1_0::IMemoryTest;
using ::android::hidl::memory::block::V1_0::MemoryBlock;
using ::android::hidl::memory::token::V1_0::IMemoryToken;

struct Memory : public IMemoryTest {
    // Methods from ::android::hardware::tests::memory::V1_0::IMemoryTest follow.
    Return<void> haveSomeMemory(const hidl_memory& mem, haveSomeMemory_cb _hidl_cb) override;

    Return<void> fillMemory(const hidl_memory& memory_in, uint8_t filler) override;

    Return<void> haveSomeMemoryBlock(const MemoryBlock& blk,
                                     haveSomeMemoryBlock_cb _hidl_cb) override;
    Return<void> set(const ::android::hardware::hidl_memory& mem) override;

    Return<sp<IMemoryToken>> get() override;

   protected:
    sp<HidlMemoryToken> mSavedMemoryToken;
};

extern "C" IMemoryTest* HIDL_FETCH_IMemoryTest(const char* name);

}  // namespace implementation
}  // namespace V1_0
}  // namespace memory
}  // namespace tests
}  // namespace hardware
}  // namespace android

#endif  // ANDROID_HARDWARE_TESTS_MEMORY_V1_0_MEMORY_TEST_H
