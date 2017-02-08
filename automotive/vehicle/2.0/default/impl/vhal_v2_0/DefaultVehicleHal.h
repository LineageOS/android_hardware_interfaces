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

#include <memory>
#include <sys/socket.h>
#include <thread>

#include <utils/SystemClock.h>

#include <vhal_v2_0/VehicleHal.h>
#include <vhal_v2_0/Obd2SensorStore.h>

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
    DefaultVehicleHal() : mThread() {}
    ~DefaultVehicleHal() override {
        // Notify thread to finish and wait for it to terminate
        mExit = 1;

        // Close emulator socket if it is open
        {
            std::lock_guard<std::mutex> lock(mTxMutex);
            if (mCurSocket != -1) {
                close(mCurSocket);
                mCurSocket = -1;
            }
        }

        mThread.join();
    }

    std::vector<VehiclePropConfig> listProperties() override {
        return std::vector<VehiclePropConfig>(std::begin(kVehicleProperties),
                                              std::end(kVehicleProperties));
    }

    VehiclePropValuePtr get(const VehiclePropValue& requestedPropValue,
                            StatusCode* outStatus) override;

    void onCreate() override;

    StatusCode set(const VehiclePropValue& propValue) override;

    StatusCode subscribe(int32_t property, int32_t areas, float sampleRate) {
        ALOGD("%s: not implemented: prop=0x%x, areas=0x%x, rate=%f", __FUNCTION__, property,
              areas, sampleRate);
        return StatusCode::OK;
    }

    StatusCode unsubscribe(int32_t property) {
        ALOGD("%s: not implemented: prop=0x%x", __FUNCTION__, property);
        return StatusCode::OK;
    }

private:
    void doGetConfig(emulator::EmulatorMessage& rxMsg, emulator::EmulatorMessage& respMsg);
    void doGetConfigAll(emulator::EmulatorMessage& rxMsg, emulator::EmulatorMessage& respMsg);
    void doGetProperty(emulator::EmulatorMessage& rxMsg, emulator::EmulatorMessage& respMsg);
    void doGetPropertyAll(emulator::EmulatorMessage& rxMsg, emulator::EmulatorMessage& respMsg);
    void doSetProperty(emulator::EmulatorMessage& rxMsg, emulator::EmulatorMessage& respMsg);
    VehiclePropValue* getVehiclePropValueLocked(int32_t propId, int32_t areaId);
    void initObd2LiveFrame(VehiclePropConfig& propConfig);
    void initObd2FreezeFrame(VehiclePropConfig& propConfig);
    void parseRxProtoBuf(std::vector<uint8_t>& msg);
    void populateProtoVehicleConfig(emulator::VehiclePropConfig* protoCfg,
                                    const VehiclePropConfig& cfg);
    void populateProtoVehiclePropValue(emulator::VehiclePropValue* protoVal,
                                       const VehiclePropValue* val);
    void setDefaultValue(VehiclePropValue* prop);
    void rxMsg(void);
    void rxThread(void);
    void txMsg(emulator::EmulatorMessage& txMsg);
    StatusCode updateProperty(const VehiclePropValue& propValue);
    StatusCode fillObd2LiveFrame(VehiclePropValue* v);
    StatusCode fillObd2FreezeFrame(const VehiclePropValue& requestedPropValue,
            VehiclePropValue* v);
    StatusCode fillObd2DtcInfo(VehiclePropValue *v);
    StatusCode clearObd2FreezeFrames(const VehiclePropValue& propValue);
private:
    // TODO:  Use a hashtable to support indexing props
    std::vector<std::unique_ptr<VehiclePropValue>> mProps;
    std::atomic<int> mCurSocket;
    std::atomic<int> mExit;
    std::unique_ptr<VehiclePropValue> mLiveObd2Frame {nullptr};
    std::vector<std::unique_ptr<VehiclePropValue>> mFreezeObd2Frames;
    std::mutex mPropsMutex;
    int mSocket;
    std::mutex mTxMutex;
    std::thread mThread;
};

}  // impl

}  // namespace V2_0
}  // namespace vehicle
}  // namespace automotive
}  // namespace hardware
}  // namespace android


#endif  // android_hardware_automotive_vehicle_V2_0_impl_DefaultVehicleHal_H_
