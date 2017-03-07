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

#define LOG_TAG "DefaultVehicleHal_v2_0"
#include <android/log.h>

#include <algorithm>
#include <android-base/properties.h>
#include <cstdio>

#include "DefaultVehicleHal.h"
#include "PipeComm.h"
#include "SocketComm.h"
#include "VehicleHalProto.pb.h"

namespace android {
namespace hardware {
namespace automotive {
namespace vehicle {
namespace V2_0 {

namespace impl {

void DefaultVehicleHal::doGetConfig(emulator::EmulatorMessage& rxMsg,
                                    emulator::EmulatorMessage& respMsg) {
    std::vector<VehiclePropConfig> configs = listProperties();
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

void DefaultVehicleHal::doGetConfigAll(emulator::EmulatorMessage& /* rxMsg */,
                                       emulator::EmulatorMessage& respMsg) {
    std::vector<VehiclePropConfig> configs = listProperties();

    respMsg.set_msg_type(emulator::GET_CONFIG_ALL_RESP);
    respMsg.set_status(emulator::RESULT_OK);

    for (auto& config : configs) {
        emulator::VehiclePropConfig* protoCfg = respMsg.add_config();
        populateProtoVehicleConfig(protoCfg, config);
    }
}

void DefaultVehicleHal::doGetProperty(emulator::EmulatorMessage& rxMsg,
                                      emulator::EmulatorMessage& respMsg) {
    int32_t areaId = 0;
    emulator::VehiclePropGet getProp = rxMsg.prop(0);
    int32_t propId = getProp.prop();
    emulator::Status status = emulator::ERROR_INVALID_PROPERTY;
    VehiclePropValue* val;

    respMsg.set_msg_type(emulator::GET_PROPERTY_RESP);

    if (getProp.has_area_id()) {
        areaId = getProp.area_id();
    }

    {
        std::lock_guard<std::mutex> lock(mPropsMutex);

        val = getVehiclePropValueLocked(propId, areaId);
        if (val != nullptr) {
            emulator::VehiclePropValue* protoVal = respMsg.add_value();
            populateProtoVehiclePropValue(protoVal, val);
            status = emulator::RESULT_OK;
        }
    }

    respMsg.set_status(status);
}

void DefaultVehicleHal::doGetPropertyAll(emulator::EmulatorMessage& /* rxMsg */,
                                         emulator::EmulatorMessage& respMsg) {
    respMsg.set_msg_type(emulator::GET_PROPERTY_ALL_RESP);
    respMsg.set_status(emulator::RESULT_OK);

    {
        std::lock_guard<std::mutex> lock(mPropsMutex);

        for (auto& prop : mProps) {
            emulator::VehiclePropValue* protoVal = respMsg.add_value();
            populateProtoVehiclePropValue(protoVal, prop.second.get());
        }
    }
}

void DefaultVehicleHal::doSetProperty(emulator::EmulatorMessage& rxMsg,
                                      emulator::EmulatorMessage& respMsg) {
    emulator::VehiclePropValue protoVal = rxMsg.value(0);
    VehiclePropValue val;

    respMsg.set_msg_type(emulator::SET_PROPERTY_RESP);

    val.prop = protoVal.prop();
    val.areaId = protoVal.area_id();
    val.timestamp = elapsedRealtimeNano();

    // Copy value data if it is set.  This automatically handles complex data types if needed.
    if (protoVal.has_string_value()) {
        val.value.stringValue = protoVal.string_value().c_str();
    }

    if (protoVal.has_bytes_value()) {
        std::vector<uint8_t> tmp(protoVal.bytes_value().begin(), protoVal.bytes_value().end());
        val.value.bytes = tmp;
    }

    if (protoVal.int32_values_size() > 0) {
        std::vector<int32_t> int32Values = std::vector<int32_t>(protoVal.int32_values_size());
        for (int i=0; i<protoVal.int32_values_size(); i++) {
            int32Values[i] = protoVal.int32_values(i);
        }
        val.value.int32Values = int32Values;
    }

    if (protoVal.int64_values_size() > 0) {
        std::vector<int64_t> int64Values = std::vector<int64_t>(protoVal.int64_values_size());
        for (int i=0; i<protoVal.int64_values_size(); i++) {
            int64Values[i] = protoVal.int64_values(i);
        }
        val.value.int64Values = int64Values;
    }

    if (protoVal.float_values_size() > 0) {
        std::vector<float> floatValues = std::vector<float>(protoVal.float_values_size());
        for (int i=0; i<protoVal.float_values_size(); i++) {
            floatValues[i] = protoVal.float_values(i);
        }
        val.value.floatValues = floatValues;
    }

    if (updateProperty(val) == StatusCode::OK) {
        // Send property up to VehicleHalManager via callback
        auto& pool = *getValuePool();
        VehiclePropValuePtr v = pool.obtain(val);

        doHalEvent(std::move(v));
        respMsg.set_status(emulator::RESULT_OK);
    } else {
        respMsg.set_status(emulator::ERROR_INVALID_PROPERTY);
    }
}

// This function should only be called while mPropsMutex is locked.
VehiclePropValue* DefaultVehicleHal::getVehiclePropValueLocked(int32_t propId, int32_t areaId) {
    if (getPropArea(propId) == VehicleArea::GLOBAL) {
        // In VehicleHal, global properties have areaId = -1.  We use 0.
        areaId = 0;
    }

    auto prop = mProps.find(std::make_pair(propId, areaId));
    if (prop != mProps.end()) {
        return prop->second.get();
    }
    ALOGW("%s: Property not found:  propId = 0x%x, areaId = 0x%x", __func__, propId, areaId);
    return nullptr;
}

void DefaultVehicleHal::parseRxProtoBuf(std::vector<uint8_t>& msg) {
    emulator::EmulatorMessage rxMsg;
    emulator::EmulatorMessage respMsg;

    if (rxMsg.ParseFromArray(msg.data(), msg.size())) {
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

        // Send the reply
        txMsg(respMsg);
    } else {
        ALOGE("%s: ParseFromString() failed. msgSize=%d", __func__, static_cast<int>(msg.size()));
    }
}

// Copies internal VehiclePropConfig data structure to protobuf VehiclePropConfig
void DefaultVehicleHal::populateProtoVehicleConfig(emulator::VehiclePropConfig* protoCfg,
                                                   const VehiclePropConfig& cfg) {
    protoCfg->set_prop(cfg.prop);
    protoCfg->set_access(toInt(cfg.access));
    protoCfg->set_change_mode(toInt(cfg.changeMode));
    protoCfg->set_value_type(toInt(getPropType(cfg.prop)));

    if (!isGlobalProp(cfg.prop)) {
        protoCfg->set_supported_areas(cfg.supportedAreas);
    }

    for (auto& configElement : cfg.configArray) {
        protoCfg->add_config_array(configElement);
    }

    if (cfg.configString.size() > 0) {
        protoCfg->set_config_string(cfg.configString.c_str(), cfg.configString.size());
    }

    // Populate the min/max values based on property type
    switch (getPropType(cfg.prop)) {
    case VehiclePropertyType::STRING:
    case VehiclePropertyType::BOOLEAN:
    case VehiclePropertyType::INT32_VEC:
    case VehiclePropertyType::FLOAT_VEC:
    case VehiclePropertyType::BYTES:
    case VehiclePropertyType::COMPLEX:
        // Do nothing.  These types don't have min/max values
        break;
    case VehiclePropertyType::INT64:
        if (cfg.areaConfigs.size() > 0) {
            emulator::VehicleAreaConfig* aCfg = protoCfg->add_area_configs();
            aCfg->set_min_int64_value(cfg.areaConfigs[0].minInt64Value);
            aCfg->set_max_int64_value(cfg.areaConfigs[0].maxInt64Value);
        }
        break;
    case VehiclePropertyType::FLOAT:
        if (cfg.areaConfigs.size() > 0) {
            emulator::VehicleAreaConfig* aCfg = protoCfg->add_area_configs();
            aCfg->set_min_float_value(cfg.areaConfigs[0].minFloatValue);
            aCfg->set_max_float_value(cfg.areaConfigs[0].maxFloatValue);
        }
        break;
    case VehiclePropertyType::INT32:
        if (cfg.areaConfigs.size() > 0) {
            emulator::VehicleAreaConfig* aCfg = protoCfg->add_area_configs();
            aCfg->set_min_int32_value(cfg.areaConfigs[0].minInt32Value);
            aCfg->set_max_int32_value(cfg.areaConfigs[0].maxInt32Value);
        }
        break;
    default:
        ALOGW("%s: Unknown property type:  0x%x", __func__, toInt(getPropType(cfg.prop)));
        break;
    }

    protoCfg->set_min_sample_rate(cfg.minSampleRate);
    protoCfg->set_max_sample_rate(cfg.maxSampleRate);
}

// Copies internal VehiclePropValue data structure to protobuf VehiclePropValue
void DefaultVehicleHal::populateProtoVehiclePropValue(emulator::VehiclePropValue* protoVal,
                                                      const VehiclePropValue* val) {
    protoVal->set_prop(val->prop);
    protoVal->set_value_type(toInt(getPropType(val->prop)));
    protoVal->set_timestamp(val->timestamp);
    protoVal->set_area_id(val->areaId);

    // Copy value data if it is set.
    //  - for bytes and strings, this is indicated by size > 0
    //  - for int32, int64, and float, copy the values if vectors have data
    if (val->value.stringValue.size() > 0) {
        protoVal->set_string_value(val->value.stringValue.c_str(), val->value.stringValue.size());
    }

    if (val->value.bytes.size() > 0) {
        protoVal->set_bytes_value(val->value.bytes.data(), val->value.bytes.size());
    }

    for (auto& int32Value : val->value.int32Values) {
        protoVal->add_int32_values(int32Value);
    }

    for (auto& int64Value : val->value.int64Values) {
        protoVal->add_int64_values(int64Value);
    }

    for (auto& floatValue : val->value.floatValues) {
        protoVal->add_float_values(floatValue);
    }
}

void DefaultVehicleHal::rxMsg() {
    int  numBytes = 0;

    while (mExit == 0) {
        std::vector<uint8_t> msg = mComm->read();

        if (msg.size() > 0) {
            // Received a message.
            parseRxProtoBuf(msg);
        } else {
            // This happens when connection is closed
            ALOGD("%s: numBytes=%d, msgSize=%d", __func__, numBytes,
                  static_cast<int32_t>(msg.size()));
            break;
        }
    }
}

void DefaultVehicleHal::rxThread() {
    bool isEmulator = android::base::GetBoolProperty("ro.kernel.qemu", false);

    if (isEmulator) {
        // Initialize pipe to Emulator
        mComm.reset(new PipeComm);
    } else {
        // Initialize socket over ADB
        mComm.reset(new SocketComm);
    }

    int retVal = mComm->open();

    if (retVal == 0) {
        // Comms are properly opened
        while (mExit == 0) {
            retVal = mComm->connect();

            if (retVal >= 0) {
                rxMsg();
            }

            // Check every 100ms for a new connection
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
        }
    }
}

// This function sets the default value of a property if we are interested in setting it.
// TODO:  Co-locate the default values with the configuration structure, to make it easier to
//          add new properties and their defaults.
void DefaultVehicleHal::setDefaultValue(VehiclePropValue* prop) {
    switch (prop->prop) {
    case toInt(VehicleProperty::INFO_MAKE):
        prop->value.stringValue = "Default Car";
        break;
    case toInt(VehicleProperty::PERF_VEHICLE_SPEED):
        prop->value.floatValues[0] = 0;
        break;
    case toInt(VehicleProperty::CURRENT_GEAR):
        prop->value.int32Values[0] = toInt(VehicleGear::GEAR_PARK);
        break;
    case toInt(VehicleProperty::PARKING_BRAKE_ON):
        prop->value.int32Values[0] = 1;
        break;
    case toInt(VehicleProperty::FUEL_LEVEL_LOW):
        prop->value.int32Values[0] = 0;
        break;
    case toInt(VehicleProperty::HVAC_POWER_ON):
        prop->value.int32Values[0] = 1;
        break;
    case toInt(VehicleProperty::HVAC_DEFROSTER):
        prop->value.int32Values[0] = 0;
        break;
    case toInt(VehicleProperty::HVAC_RECIRC_ON):
        prop->value.int32Values[0] = 1;
        break;
    case toInt(VehicleProperty::HVAC_AC_ON):
        prop->value.int32Values[0] = 1;
        break;
    case toInt(VehicleProperty::HVAC_AUTO_ON):
        prop->value.int32Values[0] = 1;
        break;
    case toInt(VehicleProperty::HVAC_FAN_SPEED):
        prop->value.int32Values[0] = 3;
        break;
    case toInt(VehicleProperty::HVAC_FAN_DIRECTION):
        prop->value.int32Values[0] = toInt(VehicleHvacFanDirection::FACE);
        break;
    case toInt(VehicleProperty::HVAC_TEMPERATURE_SET):
        prop->value.floatValues[0] = 16;
        break;
    case toInt(VehicleProperty::ENV_OUTSIDE_TEMPERATURE):
        prop->value.floatValues[0] = 25;
        break;
    case toInt(VehicleProperty::NIGHT_MODE):
        prop->value.int32Values[0] = 0;
        break;
    case toInt(VehicleProperty::DRIVING_STATUS):
        prop->value.int32Values[0] = toInt(VehicleDrivingStatus::UNRESTRICTED);
        break;
    case toInt(VehicleProperty::GEAR_SELECTION):
        prop->value.int32Values[0] = toInt(VehicleGear::GEAR_PARK);
        break;
    case toInt(VehicleProperty::INFO_FUEL_CAPACITY):
        prop->value.floatValues[0] = 0.75f;
        break;
    case toInt(VehicleProperty::ENGINE_OIL_TEMP):
        prop->value.floatValues[0] = 101;
        break;
    case toInt(VehicleProperty::DISPLAY_BRIGHTNESS):
        prop->value.int32Values[0] = 7;
        break;
    case toInt(VehicleProperty::IGNITION_STATE):
        prop->value.int32Values[0] = toInt(VehicleIgnitionState::ON);
        break;
    default:
        ALOGW("%s: propId=0x%x not found", __func__, prop->prop);
        break;
    }
}

// Transmit a reply back to the emulator
void DefaultVehicleHal::txMsg(emulator::EmulatorMessage& txMsg) {
    int numBytes = txMsg.ByteSize();
    std::vector<uint8_t> msg(numBytes);

    if (txMsg.SerializeToArray(msg.data(), msg.size())) {
        int retVal = 0;

        // Send the message
        if (mExit == 0) {
            mComm->write(msg);
        }

        if (retVal < 0) {
            ALOGE("%s: Failed to tx message: retval=%d, errno=%d", __func__, retVal, errno);
        }
    } else {
        ALOGE("%s: SerializeToString failed!", __func__);
    }
}

// Updates the property value held in the HAL
StatusCode DefaultVehicleHal::updateProperty(const VehiclePropValue& propValue) {
    auto propId = propValue.prop;
    auto areaId = propValue.areaId;
    StatusCode status = StatusCode::INVALID_ARG;

    {
        std::lock_guard<std::mutex> lock(mPropsMutex);

        VehiclePropValue* internalPropValue = getVehiclePropValueLocked(propId, areaId);
        if (internalPropValue != nullptr) {
            internalPropValue->value = propValue.value;
            internalPropValue->timestamp = propValue.timestamp;
            status = StatusCode::OK;
        }
    }
    return status;
}

VehicleHal::VehiclePropValuePtr DefaultVehicleHal::get(
        const VehiclePropValue& requestedPropValue, StatusCode* outStatus) {
    auto areaId = requestedPropValue.areaId;
    auto& pool = *getValuePool();
    auto propId = requestedPropValue.prop;
    StatusCode status;
    VehiclePropValuePtr v = nullptr;

    switch (propId) {
    default:
        {
            std::lock_guard<std::mutex> lock(mPropsMutex);

            VehiclePropValue *internalPropValue = getVehiclePropValueLocked(propId, areaId);
            if (internalPropValue != nullptr) {
                v = pool.obtain(*internalPropValue);
            }
        }

        if (v != nullptr) {
            status = StatusCode::OK;
        } else {
            status = StatusCode::INVALID_ARG;
        }
        break;
    }

    *outStatus = status;
    return v;
}

StatusCode DefaultVehicleHal::set(const VehiclePropValue& propValue) {
    auto propId = propValue.prop;
    StatusCode status;
    switch (propId) {
        default:
            if (mHvacPowerProps.find(VehicleProperty(propId)) !=
                    mHvacPowerProps.end()) {
                auto prop = mProps.find(
                    std::make_pair(toInt(VehicleProperty::HVAC_POWER_ON), 0));
                if (prop != mProps.end()) {
                    if (prop->second->value.int32Values.size() == 1 &&
                        prop->second->value.int32Values[0] == 0) {
                        status = StatusCode::NOT_AVAILABLE;
                        break;
                    }
                }
            }
            status = updateProperty(propValue);
            if (status == StatusCode::OK) {
                // Send property update to emulator
                emulator::EmulatorMessage msg;
                emulator::VehiclePropValue *val = msg.add_value();
                populateProtoVehiclePropValue(val, &propValue);
                msg.set_status(emulator::RESULT_OK);
                msg.set_msg_type(emulator::SET_PROPERTY_ASYNC);
                txMsg(msg);
            }
            break;
    }

    return status;
}

// Parse supported properties list and generate vector of property values to hold current values.
void DefaultVehicleHal::onCreate() {
    // Initialize member variables
    mExit = 0;

    for (auto& prop : kHvacPowerProperties) {
        mHvacPowerProps.insert(prop);
    }

    // Get the list of configurations supported by this HAL
    std::vector<VehiclePropConfig> configs = listProperties();

    for (auto& cfg : configs) {
        VehiclePropertyType propType = getPropType(cfg.prop);
        int32_t supportedAreas = cfg.supportedAreas;
        int32_t vecSize;

        // Set the vector size based on property type
        switch (propType) {
        case VehiclePropertyType::BOOLEAN:
        case VehiclePropertyType::INT32:
        case VehiclePropertyType::INT64:
        case VehiclePropertyType::FLOAT:
            vecSize = 1;
            break;
        case VehiclePropertyType::INT32_VEC:
        case VehiclePropertyType::FLOAT_VEC:
        case VehiclePropertyType::BYTES:
            // TODO:  Add proper support for these types
            vecSize = 1;
            break;
        case VehiclePropertyType::STRING:
            // Require individual handling
            vecSize = 0;
            break;
        case VehiclePropertyType::COMPLEX:
            switch (cfg.prop) {
            default:
                // Need to handle each complex property separately
                break;
            }
            continue;
        default:
            ALOGE("%s: propType=0x%x not found", __func__, propType);
            vecSize = 0;
            break;
        }

        //  A global property will have supportedAreas = 0
        if (getPropArea(cfg.prop) == VehicleArea::GLOBAL) {
            supportedAreas = 0;
        }

        // This loop is a do-while so it executes at least once to handle global properties
        do {
            int32_t curArea = supportedAreas;

            // Clear the right-most bit of supportedAreas
            supportedAreas &= supportedAreas - 1;

            // Set curArea to the previously cleared bit
            curArea ^= supportedAreas;

            // Create a separate instance for each individual zone
            std::unique_ptr<VehiclePropValue> prop = createVehiclePropValue(propType, vecSize);
            prop->areaId = curArea;
            prop->prop = cfg.prop;
            setDefaultValue(prop.get());
            mProps[std::make_pair(prop->prop, prop->areaId)] = std::move(prop);
        } while (supportedAreas != 0);
    }

    // Start rx thread
    mThread = std::thread(&DefaultVehicleHal::rxThread, this);
}

void DefaultVehicleHal::onContinuousPropertyTimer(const std::vector<int32_t>& properties) {
    VehiclePropValuePtr v;

    auto& pool = *getValuePool();

    for (int32_t property : properties) {
        if (isContinuousProperty(property)) {
            // In real implementation this value should be read from sensor, random
            // value used for testing purpose only.
            std::lock_guard<std::mutex> lock(mPropsMutex);

            VehiclePropValue *internalPropValue = getVehiclePropValueLocked(property);
            if (internalPropValue != nullptr) {
                v = pool.obtain(*internalPropValue);
            }
            if (VehiclePropertyType::FLOAT == getPropType(property)) {
                // Just get some randomness to continuous properties to see slightly differnt values
                // on the other end.
                v->value.floatValues[0] = v->value.floatValues[0] + std::rand() % 5;
            }
        } else {
            ALOGE("Unexpected onContinuousPropertyTimer for property: 0x%x", property);
        }

        if (v.get()) {
            v->timestamp = elapsedRealtimeNano();
            doHalEvent(std::move(v));
        }
    }
}

StatusCode DefaultVehicleHal::subscribe(int32_t property, int32_t,
                                        float sampleRate) {
    ALOGI("subscribe called for property: 0x%x, sampleRate: %f", property, sampleRate);

    if (isContinuousProperty(property)) {
        mRecurrentTimer.registerRecurrentEvent(hertzToNanoseconds(sampleRate), property);
    }
    return StatusCode::OK;
}

StatusCode DefaultVehicleHal::unsubscribe(int32_t property) {
    ALOGI("%s propId: 0x%x", __func__, property);
    if (isContinuousProperty(property)) {
        mRecurrentTimer.unregisterRecurrentEvent(property);
    }
    return StatusCode::OK;
}

const VehiclePropConfig* DefaultVehicleHal::getPropConfig(int32_t propId) const {
    auto it = mPropConfigMap.find(propId);
    return it == mPropConfigMap.end() ? nullptr : it->second;
}

bool DefaultVehicleHal::isContinuousProperty(int32_t propId) const {
    const VehiclePropConfig* config = getPropConfig(propId);
    if (config == nullptr) {
        ALOGW("Config not found for property: 0x%x", propId);
        return false;
    }
    return config->changeMode == VehiclePropertyChangeMode::CONTINUOUS;
}

}  // impl

}  // namespace V2_0
}  // namespace vehicle
}  // namespace automotive
}  // namespace hardware
}  // namespace android
