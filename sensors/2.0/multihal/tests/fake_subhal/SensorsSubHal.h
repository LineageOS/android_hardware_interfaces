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

#pragma once

#include "SubHal.h"

#include "Sensor.h"

#include <vector>

namespace android {
namespace hardware {
namespace sensors {
namespace V2_0 {
namespace subhal {
namespace implementation {

using ::android::hardware::sensors::V1_0::OperationMode;
using ::android::hardware::sensors::V1_0::Result;
using ::android::hardware::sensors::V2_0::implementation::IHalProxyCallback;

/**
 * Implementation of a ISensorsSubHal that can be used to test the implementation of multihal 2.0.
 * See the README file for more details on how this class can be used for testing.
 */
class SensorsSubHal : public ISensorsSubHal, public ISensorsEventCallback {
    using Event = ::android::hardware::sensors::V1_0::Event;
    using RateLevel = ::android::hardware::sensors::V1_0::RateLevel;
    using SharedMemInfo = ::android::hardware::sensors::V1_0::SharedMemInfo;

  public:
    SensorsSubHal();

    // Methods from ::android::hardware::sensors::V2_0::ISensors follow.
    virtual Return<void> getSensorsList(getSensorsList_cb _hidl_cb) override;

    virtual Return<Result> setOperationMode(OperationMode mode) override;

    OperationMode getOperationMode() const { return mCurrentOperationMode; }

    Return<Result> activate(int32_t sensorHandle, bool enabled) override;

    Return<Result> batch(int32_t sensorHandle, int64_t samplingPeriodNs,
                         int64_t maxReportLatencyNs) override;

    Return<Result> flush(int32_t sensorHandle) override;

    Return<Result> injectSensorData(const Event& event) override;

    Return<void> registerDirectChannel(const SharedMemInfo& mem,
                                       registerDirectChannel_cb _hidl_cb) override;

    Return<Result> unregisterDirectChannel(int32_t channelHandle) override;

    Return<void> configDirectReport(int32_t sensorHandle, int32_t channelHandle, RateLevel rate,
                                    configDirectReport_cb _hidl_cb) override;

    Return<void> debug(const hidl_handle& fd, const hidl_vec<hidl_string>& args) override;

    // Methods from ::android::hardware::sensors::V2_0::implementation::ISensorsSubHal follow.
    const std::string getName() override {
#ifdef SUB_HAL_NAME
        return SUB_HAL_NAME;
#else   // SUB_HAL_NAME
        return "FakeSubHal";
#endif  // SUB_HAL_NAME
    }

    Return<Result> initialize(const sp<IHalProxyCallback>& halProxyCallback) override;

    // Method from ISensorsEventCallback.
    void postEvents(const std::vector<Event>& events, bool wakeup) override;

  protected:
    template <class SensorType>
    void AddSensor() {
        std::shared_ptr<SensorType> sensor =
                std::make_shared<SensorType>(mNextHandle++ /* sensorHandle */, this /* callback */);
        mSensors[sensor->getSensorInfo().sensorHandle] = sensor;
    }

    /**
     * A map of the available sensors
     */
    std::map<int32_t, std::shared_ptr<Sensor>> mSensors;

    /**
     * Callback used to communicate to the HalProxy when dynamic sensors are connected /
     * disconnected, sensor events need to be sent to the framework, and when a wakelock should be
     * acquired.
     */
    sp<IHalProxyCallback> mCallback;

  private:
    /**
     * The current operation mode of the multihal framework. Ensures that all subhals are set to
     * the same operation mode.
     */
    OperationMode mCurrentOperationMode = OperationMode::NORMAL;

    /**
     * The next available sensor handle
     */
    int32_t mNextHandle;
};

// SubHal that has continuous sensors for testing purposes.
class ContinuousSensorsSubHal : public SensorsSubHal {
  public:
    ContinuousSensorsSubHal();
};

// SubHal that has on-change sensors for testing purposes.
class OnChangeSensorsSubHal : public SensorsSubHal {
  public:
    OnChangeSensorsSubHal();
};

// SubHal that has both continuous and on-change sensors for testing purposes.
class AllSensorsSubHal : public SensorsSubHal {
  public:
    AllSensorsSubHal();
};

class SetOperationModeFailingSensorsSubHal : public AllSensorsSubHal {
  public:
    Return<Result> setOperationMode(OperationMode mode) override;
};

class AllSupportDirectChannelSensorsSubHal : public AllSensorsSubHal {
  public:
    Return<void> getSensorsList(getSensorsList_cb _hidl_cb) override;
};

class DoesNotSupportDirectChannelSensorsSubHal : public AllSensorsSubHal {
  public:
    Return<void> getSensorsList(getSensorsList_cb _hidl_cb) override;
};

class AddAndRemoveDynamicSensorsSubHal : public AllSensorsSubHal {
  public:
    void addDynamicSensors(const std::vector<SensorInfo>& sensorsAdded);
    void removeDynamicSensors(const std::vector<int32_t>& sensorHandlesAdded);
};

}  // namespace implementation
}  // namespace subhal
}  // namespace V2_0
}  // namespace sensors
}  // namespace hardware
}  // namespace android
