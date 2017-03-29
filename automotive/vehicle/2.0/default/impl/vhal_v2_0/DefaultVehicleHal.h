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

#ifndef android_hardware_automotive_vehicle_V2_0_impl_DefaultVehicleHal_H_
#define android_hardware_automotive_vehicle_V2_0_impl_DefaultVehicleHal_H_

#include <map>
#include <memory>
#include <sys/socket.h>
#include <thread>
#include <unordered_set>

#include <utils/SystemClock.h>

#include "CommBase.h"
#include "VehicleHalProto.pb.h"

#include <vhal_v2_0/RecurrentTimer.h>
#include <vhal_v2_0/VehicleHal.h>

#include "DefaultConfig.h"
#include "VehicleHalProto.pb.h"

namespace android {
namespace hardware {
namespace automotive {
namespace vehicle {
namespace V2_0 {

namespace impl {

class DefaultVehicleHal : public VehicleHal {
public:
    class CustomVehiclePropertyHandler {
    public:
        virtual VehiclePropValue* get() = 0;
        virtual StatusCode set(const VehiclePropValue& propValue) = 0;
        virtual ~CustomVehiclePropertyHandler() = default;
    };

protected:
    class StoredValueCustomVehiclePropertyHandler :
        public CustomVehiclePropertyHandler {
    public:
        VehiclePropValue* get() override {
            return mPropValue.get();
        }

        StatusCode set(const VehiclePropValue& propValue) {
            *mPropValue = propValue;
            return StatusCode::OK;
        }
private:
    std::unique_ptr<VehiclePropValue> mPropValue{new VehiclePropValue()};
};

public:
  DefaultVehicleHal() : mRecurrentTimer(
            std::bind(&DefaultVehicleHal::onContinuousPropertyTimer, this, std::placeholders::_1)) {
        for (size_t i = 0; i < arraysize(kVehicleProperties); i++) {
            const auto* config = &kVehicleProperties[i].config;
            mPropConfigMap[config->prop] = config;
        }
    }

    ~DefaultVehicleHal() override {
        // Notify thread to finish and wait for it to terminate
        mExit = 1;

        // Close emulator socket if it is open
        mComm->stop();

        mThread.join();
    }

    void onCreate() override;
    std::vector<VehiclePropConfig> listProperties() override;
    VehiclePropValuePtr get(const VehiclePropValue& requestedPropValue,
                            StatusCode* outStatus) override;
    StatusCode set(const VehiclePropValue& propValue) override;
    StatusCode subscribe(int32_t property, int32_t areas, float sampleRate) override;
    StatusCode unsubscribe(int32_t property) override;

    /**
     * Add custom property information to this HAL instance.
     *
     * This is useful for allowing later versions of Vehicle HAL to coalesce
     * the list of properties they support with a previous version of the HAL.
     *
     * @param property The identifier of the new property
     * @param handler The object that will handle get/set requests
     * @return OK on success, an error code on failure
     */
    virtual StatusCode addCustomProperty(int32_t,
        std::unique_ptr<CustomVehiclePropertyHandler>&&);

    /**
     * Add custom property information to this HAL instance.
     *
     * This is useful for allowing later versions of Vehicle HAL to coalesce
     * the list of properties they support with a previous version of the HAL.
     *
     * @param initialValue The initial value for the new property. This is not
     *                     constant data, as later set() operations can change
     *                     this value at will
     * @return OK on success, an error code on failure
     */
    virtual StatusCode addCustomProperty(const VehiclePropValue& initialValue) {
        auto handler = std::make_unique<StoredValueCustomVehiclePropertyHandler>();
        StatusCode response = handler->set(initialValue);
        return StatusCode::OK == response
               ? addCustomProperty(initialValue.prop, std::move(handler))
               : response;
    }

private:
    void doGetConfig(emulator::EmulatorMessage& rxMsg, emulator::EmulatorMessage& respMsg);
    void doGetConfigAll(emulator::EmulatorMessage& rxMsg, emulator::EmulatorMessage& respMsg);
    void doGetProperty(emulator::EmulatorMessage& rxMsg, emulator::EmulatorMessage& respMsg);
    void doGetPropertyAll(emulator::EmulatorMessage& rxMsg, emulator::EmulatorMessage& respMsg);
    void doSetProperty(emulator::EmulatorMessage& rxMsg, emulator::EmulatorMessage& respMsg);
    VehiclePropValue* getVehiclePropValueLocked(int32_t propId, int32_t areaId = 0);
    const VehiclePropConfig* getPropConfig(int32_t propId) const;
    bool isContinuousProperty(int32_t propId) const;
    void parseRxProtoBuf(std::vector<uint8_t>& msg);
    void populateProtoVehicleConfig(emulator::VehiclePropConfig* protoCfg,
                                    const VehiclePropConfig& cfg);
    void populateProtoVehiclePropValue(emulator::VehiclePropValue* protoVal,
                                       const VehiclePropValue* val);
    void rxMsg();
    void rxThread();
    void txMsg(emulator::EmulatorMessage& txMsg);
    StatusCode updateProperty(const VehiclePropValue& propValue);

    constexpr std::chrono::nanoseconds hertzToNanoseconds(float hz) const {
        return std::chrono::nanoseconds(static_cast<int64_t>(1000000000L / hz));
    }

    void onContinuousPropertyTimer(const std::vector<int32_t>& properties);

private:
    std::map<
        std::pair<int32_t /*VehicleProperty*/, int32_t /*areaId*/>,
        std::unique_ptr<CustomVehiclePropertyHandler>> mProps;
    std::atomic<int> mExit;
    std::unordered_set<int32_t> mHvacPowerProps;
    std::mutex mPropsMutex;
    std::thread mThread;
    std::unique_ptr<CommBase> mComm{nullptr};
    RecurrentTimer mRecurrentTimer;
    std::unordered_map<int32_t, const VehiclePropConfig*> mPropConfigMap;
};

}  // impl

}  // namespace V2_0
}  // namespace vehicle
}  // namespace automotive
}  // namespace hardware
}  // namespace android


#endif  // android_hardware_automotive_vehicle_V2_0_impl_DefaultVehicleHal_H_
