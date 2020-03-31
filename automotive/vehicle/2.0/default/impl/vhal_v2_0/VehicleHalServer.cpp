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
        onPropertyValueFromCar(*updatedPropValue, updateStatus);
    }
}

std::vector<VehiclePropConfig> VehicleHalServer::onGetAllPropertyConfig() const {
    std::vector<VehiclePropConfig> vehiclePropConfigs;
    constexpr size_t numOfVehiclePropConfigs =
            sizeof(kVehicleProperties) / sizeof(kVehicleProperties[0]);
    vehiclePropConfigs.reserve(numOfVehiclePropConfigs);
    for (auto& it : kVehicleProperties) {
        vehiclePropConfigs.emplace_back(it.config);
    }
    return vehiclePropConfigs;
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
        case INITIAL_USER_INFO:
            return onSetInitialUserInfoResponse(value, updateStatus);
        case SWITCH_USER:
            return onSetSwitchUserResponse(value, updateStatus);
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

/**
 * INITIAL_USER_INFO is called by Android when it starts, and it's expecting a property change
 * indicating what the initial user should be.
 *
 * During normal circumstances, the emulator will reply right away, passing a response if
 * InitialUserInfoResponseAction::DEFAULT (so Android could use its own logic to decide which user
 * to boot).
 *
 * But during development / testing, the behavior can be changed using lshal dump, which must use
 * the areaId to indicate what should happen next.
 *
 * So, the behavior of set(INITIAL_USER_INFO) is:
 *
 * - if it has an areaId, store the property into mInitialUserResponseFromCmd (as it was called by
 *   lshal).
 * - else if mInitialUserResponseFromCmd is not set, return a response with the same request id and
 *   InitialUserInfoResponseAction::DEFAULT
 * - else the behavior is defined by the areaId on mInitialUserResponseFromCmd:
 * - if it's 1, reply with mInitialUserResponseFromCmd and the right request id
 * - if it's 2, reply with mInitialUserResponseFromCmd but a wrong request id (so Android can test
 *   this error scenario)
 * - if it's 3, then don't send a property change (so Android can emulate a timeout)
 *
 */
StatusCode VehicleHalServer::onSetInitialUserInfoResponse(const VehiclePropValue& value,
                                                          bool updateStatus) {
    // TODO: LOG calls below might be more suited to be DEBUG, but those are not being logged
    // (even when explicitly calling setprop log.tag. As this class should be using ALOG instead of
    // LOG, it's not worth investigating why...

    if (value.value.int32Values.size() == 0) {
        LOG(ERROR) << "set(INITIAL_USER_INFO): no int32values, ignoring it: " << toString(value);
        return StatusCode::INVALID_ARG;
    }

    if (value.areaId != 0) {
        LOG(INFO) << "set(INITIAL_USER_INFO) called from lshal; storing it: " << toString(value);
        mInitialUserResponseFromCmd.reset(new VehiclePropValue(value));
        return StatusCode::OK;
    }
    LOG(INFO) << "set(INITIAL_USER_INFO) called from Android: " << toString(value);

    int32_t requestId = value.value.int32Values[0];

    // Create the update property and set common values
    auto updatedValue = createVehiclePropValue(VehiclePropertyType::MIXED, 0);
    updatedValue->prop = INITIAL_USER_INFO;
    updatedValue->timestamp = elapsedRealtimeNano();

    if (mInitialUserResponseFromCmd == nullptr) {
        updatedValue->value.int32Values.resize(2);
        updatedValue->value.int32Values[0] = requestId;
        updatedValue->value.int32Values[1] = (int32_t)InitialUserInfoResponseAction::DEFAULT;
        LOG(INFO) << "no lshal response; returning InitialUserInfoResponseAction::DEFAULT: "
                  << toString(*updatedValue);
        onPropertyValueFromCar(*updatedValue, updateStatus);
        return StatusCode::OK;
    }

    // mInitialUserResponseFromCmd is used for just one request
    std::unique_ptr<VehiclePropValue> response = std::move(mInitialUserResponseFromCmd);

    // TODO(b/150409377): rather than populate the raw values directly, it should use the
    // libraries that convert a InitialUserInfoResponse into a VehiclePropValue)

    switch (response->areaId) {
        case 1:
            LOG(INFO) << "returning response with right request id";
            *updatedValue = *response;
            updatedValue->areaId = 0;
            updatedValue->value.int32Values[0] = requestId;
            break;
        case 2:
            LOG(INFO) << "returning response with wrong request id";
            *updatedValue = *response;
            updatedValue->areaId = 0;
            updatedValue->value.int32Values[0] = -requestId;
            break;
        case 3:
            LOG(INFO) << "not generating a property change event because of lshal prop: "
                      << toString(*response);
            return StatusCode::OK;
        default:
            LOG(ERROR) << "invalid action on lshal response: " << toString(*response);
            return StatusCode::INTERNAL_ERROR;
    }

    LOG(INFO) << "updating property to: " << toString(*updatedValue);
    onPropertyValueFromCar(*updatedValue, updateStatus);
    return StatusCode::OK;
}

/**
 * Used to emulate SWITCH_USER - see onSetInitialUserInfoResponse() for usage.
 */
StatusCode VehicleHalServer::onSetSwitchUserResponse(const VehiclePropValue& value,
                                                     bool updateStatus) {
    if (value.value.int32Values.size() == 0) {
        LOG(ERROR) << "set(SWITCH_USER): no int32values, ignoring it: " << toString(value);
        return StatusCode::INVALID_ARG;
    }

    if (value.areaId != 0) {
        LOG(INFO) << "set(SWITCH_USER) called from lshal; storing it: " << toString(value);
        mSwitchUserResponseFromCmd.reset(new VehiclePropValue(value));
        return StatusCode::OK;
    }
    LOG(INFO) << "set(SWITCH_USER) called from Android: " << toString(value);

    int32_t requestId = value.value.int32Values[0];

    // Create the update property and set common values
    auto updatedValue = createVehiclePropValue(VehiclePropertyType::MIXED, 0);
    updatedValue->prop = SWITCH_USER;
    updatedValue->timestamp = elapsedRealtimeNano();

    if (mSwitchUserResponseFromCmd == nullptr) {
        updatedValue->value.int32Values.resize(3);
        updatedValue->value.int32Values[0] = requestId;
        updatedValue->value.int32Values[1] = (int32_t)SwitchUserMessageType::VEHICLE_RESPONSE;
        updatedValue->value.int32Values[2] = (int32_t)SwitchUserStatus::SUCCESS;
        LOG(INFO) << "no lshal response; returning VEHICLE_RESPONSE / SUCCESS: "
                  << toString(*updatedValue);
        onPropertyValueFromCar(*updatedValue, updateStatus);
        return StatusCode::OK;
    }

    // mSwitchUserResponseFromCmd is used for just one request
    std::unique_ptr<VehiclePropValue> response = std::move(mSwitchUserResponseFromCmd);

    // TODO(b/150409377): move code below to a local function like sendUserHalResponse(),
    // as it's the same for all (like onSetInitialUserInfoResponse)

    switch (response->areaId) {
        case 1:
            LOG(INFO) << "returning response with right request id";
            *updatedValue = *response;
            updatedValue->areaId = 0;
            updatedValue->value.int32Values[0] = requestId;
            break;
        case 2:
            LOG(INFO) << "returning response with wrong request id";
            *updatedValue = *response;
            updatedValue->areaId = 0;
            updatedValue->value.int32Values[0] = -requestId;
            break;
        case 3:
            LOG(INFO) << "not generating a property change event because of lshal prop: "
                      << toString(*response);
            return StatusCode::OK;
        default:
            LOG(ERROR) << "invalid action on lshal response: " << toString(*response);
            return StatusCode::INTERNAL_ERROR;
    }

    LOG(INFO) << "updating property to: " << toString(*updatedValue);
    onPropertyValueFromCar(*updatedValue, updateStatus);

    return StatusCode::OK;
}

}  // namespace android::hardware::automotive::vehicle::V2_0::impl
