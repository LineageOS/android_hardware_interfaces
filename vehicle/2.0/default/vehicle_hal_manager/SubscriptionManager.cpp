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

#include "SubscriptionManager.h"

#include <cmath>

#include <android/log.h>

#include "VehicleUtils.h"

namespace android {
namespace hardware {
namespace vehicle {
namespace V2_0 {

bool mergeSubscribeOptions(const SubscribeOptions &oldOpts,
                           const SubscribeOptions &newOpts,
                           SubscribeOptions *outResult) {

    int32_t updatedAreas = oldOpts.vehicleAreas;
    if (updatedAreas != kAllSupportedAreas) {
        updatedAreas = newOpts.vehicleAreas != kAllSupportedAreas
            ? updatedAreas | newOpts.vehicleAreas
            : kAllSupportedAreas;
    }

    float updatedRate = std::max(oldOpts.sampleRate, newOpts.sampleRate);
    SubscribeFlags updatedFlags = SubscribeFlags(oldOpts.flags | newOpts.flags);

    bool updated = updatedRate > oldOpts.sampleRate
                   || updatedAreas != oldOpts.vehicleAreas
                   || updatedFlags != oldOpts.flags;
    if (updated) {
        *outResult = oldOpts;
        outResult->vehicleAreas = updatedAreas;
        outResult->sampleRate = updatedRate;
        outResult->flags = updatedFlags;
    }

    return updated;
}

void HalClient::addOrUpdateSubscription(const SubscribeOptions &opts)  {
    auto it = mSubscriptions.find(opts.propId);
    if (it == mSubscriptions.end()) {
        mSubscriptions.emplace(opts.propId, opts);
    } else {
        const SubscribeOptions& oldOpts = it->second;
        SubscribeOptions updatedOptions;
        if (mergeSubscribeOptions(oldOpts, opts, &updatedOptions)) {
            mSubscriptions.erase(it);
            mSubscriptions.emplace(opts.propId, updatedOptions);
        }
    }
}

bool HalClient::isSubscribed(VehicleProperty propId, int32_t areaId, SubscribeFlags flags) {
    auto it = mSubscriptions.find(propId);
    if (it == mSubscriptions.end()) {
        return false;
    }
    const SubscribeOptions& opts = it->second;
    bool res = (opts.flags & flags)
           && (opts.vehicleAreas == 0 || areaId == 0 || opts.vehicleAreas & areaId);
    return res;
}

std::list<SubscribeOptions> SubscriptionManager::addOrUpdateSubscription(
        const sp<IVehicleCallback> &callback,
        const hidl_vec<SubscribeOptions> &optionList) {
    std::list<SubscribeOptions> updatedSubscriptions;

    MuxGuard g(mLock);

    const sp<HalClient>& client = getOrCreateHalClientLocked(callback);

    for (size_t i = 0; i < optionList.size(); i++) {
        const SubscribeOptions& opts = optionList[i];
        client->addOrUpdateSubscription(opts);

        addClientToPropMapLocked(opts.propId, client);

        if (SubscribeFlags::HAL_EVENT & opts.flags) {
            SubscribeOptions updated;
            if (updateHalEventSubscriptionLocked(opts, &updated)) {
                updatedSubscriptions.push_back(updated);
            }
        }
    }

    return updatedSubscriptions;
}

std::list<HalClientValues> SubscriptionManager::distributeValuesToClients(
        const std::vector<recyclable_ptr<VehiclePropValue>>& propValues,
        SubscribeFlags flags) const {
    std::map<sp<HalClient>, std::list<VehiclePropValue*>> clientValuesMap;

    {
        MuxGuard g(mLock);
        for (const auto& propValue: propValues) {
            VehiclePropValue* v = propValue.get();
            auto clients = getSubscribedClientsLocked(
                v->prop, v->areaId, flags);
            for (const auto& client : clients) {
                clientValuesMap[client].push_back(v);
            }
        }
    }

    std::list<HalClientValues> clientValues;
    for (const auto& entry : clientValuesMap) {
        clientValues.push_back(HalClientValues {
            .client = entry.first,
            .values = entry.second
        });
    }

    return clientValues;
}

std::list<sp<HalClient>> SubscriptionManager::getSubscribedClients(
    VehicleProperty propId, int32_t area, SubscribeFlags flags) const {
    MuxGuard g(mLock);
    return getSubscribedClientsLocked(propId, area, flags);
}

std::list<sp<HalClient>> SubscriptionManager::getSubscribedClientsLocked(
        VehicleProperty propId, int32_t area, SubscribeFlags flags) const {
    std::list<sp<HalClient>> subscribedClients;

    sp<HalClientVector> propClients = getClientsForPropertyLocked(propId);
    if (propClients.get() != nullptr) {
        for (size_t i = 0; i < propClients->size(); i++) {
            const auto& client = propClients->itemAt(i);
            if (client->isSubscribed(propId, area, flags)) {
                subscribedClients.push_back(client);
            }
        }
    }

    return subscribedClients;
}

bool SubscriptionManager::updateHalEventSubscriptionLocked(
        const SubscribeOptions &opts, SubscribeOptions *outUpdated) {
    bool updated = false;
    auto it = mHalEventSubscribeOptions.find(opts.propId);
    if (it == mHalEventSubscribeOptions.end()) {
        *outUpdated = opts;
        mHalEventSubscribeOptions.emplace(opts.propId, opts);
        updated = true;
    } else {
        const SubscribeOptions& oldOpts = it->second;

        if (mergeSubscribeOptions(oldOpts, opts, outUpdated)) {
            mHalEventSubscribeOptions.erase(opts.propId);
            mHalEventSubscribeOptions.emplace(opts.propId, *outUpdated);
            updated = true;
        }
    }

    return updated;
}

void SubscriptionManager::addClientToPropMapLocked(
        VehicleProperty propId, const sp<HalClient> &client) {
    auto it = mPropToClients.find(propId);
    sp<HalClientVector> propClients;
    if (it == mPropToClients.end()) {
        propClients = new HalClientVector();
        mPropToClients.insert(std::make_pair(propId, propClients));
    } else {
        propClients = it->second;
    }
    propClients->addOrUpdate(client);
}

sp<HalClientVector> SubscriptionManager::getClientsForPropertyLocked(
        VehicleProperty propId) const {
    auto it = mPropToClients.find(propId);
    return it == mPropToClients.end() ? nullptr : it->second;
}

sp<HalClient> SubscriptionManager::getOrCreateHalClientLocked(
        const sp<IVehicleCallback>& callback) {
    auto it = mClients.find(callback);
    if (it == mClients.end()) {
        IPCThreadState* self = IPCThreadState::self();
        pid_t pid = self->getCallingPid();
        uid_t uid = self->getCallingUid();
        sp<HalClient> client = new HalClient(callback, pid, uid);
        mClients.emplace(callback, client);
        return client;
    } else {
        return it->second;
    }
}

bool SubscriptionManager::unsubscribe(const sp<IVehicleCallback>& callback,
                                      VehicleProperty propId) {
    MuxGuard g(mLock);
    auto propertyClients = getClientsForPropertyLocked(propId);
    auto clientIter = mClients.find(callback);
    if (clientIter == mClients.end()) {
        ALOGW("Unable to unsubscribe: no callback found, propId: 0x%x", propId);
    } else {
        auto client = clientIter->second;

        if (propertyClients != nullptr) {
            propertyClients->remove(client);

            if (propertyClients->isEmpty()) {
                mPropToClients.erase(propId);
            }
        }

        bool isClientSubscribedToOtherProps = false;
        for (const auto& propClient : mPropToClients) {
            if (propClient.second->indexOf(client) >= 0) {
                isClientSubscribedToOtherProps = true;
                break;
            }
        }

        if (!isClientSubscribedToOtherProps) {
            mClients.erase(clientIter);
        }
    }

    return (propertyClients == nullptr || propertyClients->isEmpty())
            ? mHalEventSubscribeOptions.erase(propId) == 1
            : false;
}


}  // namespace V2_0
}  // namespace vehicle
}  // namespace hardware
}  // namespace android
