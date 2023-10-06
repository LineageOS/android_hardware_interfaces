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

#define LOG_TAG "JsonFakeValueGenerator"

#include "JsonFakeValueGenerator.h"

#include <fstream>
#include <type_traits>
#include <typeinfo>

#include <Obd2SensorStore.h>
#include <VehicleUtils.h>
#include <android/binder_enums.h>
#include <utils/Log.h>
#include <utils/SystemClock.h>

namespace android {
namespace hardware {
namespace automotive {
namespace vehicle {
namespace fake {

namespace {

using ::aidl::android::hardware::automotive::vehicle::DiagnosticFloatSensorIndex;
using ::aidl::android::hardware::automotive::vehicle::DiagnosticIntegerSensorIndex;
using ::aidl::android::hardware::automotive::vehicle::RawPropValues;
using ::aidl::android::hardware::automotive::vehicle::VehicleProperty;
using ::aidl::android::hardware::automotive::vehicle::VehiclePropertyType;
using ::aidl::android::hardware::automotive::vehicle::VehiclePropValue;

bool isDiagnosticProperty(int32_t prop) {
    return prop == toInt(VehicleProperty::OBD2_LIVE_FRAME) ||
           prop == toInt(VehicleProperty::OBD2_FREEZE_FRAME);
}

void setBit(std::vector<uint8_t>& bytes, size_t idx) {
    uint8_t mask = 1 << (idx % 8);
    bytes[idx / 8] |= mask;
}

template <typename T>
void copyJsonArray(const Json::Value& jsonArray, std::vector<T>& dest) {
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

void copyMixedValueJson(const Json::Value& jsonValue, RawPropValues& dest) {
    copyJsonArray(jsonValue["int32Values"], dest.int32Values);
    copyJsonArray(jsonValue["int64Values"], dest.int64Values);
    copyJsonArray(jsonValue["floatValues"], dest.floatValues);
    dest.stringValue = jsonValue["stringValue"].asString();
}

std::vector<uint8_t> generateDiagnosticBytes(const RawPropValues& diagnosticValue) {
    size_t lastIntegerSensorIndex = static_cast<size_t>(
            obd2frame::Obd2SensorStore::getLastIndex<DiagnosticIntegerSensorIndex>());
    size_t lastFloatSensorIndex = static_cast<size_t>(
            obd2frame::Obd2SensorStore::getLastIndex<DiagnosticFloatSensorIndex>());

    size_t byteSize = (lastIntegerSensorIndex + lastFloatSensorIndex + 2);
    std::vector<uint8_t> bytes((byteSize + 7) / 8);

    auto& int32Values = diagnosticValue.int32Values;
    for (size_t i = 0; i < int32Values.size(); i++) {
        if (int32Values[i] != 0) {
            setBit(bytes, i);
        }
    }

    auto& floatValues = diagnosticValue.floatValues;
    for (size_t i = 0; i < floatValues.size(); i++) {
        if (floatValues[i] != 0.0) {
            setBit(bytes, i + lastIntegerSensorIndex + 1);
        }
    }
    return bytes;
}

std::vector<VehiclePropValue> parseFakeValueJson(std::istream& is) {
    std::vector<VehiclePropValue> fakeVhalEvents;

    Json::CharReaderBuilder builder;
    Json::Value rawEvents;
    std::string errorMessage;
    if (!Json::parseFromStream(builder, is, &rawEvents, &errorMessage)) {
        ALOGE("%s: Failed to parse fake data JSON file. Error: %s", __func__, errorMessage.c_str());
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
                copyMixedValueJson(rawEventValue, value);
                if (isDiagnosticProperty(event.prop)) {
                    value.byteValues = generateDiagnosticBytes(value);
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

}  // namespace

JsonFakeValueGenerator::JsonFakeValueGenerator(const std::string& path)
    : JsonFakeValueGenerator(path, /*iteration=*/1) {}

JsonFakeValueGenerator::JsonFakeValueGenerator(const std::string& path, int32_t iteration) {
    initWithPath(path, iteration);
}

JsonFakeValueGenerator::JsonFakeValueGenerator(const VehiclePropValue& request) {
    const auto& v = request.value;
    // Iterate infinitely if iteration number is not provided
    int32_t numOfIterations = v.int32Values.size() < 2 ? -1 : v.int32Values[1];

    initWithPath(v.stringValue, numOfIterations);
}

JsonFakeValueGenerator::JsonFakeValueGenerator([[maybe_unused]] bool unused,
                                               const std::string& content, int32_t iteration) {
    std::istringstream iss(content);
    initWithStream(iss, iteration);
}

void JsonFakeValueGenerator::initWithPath(const std::string& path, int32_t iteration) {
    std::ifstream ifs(path);
    if (!ifs) {
        ALOGE("%s: couldn't open %s for parsing.", __func__, path.c_str());
        return;
    }
    initWithStream(ifs, iteration);
}

void JsonFakeValueGenerator::initWithStream(std::istream& is, int32_t iteration) {
    mEvents = parseFakeValueJson(is);
    mNumOfIterations = iteration;
}

const std::vector<VehiclePropValue>& JsonFakeValueGenerator::getAllEvents() {
    return mEvents;
}

std::optional<VehiclePropValue> JsonFakeValueGenerator::nextEvent() {
    if (mNumOfIterations == 0 || mEvents.size() == 0) {
        return std::nullopt;
    }

    VehiclePropValue generatedValue = mEvents[mEventIndex];

    if (mLastEventTimestamp == 0) {
        mLastEventTimestamp = elapsedRealtimeNano();
    } else {
        int64_t nextEventTime = 0;
        if (mEventIndex > 0) {
            // All events (start from 2nd one) are supposed to happen in the future with a delay
            // equals to the duration between previous and current event.
            nextEventTime = mLastEventTimestamp +
                            (mEvents[mEventIndex].timestamp - mEvents[mEventIndex - 1].timestamp);
        } else {
            // We are starting another iteration, immediately send the next event after 1ms.
            nextEventTime = mLastEventTimestamp + 1000000;
        }
        // Prevent overflow.
        assert(nextEventTime > mLastEventTimestamp);
        mLastEventTimestamp = nextEventTime;
    }

    mEventIndex++;
    if (mEventIndex == mEvents.size()) {
        mEventIndex = 0;
        if (mNumOfIterations > 0) {
            mNumOfIterations--;
        }
    }
    generatedValue.timestamp = mLastEventTimestamp;

    return generatedValue;
}

bool JsonFakeValueGenerator::hasNext() {
    return mNumOfIterations != 0 && mEvents.size() > 0;
}

}  // namespace fake
}  // namespace vehicle
}  // namespace automotive
}  // namespace hardware
}  // namespace android
