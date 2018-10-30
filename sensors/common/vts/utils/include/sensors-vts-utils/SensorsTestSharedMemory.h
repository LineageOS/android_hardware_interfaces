/*
 * Copyright (C) 2018 The Android Open Source Project
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

#ifndef ANDROID_SENSORS_TEST_SHARED_MEMORY_H
#define ANDROID_SENSORS_TEST_SHARED_MEMORY_H

#include "GrallocWrapper.h"

#include <android-base/macros.h>
#include <android/hardware/sensors/1.0/types.h>

#include <cutils/ashmem.h>

class SensorsTestSharedMemory {
    using SharedMemType = ::android::hardware::sensors::V1_0::SharedMemType;
    using SharedMemInfo = ::android::hardware::sensors::V1_0::SharedMemInfo;
    using Event = ::android::hardware::sensors::V1_0::Event;

   public:
    static SensorsTestSharedMemory* create(SharedMemType type, size_t size);
    SharedMemInfo getSharedMemInfo() const;
    char* getBuffer() const;
    size_t getSize() const;
    std::vector<Event> parseEvents(int64_t lastCounter = -1, size_t offset = 0) const;
    virtual ~SensorsTestSharedMemory();

   private:
    SensorsTestSharedMemory(SharedMemType type, size_t size);

    SharedMemType mType;
    native_handle_t* mNativeHandle;
    size_t mSize;
    char* mBuffer;
    std::unique_ptr<::android::GrallocWrapper> mGrallocWrapper;

    DISALLOW_COPY_AND_ASSIGN(SensorsTestSharedMemory);
};

#endif  // ANDROID_SENSORS_TEST_SHARED_MEMORY_H
