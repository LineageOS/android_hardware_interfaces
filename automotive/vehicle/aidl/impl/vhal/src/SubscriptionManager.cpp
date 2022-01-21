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

#include <math/HashCombine.h>
#include <utils/Log.h>

namespace android {
namespace hardware {
namespace automotive {
namespace vehicle {

namespace {

constexpr float ONE_SECOND_IN_NANO = 1'000'000'000.;

}  // namespace

using ::aidl::android::hardware::automotive::vehicle::IVehicleCallback;
using ::aidl::android::hardware::automotive::vehicle::SubscribeOptions;
using ::aidl::android::hardware::automotive::vehicle::VehiclePropValue;
using ::android::base::Error;
using ::android::base::Result;
using ::ndk::ScopedAStatus;

bool SubscriptionManager::PropIdAreaId::operator==(const PropIdAreaId& other) const {
    return areaId == other.areaId && propId == other.propId;
}

size_t SubscriptionManager::PropIdAreaIdHash::operator()(PropIdAreaId const& propIdAreaId) const {
    size_t res = 0;
    hashCombine(res, propIdAreaId.propId);
    hashCombine(res, propIdAreaId.areaId);
    return res;
}

SubscriptionManager::SubscriptionManager(GetValueFunc&& action)
    : mTimer(std::make_shared<RecurrentTimer>()), mGetValue(std::move(action)) {}

SubscriptionManager::~SubscriptionManager() {
    std::scoped_lock<std::mutex> lockGuard(mLock);

    mClientsByPropIdArea.clear();
    // RecurrentSubscription has reference to mGetValue, so it must be destroyed before mGetValue is
    // destroyed.
    mSubscriptionsByClient.clear();
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

Result<void> SubscriptionManager::subscribe(const std::shared_ptr<IVehicleCallback>& callback,
                                            const std::vector<SubscribeOptions>& options,
                                            bool isContinuousProperty) {
    std::scoped_lock<std::mutex> lockGuard(mLock);

    std::vector<int64_t> intervals;
    for (const auto& option : options) {
        float sampleRate = option.sampleRate;

        if (isContinuousProperty) {
            auto intervalResult = getInterval(sampleRate);
            if (!intervalResult.ok()) {
                return intervalResult.error();
            }
            intervals.push_back(intervalResult.value());
        }

        if (option.areaIds.empty()) {
            ALOGE("area IDs to subscribe must not be empty");
            return Error() << "area IDs to subscribe must not be empty";
        }
    }

    size_t intervalIndex = 0;
    ClientIdType clientId = callback->asBinder().get();
    for (const auto& option : options) {
        int32_t propId = option.propId;
        const std::vector<int32_t>& areaIds = option.areaIds;
        int64_t interval = 0;
        if (isContinuousProperty) {
            interval = intervals[intervalIndex];
            intervalIndex++;
        }
        for (int32_t areaId : areaIds) {
            PropIdAreaId propIdAreaId = {
                    .propId = propId,
                    .areaId = areaId,
            };
            if (isContinuousProperty) {
                VehiclePropValue propValueRequest{
                        .prop = propId,
                        .areaId = areaId,
                };
                mSubscriptionsByClient[clientId][propIdAreaId] =
                        std::make_unique<RecurrentSubscription>(
                                mTimer,
                                [this, callback, propValueRequest] {
                                    mGetValue(callback, propValueRequest);
                                },
                                interval);
            } else {
                mSubscriptionsByClient[clientId][propIdAreaId] =
                        std::make_unique<OnChangeSubscription>();
            }
            mClientsByPropIdArea[propIdAreaId][clientId] = callback;
        }
    }
    return {};
}

Result<void> SubscriptionManager::unsubscribe(SubscriptionManager::ClientIdType clientId,
                                              const std::vector<int32_t>& propIds) {
    std::scoped_lock<std::mutex> lockGuard(mLock);

    if (mSubscriptionsByClient.find(clientId) == mSubscriptionsByClient.end()) {
        return Error() << "No property was subscribed for the callback";
    }
    std::unordered_set<int32_t> subscribedPropIds;
    for (auto const& [propIdAreaId, _] : mSubscriptionsByClient[clientId]) {
        subscribedPropIds.insert(propIdAreaId.propId);
    }

    for (int32_t propId : propIds) {
        if (subscribedPropIds.find(propId) == subscribedPropIds.end()) {
            return Error() << "property ID: " << propId << " is not subscribed";
        }
    }

    auto& subscriptions = mSubscriptionsByClient[clientId];
    auto it = subscriptions.begin();
    while (it != subscriptions.end()) {
        int32_t propId = it->first.propId;
        if (std::find(propIds.begin(), propIds.end(), propId) != propIds.end()) {
            auto& clients = mClientsByPropIdArea[it->first];
            clients.erase(clientId);
            if (clients.empty()) {
                mClientsByPropIdArea.erase(it->first);
            }
            it = subscriptions.erase(it);
        } else {
            it++;
        }
    }
    if (subscriptions.empty()) {
        mSubscriptionsByClient.erase(clientId);
    }
    return {};
}

Result<void> SubscriptionManager::unsubscribe(SubscriptionManager::ClientIdType clientId) {
    std::scoped_lock<std::mutex> lockGuard(mLock);

    if (mSubscriptionsByClient.find(clientId) == mSubscriptionsByClient.end()) {
        return Error() << "No property was subscribed for this client";
    }

    auto& subscriptions = mSubscriptionsByClient[clientId];
    for (auto const& [propIdAreaId, _] : subscriptions) {
        auto& clients = mClientsByPropIdArea[propIdAreaId];
        clients.erase(clientId);
        if (clients.empty()) {
            mClientsByPropIdArea.erase(propIdAreaId);
        }
    }
    mSubscriptionsByClient.erase(clientId);
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
        for (const auto& [clientId, client] : mClientsByPropIdArea[propIdAreaId]) {
            if (!mSubscriptionsByClient[clientId][propIdAreaId]->isOnChange()) {
                continue;
            }
            clients[client].push_back(&value);
        }
    }
    return clients;
}

bool SubscriptionManager::isEmpty() {
    std::scoped_lock<std::mutex> lockGuard(mLock);
    return mSubscriptionsByClient.empty() && mClientsByPropIdArea.empty();
}

SubscriptionManager::RecurrentSubscription::RecurrentSubscription(
        std::shared_ptr<RecurrentTimer> timer, std::function<void()>&& action, int64_t interval)
    : mAction(std::make_shared<std::function<void()>>(action)), mTimer(timer) {
    mTimer->registerTimerCallback(interval, mAction);
}

SubscriptionManager::RecurrentSubscription::~RecurrentSubscription() {
    mTimer->unregisterTimerCallback(mAction);
}

bool SubscriptionManager::RecurrentSubscription::isOnChange() {
    return false;
}

bool SubscriptionManager::OnChangeSubscription::isOnChange() {
    return true;
}

}  // namespace vehicle
}  // namespace automotive
}  // namespace hardware
}  // namespace android
