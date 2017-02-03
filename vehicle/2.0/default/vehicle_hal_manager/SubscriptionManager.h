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

#ifndef android_hardware_vehicle_V2_0_SubscriptionManager_H_
#define android_hardware_vehicle_V2_0_SubscriptionManager_H_

#include <memory>
#include <map>
#include <set>
#include <list>

#include <android/log.h>
#include <hwbinder/IPCThreadState.h>

#include <android/hardware/vehicle/2.0/IVehicle.h>

#include "ConcurrentQueue.h"
#include "VehicleObjectPool.h"

namespace android {
namespace hardware {
namespace vehicle {
namespace V2_0 {

class HalClient : public android::RefBase {
public:
    HalClient(const sp<IVehicleCallback> &callback,
              int32_t pid,
              int32_t uid)
        : mCallback(callback), mPid(pid), mUid(uid) {}

    virtual ~HalClient() {}
public:
    sp<IVehicleCallback> getCallback() const {
        return mCallback;
    }

    void addOrUpdateSubscription(const SubscribeOptions &opts);

    bool isSubscribed(int32_t propId,
                      int32_t areaId,
                      SubscribeFlags flags);

private:
    const sp<IVehicleCallback> mCallback;
    const int32_t mPid;
    const int32_t mUid;

    std::map<int32_t, SubscribeOptions> mSubscriptions;
};

class HalClientVector : private SortedVector<sp<HalClient>> , public RefBase {
public:
    virtual ~HalClientVector() {}

    inline void addOrUpdate(const sp<HalClient> &client) {
        SortedVector::add(client);
    }

    using SortedVector::remove;
    using SortedVector::size;
    using SortedVector::indexOf;
    using SortedVector::itemAt;
    using SortedVector::isEmpty;
};

struct HalClientValues {
    sp<HalClient> client;
    std::list<VehiclePropValue *> values;
};

class SubscriptionManager {
public:
    virtual ~SubscriptionManager() {}

    /**
     * Updates subscription. Returns the vector of properties subscription that
     * needs to be updated in VehicleHAL.
     */
    std::list<SubscribeOptions> addOrUpdateSubscription(
            const sp<IVehicleCallback>& callback,
            const hidl_vec<SubscribeOptions>& optionList);

    /**
     * Returns a list of IVehicleCallback -> list of VehiclePropValue ready for
     * dispatching to its clients.
     */
    std::list<HalClientValues> distributeValuesToClients(
            const std::vector<recyclable_ptr<VehiclePropValue>>& propValues,
            SubscribeFlags flags) const;

    std::list<sp<HalClient>> getSubscribedClients(
        int32_t propId, int32_t area, SubscribeFlags flags) const;

    /**
     * Returns true the client was unsubscribed successfully and there are
     * no more clients subscribed to given propId.
     */
    bool unsubscribe(const sp<IVehicleCallback>& callback,
                     int32_t propId);
private:
    std::list<sp< HalClient>> getSubscribedClientsLocked(
            int32_t propId, int32_t area, SubscribeFlags flags) const;

    bool updateHalEventSubscriptionLocked(const SubscribeOptions &opts,
                                          SubscribeOptions *out);

    void addClientToPropMapLocked(int32_t propId,
                                  const sp<HalClient> &client);

    sp<HalClientVector> getClientsForPropertyLocked(
            int32_t propId) const;

    sp<HalClient> getOrCreateHalClientLocked(
            const sp<IVehicleCallback> &callback);

private:
    using MuxGuard = std::lock_guard<std::mutex>;

    mutable std::mutex mLock;

    std::map<sp<IVehicleCallback>, sp<HalClient>> mClients;
    std::map<int32_t, sp<HalClientVector>> mPropToClients;
    std::map<int32_t, SubscribeOptions> mHalEventSubscribeOptions;
};


}  // namespace V2_0
}  // namespace vehicle
}  // namespace hardware
}  // namespace android


#endif // android_hardware_vehicle_V2_0_SubscriptionManager_H_
