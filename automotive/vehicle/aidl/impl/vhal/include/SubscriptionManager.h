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

#include "RecurrentTimer.h"

#include <VehicleHalTypes.h>

#include <aidl/android/hardware/automotive/vehicle/IVehicleCallback.h>
#include <android-base/result.h>
#include <android-base/thread_annotations.h>

#include <mutex>
#include <unordered_map>
#include <unordered_set>
#include <vector>

namespace android {
namespace hardware {
namespace automotive {
namespace vehicle {

// A thread-safe subscription manager that manages all VHAL subscriptions.
class SubscriptionManager final {
  public:
    using CallbackType =
            std::shared_ptr<::aidl::android::hardware::automotive::vehicle::IVehicleCallback>;
    using GetValueFunc = std::function<void(
            const CallbackType& callback,
            const ::aidl::android::hardware::automotive::vehicle::VehiclePropValue& value)>;

    explicit SubscriptionManager(GetValueFunc&& action);
    ~SubscriptionManager();

    // Subscribes to properties according to {@code SubscribeOptions}. Note that all option must
    // contain non-empty areaIds field, which contains all area IDs to subscribe. As a result,
    // the options here is different from the options passed from VHAL client.
    // Returns error if any of the subscribe options is not valid. If error is returned, no
    // properties would be subscribed.
    // Returns ok if all the options are parsed correctly and all the properties are subscribed.
    ::android::base::Result<void> subscribe(
            const CallbackType& callback,
            const std::vector<::aidl::android::hardware::automotive::vehicle::SubscribeOptions>&
                    options,
            bool isContinuousProperty);

    // Unsubscribes from the properties for the callback.
    // Returns error if the callback was not subscribed before or one of the given property was not
    // subscribed. If error is returned, no property would be unsubscribed.
    // Returns ok if all the requested properties for the callback are unsubscribed.
    ::android::base::Result<void> unsubscribe(const CallbackType& callback,
                                              const std::vector<int32_t>& propIds);

    // Unsubscribes to all the properties for the callback.
    // Returns error if the callback was not subscribed before. If error is returned, no property
    // would be unsubscribed.
    // Returns ok if all the properties for the callback are unsubscribed.
    ::android::base::Result<void> unsubscribe(const CallbackType& callback);

    // For a list of updated properties, returns a map that maps clients subscribing to
    // the updated properties to a list of updated values. This would only return on-change property
    // clients that should be informed for the given updated values.
    std::unordered_map<
            std::shared_ptr<::aidl::android::hardware::automotive::vehicle::IVehicleCallback>,
            std::vector<const ::aidl::android::hardware::automotive::vehicle::VehiclePropValue*>>
    getSubscribedClients(
            const std::vector<::aidl::android::hardware::automotive::vehicle::VehiclePropValue>&
                    updatedValues);

    // Checks whether the sample rate is valid.
    static bool checkSampleRate(float sampleRate);

  private:
    struct PropIdAreaId {
        int32_t propId;
        int32_t areaId;

        bool operator==(const PropIdAreaId& other) const;
    };

    struct PropIdAreaIdHash {
        size_t operator()(const PropIdAreaId& propIdAreaId) const;
    };

    // A class to represent a registered subscription.
    class Subscription {
      public:
        Subscription() = default;

        Subscription(const Subscription&) = delete;

        virtual ~Subscription() = default;

        virtual bool isOnChange();
    };

    // A subscription for OnContinuous property. The registered action would be called recurrently
    // until this class is destructed.
    class RecurrentSubscription final : public Subscription {
      public:
        explicit RecurrentSubscription(std::shared_ptr<RecurrentTimer> timer,
                                       std::function<void()>&& action, int64_t interval);
        ~RecurrentSubscription();

        bool isOnChange() override;

      private:
        std::shared_ptr<std::function<void()>> mAction;
        std::shared_ptr<RecurrentTimer> mTimer;
    };

    // A subscription for OnChange property.
    class OnChangeSubscription final : public Subscription {
      public:
        bool isOnChange() override;
    };

    mutable std::mutex mLock;
    std::unordered_map<PropIdAreaId, std::unordered_set<CallbackType>, PropIdAreaIdHash>
            mClientsByPropIdArea GUARDED_BY(mLock);
    std::unordered_map<CallbackType, std::unordered_map<PropIdAreaId, std::unique_ptr<Subscription>,
                                                        PropIdAreaIdHash>>
            mSubscriptionsByClient GUARDED_BY(mLock);
    // RecurrentTimer is thread-safe.
    std::shared_ptr<RecurrentTimer> mTimer;
    const GetValueFunc mGetValue;

    static ::android::base::Result<int64_t> getInterval(float sampleRate);
};

}  // namespace vehicle
}  // namespace automotive
}  // namespace hardware
}  // namespace android

#endif  // android_hardware_automotive_vehicle_aidl_impl_vhal_include_SubscriptionManager_H_
