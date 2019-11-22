/*
 * Copyright (C) 2017 The Android Open Source Project
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

#ifndef android_hardware_automotive_vehicle_V2_0_impl_VehicleHalEmulator_H_
#define android_hardware_automotive_vehicle_V2_0_impl_VehicleHalEmulator_H_

#include <log/log.h>
#include <memory>
#include <thread>
#include <vector>

#include "vhal_v2_0/VehicleHal.h"

#include "CommConn.h"
#include "PipeComm.h"
#include "SocketComm.h"
#include "VehicleHalProto.pb.h"

namespace android {
namespace hardware {
namespace automotive {
namespace vehicle {
namespace V2_0 {

namespace impl {

class VehicleEmulator;  // Forward declaration.

/** Extension of VehicleHal that used by VehicleEmulator. */
class EmulatedVehicleHalIface : public VehicleHal {
public:
    virtual bool setPropertyFromVehicle(const VehiclePropValue& propValue) = 0;
    virtual std::vector<VehiclePropValue> getAllProperties() const = 0;

    void registerEmulator(VehicleEmulator* emulator) {
        ALOGI("%s, emulator: %p", __func__, emulator);
        std::lock_guard<std::mutex> g(mEmulatorLock);
        mEmulator = emulator;
    }

protected:
    VehicleEmulator* getEmulatorOrDie() {
        std::lock_guard<std::mutex> g(mEmulatorLock);
        if (mEmulator == nullptr) abort();
        return mEmulator;
    }

private:
    mutable std::mutex mEmulatorLock;
    VehicleEmulator* mEmulator;
};

/**
 * Emulates vehicle by providing controlling interface from host side either through ADB or Pipe.
 */
class VehicleEmulator : public MessageProcessor {
   public:
    VehicleEmulator(EmulatedVehicleHalIface* hal);
    virtual ~VehicleEmulator();

    void doSetValueFromClient(const VehiclePropValue& propValue);
    void processMessage(vhal_proto::EmulatorMessage const& rxMsg,
                        vhal_proto::EmulatorMessage& respMsg) override;

   private:
    friend class ConnectionThread;
    using EmulatorMessage = vhal_proto::EmulatorMessage;

    void doGetConfig(EmulatorMessage const& rxMsg, EmulatorMessage& respMsg);
    void doGetConfigAll(EmulatorMessage const& rxMsg, EmulatorMessage& respMsg);
    void doGetProperty(EmulatorMessage const& rxMsg, EmulatorMessage& respMsg);
    void doGetPropertyAll(EmulatorMessage const& rxMsg, EmulatorMessage& respMsg);
    void doSetProperty(EmulatorMessage const& rxMsg, EmulatorMessage& respMsg);
    void populateProtoVehicleConfig(vhal_proto::VehiclePropConfig* protoCfg,
                                    const VehiclePropConfig& cfg);
    void populateProtoVehiclePropValue(vhal_proto::VehiclePropValue* protoVal,
                                       const VehiclePropValue* val);

private:
    EmulatedVehicleHalIface* mHal;
    std::unique_ptr<SocketComm> mSocketComm;
    std::unique_ptr<PipeComm> mPipeComm;
};

}  // impl

}  // namespace V2_0
}  // namespace vehicle
}  // namespace automotive
}  // namespace hardware
}  // namespace android

#endif // android_hardware_automotive_vehicle_V2_0_impl_VehicleHalEmulator_H_
