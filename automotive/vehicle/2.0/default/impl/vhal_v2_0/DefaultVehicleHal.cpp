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
#define LOG_TAG "DefaultVehicleHal_v2_0"

#include <android-base/chrono_utils.h>
#include <assert.h>
#include <stdio.h>
#include <utils/Log.h>
#include <utils/SystemClock.h>
#include <vhal_v2_0/RecurrentTimer.h>
#include <unordered_set>

#include "FakeObd2Frame.h"
#include "PropertyUtils.h"
#include "VehicleUtils.h"

#include "DefaultVehicleHal.h"

namespace android {
namespace hardware {
namespace automotive {
namespace vehicle {
namespace V2_0 {

namespace impl {

namespace {
constexpr std::chrono::nanoseconds kHeartBeatIntervalNs = 3s;

const VehicleAreaConfig* getAreaConfig(const VehiclePropValue& propValue,
                                       const VehiclePropConfig* config) {
    if (isGlobalProp(propValue.prop)) {
        if (config->areaConfigs.size() == 0) {
            return nullptr;
        }
        return &(config->areaConfigs[0]);
    } else {
        for (auto& c : config->areaConfigs) {
            if (c.areaId == propValue.areaId) {
                return &c;
            }
        }
    }
    return nullptr;
}

}  // namespace

VehicleHal::VehiclePropValuePtr DefaultVehicleHal::createVhalHeartBeatProp() {
    VehicleHal::VehiclePropValuePtr v = getValuePool()->obtainInt64(uptimeMillis());
    v->prop = static_cast<int32_t>(VehicleProperty::VHAL_HEARTBEAT);
    v->areaId = 0;
    v->status = VehiclePropertyStatus::AVAILABLE;
    return v;
}

DefaultVehicleHal::DefaultVehicleHal(VehiclePropertyStore* propStore, VehicleHalClient* client)
    : mPropStore(propStore), mRecurrentTimer(getTimerAction()), mVehicleClient(client) {
    initStaticConfig();
    mVehicleClient->registerPropertyValueCallback(
            [this](const VehiclePropValue& value, bool updateStatus) {
                onPropertyValue(value, updateStatus);
            });
}

VehicleHal::VehiclePropValuePtr DefaultVehicleHal::getUserHalProp(
        const VehiclePropValue& requestedPropValue, StatusCode* outStatus) {
    auto propId = requestedPropValue.prop;
    ALOGI("get(): getting value for prop %d from User HAL", propId);
    const auto& ret = mFakeUserHal.onGetProperty(requestedPropValue);
    VehicleHal::VehiclePropValuePtr v = nullptr;
    if (!ret.ok()) {
        ALOGE("get(): User HAL returned error: %s", ret.error().message().c_str());
        *outStatus = StatusCode(ret.error().code());
    } else {
        auto value = ret.value().get();
        if (value != nullptr) {
            ALOGI("get(): User HAL returned value: %s", toString(*value).c_str());
            v = getValuePool()->obtain(*value);
            *outStatus = StatusCode::OK;
        } else {
            ALOGE("get(): User HAL returned null value");
            *outStatus = StatusCode::INTERNAL_ERROR;
        }
    }
    return v;
}

VehicleHal::VehiclePropValuePtr DefaultVehicleHal::get(const VehiclePropValue& requestedPropValue,
                                                       StatusCode* outStatus) {
    auto propId = requestedPropValue.prop;
    ALOGV("get(%d)", propId);

    if (mFakeUserHal.isSupported(propId)) {
        return getUserHalProp(requestedPropValue, outStatus);
    }

    VehiclePropValuePtr v = nullptr;
    if (propId == OBD2_FREEZE_FRAME) {
        v = getValuePool()->obtainComplex();
        *outStatus = fillObd2FreezeFrame(mPropStore, requestedPropValue, v.get());
        return v;
    }

    if (propId == OBD2_FREEZE_FRAME_INFO) {
        v = getValuePool()->obtainComplex();
        *outStatus = fillObd2DtcInfo(mPropStore, v.get());
        return v;
    }

    auto internalPropValue = mPropStore->readValueOrNull(requestedPropValue);
    if (internalPropValue != nullptr) {
        v = getValuePool()->obtain(*internalPropValue);
    }

    if (!v) {
        *outStatus = StatusCode::INVALID_ARG;
    } else if (v->status == VehiclePropertyStatus::AVAILABLE) {
        *outStatus = StatusCode::OK;
    } else {
        *outStatus = StatusCode::TRY_AGAIN;
    }
    return v;
}

std::vector<VehiclePropConfig> DefaultVehicleHal::listProperties() {
    return mPropStore->getAllConfigs();
}

bool DefaultVehicleHal::dump(const hidl_handle& fd, const hidl_vec<hidl_string>& options) {
    int nativeFd = fd->data[0];
    if (nativeFd < 0) {
        ALOGW("Invalid fd from HIDL handle: %d", nativeFd);
        return false;
    }
    if (options.size() > 0) {
        if (options[0] == "--help") {
            std::string buffer;
            buffer += "Fake user hal usage:\n";
            buffer += mFakeUserHal.showDumpHelp();
            buffer += "\n";
            buffer += "VHAL server debug usage:\n";
            buffer += "--debughal: send debug command to VHAL server, see '--debughal --help'\n";
            buffer += "\n";
            dprintf(nativeFd, "%s", buffer.c_str());
            return false;
        } else if (options[0] == kUserHalDumpOption) {
            dprintf(nativeFd, "%s", mFakeUserHal.dump("").c_str());
            return false;
        }
    } else {
        // No options, dump the fake user hal state first and then send command to VHAL server
        // to dump its state.
        std::string buffer;
        buffer += "Fake user hal state:\n";
        buffer += mFakeUserHal.dump("  ");
        buffer += "\n";
        dprintf(nativeFd, "%s", buffer.c_str());
    }

    return mVehicleClient->dump(fd, options);
}

StatusCode DefaultVehicleHal::checkPropValue(const VehiclePropValue& value,
                                             const VehiclePropConfig* config) {
    int32_t property = value.prop;
    VehiclePropertyType type = getPropType(property);
    switch (type) {
        case VehiclePropertyType::BOOLEAN:
        case VehiclePropertyType::INT32:
            if (value.value.int32Values.size() != 1) {
                return StatusCode::INVALID_ARG;
            }
            break;
        case VehiclePropertyType::INT32_VEC:
            if (value.value.int32Values.size() < 1) {
                return StatusCode::INVALID_ARG;
            }
            break;
        case VehiclePropertyType::INT64:
            if (value.value.int64Values.size() != 1) {
                return StatusCode::INVALID_ARG;
            }
            break;
        case VehiclePropertyType::INT64_VEC:
            if (value.value.int64Values.size() < 1) {
                return StatusCode::INVALID_ARG;
            }
            break;
        case VehiclePropertyType::FLOAT:
            if (value.value.floatValues.size() != 1) {
                return StatusCode::INVALID_ARG;
            }
            break;
        case VehiclePropertyType::FLOAT_VEC:
            if (value.value.floatValues.size() < 1) {
                return StatusCode::INVALID_ARG;
            }
            break;
        case VehiclePropertyType::BYTES:
            // We allow setting an empty bytes array.
            break;
        case VehiclePropertyType::STRING:
            // We allow setting an empty string.
            break;
        case VehiclePropertyType::MIXED:
            if (getPropGroup(property) == VehiclePropertyGroup::VENDOR) {
                // We only checks vendor mixed properties.
                return checkVendorMixedPropValue(value, config);
            }
            break;
        default:
            ALOGW("Unknown property type: %d", type);
            return StatusCode::INVALID_ARG;
    }
    return StatusCode::OK;
}

StatusCode DefaultVehicleHal::checkVendorMixedPropValue(const VehiclePropValue& value,
                                                        const VehiclePropConfig* config) {
    auto configArray = config->configArray;
    // configArray[0], 1 indicates the property has a String value, we allow the string value to
    // be empty.

    size_t int32Count = 0;
    // configArray[1], 1 indicates the property has a Boolean value.
    if (configArray[1] == 1) {
        int32Count++;
    }
    // configArray[2], 1 indicates the property has an Integer value.
    if (configArray[2] == 1) {
        int32Count++;
    }
    // configArray[3], the number indicates the size of Integer[] in the property.
    int32Count += static_cast<size_t>(configArray[3]);
    if (value.value.int32Values.size() != int32Count) {
        return StatusCode::INVALID_ARG;
    }

    size_t int64Count = 0;
    // configArray[4], 1 indicates the property has a Long value.
    if (configArray[4] == 1) {
        int64Count++;
    }
    // configArray[5], the number indicates the size of Long[] in the property.
    int64Count += static_cast<size_t>(configArray[5]);
    if (value.value.int64Values.size() != int64Count) {
        return StatusCode::INVALID_ARG;
    }

    size_t floatCount = 0;
    // configArray[6], 1 indicates the property has a Float value.
    if (configArray[6] == 1) {
        floatCount++;
    }
    // configArray[7], the number indicates the size of Float[] in the property.
    floatCount += static_cast<size_t>(configArray[7]);
    if (value.value.floatValues.size() != floatCount) {
        return StatusCode::INVALID_ARG;
    }

    // configArray[8], the number indicates the size of byte[] in the property.
    if (configArray[8] != 0 && value.value.bytes.size() != static_cast<size_t>(configArray[8])) {
        return StatusCode::INVALID_ARG;
    }
    return StatusCode::OK;
}

StatusCode DefaultVehicleHal::checkValueRange(const VehiclePropValue& value,
                                              const VehicleAreaConfig* areaConfig) {
    if (areaConfig == nullptr) {
        return StatusCode::OK;
    }
    int32_t property = value.prop;
    VehiclePropertyType type = getPropType(property);
    switch (type) {
        case VehiclePropertyType::INT32:
            if (areaConfig->minInt32Value == 0 && areaConfig->maxInt32Value == 0) {
                break;
            }
            // We already checked this in checkPropValue.
            assert(value.value.int32Values.size() > 0);
            if (value.value.int32Values[0] < areaConfig->minInt32Value ||
                value.value.int32Values[0] > areaConfig->maxInt32Value) {
                return StatusCode::INVALID_ARG;
            }
            break;
        case VehiclePropertyType::INT64:
            if (areaConfig->minInt64Value == 0 && areaConfig->maxInt64Value == 0) {
                break;
            }
            // We already checked this in checkPropValue.
            assert(value.value.int64Values.size() > 0);
            if (value.value.int64Values[0] < areaConfig->minInt64Value ||
                value.value.int64Values[0] > areaConfig->maxInt64Value) {
                return StatusCode::INVALID_ARG;
            }
            break;
        case VehiclePropertyType::FLOAT:
            if (areaConfig->minFloatValue == 0 && areaConfig->maxFloatValue == 0) {
                break;
            }
            // We already checked this in checkPropValue.
            assert(value.value.floatValues.size() > 0);
            if (value.value.floatValues[0] < areaConfig->minFloatValue ||
                value.value.floatValues[0] > areaConfig->maxFloatValue) {
                return StatusCode::INVALID_ARG;
            }
            break;
        default:
            // We don't check the rest of property types. Additional logic needs to be added if
            // required for real implementation. E.g., you might want to enforce the range
            // checks on vector as well or you might want to check the range for mixed property.
            break;
    }
    return StatusCode::OK;
}

StatusCode DefaultVehicleHal::setUserHalProp(const VehiclePropValue& propValue) {
    ALOGI("onSetProperty(): property %d will be handled by UserHal", propValue.prop);

    const auto& ret = mFakeUserHal.onSetProperty(propValue);
    if (!ret.ok()) {
        ALOGE("onSetProperty(): HAL returned error: %s", ret.error().message().c_str());
        return StatusCode(ret.error().code());
    }
    auto updatedValue = ret.value().get();
    if (updatedValue != nullptr) {
        ALOGI("onSetProperty(): updating property returned by HAL: %s",
              toString(*updatedValue).c_str());
        onPropertyValue(*updatedValue, true);
    }
    return StatusCode::OK;
}

StatusCode DefaultVehicleHal::set(const VehiclePropValue& propValue) {
    if (propValue.status != VehiclePropertyStatus::AVAILABLE) {
        // Android side cannot set property status - this value is the
        // purview of the HAL implementation to reflect the state of
        // its underlying hardware
        return StatusCode::INVALID_ARG;
    }

    if (mFakeUserHal.isSupported(propValue.prop)) {
        return setUserHalProp(propValue);
    }

    std::unordered_set<int32_t> powerProps(std::begin(kHvacPowerProperties),
                                           std::end(kHvacPowerProperties));
    if (powerProps.count(propValue.prop)) {
        auto hvacPowerOn = mPropStore->readValueOrNull(
                toInt(VehicleProperty::HVAC_POWER_ON),
                (VehicleAreaSeat::ROW_1_LEFT | VehicleAreaSeat::ROW_1_RIGHT |
                 VehicleAreaSeat::ROW_2_LEFT | VehicleAreaSeat::ROW_2_CENTER |
                 VehicleAreaSeat::ROW_2_RIGHT));

        if (hvacPowerOn && hvacPowerOn->value.int32Values.size() == 1 &&
            hvacPowerOn->value.int32Values[0] == 0) {
            return StatusCode::NOT_AVAILABLE;
        }
    }

    if (propValue.prop == OBD2_FREEZE_FRAME_CLEAR) {
        return clearObd2FreezeFrames(mPropStore, propValue);
    }
    if (propValue.prop == VEHICLE_MAP_SERVICE) {
        // Placeholder for future implementation of VMS property in the default hal. For
        // now, just returns OK; otherwise, hal clients crash with property not supported.
        return StatusCode::OK;
    }

    int32_t property = propValue.prop;
    const VehiclePropConfig* config = mPropStore->getConfigOrNull(property);
    if (config == nullptr) {
        ALOGW("no config for prop 0x%x", property);
        return StatusCode::INVALID_ARG;
    }
    const VehicleAreaConfig* areaConfig = getAreaConfig(propValue, config);
    if (!isGlobalProp(property) && areaConfig == nullptr) {
        // Ignore areaId for global property. For non global property, check whether areaId is
        // allowed. areaId must appear in areaConfig.
        ALOGW("invalid area ID: 0x%x for prop 0x%x, not listed in config", propValue.areaId,
              property);
        return StatusCode::INVALID_ARG;
    }
    auto status = checkPropValue(propValue, config);
    if (status != StatusCode::OK) {
        ALOGW("invalid property value: %s", toString(propValue).c_str());
        return status;
    }
    status = checkValueRange(propValue, areaConfig);
    if (status != StatusCode::OK) {
        ALOGW("property value out of range: %s", toString(propValue).c_str());
        return status;
    }

    auto currentPropValue = mPropStore->readValueOrNull(propValue);
    if (currentPropValue && currentPropValue->status != VehiclePropertyStatus::AVAILABLE) {
        // do not allow Android side to set() a disabled/error property
        return StatusCode::NOT_AVAILABLE;
    }

    // Send the value to the vehicle server, the server will talk to the (real or emulated) car
    return mVehicleClient->setProperty(propValue, /*updateStatus=*/false);
}

// Parse supported properties list and generate vector of property values to hold current values.
void DefaultVehicleHal::onCreate() {
    auto configs = mVehicleClient->getAllPropertyConfig();

    for (const auto& cfg : configs) {
        if (isDiagnosticProperty(cfg)) {
            // do not write an initial empty value for the diagnostic properties
            // as we will initialize those separately.
            continue;
        }

        int32_t numAreas = isGlobalProp(cfg.prop) ? 1 : cfg.areaConfigs.size();

        for (int i = 0; i < numAreas; i++) {
            int32_t curArea = isGlobalProp(cfg.prop) ? 0 : cfg.areaConfigs[i].areaId;

            // Create a separate instance for each individual zone
            VehiclePropValue prop = {
                    .areaId = curArea,
                    .prop = cfg.prop,
                    .status = VehiclePropertyStatus::UNAVAILABLE,
            };
            // Allow the initial values to set status.
            mPropStore->writeValue(prop, /*updateStatus=*/true);
        }
    }

    mVehicleClient->triggerSendAllValues();

    initObd2LiveFrame(mPropStore, *mPropStore->getConfigOrDie(OBD2_LIVE_FRAME));
    initObd2FreezeFrame(mPropStore, *mPropStore->getConfigOrDie(OBD2_FREEZE_FRAME));

    registerHeartBeatEvent();
}

DefaultVehicleHal::~DefaultVehicleHal() {
    mRecurrentTimer.unregisterRecurrentEvent(static_cast<int32_t>(VehicleProperty::VHAL_HEARTBEAT));
}

void DefaultVehicleHal::registerHeartBeatEvent() {
    mRecurrentTimer.registerRecurrentEvent(kHeartBeatIntervalNs,
                                           static_cast<int32_t>(VehicleProperty::VHAL_HEARTBEAT));
}

VehicleHal::VehiclePropValuePtr DefaultVehicleHal::doInternalHealthCheck() {
    VehicleHal::VehiclePropValuePtr v = nullptr;

    // This is an example of very simple health checking. VHAL is considered healthy if we can read
    // PERF_VEHICLE_SPEED. The more comprehensive health checking is required.
    VehiclePropValue propValue = {
            .prop = static_cast<int32_t>(VehicleProperty::PERF_VEHICLE_SPEED),
    };
    auto internalPropValue = mPropStore->readValueOrNull(propValue);
    if (internalPropValue != nullptr) {
        v = createVhalHeartBeatProp();
    } else {
        ALOGW("VHAL health check failed");
    }
    return v;
}

void DefaultVehicleHal::onContinuousPropertyTimer(const std::vector<int32_t>& properties) {
    auto& pool = *getValuePool();
    for (int32_t property : properties) {
        std::vector<VehiclePropValuePtr> events;
        if (isContinuousProperty(property)) {
            const VehiclePropConfig* config = mPropStore->getConfigOrNull(property);
            std::vector<int32_t> areaIds;
            if (isGlobalProp(property)) {
                areaIds.push_back(0);
            } else {
                for (auto& c : config->areaConfigs) {
                    areaIds.push_back(c.areaId);
                }
            }

            for (int areaId : areaIds) {
                auto v = pool.obtain(*mPropStore->refreshTimestamp(property, areaId));
                if (v.get()) {
                    events.push_back(std::move(v));
                }
            }
        } else if (property == static_cast<int32_t>(VehicleProperty::VHAL_HEARTBEAT)) {
            // VHAL_HEARTBEAT is not a continuous value, but it needs to be updated periodically.
            // So, the update is done through onContinuousPropertyTimer.
            auto v = doInternalHealthCheck();
            if (!v.get()) {
                // Internal health check failed.
                continue;
            }
            mPropStore->writeValueWithCurrentTimestamp(v.get(), /*updateStatus=*/true);
            events.push_back(std::move(v));
        } else {
            ALOGE("Unexpected onContinuousPropertyTimer for property: 0x%x", property);
            continue;
        }

        for (VehiclePropValuePtr& event : events) {
            doHalEvent(std::move(event));
        }
    }
}

RecurrentTimer::Action DefaultVehicleHal::getTimerAction() {
    return [this](const std::vector<int32_t>& properties) {
        onContinuousPropertyTimer(properties);
    };
}

StatusCode DefaultVehicleHal::subscribe(int32_t property, float sampleRate) {
    ALOGI("%s propId: 0x%x, sampleRate: %f", __func__, property, sampleRate);

    if (!isContinuousProperty(property)) {
        return StatusCode::INVALID_ARG;
    }

    // If the config does not exist, isContinuousProperty should return false.
    const VehiclePropConfig* config = mPropStore->getConfigOrNull(property);
    if (sampleRate < config->minSampleRate || sampleRate > config->maxSampleRate) {
        ALOGW("sampleRate out of range");
        return StatusCode::INVALID_ARG;
    }

    mRecurrentTimer.registerRecurrentEvent(hertzToNanoseconds(sampleRate), property);
    return StatusCode::OK;
}

StatusCode DefaultVehicleHal::unsubscribe(int32_t property) {
    ALOGI("%s propId: 0x%x", __func__, property);
    if (!isContinuousProperty(property)) {
        return StatusCode::INVALID_ARG;
    }
    // If the event was not registered before, this would do nothing.
    mRecurrentTimer.unregisterRecurrentEvent(property);
    return StatusCode::OK;
}

bool DefaultVehicleHal::isContinuousProperty(int32_t propId) const {
    const VehiclePropConfig* config = mPropStore->getConfigOrNull(propId);
    if (config == nullptr) {
        ALOGW("Config not found for property: 0x%x", propId);
        return false;
    }
    return config->changeMode == VehiclePropertyChangeMode::CONTINUOUS;
}

void DefaultVehicleHal::onPropertyValue(const VehiclePropValue& value, bool updateStatus) {
    VehiclePropValuePtr updatedPropValue = getValuePool()->obtain(value);

    if (mPropStore->writeValueWithCurrentTimestamp(updatedPropValue.get(), updateStatus)) {
        doHalEvent(std::move(updatedPropValue));
    }
}

void DefaultVehicleHal::initStaticConfig() {
    auto configs = mVehicleClient->getAllPropertyConfig();
    for (auto&& cfg : configs) {
        VehiclePropertyStore::TokenFunction tokenFunction = nullptr;

        switch (cfg.prop) {
            case OBD2_FREEZE_FRAME: {
                // We use timestamp as token for OBD2_FREEZE_FRAME
                tokenFunction = [](const VehiclePropValue& propValue) {
                    return propValue.timestamp;
                };
                break;
            }
            default:
                break;
        }

        mPropStore->registerProperty(cfg, tokenFunction);
    }
}

}  // namespace impl

}  // namespace V2_0
}  // namespace vehicle
}  // namespace automotive
}  // namespace hardware
}  // namespace android
