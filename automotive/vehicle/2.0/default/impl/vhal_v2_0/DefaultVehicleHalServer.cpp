/*
 * Copyright (C) 2021 The Android Open Source Project
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

#define LOG_TAG "DefaultVehicleHalServer"

#include <fstream>
#include <regex>

#include <android-base/format.h>
#include <android-base/logging.h>
#include <android-base/parsedouble.h>
#include <android-base/parseint.h>
#include <android-base/properties.h>
#include <utils/SystemClock.h>

#include "DefaultConfig.h"
#include "FakeObd2Frame.h"
#include "JsonFakeValueGenerator.h"
#include "LinearFakeValueGenerator.h"

#include "DefaultVehicleHalServer.h"

namespace android {
namespace hardware {
namespace automotive {
namespace vehicle {
namespace V2_0 {

namespace impl {

namespace {
const char* VENDOR_OVERRIDE_DIR = "/vendor/etc/vhaloverride/";
}  // namespace

void DefaultVehicleHalServer::storePropInitialValue(const ConfigDeclaration& config) {
    VehiclePropConfig cfg = config.config;

    // A global property will have only a single area
    int32_t numAreas = isGlobalProp(cfg.prop) ? 1 : cfg.areaConfigs.size();

    for (int i = 0; i < numAreas; i++) {
        int32_t curArea = isGlobalProp(cfg.prop) ? 0 : cfg.areaConfigs[i].areaId;

        // Create a separate instance for each individual zone
        VehiclePropValue prop = {
                .areaId = curArea,
                .prop = cfg.prop,
        };

        if (config.initialAreaValues.empty()) {
            prop.value = config.initialValue;
        } else if (auto valueForAreaIt = config.initialAreaValues.find(curArea);
                   valueForAreaIt != config.initialAreaValues.end()) {
            prop.value = valueForAreaIt->second;
        } else {
            LOG(WARNING) << __func__ << " failed to get default value for"
                         << " prop 0x" << std::hex << cfg.prop << " area 0x" << std::hex << curArea;
            prop.status = VehiclePropertyStatus::UNAVAILABLE;
        }

        mServerSidePropStore.writeValue(prop, true);
    }
}

DefaultVehicleHalServer::DefaultVehicleHalServer() {
    for (auto& it : kVehicleProperties) {
        VehiclePropConfig cfg = it.config;
        mServerSidePropStore.registerProperty(cfg);
        // Skip diagnostic properties since there is special logic to handle those.
        if (isDiagnosticProperty(cfg)) {
            continue;
        }
        storePropInitialValue(it);
    }
    maybeOverrideProperties(VENDOR_OVERRIDE_DIR);
}

void DefaultVehicleHalServer::sendAllValuesToClient() {
    constexpr bool update_status = true;
    auto values = mServerSidePropStore.readAllValues();
    for (const auto& value : values) {
        onPropertyValueFromCar(value, update_status);
    }
}

GeneratorHub* DefaultVehicleHalServer::getGeneratorHub() {
    return &mGeneratorHub;
}

VehiclePropValuePool* DefaultVehicleHalServer::getValuePool() const {
    if (!mValuePool) {
        LOG(WARNING) << __func__ << ": Value pool not set!";
    }
    return mValuePool;
}

void DefaultVehicleHalServer::setValuePool(VehiclePropValuePool* valuePool) {
    if (!valuePool) {
        LOG(WARNING) << __func__ << ": Setting value pool to nullptr!";
    }
    mValuePool = valuePool;
}

void DefaultVehicleHalServer::onFakeValueGenerated(const VehiclePropValue& value) {
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

std::vector<VehiclePropConfig> DefaultVehicleHalServer::onGetAllPropertyConfig() const {
    return mServerSidePropStore.getAllConfigs();
}

DefaultVehicleHalServer::VehiclePropValuePtr DefaultVehicleHalServer::createApPowerStateReq(
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

DefaultVehicleHalServer::VehiclePropValuePtr DefaultVehicleHalServer::createHwInputKeyProp(
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

StatusCode DefaultVehicleHalServer::onSetProperty(const VehiclePropValue& value,
                                                  bool updateStatus) {
    LOG(DEBUG) << "onSetProperty(" << value.prop << ")";

    // Some properties need to be treated non-trivially
    switch (value.prop) {
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

IVehicleServer::DumpResult DefaultVehicleHalServer::onDump(
        const std::vector<std::string>& options) {
    DumpResult result;
    if (options.size() == 0) {
        // No options, dump all stored properties.
        result.callerShouldDumpState = true;
        result.buffer += "Server side properties: \n";
        auto values = mServerSidePropStore.readAllValues();
        size_t i = 0;
        for (const auto& value : values) {
            result.buffer += fmt::format("[{}]: {}\n", i, toString(value));
            i++;
        }
        return result;
    }
    if (options[0] != "--debughal") {
        // We only expect "debughal" command. This might be some commands that the caller knows
        // about, so let caller handle it.
        result.callerShouldDumpState = true;
        return result;
    }

    return debugCommand(options);
}

IVehicleServer::DumpResult DefaultVehicleHalServer::debugCommand(
        const std::vector<std::string>& options) {
    DumpResult result;
    // This is a debug command for the HAL, caller should not continue to dump state.
    result.callerShouldDumpState = false;

    if (options.size() < 2) {
        result.buffer += "No command specified\n";
        result.buffer += getHelpInfo();
        return result;
    }

    std::string command = options[1];
    if (command == "--help") {
        result.buffer += getHelpInfo();
        return result;
    } else if (command == "--genfakedata") {
        return genFakeDataCommand(options);
    } else if (command == "--setint" || command == "--setfloat" || command == "--setbool") {
        return setValueCommand(options);
    }

    result.buffer += "Unknown command: \"" + command + "\"\n";
    result.buffer += getHelpInfo();
    return result;
}

std::string DefaultVehicleHalServer::getHelpInfo() {
    return "Help: \n"
           "Generate Fake Data: \n"
           "\tStart a linear generator: \n"
           "\t--debughal --genfakedata --startlinear [propID(int32)] [middleValue(float)] "
           "[currentValue(float)] [dispersion(float)] [increment(float)] [interval(int64)]\n"
           "\tStop a linear generator: \n"
           "\t--debughal --genfakedata --stoplinear [propID(int32)]\n"
           "\tStart a json generator: \n"
           "\t--debughal --genfakedata --startjson [jsonFilePath(string)] "
           "[repetition(int32)(optional)]\n"
           "\tStop a json generator: \n"
           "\t--debughal --genfakedata --stopjson [jsonFilePath(string)]\n"
           "\tGenerate key press: \n"
           "\t--debughal --genfakedata --keypress [keyCode(int32)] [display[int32]]\n"
           "\tSet a int property value: \n"
           "\t--setint [propID(int32)] [value(int32)] [timestamp(int64)] "
           "[areaID(int32)(optional)]\n"
           "\tSet a boolean property value: \n"
           "\t--setbool [propID(int32)] [value(\"true\"/\"false\")] [timestamp(int64)] "
           "[areaID(int32)(optional)]\n"
           "\tSet a float property value: \n"
           "\t--setfloat [propID(int32)] [value(float)] [timestamp(int64)] "
           "[areaID(int32)(optional)]\n";
}

IVehicleServer::DumpResult DefaultVehicleHalServer::genFakeDataCommand(
        const std::vector<std::string>& options) {
    DumpResult result;
    // This is a debug command for the HAL, caller should not continue to dump state.
    result.callerShouldDumpState = false;

    if (options.size() < 3) {
        result.buffer += "No subcommand specified for genfakedata\n";
        result.buffer += getHelpInfo();
        return result;
    }

    std::string command = options[2];
    if (command == "--startlinear") {
        LOG(INFO) << __func__ << "FakeDataCommand::StartLinear";
        // --debughal --genfakedata --startlinear [propID(int32)] [middleValue(float)]
        // [currentValue(float)] [dispersion(float)] [increment(float)] [interval(int64)]
        if (options.size() != 9) {
            result.buffer +=
                    "incorrect argument count, need 9 arguments for --genfakedata --startlinear\n";
            result.buffer += getHelpInfo();
            return result;
        }
        int32_t propId;
        float middleValue;
        float currentValue;
        float dispersion;
        float increment;
        int64_t interval;
        if (!android::base::ParseInt(options[3], &propId)) {
            result.buffer += "failed to parse propdID as int: \"" + options[3] + "\"\n";
            result.buffer += getHelpInfo();
            return result;
        }
        if (!android::base::ParseFloat(options[4], &middleValue)) {
            result.buffer += "failed to parse middleValue as float: \"" + options[4] + "\"\n";
            result.buffer += getHelpInfo();
            return result;
        }
        if (!android::base::ParseFloat(options[5], &currentValue)) {
            result.buffer += "failed to parse currentValue as float: \"" + options[5] + "\"\n";
            result.buffer += getHelpInfo();
            return result;
        }
        if (!android::base::ParseFloat(options[6], &dispersion)) {
            result.buffer += "failed to parse dispersion as float: \"" + options[6] + "\"\n";
            result.buffer += getHelpInfo();
            return result;
        }
        if (!android::base::ParseFloat(options[7], &increment)) {
            result.buffer += "failed to parse increment as float: \"" + options[7] + "\"\n";
            result.buffer += getHelpInfo();
            return result;
        }
        if (!android::base::ParseInt(options[8], &interval)) {
            result.buffer += "failed to parse interval as int: \"" + options[8] + "\"\n";
            result.buffer += getHelpInfo();
            return result;
        }
        auto generator = std::make_unique<LinearFakeValueGenerator>(
                propId, middleValue, currentValue, dispersion, increment, interval);
        getGeneratorHub()->registerGenerator(propId, std::move(generator));
        return result;
    } else if (command == "--stoplinear") {
        LOG(INFO) << __func__ << "FakeDataCommand::StopLinear";
        // --debughal --genfakedata --stoplinear [propID(int32)]
        if (options.size() != 4) {
            result.buffer +=
                    "incorrect argument count, need 4 arguments for --genfakedata --stoplinear\n";
            result.buffer += getHelpInfo();
            return result;
        }
        int32_t propId;
        if (!android::base::ParseInt(options[3], &propId)) {
            result.buffer += "failed to parse propdID as int: \"" + options[3] + "\"\n";
            result.buffer += getHelpInfo();
            return result;
        }
        getGeneratorHub()->unregisterGenerator(propId);
        return result;
    } else if (command == "--startjson") {
        LOG(INFO) << __func__ << "FakeDataCommand::StartJson";
        // --debughal --genfakedata --startjson [jsonFilePath(string)] [repetition(int32)(optional)]
        if (options.size() != 4 && options.size() != 5) {
            result.buffer +=
                    "incorrect argument count, need 4 or 5 arguments for --genfakedata "
                    "--startjson\n";
            result.buffer += getHelpInfo();
            return result;
        }
        std::string fileName = options[3];
        int32_t cookie = std::hash<std::string>()(fileName);
        // Iterate infinitely if repetition number is not provided
        int32_t repetition = -1;
        if (options.size() == 5) {
            if (!android::base::ParseInt(options[4], &repetition)) {
                result.buffer += "failed to parse repetition as int: \"" + options[4] + "\"\n";
                result.buffer += getHelpInfo();
                return result;
            }
        }
        auto generator = std::make_unique<JsonFakeValueGenerator>(fileName, repetition);
        if (!generator->hasNext()) {
            result.buffer += "invalid JSON file, no events";
            return result;
        }
        getGeneratorHub()->registerGenerator(cookie, std::move(generator));
        return result;
    } else if (command == "--stopjson") {
        LOG(INFO) << __func__ << "FakeDataCommand::StopJson";
        // --debughal --genfakedata --stopjson [jsonFilePath(string)]
        if (options.size() != 4) {
            result.buffer +=
                    "incorrect argument count, need 4 arguments for --genfakedata --stopjson\n";
            result.buffer += getHelpInfo();
            return result;
        }
        std::string fileName = options[3];
        int32_t cookie = std::hash<std::string>()(fileName);
        getGeneratorHub()->unregisterGenerator(cookie);
        return result;
    } else if (command == "--keypress") {
        LOG(INFO) << __func__ << "FakeDataCommand::KeyPress";
        int32_t keyCode;
        int32_t display;
        // --debughal --genfakedata --keypress [keyCode(int32)] [display[int32]]
        if (options.size() != 5) {
            result.buffer +=
                    "incorrect argument count, need 5 arguments for --genfakedata --keypress\n";
            result.buffer += getHelpInfo();
            return result;
        }
        if (!android::base::ParseInt(options[3], &keyCode)) {
            result.buffer += "failed to parse keyCode as int: \"" + options[3] + "\"\n";
            result.buffer += getHelpInfo();
            return result;
        }
        if (!android::base::ParseInt(options[4], &display)) {
            result.buffer += "failed to parse display as int: \"" + options[4] + "\"\n";
            result.buffer += getHelpInfo();
            return result;
        }
        // Send back to HAL
        onPropertyValueFromCar(
                *createHwInputKeyProp(VehicleHwKeyInputAction::ACTION_DOWN, keyCode, display),
                /*updateStatus=*/true);
        onPropertyValueFromCar(
                *createHwInputKeyProp(VehicleHwKeyInputAction::ACTION_UP, keyCode, display),
                /*updateStatus=*/true);
        return result;
    }

    result.buffer += "Unknown command: \"" + command + "\"\n";
    result.buffer += getHelpInfo();
    return result;
}

