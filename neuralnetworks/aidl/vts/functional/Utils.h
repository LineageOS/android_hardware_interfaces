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

#ifndef ANDROID_HARDWARE_NEURALNETWORKS_AIDL_UTILS_H
#define ANDROID_HARDWARE_NEURALNETWORKS_AIDL_UTILS_H

#include <android-base/logging.h>
#include <android/hardware_buffer.h>
#include <gtest/gtest.h>

#include <algorithm>
#include <iosfwd>
#include <string>
#include <utility>
#include <vector>

#include <aidl/android/hardware/neuralnetworks/IDevice.h>
#include <aidl/android/hardware/neuralnetworks/Memory.h>
#include <aidl/android/hardware/neuralnetworks/Operand.h>
#include <aidl/android/hardware/neuralnetworks/OperandType.h>
#include <aidl/android/hardware/neuralnetworks/Priority.h>
#include <aidl/android/hardware/neuralnetworks/Request.h>

#include <TestHarness.h>
#include <nnapi/SharedMemory.h>

namespace aidl::android::hardware::neuralnetworks {

namespace nn = ::android::nn;

inline constexpr Priority kDefaultPriority = Priority::MEDIUM;

inline constexpr Timing kNoTiming = {.timeOnDeviceNs = -1, .timeInDriverNs = -1};
inline constexpr int64_t kNoDeadline = -1;
inline constexpr int64_t kOmittedTimeoutDuration = -1;
inline constexpr int64_t kNoDuration = -1;
inline const std::vector<uint8_t> kEmptyCacheToken(IDevice::BYTE_SIZE_OF_CACHE_TOKEN);

// Returns the amount of space needed to store a value of the specified type.
//
// Aborts if the specified type is an extension type or OEM type.
uint32_t sizeOfData(OperandType type);

// Returns the amount of space needed to store a value of the dimensions and
// type of this operand. For a non-extension, non-OEM tensor with unspecified
// rank or at least one unspecified dimension, returns zero.
//
// Aborts if the specified type is an extension type or OEM type.
uint32_t sizeOfData(const Operand& operand);

// Convenience class to manage the lifetime of memory resources.
class TestMemoryBase {
    DISALLOW_COPY_AND_ASSIGN(TestMemoryBase);

  public:
    TestMemoryBase() = default;
    virtual ~TestMemoryBase() = default;
    uint8_t* getPointer() const { return mPtr; }
    const Memory* getAidlMemory() const { return &mAidlMemory; }

  protected:
    uint8_t* mPtr = nullptr;
    Memory mAidlMemory;
    bool mIsValid = false;
};

class TestAshmem : public TestMemoryBase {
  public:
    // If aidlReadonly is true, getAidlMemory will return a sAIDL memory with readonly access;
    // otherwise, the sAIDL memory has read-write access. This only affects the sAIDL memory.
    // getPointer will always return a valid address with read-write access.
    static std::unique_ptr<TestAshmem> create(uint32_t size, bool aidlReadonly = false);

    // Prefer TestAshmem::create.
    // The constructor calls initialize, which constructs the memory resources. This is a workaround
    // that gtest macros cannot be used directly in a constructor.
    TestAshmem(uint32_t size, bool aidlReadonly) { initialize(size, aidlReadonly); }

  private:
    void initialize(uint32_t size, bool aidlReadonly);
    nn::Mapping mMappedMemory;
};

class TestBlobAHWB : public TestMemoryBase {
  public:
    static std::unique_ptr<TestBlobAHWB> create(uint32_t size);

    // Prefer TestBlobAHWB::create.
    // The constructor calls initialize, which constructs the memory resources. This is a
    // workaround that gtest macros cannot be used directly in a constructor.
    TestBlobAHWB(uint32_t size) { initialize(size); }

  private:
    void initialize(uint32_t size);
    nn::SharedMemory mMemory;
    nn::Mapping mMapping;
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
    Request createRequest(const test_helper::TestModel& testModel,
                          MemoryType memoryType = MemoryType::ASHMEM);

    // After execution, copy out output results from the output memory pool.
    std::vector<test_helper::TestBuffer> getOutputBuffers(const Request& request) const;

  private:
    std::unique_ptr<TestMemoryBase> mInputMemory, mOutputMemory;
};

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

// pretty-print values for error messages
::std::ostream& operator<<(::std::ostream& os, ErrorStatus errorStatus);

}  // namespace aidl::android::hardware::neuralnetworks

#endif  // ANDROID_HARDWARE_NEURALNETWORKS_AIDL_UTILS_H
