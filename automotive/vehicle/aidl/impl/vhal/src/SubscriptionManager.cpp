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

#include "SubscriptionManager.h"

#include <VehicleUtils.h>
#include <android-base/stringprintf.h>
#include <utils/Log.h>
#include <utils/SystemClock.h>

#include <inttypes.h>

namespace android {
namespace hardware {
namespace automotive {
namespace vehicle {

namespace {

using ::aidl::android::hardware::automotive::vehicle::IVehicleCallback;
using ::aidl::android::hardware::automotive::vehicle::StatusCode;
using ::aidl::android::hardware::automotive::vehicle::SubscribeOptions;
using ::aidl::android::hardware::automotive::vehicle::VehiclePropError;
using ::aidl::android::hardware::automotive::vehicle::VehiclePropValue;
using ::android::base::Error;
using ::android::base::Result;
using ::android::base::StringPrintf;
using ::ndk::ScopedAStatus;

constexpr float ONE_SECOND_IN_NANOS = 1'000'000'000.;

SubscribeOptions newSubscribeOptions(int32_t propId, int32_t areaId, float sampleRateHz,
                                     bool enableVur) {
    SubscribeOptions subscribedOptions;
    subscribedOptions.propId = propId;
    subscribedOptions.areaIds = {areaId};
    subscribedOptions.sampleRate = sampleRateHz;
    subscribedOptions.enableVariableUpdateRate = enableVur;

    return subscribedOptions;
}

}  // namespace

SubscriptionManager::SubscriptionManager(IVehicleHardware* vehicleHardware)
    : mVehicleHardware(vehicleHardware) {}

SubscriptionManager::~SubscriptionManager() {
    std::scoped_lock<std::mutex> lockGuard(mLock);

    mClientsByPropIdAreaId.clear();
    mSubscribedPropsByClient.clear();
}

bool SubscriptionManager::checkSampleRateHz(float sampleRateHz) {
    return getIntervalNanos(sampleRateHz).ok();
}

Result<int64_t> SubscriptionManager::getIntervalNanos(float sampleRateHz) {
    int64_t intervalNanos = 0;
    if (sampleRateHz <= 0) {
        return Error() << "invalid sample rate, must be a positive number";
    }
    if (sampleRateHz <= (ONE_SECOND_IN_NANOS / static_cast<float>(INT64_MAX))) {
        return Error() << "invalid sample rate: " << sampleRateHz << ", too small";
    }
    intervalNanos = static_cast<int64_t>(ONE_SECOND_IN_NANOS / sampleRateHz);
    return intervalNanos;
}

void ContSubConfigs::refreshCombinedConfig() {
    float maxSampleRateHz = 0.;
    bool enableVur = true;
    // This is not called frequently so a brute-focre is okay. More efficient way exists but this
    // is simpler.
    for (const auto& [_, subConfig] : mConfigByClient) {
        if (subConfig.sampleRateHz > maxSampleRateHz) {
            maxSampleRateHz = subConfig.sampleRateHz;
        }
        if (!subConfig.enableVur) {
            // If one client does not enable variable update rate, we cannot enable variable update
            // rate in IVehicleHardware.
            enableVur = false;
        }
    }
    mMaxSampleRateHz = maxSampleRateHz;
    mEnableVur = enableVur;
}

void ContSubConfigs::addClient(const ClientIdType& clientId, float sampleRateHz, bool enableVur) {
    mConfigByClient[clientId] = {
            .sampleRateHz = sampleRateHz,
            .enableVur = enableVur,
    };
    refreshCombinedConfig();
}

void ContSubConfigs::removeClient(const ClientIdType& clientId) {
    mConfigByClient.erase(clientId);
    refreshCombinedConfig();
}

float ContSubConfigs::getMaxSampleRateHz() const {
    return mMaxSampleRateHz;
}

bool ContSubConfigs::isVurEnabled() const {
    return mEnableVur;
}

bool ContSubConfigs::isVurEnabledForClient(const ClientIdType& clientId) {
    return mConfigByClient[clientId].enableVur;
}

VhalResult<void> SubscriptionManager::addOnChangeSubscriberLocked(
        const PropIdAreaId& propIdAreaId) {
    if (mClientsByPropIdAreaId.find(propIdAreaId) != mClientsByPropIdAreaId.end()) {
        // This propId, areaId is already subscribed, ignore the request.
        return {};
    }

    int32_t propId = propIdAreaId.propId;
    int32_t areaId = propIdAreaId.areaId;
    if (auto status = mVehicleHardware->subscribe(
                newSubscribeOptions(propId, areaId, /*updateRateHz=*/0, /*enableVur*/ false));
        status != StatusCode::OK) {
        return StatusError(status)
               << StringPrintf("failed subscribe for prop: %s, areaId: %" PRId32,
                               propIdToString(propId).c_str(), areaId);
    }
    return {};
}

VhalResult<void> SubscriptionManager::addContinuousSubscriberLocked(
        const ClientIdType& clientId, const PropIdAreaId& propIdAreaId, float sampleRateHz,
        bool enableVur) {
    // Make a copy so that we don't modify 'mContSubConfigsByPropIdArea' on failure cases.
    ContSubConfigs newConfig = mContSubConfigsByPropIdArea[propIdAreaId];
    newConfig.addClient(clientId, sampleRateHz, enableVur);
    return updateContSubConfigsLocked(propIdAreaId, newConfig);
}

VhalResult<void> SubscriptionManager::removeContinuousSubscriberLocked(
        const ClientIdType& clientId, const PropIdAreaId& propIdAreaId) {
    // Make a copy so that we don't modify 'mContSubConfigsByPropIdArea' on failure cases.
    ContSubConfigs newConfig = mContSubConfigsByPropIdArea[propIdAreaId];
    newConfig.removeClient(clientId);
    return updateContSubConfigsLocked(propIdAreaId, newConfig);
}

VhalResult<void> SubscriptionManager::removeOnChangeSubscriberLocked(
        const PropIdAreaId& propIdAreaId) {
    if (mClientsByPropIdAreaId[propIdAreaId].size() > 1) {
        // After unsubscribing this client, there is still client subscribed, so do nothing.
        return {};
    }

    int32_t propId = propIdAreaId.propId;
    int32_t areaId = propIdAreaId.areaId;
    if (auto status = mVehicleHardware->unsubscribe(propId, areaId); status != StatusCode::OK) {
        return StatusError(status)
               << StringPrintf("failed unsubscribe for prop: %s, areaId: %" PRId32,
                               propIdToString(propId).c_str(), areaId);
    }
    return {};
}

VhalResult<void> SubscriptionManager::updateContSubConfigsLocked(const PropIdAreaId& propIdAreaId,
                                                                 const ContSubConfigs& newConfig) {
    const auto& oldConfig = mContSubConfigsByPropIdArea[propIdAreaId];
    float newRateHz = newConfig.getMaxSampleRateHz();
    float oldRateHz = oldConfig.getMaxSampleRateHz();
    if (newRateHz == oldRateHz && newConfig.isVurEnabled() == oldConfig.isVurEnabled()) {
        mContSubConfigsByPropIdArea[propIdAreaId] = newConfig;
        return {};
    }
    int32_t propId = propIdAreaId.propId;
    int32_t areaId = propIdAreaId.areaId;
    if (newRateHz != oldRateHz) {
        if (auto status = mVehicleHardware->updateSampleRate(propId, areaId, newRateHz);
            status != StatusCode::OK) {
            return StatusError(status)
                   << StringPrintf("failed to update sample rate for prop: %s, areaId: %" PRId32
                                   ", sample rate: %f HZ",
                                   propIdToString(propId).c_str(), areaId, newRateHz);
        }
    }
    if (newRateHz != 0) {
        if (auto status = mVehicleHardware->subscribe(
                    newSubscribeOptions(propId, areaId, newRateHz, newConfig.isVurEnabled()));
            status != StatusCode::OK) {
            return StatusError(status) << StringPrintf(
                           "failed subscribe for prop: %s, areaId"
                           ": %" PRId32 ", sample rate: %f HZ",
                           propIdToString(propId).c_str(), areaId, newRateHz);
        }
    } else {
        if (auto status = mVehicleHardware->unsubscribe(propId, areaId); status != StatusCode::OK) {
            return StatusError(status) << StringPrintf(
                           "failed unsubscribe for prop: %s, areaId"
                           ": %" PRId32,
                           propIdToString(propId).c_str(), areaId);
        }
    }
    mContSubConfigsByPropIdArea[propIdAreaId] = newConfig;
    return {};
}

VhalResult<void> SubscriptionManager::subscribe(const std::shared_ptr<IVehicleCallback>& callback,
                                                const std::vector<SubscribeOptions>& options,
                                                bool isContinuousProperty) {
    std::scoped_lock<std::mutex> lockGuard(mLock);

    for (const auto& option : options) {
        float sampleRateHz = option.sampleRate;

        if (isContinuousProperty) {
            if (auto result = getIntervalNanos(sampleRateHz); !result.ok()) {
                return StatusError(StatusCode::INVALID_ARG) << result.error().message();
            }
        }

        if (option.areaIds.empty()) {
            ALOGE("area IDs to subscribe must not be empty");
            return StatusError(StatusCode::INVALID_ARG)
                   << "area IDs to subscribe must not be empty";
        }
    }

    ClientIdType clientId = callback->asBinder().get();

    for (const auto& option : options) {
        int32_t propId = option.propId;
        const std::vector<int32_t>& areaIds = option.areaIds;
        for (int32_t areaId : areaIds) {
            PropIdAreaId propIdAreaId = {
                    .propId = propId,
                    .areaId = areaId,
            };
            VhalResult<void> result;
            if (isContinuousProperty) {
                result = addContinuousSubscriberLocked(clientId, propIdAreaId, option.sampleRate,
                                                       option.enableVariableUpdateRate);
            } else {
                result = addOnChangeSubscriberLocked(propIdAreaId);
            }

            if (!result.ok()) {
                return result;
            }

            mSubscribedPropsByClient[clientId].insert(propIdAreaId);
            mClientsByPropIdAreaId[propIdAreaId][clientId] = callback;
        }
    }
    return {};
}

VhalResult<void> SubscriptionManager::unsubscribePropIdAreaIdLocked(
        SubscriptionManager::ClientIdType clientId, const PropIdAreaId& propIdAreaId) {
    if (mContSubConfigsByPropIdArea.find(propIdAreaId) != mContSubConfigsByPropIdArea.end()) {
        // This is a subscribed continuous property.
        if (auto result = removeContinuousSubscriberLocked(clientId, propIdAreaId); !result.ok()) {
            return result;
        }
    } else {
        if (mClientsByPropIdAreaId.find(propIdAreaId) == mClientsByPropIdAreaId.end()) {
            ALOGW("Unsubscribe: The property: %s, areaId: %" PRId32
                  " was not previously subscribed, do nothing",
                  propIdToString(propIdAreaId.propId).c_str(), propIdAreaId.areaId);
            return {};
        }
        // This is an on-change property.
        if (auto result = removeOnChangeSubscriberLocked(propIdAreaId); !result.ok()) {
            return result;
        }
    }

    auto& clients = mClientsByPropIdAreaId[propIdAreaId];
    clients.erase(clientId);
    if (clients.empty()) {
        mClientsByPropIdAreaId.erase(propIdAreaId);
        mContSubConfigsByPropIdArea.erase(propIdAreaId);
    }
    return {};
}

VhalResult<void> SubscriptionManager::unsubscribe(SubscriptionManager::ClientIdType clientId,
                                                  const std::vector<int32_t>& propIds) {
    std::scoped_lock<std::mutex> lockGuard(mLock);

    if (mSubscribedPropsByClient.find(clientId) == mSubscribedPropsByClient.end()) {
        return StatusError(StatusCode::INVALID_ARG)
               << "No property was subscribed for the callback";
    }

    std::vector<PropIdAreaId> propIdAreaIdsToUnsubscribe;
    std::unordered_set<int32_t> propIdSet;
    for (int32_t propId : propIds) {
        propIdSet.insert(propId);
    }
    auto& subscribedPropIdsAreaIds = mSubscribedPropsByClient[clientId];
    for (const auto& propIdAreaId : subscribedPropIdsAreaIds) {
        if (propIdSet.find(propIdAreaId.propId) != propIdSet.end()) {
            propIdAreaIdsToUnsubscribe.push_back(propIdAreaId);
        }
    }

    for (const auto& propIdAreaId : propIdAreaIdsToUnsubscribe) {
        if (auto result = unsubscribePropIdAreaIdLocked(clientId, propIdAreaId); !result.ok()) {
            return result;
        }
        subscribedPropIdsAreaIds.erase(propIdAreaId);
    }

    if (subscribedPropIdsAreaIds.empty()) {
        mSubscribedPropsByClient.erase(clientId);
    }
    return {};
}

VhalResult<void> SubscriptionManager::unsubscribe(SubscriptionManager::ClientIdType clientId) {
    std::scoped_lock<std::mutex> lockGuard(mLock);

    if (mSubscribedPropsByClient.find(clientId) == mSubscribedPropsByClient.end()) {
        return StatusError(StatusCode::INVALID_ARG) << "No property was subscribed for this client";
    }

    auto& subscriptions = mSubscribedPropsByClient[clientId];
    for (auto const& propIdAreaId : subscriptions) {
        if (auto result = unsubscribePropIdAreaIdLocked(clientId, propIdAreaId); !result.ok()) {
            return result;
        }
    }
    mSubscribedPropsByClient.erase(clientId);
    return {};
}

bool SubscriptionManager::isValueUpdatedLocked(const std::shared_ptr<IVehicleCallback>& callback,
                                               const VehiclePropValue& value) {
    const auto& it = mContSubValuesByCallback[callback].find(value);
    if (it == mContSubValuesByCallback[callback].end()) {
        mContSubValuesByCallback[callback].insert(value);
        return true;
    }

    if (it->timestamp > value.timestamp) {
        ALOGE("The updated property value: %s is outdated, ignored", value.toString().c_str());
        return false;
    }

    if (it->value == value.value && it->status == value.status) {
        // Even though the property value is the same, we need to store the new property event to
        // update the timestamp.
        mContSubValuesByCallback[callback].insert(value);
        ALOGD("The updated property value for propId: %" PRId32 ", areaId: %" PRId32
              " has the "
              "same value and status, ignored if VUR is enabled",
              it->prop, it->areaId);
        return false;
    }

    mContSubValuesByCallback[callback].insert(value);
    return true;
}

std::unordered_map<std::shared_ptr<IVehicleCallback>, std::vector<VehiclePropValue>>
SubscriptionManager::getSubscribedClients(std::vector<VehiclePropValue>&& updatedValues) {
    std::scoped_lock<std::mutex> lockGuard(mLock);
    std::unordered_map<std::shared_ptr<IVehicleCallback>, std::vector<VehiclePropValue>> clients;

    for (auto& value : updatedValues) {
        PropIdAreaId propIdAreaId{
                .propId = value.prop,
                .areaId = value.areaId,
        };
        if (mClientsByPropIdAreaId.find(propIdAreaId) == mClientsByPropIdAreaId.end()) {
            continue;
        }

        for (const auto& [client, callback] : mClientsByPropIdAreaId[propIdAreaId]) {
            auto& subConfigs = mContSubConfigsByPropIdArea[propIdAreaId];
            // If client wants VUR (and VUR is supported as checked in DefaultVehicleHal), it is
            // possible that VUR is not enabled in IVehicleHardware because another client does not
            // enable VUR. We will implement VUR filtering here for the client that enables it.
            if (subConfigs.isVurEnabledForClient(client) && !subConfigs.isVurEnabled()) {
                if (isValueUpdatedLocked(callback, value)) {
                    clients[callback].push_back(value);
                }
            } else {
                clients[callback].push_back(value);
            }
        }
    }
    return clients;
}

std::unordered_map<std::shared_ptr<IVehicleCallback>, std::vector<VehiclePropError>>
SubscriptionManager::getSubscribedClientsForErrorEvents(
        const std::vector<SetValueErrorEvent>& errorEvents) {
    std::scoped_lock<std::mutex> lockGuard(mLock);
    std::unordered_map<std::shared_ptr<IVehicleCallback>, std::vector<VehiclePropError>> clients;

    for (const auto& errorEvent : errorEvents) {
        PropIdAreaId propIdAreaId{
                .propId = errorEvent.propId,
                .areaId = errorEvent.areaId,
        };
        if (mClientsByPropIdAreaId.find(propIdAreaId) == mClientsByPropIdAreaId.end()) {
            continue;
        }

        for (const auto& [_, client] : mClientsByPropIdAreaId[propIdAreaId]) {
            clients[client].push_back({
                    .propId = errorEvent.propId,
                    .areaId = errorEvent.areaId,
                    .errorCode = errorEvent.errorCode,
            });
        }
    }
    return clients;
}

bool SubscriptionManager::isEmpty() {
    std::scoped_lock<std::mutex> lockGuard(mLock);
    return mSubscribedPropsByClient.empty() && mClientsByPropIdAreaId.empty();
}

size_t SubscriptionManager::countClients() {
    std::scoped_lock<std::mutex> lockGuard(mLock);
    return mSubscribedPropsByClient.size();
}

}  // namespace vehicle
}  // namespace automotive
}  // namespace hardware
}  // namespace android
