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

#define LOG_TAG "VehicleHalServer"

#include "VehicleHalServer.h"

#include <fstream>

#include <android-base/logging.h>
#include <utils/SystemClock.h>

#include "DefaultConfig.h"
#include "JsonFakeValueGenerator.h"
#include "LinearFakeValueGenerator.h"
#include "Obd2SensorStore.h"

namespace android::hardware::automotive::vehicle::V2_0::impl {

static bool isDiagnosticProperty(VehiclePropConfig propConfig) {
    switch (propConfig.prop) {
        case OBD2_LIVE_FRAME:
        case OBD2_FREEZE_FRAME:
        case OBD2_FREEZE_FRAME_CLEAR:
        case OBD2_FREEZE_FRAME_INFO:
            return true;
    }
    return false;
}

VehicleHalServer::VehicleHalServer() {
    constexpr bool shouldUpdateStatus = true;

    for (auto& it : kVehicleProperties) {
        VehiclePropConfig cfg = it.config;

        mServerSidePropStore.registerProperty(cfg);

        if (isDiagnosticProperty(cfg)) {
            continue;
        }

        // A global property will have only a single area
        int32_t numAreas = isGlobalProp(cfg.prop) ? 1 : cfg.areaConfigs.size();

        for (int i = 0; i < numAreas; i++) {
            int32_t curArea = isGlobalProp(cfg.prop) ? 0 : cfg.areaConfigs[i].areaId;

            // Create a separate instance for each individual zone
            VehiclePropValue prop = {
                    .areaId = curArea,
                    .prop = cfg.prop,
            };

            if (it.initialAreaValues.empty()) {
                prop.value = it.initialValue;
            } else if (auto valueForAreaIt = it.initialAreaValues.find(curArea);
                       valueForAreaIt != it.initialAreaValues.end()) {
                prop.value = valueForAreaIt->second;
            } else {
                LOG(WARNING) << __func__ << " failed to get default value for"
                             << " prop 0x" << std::hex << cfg.prop << " area 0x" << std::hex
                             << curArea;
                prop.status = VehiclePropertyStatus::UNAVAILABLE;
            }

            mServerSidePropStore.writeValue(prop, shouldUpdateStatus);
        }
    }
}

void VehicleHalServer::sendAllValuesToClient() {
    constexpr bool update_status = true;
    auto values = mServerSidePropStore.readAllValues();
    for (const auto& value : values) {
        onPropertyValueFromCar(value, update_status);
    }
}

GeneratorHub* VehicleHalServer::getGenerator() {
    return &mGeneratorHub;
}

VehiclePropValuePool* VehicleHalServer::getValuePool() const {
    if (!mValuePool) {
        LOG(WARNING) << __func__ << ": Value pool not set!";
    }
    return mValuePool;
}

void VehicleHalServer::setValuePool(VehiclePropValuePool* valuePool) {
    if (!valuePool) {
        LOG(WARNING) << __func__ << ": Setting value pool to nullptr!";
    }
    mValuePool = valuePool;
}

void VehicleHalServer::onFakeValueGenerated(const VehiclePropValue& value) {
    constexpr bool updateStatus = true;
    LOG(DEBUG) << __func__ << ": " << toString(value);
    auto updatedPropValue = getValuePool()->obtain(value);
    if (updatedPropValue) {
        updatedPropValue->timestamp = value.timestamp;
        updatedPropValue->status = VehiclePropertyStatus::AVAILABLE;
        mServerSidePropStore.writeValue(*updatedPropValue, updateStatus);
        onPropertyValueFromCar(*updatedPropValue, updateStatus);
    }
}

std::vector<VehiclePropConfig> VehicleHalServer::onGetAllPropertyConfig() const {
    return mServerSidePropStore.getAllConfigs();
}

StatusCode VehicleHalServer::handleGenerateFakeDataRequest(const VehiclePropValue& request) {
    constexpr bool updateStatus = true;

    LOG(INFO) << __func__;
    const auto& v = request.value;
    if (!v.int32Values.size()) {
        LOG(ERROR) << __func__ << ": expected at least \"command\" field in int32Values";
        return StatusCode::INVALID_ARG;
    }

    FakeDataCommand command = static_cast<FakeDataCommand>(v.int32Values[0]);

    switch (command) {
        case FakeDataCommand::StartLinear: {
            LOG(INFO) << __func__ << ", FakeDataCommand::StartLinear";
            if (v.int32Values.size() < 2) {
                LOG(ERROR) << __func__ << ": expected property ID in int32Values";
                return StatusCode::INVALID_ARG;
            }
            if (!v.int64Values.size()) {
                LOG(ERROR) << __func__ << ": interval is not provided in int64Values";
                return StatusCode::INVALID_ARG;
            }
            if (v.floatValues.size() < 3) {
                LOG(ERROR) << __func__ << ": expected at least 3 elements in floatValues, got: "
                           << v.floatValues.size();
                return StatusCode::INVALID_ARG;
            }
            int32_t cookie = v.int32Values[1];
            getGenerator()->registerGenerator(cookie,
                                              std::make_unique<LinearFakeValueGenerator>(request));
            break;
        }
        case FakeDataCommand::StartJson: {
            LOG(INFO) << __func__ << ", FakeDataCommand::StartJson";
            if (v.stringValue.empty()) {
                LOG(ERROR) << __func__ << ": path to JSON file is missing";
                return StatusCode::INVALID_ARG;
            }
            int32_t cookie = std::hash<std::string>()(v.stringValue);
            getGenerator()->registerGenerator(cookie,
                                              std::make_unique<JsonFakeValueGenerator>(request));
            break;
        }
        case FakeDataCommand::StopLinear: {
            LOG(INFO) << __func__ << ", FakeDataCommand::StopLinear";
            if (v.int32Values.size() < 2) {
                LOG(ERROR) << __func__ << ": expected property ID in int32Values";
                return StatusCode::INVALID_ARG;
            }
            int32_t cookie = v.int32Values[1];
            getGenerator()->unregisterGenerator(cookie);
            break;
        }
        case FakeDataCommand::StopJson: {
            LOG(INFO) << __func__ << ", FakeDataCommand::StopJson";
            if (v.stringValue.empty()) {
                LOG(ERROR) << __func__ << ": path to JSON file is missing";
                return StatusCode::INVALID_ARG;
            }
            int32_t cookie = std::hash<std::string>()(v.stringValue);
            getGenerator()->unregisterGenerator(cookie);
            break;
        }
        case FakeDataCommand::KeyPress: {
            LOG(INFO) << __func__ << ", FakeDataCommand::KeyPress";
            int32_t keyCode = request.value.int32Values[2];
            int32_t display = request.value.int32Values[3];
            // Send back to HAL
            onPropertyValueFromCar(
                    *createHwInputKeyProp(VehicleHwKeyInputAction::ACTION_DOWN, keyCode, display),
                    updateStatus);
            onPropertyValueFromCar(
                    *createHwInputKeyProp(VehicleHwKeyInputAction::ACTION_UP, keyCode, display),
                    updateStatus);
            break;
        }
        default: {
            LOG(ERROR) << __func__ << ": unexpected command: " << toInt(command);
            return StatusCode::INVALID_ARG;
        }
    }
    return StatusCode::OK;
}

VehicleHalServer::VehiclePropValuePtr VehicleHalServer::createApPowerStateReq(
        VehicleApPowerStateReq state, int32_t param) {
    auto req = getValuePool()->obtain(VehiclePropertyType::INT32_VEC, 2);
    req->prop = toInt(VehicleProperty::AP_POWER_STATE_REQ);
    req->areaId = 0;
    req->timestamp = elapsedRealtimeNano();
    req->status = VehiclePropertyStatus::AVAILABLE;
    req->value.int32Values[0] = toInt(state);
    req->value.int32Values[1] = param;
    return req;
}

VehicleHalServer::VehiclePropValuePtr VehicleHalServer::createHwInputKeyProp(
        VehicleHwKeyInputAction action, int32_t keyCode, int32_t targetDisplay) {
    auto keyEvent = getValuePool()->obtain(VehiclePropertyType::INT32_VEC, 3);
    keyEvent->prop = toInt(VehicleProperty::HW_KEY_INPUT);
    keyEvent->areaId = 0;
    keyEvent->timestamp = elapsedRealtimeNano();
    keyEvent->status = VehiclePropertyStatus::AVAILABLE;
    keyEvent->value.int32Values[0] = toInt(action);
    keyEvent->value.int32Values[1] = keyCode;
    keyEvent->value.int32Values[2] = targetDisplay;
    return keyEvent;
}

StatusCode VehicleHalServer::onSetProperty(const VehiclePropValue& value, bool updateStatus) {
    LOG(DEBUG) << "onSetProperty(" << value.prop << ")";

    // Some properties need to be treated non-trivially
    switch (value.prop) {
        case kGenerateFakeDataControllingProperty:
            return handleGenerateFakeDataRequest(value);

        // set the value from vehicle side, used in end to end test.
        case kSetIntPropertyFromVehicleForTest: {
            auto updatedPropValue = createVehiclePropValue(VehiclePropertyType::INT32, 1);
            updatedPropValue->prop = value.value.int32Values[0];
            updatedPropValue->value.int32Values[0] = value.value.int32Values[1];
            updatedPropValue->timestamp = value.value.int64Values[0];
            updatedPropValue->areaId = value.areaId;
            onPropertyValueFromCar(*updatedPropValue, updateStatus);
            return StatusCode::OK;
        }
        case kSetFloatPropertyFromVehicleForTest: {
            auto updatedPropValue = createVehiclePropValue(VehiclePropertyType::FLOAT, 1);
            updatedPropValue->prop = value.value.int32Values[0];
            updatedPropValue->value.floatValues[0] = value.value.floatValues[0];
            updatedPropValue->timestamp = value.value.int64Values[0];
            updatedPropValue->areaId = value.areaId;
            onPropertyValueFromCar(*updatedPropValue, updateStatus);
            return StatusCode::OK;
        }
        case kSetBooleanPropertyFromVehicleForTest: {
            auto updatedPropValue = createVehiclePropValue(VehiclePropertyType::BOOLEAN, 1);
            updatedPropValue->prop = value.value.int32Values[1];
            updatedPropValue->value.int32Values[0] = value.value.int32Values[0];
            updatedPropValue->timestamp = value.value.int64Values[0];
            updatedPropValue->areaId = value.areaId;
            onPropertyValueFromCar(*updatedPropValue, updateStatus);
            return StatusCode::OK;
        }

        case AP_POWER_STATE_REPORT:
            switch (value.value.int32Values[0]) {
                case toInt(VehicleApPowerStateReport::DEEP_SLEEP_EXIT):
                case toInt(VehicleApPowerStateReport::SHUTDOWN_CANCELLED):
                case toInt(VehicleApPowerStateReport::WAIT_FOR_VHAL):
                    // CPMS is in WAIT_FOR_VHAL state, simply move to ON
                    // Send back to HAL
                    // ALWAYS update status for generated property value
                    onPropertyValueFromCar(*createApPowerStateReq(VehicleApPowerStateReq::ON, 0),
                                           true /* updateStatus */);
                    break;
                case toInt(VehicleApPowerStateReport::DEEP_SLEEP_ENTRY):
                case toInt(VehicleApPowerStateReport::SHUTDOWN_START):
                    // CPMS is in WAIT_FOR_FINISH state, send the FINISHED command
                    // Send back to HAL
                    // ALWAYS update status for generated property value
                    onPropertyValueFromCar(
                            *createApPowerStateReq(VehicleApPowerStateReq::FINISHED, 0),
                            true /* updateStatus */);
                    break;
                case toInt(VehicleApPowerStateReport::ON):
                case toInt(VehicleApPowerStateReport::SHUTDOWN_POSTPONE):
                case toInt(VehicleApPowerStateReport::SHUTDOWN_PREPARE):
                    // Do nothing
                    break;
                default:
                    // Unknown state
                    break;
            }
            break;

#ifdef ENABLE_VENDOR_CLUSTER_PROPERTY_FOR_TESTING
        case toInt(VehicleProperty::CLUSTER_REPORT_STATE):
        case toInt(VehicleProperty::CLUSTER_REQUEST_DISPLAY):
        case toInt(VehicleProperty::CLUSTER_NAVIGATION_STATE):
        case VENDOR_CLUSTER_SWITCH_UI:
        case VENDOR_CLUSTER_DISPLAY_STATE: {
            auto updatedPropValue = createVehiclePropValue(getPropType(value.prop), 0);
            updatedPropValue->prop = value.prop & ~toInt(VehiclePropertyGroup::MASK);
            if (isSystemProperty(value.prop)) {
                updatedPropValue->prop |= toInt(VehiclePropertyGroup::VENDOR);
            } else {
                updatedPropValue->prop |= toInt(VehiclePropertyGroup::SYSTEM);
            }
            updatedPropValue->value = value.value;
            updatedPropValue->timestamp = elapsedRealtimeNano();
            updatedPropValue->areaId = value.areaId;
            onPropertyValueFromCar(*updatedPropValue, updateStatus);
            return StatusCode::OK;
        }
#endif  // ENABLE_VENDOR_CLUSTER_PROPERTY_FOR_TESTING

        default:
            break;
    }

    // In the real vhal, the value will be sent to Car ECU.
    // We just pretend it is done here and send back to HAL
    auto updatedPropValue = getValuePool()->obtain(value);
    updatedPropValue->timestamp = elapsedRealtimeNano();

    mServerSidePropStore.writeValue(*updatedPropValue, updateStatus);
    onPropertyValueFromCar(*updatedPropValue, updateStatus);
    return StatusCode::OK;
}

}  // namespace android::hardware::automotive::vehicle::V2_0::impl
