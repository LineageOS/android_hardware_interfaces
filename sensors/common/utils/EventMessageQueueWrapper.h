/*
 * Copyright (C) 2020 The Android Open Source Project
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

#ifndef ANDROID_HARDWARE_SENSORS_V2_1_EVENTMESSAGEQUEUEWRAPPER_H
#define ANDROID_HARDWARE_SENSORS_V2_1_EVENTMESSAGEQUEUEWRAPPER_H

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

class EventMessageQueueWrapperBase : public RefBase {
  public:
    virtual ~EventMessageQueueWrapperBase() {}

    virtual std::atomic<uint32_t>* getEventFlagWord() = 0;
    virtual size_t availableToRead() = 0;
    virtual bool read(V2_1::Event* events, size_t numToRead) = 0;
    virtual bool write(const std::vector<V2_1::Event>& events) = 0;
};

class EventMessageQueueWrapperV1_0 : public EventMessageQueueWrapperBase {
  public:
    using EventMessageQueue = MessageQueue<V1_0::Event, kSynchronizedReadWrite>;

    EventMessageQueueWrapperV1_0(std::unique_ptr<EventMessageQueue>& queue)
        : mQueue(std::move(queue)) {}

    const ::android::hardware::MQDescriptorSync<V1_0::Event>* getDesc() {
        return mQueue->getDesc();
    }

    virtual std::atomic<uint32_t>* getEventFlagWord() override {
        return mQueue->getEventFlagWord();
    }

    virtual size_t availableToRead() override { return mQueue->availableToRead(); }

    virtual bool read(V2_1::Event* events, size_t numToRead) override {
        return mQueue->read(reinterpret_cast<V1_0::Event*>(events), numToRead);
    }

    virtual bool write(const std::vector<V2_1::Event>& events) override {
        const std::vector<V1_0::Event>& oldEvents = convertToOldEvents(events);
        return mQueue->write(oldEvents.data(), oldEvents.size());
    }

  private:
    std::unique_ptr<EventMessageQueue> mQueue;
};

class EventMessageQueueWrapperV2_1 : public EventMessageQueueWrapperBase {
  public:
    using EventMessageQueue = MessageQueue<V2_1::Event, kSynchronizedReadWrite>;

    EventMessageQueueWrapperV2_1(std::unique_ptr<EventMessageQueue>& queue)
        : mQueue(std::move(queue)) {}

    const ::android::hardware::MQDescriptorSync<V2_1::Event>* getDesc() {
        return mQueue->getDesc();
    }

    std::atomic<uint32_t>* getEventFlagWord() override { return mQueue->getEventFlagWord(); }

    virtual size_t availableToRead() override { return mQueue->availableToRead(); }

    virtual bool read(V2_1::Event* events, size_t numToRead) override {
        return mQueue->read(events, numToRead);
    }

    bool write(const std::vector<V2_1::Event>& events) override {
        return mQueue->write(events.data(), events.size());
    }

  private:
    std::unique_ptr<EventMessageQueue> mQueue;
};

}  // namespace implementation
}  // namespace V2_1
}  // namespace sensors
}  // namespace hardware
}  // namespace android

#endif  // ANDROID_HARDWARE_SENSORS_V2_1_EVENTMESSAGEQUEUEWRAPPER_H