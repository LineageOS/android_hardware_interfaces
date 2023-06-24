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

#ifndef android_hardware_automotive_vehicle_aidl_impl_vhal_include_SubscriptionManager_H_
#define android_hardware_automotive_vehicle_aidl_impl_vhal_include_SubscriptionManager_H_

#include <IVehicleHardware.h>
#include <VehicleHalTypes.h>
#include <VehicleUtils.h>

#include <aidl/android/hardware/automotive/vehicle/IVehicleCallback.h>
#include <android-base/result.h>
#include <android-base/thread_annotations.h>

#include <mutex>
#include <optional>
#include <unordered_map>
#include <unordered_set>
#include <vector>

namespace android {
namespace hardware {
namespace automotive {
namespace vehicle {

// A class to represent all the subscription configs for a continuous [propId, areaId].
class ContSubConfigs final {
  public:
    using ClientIdType = const AIBinder*;

    void addClient(const ClientIdType& clientId, float sampleRateHz);
    void removeClient(const ClientIdType& clientId);
    float getMaxSampleRateHz() const;

  private:
    float mMaxSampleRateHz = 0.;
    std::unordered_map<ClientIdType, float> mSampleRateHzByClient;

    void refreshMaxSampleRateHz();
};

// A thread-safe subscription manager that manages all VHAL subscriptions.
class SubscriptionManager final {
  public:
    using ClientIdType = const AIBinder*;
    using CallbackType =
            std::shared_ptr<aidl::android::hardware::automotive::vehicle::IVehicleCallback>;

    explicit SubscriptionManager(IVehicleHardware* vehicleHardware);
    ~SubscriptionManager();

    // Subscribes to properties according to {@code SubscribeOptions}. Note that all option must
    // contain non-empty areaIds field, which contains all area IDs to subscribe. As a result,
    // the options here is different from the options passed from VHAL client.
    // Returns error if any of the subscribe options is not valid or one of the properties failed
    // to subscribe. Part of the properties maybe be subscribed successfully if this function
    // returns error. Caller is safe to retry since subscribing to an already subscribed property
    // is okay.
    // Returns ok if all the options are parsed correctly and all the properties are subscribed.
    VhalResult<void> subscribe(
            const CallbackType& callback,
            const std::vector<aidl::android::hardware::automotive::vehicle::SubscribeOptions>&
                    options,
            bool isContinuousProperty);

    // Unsubscribes from the properties for the client.
    // Returns error if the client was not subscribed before, or one of the given property was not
    // subscribed, or one of the property failed to unsubscribe. Caller is safe to retry since
    // unsubscribing to an already unsubscribed property is okay (it would be ignored).
    // Returns ok if all the requested properties for the client are unsubscribed.
    VhalResult<void> unsubscribe(ClientIdType client, const std::vector<int32_t>& propIds);

    // Unsubscribes from all the properties for the client.
    // Returns error if the client was not subscribed before or one of the subscribed properties
    // for the client failed to unsubscribe. Caller is safe to retry.
    // Returns ok if all the properties for the client are unsubscribed.
    VhalResult<void> unsubscribe(ClientIdType client);

    // For a list of updated properties, returns a map that maps clients subscribing to
    // the updated properties to a list of updated values. This would only return on-change property
    // clients that should be informed for the given updated values.
    std::unordered_map<
            CallbackType,
            std::vector<const aidl::android::hardware::automotive::vehicle::VehiclePropValue*>>
    getSubscribedClients(
            const std::vector<aidl::android::hardware::automotive::vehicle::VehiclePropValue>&
                    updatedValues);

    // For a list of set property error events, returns a map that maps clients subscribing to the
    // properties to a list of errors for each client.
    std::unordered_map<CallbackType,
                       std::vector<aidl::android::hardware::automotive::vehicle::VehiclePropError>>
    getSubscribedClientsForErrorEvents(const std::vector<SetValueErrorEvent>& errorEvents);

    // Checks whether the sample rate is valid.
    static bool checkSampleRateHz(float sampleRateHz);

  private:
    // Friend class for testing.
    friend class DefaultVehicleHalTest;

    IVehicleHardware* mVehicleHardware;

    mutable std::mutex mLock;
    std::unordered_map<PropIdAreaId, std::unordered_map<ClientIdType, CallbackType>,
                       PropIdAreaIdHash>
            mClientsByPropIdArea GUARDED_BY(mLock);
    std::unordered_map<ClientIdType, std::unordered_set<PropIdAreaId, PropIdAreaIdHash>>
            mSubscribedPropsByClient GUARDED_BY(mLock);
    std::unordered_map<PropIdAreaId, ContSubConfigs, PropIdAreaIdHash> mContSubConfigsByPropIdArea
            GUARDED_BY(mLock);

    VhalResult<void> addContinuousSubscriberLocked(const ClientIdType& clientId,
                                                   const PropIdAreaId& propIdAreaId,
                                                   float sampleRateHz) REQUIRES(mLock);
    VhalResult<void> removeContinuousSubscriberLocked(const ClientIdType& clientId,
                                                      const PropIdAreaId& propIdAreaId)
            REQUIRES(mLock);

    VhalResult<void> updateContSubConfigs(const PropIdAreaId& PropIdAreaId,
                                          const ContSubConfigs& newConfig) REQUIRES(mLock);

    // Checks whether the manager is empty. For testing purpose.
    bool isEmpty();

    // Get the interval in nanoseconds accroding to sample rate.
    static android::base::Result<int64_t> getIntervalNanos(float sampleRateHz);
};

}  // namespace vehicle
}  // namespace automotive
}  // namespace hardware
}  // namespace android

#endif  // android_hardware_automotive_vehicle_aidl_impl_vhal_include_SubscriptionManager_H_
