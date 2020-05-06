/*
 * Copyright (C) 2018 The Android Open Source Project
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

#define LOG_TAG "JsonFakeValueGenerator"

#include <fstream>
#include <type_traits>
#include <typeinfo>

#include <log/log.h>
#include <vhal_v2_0/VehicleUtils.h>

#include "JsonFakeValueGenerator.h"

namespace android {
namespace hardware {
namespace automotive {
namespace vehicle {
namespace V2_0 {

namespace impl {

JsonFakeValueGenerator::JsonFakeValueGenerator(const VehiclePropValue& request) {
    const auto& v = request.value;
    const char* file = v.stringValue.c_str();
    std::ifstream ifs(file);
    if (!ifs) {
        ALOGE("%s: couldn't open %s for parsing.", __func__, file);
    }
    mGenCfg = {
        .index = 0,
        .events = parseFakeValueJson(ifs),
    };
    // Iterate infinitely if repetition number is not provided
    mNumOfIterations = v.int32Values.size() < 2 ? -1 : v.int32Values[1];
}

JsonFakeValueGenerator::JsonFakeValueGenerator(std::string path) {
    std::ifstream ifs(path);
    if (!ifs) {
        ALOGE("%s: couldn't open %s for parsing.", __func__, path.c_str());
    }
    mGenCfg = {
        .index = 0,
        .events = parseFakeValueJson(ifs),
    };
    mNumOfIterations = mGenCfg.events.size();
}

std::vector<VehiclePropValue> JsonFakeValueGenerator::getAllEvents() {
    return mGenCfg.events;
}

VehiclePropValue JsonFakeValueGenerator::nextEvent() {
    VehiclePropValue generatedValue;
    if (!hasNext()) {
        return generatedValue;
    }
    TimePoint eventTime = Clock::now();
    if (mGenCfg.index != 0) {
        // All events (start from 2nd one) are supposed to happen in the future with a delay
        // equals to the duration between previous and current event.
        eventTime += Nanos(mGenCfg.events[mGenCfg.index].timestamp -
                           mGenCfg.events[mGenCfg.index - 1].timestamp);
    }
    generatedValue = mGenCfg.events[mGenCfg.index];
    generatedValue.timestamp = eventTime.time_since_epoch().count();

    mGenCfg.index++;
    if (mGenCfg.index == mGenCfg.events.size()) {
        mGenCfg.index = 0;
        if (mNumOfIterations > 0) {
            mNumOfIterations--;
        }
    }
    return generatedValue;
}

bool JsonFakeValueGenerator::hasNext() {
    return mNumOfIterations != 0 && mGenCfg.events.size() > 0;
}

std::vector<VehiclePropValue> JsonFakeValueGenerator::parseFakeValueJson(std::istream& is) {
    std::vector<VehiclePropValue> fakeVhalEvents;

    Json::Reader reader;
    Json::Value rawEvents;
    if (!reader.parse(is, rawEvents)) {
        ALOGE("%s: Failed to parse fake data JSON file. Error: %s", __func__,
              reader.getFormattedErrorMessages().c_str());
        return fakeVhalEvents;
    }

    for (Json::Value::ArrayIndex i = 0; i < rawEvents.size(); i++) {
        Json::Value rawEvent = rawEvents[i];
        if (!rawEvent.isObject()) {
            ALOGE("%s: VHAL JSON event should be an object, %s", __func__,
                  rawEvent.toStyledString().c_str());
            continue;
        }
        if (rawEvent["prop"].empty() || rawEvent["areaId"].empty() || rawEvent["value"].empty() ||
            rawEvent["timestamp"].empty()) {
            ALOGE("%s: VHAL JSON event has missing fields, skip it, %s", __func__,
                  rawEvent.toStyledString().c_str());
            continue;
        }
        VehiclePropValue event = {
                .timestamp = rawEvent["timestamp"].asInt64(),
                .areaId = rawEvent["areaId"].asInt(),
                .prop = rawEvent["prop"].asInt(),
        };

        Json::Value rawEventValue = rawEvent["value"];
        auto& value = event.value;
        int32_t count;
        switch (getPropType(event.prop)) {
            case VehiclePropertyType::BOOLEAN:
            case VehiclePropertyType::INT32:
                value.int32Values.resize(1);
                value.int32Values[0] = rawEventValue.asInt();
                break;
            case VehiclePropertyType::INT64:
                value.int64Values.resize(1);
                value.int64Values[0] = rawEventValue.asInt64();
                break;
            case VehiclePropertyType::FLOAT:
                value.floatValues.resize(1);
                value.floatValues[0] = rawEventValue.asFloat();
                break;
            case VehiclePropertyType::STRING:
                value.stringValue = rawEventValue.asString();
                break;
            case VehiclePropertyType::INT32_VEC:
                value.int32Values.resize(rawEventValue.size());
                count = 0;
                for (auto& it : rawEventValue) {
                    value.int32Values[count++] = it.asInt();
                }
                break;
            case VehiclePropertyType::MIXED:
                copyMixedValueJson(value, rawEventValue);
                if (isDiagnosticProperty(event.prop)) {
                    value.bytes = generateDiagnosticBytes(value);
                }
                break;
            default:
                ALOGE("%s: unsupported type for property: 0x%x", __func__, event.prop);
                continue;
        }
        fakeVhalEvents.push_back(event);
    }
    return fakeVhalEvents;
}

void JsonFakeValueGenerator::copyMixedValueJson(VehiclePropValue::RawValue& dest,
                                                const Json::Value& jsonValue) {
    copyJsonArray(dest.int32Values, jsonValue["int32Values"]);
    copyJsonArray(dest.int64Values, jsonValue["int64Values"]);
    copyJsonArray(dest.floatValues, jsonValue["floatValues"]);
    dest.stringValue = jsonValue["stringValue"].asString();
}

template <typename T>
void JsonFakeValueGenerator::copyJsonArray(hidl_vec<T>& dest, const Json::Value& jsonArray) {
    dest.resize(jsonArray.size());
    for (Json::Value::ArrayIndex i = 0; i < jsonArray.size(); i++) {
        if (std::is_same<T, int32_t>::value) {
            dest[i] = jsonArray[i].asInt();
        } else if (std::is_same<T, int64_t>::value) {
            dest[i] = jsonArray[i].asInt64();
        } else if (std::is_same<T, float>::value) {
            dest[i] = jsonArray[i].asFloat();
        }
    }
}

bool JsonFakeValueGenerator::isDiagnosticProperty(int32_t prop) {
    return prop == (int32_t)VehicleProperty::OBD2_LIVE_FRAME ||
           prop == (int32_t)VehicleProperty::OBD2_FREEZE_FRAME;
}

hidl_vec<uint8_t> JsonFakeValueGenerator::generateDiagnosticBytes(
    const VehiclePropValue::RawValue& diagnosticValue) {
    size_t byteSize = ((size_t)DiagnosticIntegerSensorIndex::LAST_SYSTEM_INDEX +
                       (size_t)DiagnosticFloatSensorIndex::LAST_SYSTEM_INDEX + 2);
    hidl_vec<uint8_t> bytes(byteSize % 8 == 0 ? byteSize / 8 : byteSize / 8 + 1);

    auto& int32Values = diagnosticValue.int32Values;
    for (size_t i = 0; i < int32Values.size(); i++) {
        if (int32Values[i] != 0) {
            setBit(bytes, i);
        }
    }

    auto& floatValues = diagnosticValue.floatValues;
    for (size_t i = 0; i < floatValues.size(); i++) {
        if (floatValues[i] != 0.0) {
            setBit(bytes, i + (size_t)DiagnosticIntegerSensorIndex::LAST_SYSTEM_INDEX + 1);
        }
    }
    return bytes;
}

void JsonFakeValueGenerator::setBit(hidl_vec<uint8_t>& bytes, size_t idx) {
    uint8_t mask = 1 << (idx % 8);
    bytes[idx / 8] |= mask;
}

}  // namespace impl

}  // namespace V2_0
}  // namespace vehicle
}  // namespace automotive
}  // namespace hardware
}  // namespace android
