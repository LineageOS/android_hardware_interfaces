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

#ifndef android_hardware_automotive_vehicle_aidl_impl_vhal_include_ConnectedClient_H_
#define android_hardware_automotive_vehicle_aidl_impl_vhal_include_ConnectedClient_H_

#include "PendingRequestPool.h"

#include <IVehicleHardware.h>
#include <VehicleHalTypes.h>
#include <VehicleUtils.h>

#include <aidl/android/hardware/automotive/vehicle/IVehicleCallback.h>
#include <android-base/result.h>

#include <memory>
#include <unordered_set>
#include <vector>

namespace android {
namespace hardware {
namespace automotive {
namespace vehicle {

// A class to represent a binder client with a callback interface. Each callback function, e.g.
// GetValues or SetValues for a specific binder client is a separate {@code ConnectedClient}.
// For one {@code ConnectedClient}, we use one pending request pool to manage all pending requests,
// so the request IDs must be unique for one client. We also manage a set of callback functions
// for one client, e.g. timeoutCallback which could be passed to hardware.
// This class is thread-safe.
class ConnectedClient {
  public:
    using CallbackType =
            std::shared_ptr<aidl::android::hardware::automotive::vehicle::IVehicleCallback>;

    ConnectedClient(std::shared_ptr<PendingRequestPool> requestPool, CallbackType callback);

    virtual ~ConnectedClient() = default;

    // Gets the unique ID for this client.
    const void* id();

    // Adds client requests. The requests would be registered as pending requests until
    // {@code tryFinishRequests} is called for them.
    // Returns {@code INVALID_ARG} error if any of the requestIds are duplicate with one of the
    // pending request IDs or {@code TRY_AGAIN} error if the pending request pool is full and could
    // no longer add requests.
    VhalResult<void> addRequests(const std::unordered_set<int64_t>& requestIds);

    // Marks the requests as finished. Returns a list of request IDs that was pending and has been
    // finished. It must be a set of the requested request IDs.
    std::unordered_set<int64_t> tryFinishRequests(const std::unordered_set<int64_t>& requestIds);

  protected:
    // Gets the callback to be called when the request for this client has timeout.
    virtual std::shared_ptr<const PendingRequestPool::TimeoutCallbackFunc> getTimeoutCallback() = 0;

    const std::shared_ptr<PendingRequestPool> mRequestPool;
    const CallbackType mCallback;
};

// A class to represent a client that calls {@code IVehicle.setValues} or {@code
// IVehicle.getValues}.
template <class ResultType, class ResultsType>
class GetSetValuesClient final : public ConnectedClient {
  public:
    GetSetValuesClient(std::shared_ptr<PendingRequestPool> requestPool, CallbackType callback);

    // Sends the results to this client.
    void sendResults(std::vector<ResultType>&& results);

    // Sends each result separately to this client. Each result would be sent through one callback
    // invocation.
    void sendResultsSeparately(const std::vector<ResultType>& results);

    // Gets the callback to be called when the request for this client has finished.
    std::shared_ptr<const std::function<void(std::vector<ResultType>)>> getResultCallback();

  protected:
    // Gets the callback to be called when the request for this client has timeout.
    std::shared_ptr<const PendingRequestPool::TimeoutCallbackFunc> getTimeoutCallback() override;

  private:
    // The following members are only initialized during construction.
    std::shared_ptr<const PendingRequestPool::TimeoutCallbackFunc> mTimeoutCallback;
    std::shared_ptr<const std::function<void(std::vector<ResultType>)>> mResultCallback;
};

// A class to represent a client that calls {@code IVehicle.subscribe}.
class SubscriptionClient final : public ConnectedClient {
  public:
    SubscriptionClient(std::shared_ptr<PendingRequestPool> requestPool, CallbackType callback);

    // Gets the callback to be called when the request for this client has finished.
    std::shared_ptr<const IVehicleHardware::GetValuesCallback> getResultCallback();

    // Marshals the updated values into largeParcelable and sends it through {@code onPropertyEvent}
    // callback.
    static void sendUpdatedValues(
            CallbackType callback,
            std::vector<aidl::android::hardware::automotive::vehicle::VehiclePropValue>&&
                    updatedValues);
    // Marshals the set property error events into largeParcelable and sends it through
    // {@code onPropertySetError} callback.
    static void sendPropertySetErrors(
            CallbackType callback,
            std::vector<aidl::android::hardware::automotive::vehicle::VehiclePropError>&&
                    vehiclePropErrors);

  protected:
    // Gets the callback to be called when the request for this client has timeout.
    std::shared_ptr<const PendingRequestPool::TimeoutCallbackFunc> getTimeoutCallback() override;

  private:
    // The following members are only initialized during construction.
    std::shared_ptr<const PendingRequestPool::TimeoutCallbackFunc> mTimeoutCallback;
    std::shared_ptr<const IVehicleHardware::GetValuesCallback> mResultCallback;
    std::shared_ptr<const IVehicleHardware::PropertyChangeCallback> mPropertyChangeCallback;

    static void onGetValueResults(
            const void* clientId, CallbackType callback,
            std::shared_ptr<PendingRequestPool> requestPool,
            std::vector<aidl::android::hardware::automotive::vehicle::GetValueResult> results);
};

}  // namespace vehicle
}  // namespace automotive
}  // namespace hardware
}  // namespace android

#endif  // android_hardware_automotive_vehicle_aidl_impl_vhal_include_ConnectedClient_H_
