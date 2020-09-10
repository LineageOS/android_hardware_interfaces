/*
 * Copyright 2020 The Android Open Source Project
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

#ifndef ANDROID_HARDWARE_AUTOMOTIVE_EVS_V1_1_EVSULTRASONICSARRAY_H
#define ANDROID_HARDWARE_AUTOMOTIVE_EVS_V1_1_EVSULTRASONICSARRAY_H

#include <thread>
#include <utility>

#include <android-base/macros.h>
#include <android/hidl/allocator/1.0/IAllocator.h>
#include <android/hidl/memory/1.0/IMemory.h>
#include <utils/threads.h>

#include <android/hardware/automotive/evs/1.1/IEvsUltrasonicsArray.h>
#include <android/hardware/automotive/evs/1.1/IEvsUltrasonicsArrayStream.h>
#include <android/hardware/automotive/evs/1.1/types.h>

using ::android::hardware::hidl_memory;
using ::android::hardware::automotive::evs::V1_0::EvsResult;
using ::android::hardware::automotive::evs::V1_1::IEvsUltrasonicsArray;
using ::android::hardware::automotive::evs::V1_1::IEvsUltrasonicsArrayStream;
using ::android::hardware::automotive::evs::V1_1::UltrasonicsArrayDesc;
using ::android::hardware::automotive::evs::V1_1::UltrasonicsDataFrameDesc;
using ::android::hidl::allocator::V1_0::IAllocator;
using ::android::hidl::memory::V1_0::IMemory;

namespace android {
namespace hardware {
namespace automotive {
namespace evs {
namespace V1_1 {
namespace implementation {

class EvsUltrasonicsArray : public IEvsUltrasonicsArray {
  public:
    // Methods from ::android::hardware::automotive::evs::V1_1::IEvsUltrasonicsArray follow.
    Return<void> getUltrasonicArrayInfo(getUltrasonicArrayInfo_cb _get_info_cb) override;
    Return<EvsResult> setMaxFramesInFlight(uint32_t bufferCount) override;
    Return<void> doneWithDataFrame(const UltrasonicsDataFrameDesc& dataFrameDesc) override;
    Return<EvsResult> startStream(const ::android::sp<IEvsUltrasonicsArrayStream>& stream) override;
    Return<void> stopStream() override;

    // Factory function to create a array.
    static sp<EvsUltrasonicsArray> Create(const char* deviceName);

    // Returns a ultrasonics array descriptor filled with sample data.
    static UltrasonicsArrayDesc GetDummyArrayDesc(const char* id);

    DISALLOW_COPY_AND_ASSIGN(EvsUltrasonicsArray);
    virtual ~EvsUltrasonicsArray() override;
    void forceShutdown();  // This gets called if another caller "steals" ownership

  private:
    // Structure holding the hidl memory struct and the interface to a shared memory.
    struct SharedMemory {
        hidl_memory hidlMemory;
        sp<IMemory> pIMemory;

        SharedMemory() : hidlMemory(hidl_memory()), pIMemory(nullptr){};

        SharedMemory(hidl_memory hidlMem, sp<IMemory> pIMem)
            : hidlMemory(hidlMem), pIMemory(pIMem) {}

        bool IsValid() { return (pIMemory.get() != nullptr && hidlMemory.valid()); }

        void clear() {
            hidlMemory = hidl_memory();
            pIMemory.clear();
        }
    };

    // Struct for a data frame record.
    struct DataFrameRecord {
        SharedMemory sharedMemory;
        bool inUse;
        explicit DataFrameRecord(SharedMemory shMem) : sharedMemory(shMem), inUse(false){};
    };

    enum StreamStateValues {
        STOPPED,
        RUNNING,
        STOPPING,
        DEAD,
    };

    EvsUltrasonicsArray(const char* deviceName);

    // These three functions are expected to be called while mAccessLock is held
    bool setAvailableFrames_Locked(unsigned bufferCount);
    unsigned increaseAvailableFrames_Locked(unsigned numToAdd);
    unsigned decreaseAvailableFrames_Locked(unsigned numToRemove);

    void generateDataFrames();

    SharedMemory allocateAndMapSharedMemory();

    UltrasonicsArrayDesc mArrayDesc = {};  // The properties of this ultrasonic array.

    std::thread mCaptureThread;  // The thread we'll use to synthesize frames

    sp<IEvsUltrasonicsArrayStream> mStream = nullptr;  // The callback used to deliver each frame

    sp<IAllocator> mShmemAllocator = nullptr;  // Shared memory allocator.

    std::mutex mAccessLock;
    std::vector<DataFrameRecord> mDataFrames GUARDED_BY(mAccessLock);  // Shared memory buffers.
    unsigned mFramesAllowed GUARDED_BY(mAccessLock);  // How many buffers are we currently using.
    unsigned mFramesInUse GUARDED_BY(mAccessLock);  // How many buffers are currently outstanding.

    StreamStateValues mStreamState GUARDED_BY(mAccessLock);
};

}  // namespace implementation
}  // namespace V1_1
}  // namespace evs
}  // namespace automotive
}  // namespace hardware
}  // namespace android

#endif  // ANDROID_HARDWARE_AUTOMOTIVE_EVS_V1_1_EVSULTRASONICSARRAY_H
