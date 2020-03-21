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

#ifndef ANDROID_HARDWARE_NEURALNETWORKS_V1_0_UTILS_H
#define ANDROID_HARDWARE_NEURALNETWORKS_V1_0_UTILS_H

#include <android-base/logging.h>
#include <android/hardware/neuralnetworks/1.0/types.h>
#include <android/hardware_buffer.h>
#include <android/hidl/memory/1.0/IMemory.h>
#include <algorithm>
#include <iosfwd>
#include <string>
#include <utility>
#include <vector>
#include "TestHarness.h"

namespace android::hardware::neuralnetworks {

// Convenience class to manage the lifetime of memory resources.
class TestMemoryBase {
    DISALLOW_COPY_AND_ASSIGN(TestMemoryBase);

  public:
    TestMemoryBase() = default;
    virtual ~TestMemoryBase() = default;
    uint8_t* getPointer() const { return mPtr; }
    hidl_memory getHidlMemory() const { return mHidlMemory; }

  protected:
    uint8_t* mPtr = nullptr;
    hidl_memory mHidlMemory;
    bool mIsValid = false;
};

class TestAshmem : public TestMemoryBase {
  public:
    static std::unique_ptr<TestAshmem> create(uint32_t size);

    // Prefer TestAshmem::create.
    // The constructor calls initialize, which constructs the memory resources. This is a workaround
    // that gtest macros cannot be used directly in a constructor.
    TestAshmem(uint32_t size) { initialize(size); }

  private:
    void initialize(uint32_t size);
    sp<hidl::memory::V1_0::IMemory> mMappedMemory;
};

class TestBlobAHWB : public TestMemoryBase {
  public:
    static std::unique_ptr<TestBlobAHWB> create(uint32_t size);

    // Prefer TestBlobAHWB::create.
    // The constructor calls initialize, which constructs the memory resources. This is a
    // workaround that gtest macros cannot be used directly in a constructor.
    TestBlobAHWB(uint32_t size) { initialize(size); }
    ~TestBlobAHWB();

  private:
    void initialize(uint32_t size);
    AHardwareBuffer* mAhwb = nullptr;
};

enum class MemoryType { ASHMEM, BLOB_AHWB, DEVICE };

// Manages the lifetime of memory resources used in an execution.
class ExecutionContext {
    DISALLOW_COPY_AND_ASSIGN(ExecutionContext);

  public:
    static constexpr uint32_t kInputPoolIndex = 0;
    static constexpr uint32_t kOutputPoolIndex = 1;

    ExecutionContext() = default;

    // Create HIDL Request from the TestModel struct.
    V1_0::Request createRequest(const test_helper::TestModel& testModel,
                                MemoryType memoryType = MemoryType::ASHMEM);

    // After execution, copy out output results from the output memory pool.
    std::vector<test_helper::TestBuffer> getOutputBuffers(const V1_0::Request& request) const;

  private:
    std::unique_ptr<TestMemoryBase> mInputMemory, mOutputMemory;
};

// Delete element from hidl_vec. hidl_vec doesn't support a "remove" operation,
// so this is efficiently accomplished by moving the element to the end and
// resizing the hidl_vec to one less.
template <typename Type>
inline void hidl_vec_removeAt(hidl_vec<Type>* vec, uint32_t index) {
    CHECK(vec != nullptr);
    std::rotate(vec->begin() + index, vec->begin() + index + 1, vec->end());
    vec->resize(vec->size() - 1);
}

template <typename Type>
inline uint32_t hidl_vec_push_back(hidl_vec<Type>* vec, const Type& value) {
    CHECK(vec != nullptr);
    const uint32_t index = vec->size();
    vec->resize(index + 1);
    (*vec)[index] = value;
    return index;
}

template <typename Type>
using Named = std::pair<std::string, Type>;

template <typename Type>
const std::string& getName(const Named<Type>& namedData) {
    return namedData.first;
}

template <typename Type>
const Type& getData(const Named<Type>& namedData) {
    return namedData.second;
}

std::string gtestCompliantName(std::string name);

}  // namespace android::hardware::neuralnetworks

namespace android::hardware::neuralnetworks::V1_0 {

// pretty-print values for error messages
::std::ostream& operator<<(::std::ostream& os, ErrorStatus errorStatus);
::std::ostream& operator<<(::std::ostream& os, DeviceStatus deviceStatus);

}  // namespace android::hardware::neuralnetworks::V1_0

#endif  // ANDROID_HARDWARE_NEURALNETWORKS_V1_0_UTILS_H
