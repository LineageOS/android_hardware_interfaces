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

#include <android-base/stringprintf.h>
#include <utils/Log.h>
#include <utils/SystemClock.h>

#include <inttypes.h>

namespace android {
namespace hardware {
namespace automotive {
namespace vehicle {

namespace {

constexpr float ONE_SECOND_IN_NANO = 1'000'000'000.;

}  // namespace

using ::aidl::android::hardware::automotive::vehicle::IVehicleCallback;
using ::aidl::android::hardware::automotive::vehicle::StatusCode;
using ::aidl::android::hardware::automotive::vehicle::SubscribeOptions;
using ::aidl::android::hardware::automotive::vehicle::VehiclePropError;
using ::aidl::android::hardware::automotive::vehicle::VehiclePropValue;
using ::android::base::Error;
using ::android::base::Result;
using ::android::base::StringPrintf;
using ::ndk::ScopedAStatus;

SubscriptionManager::SubscriptionManager(IVehicleHardware* hardware) : mVehicleHardware(hardware) {}

SubscriptionManager::~SubscriptionManager() {
    std::scoped_lock<std::mutex> lockGuard(mLock);

    mClientsByPropIdArea.clear();
    mSubscribedPropsByClient.clear();
}

bool SubscriptionManager::checkSampleRate(float sampleRate) {
    return getInterval(sampleRate).ok();
}

Result<int64_t> SubscriptionManager::getInterval(float sampleRate) {
    int64_t interval = 0;
    if (sampleRate <= 0) {
        return Error() << "invalid sample rate, must be a positive number";
    }
    if (sampleRate <= (ONE_SECOND_IN_NANO / static_cast<float>(INT64_MAX))) {
        return Error() << "invalid sample rate: " << sampleRate << ", too small";
    }
    interval = static_cast<int64_t>(ONE_SECOND_IN_NANO / sampleRate);
    return interval;
}

void ContSubConfigs::refreshMaxSampleRate() {
    float maxSampleRate = 0.;
    // This is not called frequently so a brute-focre is okay. More efficient way exists but this
    // is simpler.
    for (const auto& [_, sampleRate] : mSampleRates) {
        if (sampleRate > maxSampleRate) {
            maxSampleRate = sampleRate;
        }
    }
    mMaxSampleRate = maxSampleRate;
}

void ContSubConfigs::addClient(const ClientIdType& clientId, float sampleRate) {
    mSampleRates[clientId] = sampleRate;
    refreshMaxSampleRate();
}

void ContSubConfigs::removeClient(const ClientIdType& clientId) {
    mSampleRates.erase(clientId);
    refreshMaxSampleRate();
}

float ContSubConfigs::getMaxSampleRate() {
    return mMaxSampleRate;
}

VhalResult<void> SubscriptionManager::updateSampleRateLocked(const ClientIdType& clientId,
                                                             const PropIdAreaId& propIdAreaId,
                                                             float sampleRate) {
    // Make a copy so that we don't modify 'mContSubConfigsByPropIdArea' on failure cases.
    ContSubConfigs infoCopy = mContSubConfigsByPropIdArea[propIdAreaId];
    infoCopy.addClient(clientId, sampleRate);
    if (infoCopy.getMaxSampleRate() ==
        mContSubConfigsByPropIdArea[propIdAreaId].getMaxSampleRate()) {
        mContSubConfigsByPropIdArea[propIdAreaId] = infoCopy;
        return {};
    }
    float newRate = infoCopy.getMaxSampleRate();
    int32_t propId = propIdAreaId.propId;
    int32_t areaId = propIdAreaId.areaId;
    if (auto status = mVehicleHardware->updateSampleRate(propId, areaId, newRate);
        status != StatusCode::OK) {
        return StatusError(status) << StringPrintf("failed to update sample rate for prop: %" PRId32
                                                   ", area"
                                                   ": %" PRId32 ", sample rate: %f",
                                                   propId, areaId, newRate);
    }
    mContSubConfigsByPropIdArea[propIdAreaId] = infoCopy;
    return {};
}

VhalResult<void> SubscriptionManager::removeSampleRateLocked(const ClientIdType& clientId,
                                                             const PropIdAreaId& propIdAreaId) {
    // Make a copy so that we don't modify 'mContSubConfigsByPropIdArea' on failure cases.
    ContSubConfigs infoCopy = mContSubConfigsByPropIdArea[propIdAreaId];
    infoCopy.removeClient(clientId);
    if (infoCopy.getMaxSampleRate() ==
        mContSubConfigsByPropIdArea[propIdAreaId].getMaxSampleRate()) {
        mContSubConfigsByPropIdArea[propIdAreaId] = infoCopy;
        return {};
    }
    float newRate = infoCopy.getMaxSampleRate();
    int32_t propId = propIdAreaId.propId;
    int32_t areaId = propIdAreaId.areaId;
    if (auto status = mVehicleHardware->updateSampleRate(propId, areaId, newRate);
        status != StatusCode::OK) {
        return StatusError(status) << StringPrintf("failed to update sample rate for prop: %" PRId32
                                                   ", area"
                                                   ": %" PRId32 ", sample rate: %f",
                                                   propId, areaId, newRate);
    }
    mContSubConfigsByPropIdArea[propIdAreaId] = infoCopy;
    return {};
}

VhalResult<void> SubscriptionManager::subscribe(const std::shared_ptr<IVehicleCallback>& callback,
                                                const std::vector<SubscribeOptions>& options,
                                                bool isContinuousProperty) {
    std::scoped_lock<std::mutex> lockGuard(mLock);

    std::vector<int64_t> intervals;
    for (const auto& option : options) {
        float sampleRate = option.sampleRate;

        if (isContinuousProperty) {
            auto intervalResult = getInterval(sampleRate);
            if (!intervalResult.ok()) {
                return StatusError(StatusCode::INVALID_ARG) << intervalResult.error().message();
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
            if (isContinuousProperty) {
                if (auto result = updateSampleRateLocked(clientId, propIdAreaId, option.sampleRate);
                    !result.ok()) {
                    return result;
                }
            }

            mSubscribedPropsByClient[clientId].insert(propIdAreaId);
            mClientsByPropIdArea[propIdAreaId][clientId] = callback;
        }
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
    std::unordered_set<int32_t> subscribedPropIds;
    for (auto const& propIdAreaId : mSubscribedPropsByClient[clientId]) {
        subscribedPropIds.insert(propIdAreaId.propId);
    }

    for (int32_t propId : propIds) {
        if (subscribedPropIds.find(propId) == subscribedPropIds.end()) {
            return StatusError(StatusCode::INVALID_ARG)
                   << "property ID: " << propId << " is not subscribed";
        }
    }

    auto& propIdAreaIds = mSubscribedPropsByClient[clientId];
    auto it = propIdAreaIds.begin();
    while (it != propIdAreaIds.end()) {
        int32_t propId = it->propId;
        if (std::find(propIds.begin(), propIds.end(), propId) != propIds.end()) {
            if (auto result = removeSampleRateLocked(clientId, *it); !result.ok()) {
                return result;
            }

            auto& clients = mClientsByPropIdArea[*it];
            clients.erase(clientId);
            if (clients.empty()) {
                mClientsByPropIdArea.erase(*it);
                mContSubConfigsByPropIdArea.erase(*it);
            }
            it = propIdAreaIds.erase(it);
        } else {
            it++;
        }
    }
    if (propIdAreaIds.empty()) {
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
        if (auto result = removeSampleRateLocked(clientId, propIdAreaId); !result.ok()) {
            return result;
        }

        auto& clients = mClientsByPropIdArea[propIdAreaId];
        clients.erase(clientId);
        if (clients.empty()) {
            mClientsByPropIdArea.erase(propIdAreaId);
            mContSubConfigsByPropIdArea.erase(propIdAreaId);
        }
    }
    mSubscribedPropsByClient.erase(clientId);
    return {};
}

std::unordered_map<std::shared_ptr<IVehicleCallback>, std::vector<const VehiclePropValue*>>
SubscriptionManager::getSubscribedClients(const std::vector<VehiclePropValue>& updatedValues) {
    std::scoped_lock<std::mutex> lockGuard(mLock);
    std::unordered_map<std::shared_ptr<IVehicleCallback>, std::vector<const VehiclePropValue*>>
            clients;

    for (const auto& value : updatedValues) {
        PropIdAreaId propIdAreaId{
                .propId = value.prop,
                .areaId = value.areaId,
        };
        if (mClientsByPropIdArea.find(propIdAreaId) == mClientsByPropIdArea.end()) {
            continue;
        }

        for (const auto& [_, client] : mClientsByPropIdArea[propIdAreaId]) {
            clients[client].push_back(&value);
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
        if (mClientsByPropIdArea.find(propIdAreaId) == mClientsByPropIdArea.end()) {
            continue;
        }

        for (const auto& [_, client] : mClientsByPropIdArea[propIdAreaId]) {
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
    return mSubscribedPropsByClient.empty() && mClientsByPropIdArea.empty();
}

}  // namespace vehicle
}  // namespace automotive
}  // namespace hardware
}  // namespace android
