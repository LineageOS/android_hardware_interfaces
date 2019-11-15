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
#define LOG_TAG "VehicleEmulator_v2_0"
#include <android/log.h>

#include <android-base/properties.h>
#include <log/log.h>
#include <utils/SystemClock.h>
#include <algorithm>

#include <vhal_v2_0/VehicleUtils.h>

#include "PipeComm.h"
#include "ProtoMessageConverter.h"
#include "SocketComm.h"

#include "VehicleEmulator.h"

namespace android {
namespace hardware {
namespace automotive {
namespace vehicle {
namespace V2_0 {

namespace impl {

VehicleEmulator::VehicleEmulator(EmulatedVehicleHalIface* hal) : mHal{hal} {
    mHal->registerEmulator(this);

    ALOGI("Starting SocketComm");
    mSocketComm = std::make_unique<SocketComm>(this);
    mSocketComm->start();

    if (android::base::GetBoolProperty("ro.kernel.qemu", false)) {
        ALOGI("Starting PipeComm");
        mPipeComm = std::make_unique<PipeComm>(this);
        mPipeComm->start();
    }
}

VehicleEmulator::~VehicleEmulator() {
    mSocketComm->stop();
    if (mPipeComm) {
        mPipeComm->stop();
    }
}

/**
 * This is called by the HAL when a property changes. We need to notify our clients that it has
 * changed.
 */
void VehicleEmulator::doSetValueFromClient(const VehiclePropValue& propValue) {
    emulator::EmulatorMessage msg;
    emulator::VehiclePropValue *val = msg.add_value();
    populateProtoVehiclePropValue(val, &propValue);
    msg.set_status(emulator::RESULT_OK);
    msg.set_msg_type(emulator::SET_PROPERTY_ASYNC);

    mSocketComm->sendMessage(msg);
    if (mPipeComm) {
        mPipeComm->sendMessage(msg);
    }
}

void VehicleEmulator::doGetConfig(VehicleEmulator::EmulatorMessage const& rxMsg,
                                  VehicleEmulator::EmulatorMessage& respMsg) {
    std::vector<VehiclePropConfig> configs = mHal->listProperties();
    emulator::VehiclePropGet getProp = rxMsg.prop(0);

    respMsg.set_msg_type(emulator::GET_CONFIG_RESP);
    respMsg.set_status(emulator::ERROR_INVALID_PROPERTY);

    for (auto& config : configs) {
        // Find the config we are looking for
        if (config.prop == getProp.prop()) {
            emulator::VehiclePropConfig* protoCfg = respMsg.add_config();
            populateProtoVehicleConfig(protoCfg, config);
            respMsg.set_status(emulator::RESULT_OK);
            break;
        }
    }
}

void VehicleEmulator::doGetConfigAll(VehicleEmulator::EmulatorMessage const& /* rxMsg */,
                                     VehicleEmulator::EmulatorMessage& respMsg) {
    std::vector<VehiclePropConfig> configs = mHal->listProperties();

    respMsg.set_msg_type(emulator::GET_CONFIG_ALL_RESP);
    respMsg.set_status(emulator::RESULT_OK);

    for (auto& config : configs) {
        emulator::VehiclePropConfig* protoCfg = respMsg.add_config();
        populateProtoVehicleConfig(protoCfg, config);
    }
}

void VehicleEmulator::doGetProperty(VehicleEmulator::EmulatorMessage const& rxMsg,
                                    VehicleEmulator::EmulatorMessage& respMsg) {
    int32_t areaId = 0;
    emulator::VehiclePropGet getProp = rxMsg.prop(0);
    int32_t propId = getProp.prop();
    emulator::Status status = emulator::ERROR_INVALID_PROPERTY;

    respMsg.set_msg_type(emulator::GET_PROPERTY_RESP);

    if (getProp.has_area_id()) {
        areaId = getProp.area_id();
    }

    {
        VehiclePropValue request = {
                .areaId = areaId,
                .prop = propId,
        };
        StatusCode halStatus;
        auto val = mHal->get(request, &halStatus);
        if (val != nullptr) {
            emulator::VehiclePropValue* protoVal = respMsg.add_value();
            populateProtoVehiclePropValue(protoVal, val.get());
            status = emulator::RESULT_OK;
        }
    }

    respMsg.set_status(status);
}

void VehicleEmulator::doGetPropertyAll(VehicleEmulator::EmulatorMessage const& /* rxMsg */,
                                       VehicleEmulator::EmulatorMessage& respMsg) {
    respMsg.set_msg_type(emulator::GET_PROPERTY_ALL_RESP);
    respMsg.set_status(emulator::RESULT_OK);

    {
        for (const auto& prop : mHal->getAllProperties()) {
            emulator::VehiclePropValue* protoVal = respMsg.add_value();
            populateProtoVehiclePropValue(protoVal, &prop);
        }
    }
}

void VehicleEmulator::doSetProperty(VehicleEmulator::EmulatorMessage const& rxMsg,
                                    VehicleEmulator::EmulatorMessage& respMsg) {
    emulator::VehiclePropValue protoVal = rxMsg.value(0);
    VehiclePropValue val = {
            .timestamp = elapsedRealtimeNano(),
            .areaId = protoVal.area_id(),
            .prop = protoVal.prop(),
            .status = (VehiclePropertyStatus)protoVal.status(),
    };

    respMsg.set_msg_type(emulator::SET_PROPERTY_RESP);

    // Copy value data if it is set.  This automatically handles complex data types if needed.
    if (protoVal.has_string_value()) {
        val.value.stringValue = protoVal.string_value().c_str();
    }

    if (protoVal.has_bytes_value()) {
        val.value.bytes = std::vector<uint8_t> { protoVal.bytes_value().begin(),
                                                 protoVal.bytes_value().end() };
    }

    if (protoVal.int32_values_size() > 0) {
        val.value.int32Values = std::vector<int32_t> { protoVal.int32_values().begin(),
                                                       protoVal.int32_values().end() };
    }

    if (protoVal.int64_values_size() > 0) {
        val.value.int64Values = std::vector<int64_t> { protoVal.int64_values().begin(),
                                                       protoVal.int64_values().end() };
    }

    if (protoVal.float_values_size() > 0) {
        val.value.floatValues = std::vector<float> { protoVal.float_values().begin(),
                                                     protoVal.float_values().end() };
    }

    bool halRes = mHal->setPropertyFromVehicle(val);
    respMsg.set_status(halRes ? emulator::RESULT_OK : emulator::ERROR_INVALID_PROPERTY);
}

void VehicleEmulator::processMessage(emulator::EmulatorMessage const& rxMsg,
                                     emulator::EmulatorMessage& respMsg) {
    switch (rxMsg.msg_type()) {
        case emulator::GET_CONFIG_CMD:
            doGetConfig(rxMsg, respMsg);
            break;
        case emulator::GET_CONFIG_ALL_CMD:
            doGetConfigAll(rxMsg, respMsg);
            break;
        case emulator::GET_PROPERTY_CMD:
            doGetProperty(rxMsg, respMsg);
            break;
        case emulator::GET_PROPERTY_ALL_CMD:
            doGetPropertyAll(rxMsg, respMsg);
            break;
        case emulator::SET_PROPERTY_CMD:
            doSetProperty(rxMsg, respMsg);
            break;
        default:
            ALOGW("%s: Unknown message received, type = %d", __func__, rxMsg.msg_type());
            respMsg.set_status(emulator::ERROR_UNIMPLEMENTED_CMD);
            break;
    }
}

void VehicleEmulator::populateProtoVehicleConfig(emulator::VehiclePropConfig* protoCfg,
                                                 const VehiclePropConfig& cfg) {
    return proto_msg_converter::toProto(protoCfg, cfg);
}

void VehicleEmulator::populateProtoVehiclePropValue(emulator::VehiclePropValue* protoVal,
                                                    const VehiclePropValue* val) {
    return proto_msg_converter::toProto(protoVal, *val);
}

}  // impl

}  // namespace V2_0
}  // namespace vehicle
}  // namespace automotive
}  // namespace hardware
}  // namespace android
