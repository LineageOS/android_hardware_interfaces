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

#pragma once

#include "convertV2_1.h"

#include <android/hardware/sensors/2.1/types.h>
#include <fmq/MessageQueue.h>
#include <hidl/MQDescriptor.h>
#include <hidl/Status.h>
#include <log/log.h>

#include <atomic>

namespace android {
namespace hardware {
namespace sensors {
namespace V2_1 {
namespace implementation {

class WakeLockMessageQueueWrapperBase {
  public:
    virtual ~WakeLockMessageQueueWrapperBase() {}

    virtual std::atomic<uint32_t>* getEventFlagWord() = 0;
    virtual bool readBlocking(uint32_t* events, size_t numToRead, uint32_t readNotification,
                              uint32_t writeNotification, int64_t timeOutNanos,
                              ::android::hardware::EventFlag* evFlag = nullptr) = 0;
    virtual bool write(const uint32_t* wakeLock) = 0;
};

class WakeLockMessageQueueWrapperHidl : public WakeLockMessageQueueWrapperBase {
  public:
    WakeLockMessageQueueWrapperHidl(
            std::unique_ptr<::android::hardware::MessageQueue<uint32_t, kSynchronizedReadWrite>>&
                    queue)
        : mQueue(std::move(queue)) {}

    std::atomic<uint32_t>* getEventFlagWord() override { return mQueue->getEventFlagWord(); }

    bool readBlocking(uint32_t* wakeLocks, size_t numToRead, uint32_t readNotification,
                      uint32_t writeNotification, int64_t timeOutNanos,
                      ::android::hardware::EventFlag* evFlag) override {
        return mQueue->readBlocking(wakeLocks, numToRead, readNotification, writeNotification,
                                    timeOutNanos, evFlag);
    }

    bool write(const uint32_t* wakeLock) override { return mQueue->write(wakeLock); }

  private:
    std::unique_ptr<::android::hardware::MessageQueue<uint32_t, kSynchronizedReadWrite>> mQueue;
};

}  // namespace implementation
}  // namespace V2_1
}  // namespace sensors
}  // namespace hardware
}  // namespace android
