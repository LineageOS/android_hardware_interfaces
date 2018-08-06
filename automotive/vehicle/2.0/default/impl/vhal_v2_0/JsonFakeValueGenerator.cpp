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

#include <log/log.h>
#include <vhal_v2_0/VehicleUtils.h>

#include "JsonFakeValueGenerator.h"

namespace android {
namespace hardware {
namespace automotive {
namespace vehicle {
namespace V2_0 {

namespace impl {

JsonFakeValueGenerator::JsonFakeValueGenerator(const OnHalEvent& onHalEvent)
    : mOnHalEvent(onHalEvent), mThread(&JsonFakeValueGenerator::loop, this) {}

JsonFakeValueGenerator::~JsonFakeValueGenerator() {
    mStopRequested = true;
    {
        MuxGuard g(mLock);
        mGenCfg.index = 0;
        mGenCfg.events.clear();
    }
    mCond.notify_one();
    if (mThread.joinable()) {
        mThread.join();
    }
}

StatusCode JsonFakeValueGenerator::start(const VehiclePropValue& request) {
    const auto& v = request.value;
    if (v.stringValue.empty()) {
        ALOGE("%s: path to JSON file is missing", __func__);
        return StatusCode::INVALID_ARG;
    }
    const char* file = v.stringValue.c_str();
    std::ifstream ifs(file);
    if (!ifs) {
        ALOGE("%s: couldn't open %s for parsing.", __func__, file);
        return StatusCode::INTERNAL_ERROR;
    }
    std::vector<VehiclePropValue> fakeVhalEvents = parseFakeValueJson(ifs);

    {
        MuxGuard g(mLock);
        mGenCfg = {0, fakeVhalEvents};
    }
    mCond.notify_one();
    return StatusCode::OK;
}

StatusCode JsonFakeValueGenerator::stop(const VehiclePropValue& request) {
    const auto& v = request.value;
    if (!v.stringValue.empty()) {
        ALOGI("%s: %s", __func__, v.stringValue.c_str());
    }

    {
        MuxGuard g(mLock);
        mGenCfg.index = 0;
        mGenCfg.events.clear();
    }
    mCond.notify_one();
    return StatusCode::OK;
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
        VehiclePropValue event = {.prop = rawEvent["prop"].asInt(),
                                  .areaId = rawEvent["areaId"].asInt(),
                                  .timestamp = rawEvent["timestamp"].asInt64()};

        Json::Value rawEventValue = rawEvent["value"];
        auto& value = event.value;
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
            default:
                ALOGE("%s: unsupported type for property: 0x%x with value: %s", __func__,
                      event.prop, rawEventValue.asString().c_str());
                continue;
        }
        fakeVhalEvents.push_back(event);
    }
    return fakeVhalEvents;
}

void JsonFakeValueGenerator::loop() {
    static constexpr auto kInvalidTime = TimePoint(Nanos::max());

    while (!mStopRequested) {
        auto nextEventTime = kInvalidTime;
        {
            MuxGuard g(mLock);
            if (mGenCfg.index < mGenCfg.events.size()) {
                mOnHalEvent(mGenCfg.events[mGenCfg.index]);
            }
            if (!mGenCfg.events.empty() && mGenCfg.index < mGenCfg.events.size() - 1) {
                Nanos intervalNano =
                    static_cast<Nanos>(mGenCfg.events[mGenCfg.index + 1].timestamp -
                                       mGenCfg.events[mGenCfg.index].timestamp);
                nextEventTime = Clock::now() + intervalNano;
            }
            mGenCfg.index++;
        }

        std::unique_lock<std::mutex> g(mLock);
        mCond.wait_until(g, nextEventTime);
    }
}

}  // namespace impl

}  // namespace V2_0
}  // namespace vehicle
}  // namespace automotive
}  // namespace hardware
}  // namespace android
