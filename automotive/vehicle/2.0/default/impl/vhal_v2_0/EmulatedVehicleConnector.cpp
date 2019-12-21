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
#include <android-base/logging.h>
#include <utils/SystemClock.h>

#include "DefaultConfig.h"
#include "EmulatedVehicleConnector.h"
#include "JsonFakeValueGenerator.h"
#include "LinearFakeValueGenerator.h"
#include "Obd2SensorStore.h"

namespace android {
namespace hardware {
namespace automotive {
namespace vehicle {
namespace V2_0 {

namespace impl {

void EmulatedVehicleClient::onPropertyValue(const VehiclePropValue& value, bool updateStatus) {
    if (!mPropCallback) {
        LOG(ERROR) << __func__ << ": PropertyCallBackType is not registered!";
        return;
    }
    return mPropCallback(value, updateStatus);
}

void EmulatedVehicleClient::registerPropertyValueCallback(PropertyCallBackType&& callback) {
    if (mPropCallback) {
        LOG(ERROR) << __func__ << ": Cannot register multiple callbacks!";
        return;
    }
    mPropCallback = std::move(callback);
}

GeneratorHub* EmulatedVehicleServer::getGenerator() {
    return &mGeneratorHub;
}

VehiclePropValuePool* EmulatedVehicleServer::getValuePool() const {
    if (!mValuePool) {
        LOG(WARNING) << __func__ << ": Value pool not set!";
    }
    return mValuePool;
}

void EmulatedVehicleServer::setValuePool(VehiclePropValuePool* valuePool) {
    if (!valuePool) {
        LOG(WARNING) <<  __func__ << ": Setting value pool to nullptr!";
    }
    mValuePool = valuePool;
}

void EmulatedVehicleServer::onFakeValueGenerated(const VehiclePropValue& value) {
    constexpr bool updateStatus = true;
    LOG(DEBUG) << __func__ << ": " << toString(value);
    auto updatedPropValue = getValuePool()->obtain(value);
    if (updatedPropValue) {
        updatedPropValue->timestamp = value.timestamp;
        updatedPropValue->status = VehiclePropertyStatus::AVAILABLE;
        onPropertyValueFromCar(*updatedPropValue, updateStatus);
    }
}

std::vector<VehiclePropConfig> EmulatedVehicleServer::onGetAllPropertyConfig() const {
    std::vector<VehiclePropConfig> vehiclePropConfigs;
    constexpr size_t numOfVehiclePropConfigs =
            sizeof(kVehicleProperties) / sizeof(kVehicleProperties[0]);
    vehiclePropConfigs.reserve(numOfVehiclePropConfigs);
    for (auto& it : kVehicleProperties) {
        vehiclePropConfigs.emplace_back(it.config);
    }
    return vehiclePropConfigs;
}

StatusCode EmulatedVehicleServer::handleGenerateFakeDataRequest(const VehiclePropValue& request) {
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

VehicleHal::VehiclePropValuePtr EmulatedVehicleServer::createApPowerStateReq(
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

VehicleHal::VehiclePropValuePtr EmulatedVehicleServer::createHwInputKeyProp(
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

StatusCode EmulatedVehicleServer::onSetProperty(const VehiclePropValue& value, bool updateStatus) {
    // Some properties need to be treated non-trivially
    switch (value.prop) {
        case kGenerateFakeDataControllingProperty:
            return handleGenerateFakeDataRequest(value);

        // set the value from vehcile side, used in end to end test.
        case kSetIntPropertyFromVehcileForTest: {
            auto updatedPropValue = createVehiclePropValue(VehiclePropertyType::INT32, 1);
            updatedPropValue->prop = value.value.int32Values[0];
            updatedPropValue->value.int32Values[0] = value.value.int32Values[1];
            updatedPropValue->timestamp = value.value.int64Values[0];
            updatedPropValue->areaId = value.areaId;
            onPropertyValueFromCar(*updatedPropValue, updateStatus);
            return StatusCode::OK;
        }
        case kSetFloatPropertyFromVehcileForTest: {
            auto updatedPropValue = createVehiclePropValue(VehiclePropertyType::FLOAT, 1);
            updatedPropValue->prop = value.value.int32Values[0];
            updatedPropValue->value.floatValues[0] = value.value.floatValues[0];
            updatedPropValue->timestamp = value.value.int64Values[0];
            updatedPropValue->areaId = value.areaId;
            onPropertyValueFromCar(*updatedPropValue, updateStatus);
            return StatusCode::OK;
        }
        case kSetBooleanPropertyFromVehcileForTest: {
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
        default:
            break;
    }

    // In the real vhal, the value will be sent to Car ECU.
    // We just pretend it is done here and send back to HAL
    auto updatedPropValue = getValuePool()->obtain(value);
    updatedPropValue->timestamp = elapsedRealtimeNano();

    onPropertyValueFromCar(*updatedPropValue, updateStatus);
    return StatusCode::OK;
}

EmulatedPassthroughConnectorPtr makeEmulatedPassthroughConnector() {
    return std::make_unique<EmulatedPassthroughConnector>();
}

}  // namespace impl

}  // namespace V2_0
}  // namespace vehicle
}  // namespace automotive
}  // namespace hardware
}  // namespace android