void DefaultVehicleHalServer::maybeOverrideProperties(const char* overrideDir) {
    if (android::base::GetBoolProperty("persist.vendor.vhal_init_value_override", false)) {
        overrideProperties(overrideDir);
    }
}

void DefaultVehicleHalServer::overrideProperties(const char* overrideDir) {
    LOG(INFO) << "loading vendor override properties from " << overrideDir;
    if (auto dir = opendir(overrideDir)) {
        std::regex reg_json(".*[.]json", std::regex::icase);
        while (auto f = readdir(dir)) {
            if (!regex_match(f->d_name, reg_json)) {
                continue;
            }
            std::string file = overrideDir + std::string(f->d_name);
            JsonFakeValueGenerator tmpGenerator(file);

            std::vector<VehiclePropValue> propValues = tmpGenerator.getAllEvents();
            for (const VehiclePropValue& prop : propValues) {
                mServerSidePropStore.writeValue(prop, true);
            }
        }
        closedir(dir);
    }
}

IVehicleServer::DumpResult DefaultVehicleHalServer::setValueCommand(
        const std::vector<std::string>& options) {
    DumpResult result;
    // This is a debug command for the HAL, caller should not continue to dump state.
    result.callerShouldDumpState = false;
    // --debughal --set* [propID(int32)] [value] [timestamp(int64)]
    // [areaId(int32)(optional)]
    if (options.size() != 5 && options.size() != 6) {
        result.buffer +=
                "incorrect argument count, need 5 or 6 arguments for --setint or --setfloat or "
                "--setbool\n";
        result.buffer += getHelpInfo();
        return result;
    }
    std::unique_ptr<VehiclePropValue> updatedPropValue;
    int32_t propId;
    int32_t intValue;
    float floatValue;
    int64_t timestamp;
    int32_t areaId = 0;
    if (options[1] == "--setint") {
        updatedPropValue = std::move(createVehiclePropValue(VehiclePropertyType::INT32, 1));
        if (!android::base::ParseInt(options[3], &intValue)) {
            result.buffer += "failed to parse value as int: \"" + options[3] + "\"\n";
            result.buffer += getHelpInfo();
            return result;
        }
        updatedPropValue->value.int32Values[0] = intValue;
    } else if (options[1] == "--setbool") {
        updatedPropValue = std::move(createVehiclePropValue(VehiclePropertyType::BOOLEAN, 1));
        if (options[3] == "true" || options[3] == "True") {
            updatedPropValue->value.int32Values[0] = 1;
        } else if (options[3] == "false" || options[3] == "False") {
            updatedPropValue->value.int32Values[0] = 0;
        } else {
            result.buffer += "failed to parse value as bool, only accepts true/false: \"" +
                             options[3] + "\"\n";
            result.buffer += getHelpInfo();
            return result;
        }
    } else {
        updatedPropValue = std::move(createVehiclePropValue(VehiclePropertyType::FLOAT, 1));
        if (!android::base::ParseFloat(options[3], &floatValue)) {
            result.buffer += "failed to parse value as float: \"" + options[3] + "\"\n";
            result.buffer += getHelpInfo();
            return result;
        }
        updatedPropValue->value.floatValues[0] = floatValue;
    }
    if (!android::base::ParseInt(options[2], &propId)) {
        result.buffer += "failed to parse propID as int: \"" + options[2] + "\"\n";
        result.buffer += getHelpInfo();
        return result;
    }
    updatedPropValue->prop = propId;
    if (!android::base::ParseInt(options[4], &timestamp)) {
        result.buffer += "failed to parse timestamp as int: \"" + options[4] + "\"\n";
        result.buffer += getHelpInfo();
        return result;
    }
    updatedPropValue->timestamp = timestamp;
    if (options.size() == 6) {
        if (!android::base::ParseInt(options[5], &areaId)) {
            result.buffer += "failed to parse areaID as int: \"" + options[5] + "\"\n";
            result.buffer += getHelpInfo();
            return result;
        }
    }
    updatedPropValue->areaId = areaId;

    onPropertyValueFromCar(*updatedPropValue, /*updateStatus=*/true);
    return result;
}

}  // namespace impl

}  // namespace V2_0
}  // namespace vehicle
}  // namespace automotive
}  // namespace hardware
}  // namespace android
