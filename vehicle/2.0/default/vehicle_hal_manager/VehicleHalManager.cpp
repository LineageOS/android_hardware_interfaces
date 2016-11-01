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
#define LOG_TAG "android.hardware.vehicle@2.0-impl"

#include <cmath>

#include <utils/Errors.h>
#include <utils/Log.h>
#include <hidl/Status.h>

#include "VehicleHalManager.h"

namespace android {
namespace hardware {
namespace vehicle {
namespace V2_0 {

using namespace std::placeholders;

constexpr std::chrono::milliseconds kHalEventBatchingTimeWindow(10);

/**
 * Indicates what's the maximum size of hidl_vec<VehiclePropValue> we want
 * to store in reusable object pool.
 */
constexpr auto kMaxHidlVecOfVehiclPropValuePoolSize = 20;

Return<void> VehicleHalManager::getAllPropConfigs(
        getAllPropConfigs_cb _hidl_cb) {
    hidl_vec<VehiclePropConfig> hidlConfigs;
    auto& halConfig = mConfigIndex->getAllConfigs();

    hidlConfigs.setToExternal(
            const_cast<VehiclePropConfig *>(halConfig.data()),
            halConfig.size());

    _hidl_cb(hidlConfigs);

    return hardware::Return<void>();
}

Return<void> VehicleHalManager::getPropConfigs(
        const hidl_vec<VehicleProperty> &properties,
        getPropConfigs_cb _hidl_cb) {
    Vector<VehiclePropConfig> configs;
    for (size_t i = 0; i < properties.size(); i++) {
        VehicleProperty prop = properties[i];
        if (mConfigIndex->hasConfig(prop)) {
            configs.add(mConfigIndex->getConfig(prop));
        } else {
            // TODO: return error
        }
    }

    hidl_vec<VehiclePropConfig> hidlConfigs;
    hidlConfigs.setToExternal(
            const_cast<VehiclePropConfig*>(configs.array()),
            configs.size());

    _hidl_cb(hidlConfigs);

    return hardware::Return<void>();
}

Return<void> VehicleHalManager::get(
        VehicleProperty propId, int32_t areaId, get_cb _hidl_cb) {

    return hardware::Return<void>();
}

Return<StatusCode> VehicleHalManager::set(const VehiclePropValue &value) {
    // TODO(pavelm): check permission, etc
    // TODO(pavelm): check SET subscription
    // TODO(pavelm): propagate SET call to VehicleHal
    return hardware::Return<StatusCode>(StatusCode::OK);
}

Return<StatusCode> VehicleHalManager::subscribe(
        const sp<IVehicleCallback> &callback,
        const hidl_vec<SubscribeOptions> &options) {
    hidl_vec<SubscribeOptions> verifiedOptions(options);
    for (size_t i = 0; i < verifiedOptions.size(); i++) {
        SubscribeOptions& ops = verifiedOptions[i];
        VehicleProperty prop = ops.propId;

        if (!mConfigIndex->hasConfig(prop)) {
            ALOGE("Failed to subscribe: config not found for property: 0x%x",
                  prop);
            return invalidArg();
        }
        const VehiclePropConfig& config = mConfigIndex->getConfig(prop);

        if (!isSubscribable(config)) {
            ALOGE("Failed to subscribe: property is not subscribable: 0x%x",
                  prop);
            return invalidArg();
        }


        int32_t areas = isGlobalProp(prop) ? 0 : ops.vehicleAreas;
        if (areas != 0 && ((areas & config.supportedAreas) != areas)) {
            ALOGE("Failed to subscribe property 0x%x. Requested areas 0x%x are "
                  "out of supported range of 0x%x", prop, ops.vehicleAreas,
                  config.supportedAreas);
            return invalidArg();
        }

        ops.vehicleAreas = areas;
        ops.sampleRate = checkSampleRate(config, ops.sampleRate);
    }

     std::list<SubscribeOptions> updatedOptions =
         mSubscriptionManager.addOrUpdateSubscription(callback,
                                                      verifiedOptions);

    for (auto opt : updatedOptions) {
        mHal->subscribe(opt.propId, opt.vehicleAreas, opt.sampleRate);
    }

    // TODO(pavelm): call immediately onHalEvent method during subscription
    // when appropriate
    // TODO(pavelm): link to death callback (not implemented yet in HIDL)

    return ok();
}

Return<StatusCode> VehicleHalManager::unsubscribe(
        const sp<IVehicleCallback>& callback, VehicleProperty propId) {
    if (mSubscriptionManager.unsubscribe(callback, propId)) {
        mHal->unsubscribe(propId);
    }
    return ok();
}

Return<void> VehicleHalManager::debugDump(IVehicle::debugDump_cb _hidl_cb) {
    _hidl_cb("");
    return hardware::Return<void>();
}

void VehicleHalManager::init() {
    ALOGI("VehicleHalManager::init");

    mHidlVecOfVehiclePropValuePool.resize(kMaxHidlVecOfVehiclPropValuePoolSize);

    mBatchingConsumer.run(&mEventQueue,
                          kHalEventBatchingTimeWindow,
                          std::bind(&VehicleHalManager::onBatchHalEvent,
                                    this, _1));

    mHal->init(&mValueObjectPool,
               std::bind(&VehicleHalManager::onHalEvent, this, _1),
               std::bind(&VehicleHalManager::onHalError, this, _1, _2, _3));

    // Initialize index with vehicle configurations received from VehicleHal.
    mConfigIndex.reset(new VehiclePropConfigIndex(mHal->listProperties()));
}

VehicleHalManager::~VehicleHalManager() {
    mBatchingConsumer.requestStop();
    mEventQueue.deactivate();
    // We have to wait until consumer thread is fully stopped because it may
    // be in a state of running callback (onBatchHalEvent).
    mBatchingConsumer.waitStopped();
    ALOGI("VehicleHalManager::dtor");
}

void VehicleHalManager::onHalEvent(VehiclePropValuePtr v) {
    mEventQueue.push(std::move(v));
}

void VehicleHalManager::onHalError(VehicleProperty property, status_t errorCode,
                                   VehiclePropertyOperation operation) {
    // TODO(pavelm): find subscribed clients and propagate error
}

void VehicleHalManager::onBatchHalEvent(
        const std::vector<VehiclePropValuePtr>& values) {
    const auto& clientValues = mSubscriptionManager.distributeValuesToClients(
            values, SubscribeFlags::HAL_EVENT);

    for (const HalClientValues& cv : clientValues) {
        int vecSize = cv.values.size();
        hidl_vec<VehiclePropValue> vec;
        if (vecSize < kMaxHidlVecOfVehiclPropValuePoolSize) {
            vec.setToExternal(&mHidlVecOfVehiclePropValuePool[0], vecSize);
        } else {
            vec.resize(vecSize);
        }

        int i = 0;
        for (VehiclePropValue* pValue : cv.values) {
            shallowCopy(&(vec)[i++], *pValue);
        }
        cv.client->getCallback()->onPropertyEvent(vec);
    }
}

bool VehicleHalManager::isSampleRateFixed(VehiclePropertyChangeMode mode) {
    return (mode & VehiclePropertyChangeMode::ON_SET)
           || (mode & VehiclePropertyChangeMode::ON_CHANGE);
}

float VehicleHalManager::checkSampleRate(const VehiclePropConfig &config,
                                         float sampleRate) {
    if (isSampleRateFixed(config.changeMode)) {
        if (std::abs(sampleRate) > std::numeric_limits<float>::epsilon()) {
            ALOGW("Sample rate is greater than zero for on change type. "
                      "Ignoring it.");
        }
        return 0.0;
    } else {
        if (sampleRate > config.maxSampleRate) {
            ALOGW("Sample rate %f is higher than max %f. Setting sampling rate "
                      "to max.", sampleRate, config.maxSampleRate);
            return config.maxSampleRate;
        }
        if (sampleRate < config.minSampleRate) {
            ALOGW("Sample rate %f is lower than min %f. Setting sampling rate "
                      "to min.", sampleRate, config.minSampleRate);
            return config.minSampleRate;
        }
    }
    return sampleRate;  // Provided sample rate was good, no changes.
}

bool VehicleHalManager::isSubscribable(const VehiclePropConfig& config) {
    if (!(config.access & VehiclePropertyAccess::READ)) {
        ALOGW("Cannot subscribe, property 0x%x is write only", config.prop);
        return false;
    }
    if (config.changeMode == VehiclePropertyChangeMode::STATIC) {
        ALOGW("Cannot subscribe, property 0x%x is static", config.prop);
        return false;
    }

    //TODO: extend to support event notification for set from android
    if (config.changeMode == VehiclePropertyChangeMode::POLL) {
        ALOGW("Cannot subscribe, property 0x%x is poll only", config.prop);
        return false;
    }
    return true;
}

}  // namespace V2_0
}  // namespace vehicle
}  // namespace hardware
}  // namespace android
