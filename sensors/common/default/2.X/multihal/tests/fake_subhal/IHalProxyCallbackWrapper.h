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

#pragma once

#include "V2_0/SubHal.h"
#include "V2_1/SubHal.h"
#include "convertV2_1.h"

namespace android {
namespace hardware {
namespace sensors {
namespace V2_1 {
namespace subhal {
namespace implementation {

/**
 * The following callback wrapper classes abstract away common functionality across V2.0 and V2.1
 * interfaces. Much of the logic is common between the two versions and this allows users of the
 * classes to only care about the type used at initialization and then interact with either version
 * of the callback interface without worrying about the type.
 */
class IHalProxyCallbackWrapperBase {
  protected:
    using ScopedWakelock = V2_0::implementation::ScopedWakelock;

  public:
    virtual ~IHalProxyCallbackWrapperBase() {}

    virtual Return<void> onDynamicSensorsConnected(
            const hidl_vec<V2_1::SensorInfo>& sensorInfos) = 0;

    virtual Return<void> onDynamicSensorsDisconnected(const hidl_vec<int32_t>& sensorHandles) = 0;

    virtual void postEvents(const std::vector<V2_1::Event>& events, ScopedWakelock wakelock) = 0;

    virtual ScopedWakelock createScopedWakelock(bool lock) = 0;
};

template <typename T>
class HalProxyCallbackWrapperBase : public IHalProxyCallbackWrapperBase {
  public:
    HalProxyCallbackWrapperBase(sp<T> callback) : mCallback(callback){};

    Return<void> onDynamicSensorsDisconnected(const hidl_vec<int32_t>& sensorHandles) override {
        return mCallback->onDynamicSensorsDisconnected(sensorHandles);
    }

    ScopedWakelock createScopedWakelock(bool lock) override {
        return mCallback->createScopedWakelock(lock);
    }

  protected:
    sp<T> mCallback;
};

class HalProxyCallbackWrapperV2_0
    : public HalProxyCallbackWrapperBase<V2_0::implementation::IHalProxyCallback> {
  public:
    HalProxyCallbackWrapperV2_0(sp<V2_0::implementation::IHalProxyCallback> callback)
        : HalProxyCallbackWrapperBase(callback){};

    Return<void> onDynamicSensorsConnected(const hidl_vec<V2_1::SensorInfo>& sensorInfos) override {
        return mCallback->onDynamicSensorsConnected(
                V2_1::implementation::convertToOldSensorInfos(sensorInfos));
    }

    void postEvents(const std::vector<V2_1::Event>& events, ScopedWakelock wakelock) override {
        return mCallback->postEvents(V2_1::implementation::convertToOldEvents(events),
                                     std::move(wakelock));
    }
};

class HalProxyCallbackWrapperV2_1
    : public HalProxyCallbackWrapperBase<V2_1::implementation::IHalProxyCallback> {
  public:
    HalProxyCallbackWrapperV2_1(sp<V2_1::implementation::IHalProxyCallback> callback)
        : HalProxyCallbackWrapperBase(callback){};

    Return<void> onDynamicSensorsConnected(const hidl_vec<V2_1::SensorInfo>& sensorInfos) override {
        return mCallback->onDynamicSensorsConnected_2_1(sensorInfos);
    }

    void postEvents(const std::vector<V2_1::Event>& events, ScopedWakelock wakelock) {
        return mCallback->postEvents(events, std::move(wakelock));
    }
};

}  // namespace implementation
}  // namespace subhal
}  // namespace V2_1
}  // namespace sensors
}  // namespace hardware
}  // namespace android
