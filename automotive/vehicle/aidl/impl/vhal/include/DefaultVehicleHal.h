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

#ifndef android_hardware_automotive_vehicle_aidl_impl_vhal_include_DefaultVehicleHal_H_
#define android_hardware_automotive_vehicle_aidl_impl_vhal_include_DefaultVehicleHal_H_

#include "ConnectedClient.h"
#include "ParcelableUtils.h"
#include "PendingRequestPool.h"

#include <IVehicleHardware.h>
#include <VehicleUtils.h>
#include <aidl/android/hardware/automotive/vehicle/BnVehicle.h>
#include <android-base/expected.h>
#include <android-base/thread_annotations.h>
#include <android/binder_auto_utils.h>

#include <memory>
#include <mutex>
#include <unordered_map>
#include <vector>

namespace android {
namespace hardware {
namespace automotive {
namespace vehicle {

// private namespace
namespace defaultvehiclehal_impl {

constexpr int INVALID_MEMORY_FD = -1;

}  // namespace defaultvehiclehal_impl

class DefaultVehicleHal final : public ::aidl::android::hardware::automotive::vehicle::BnVehicle {
  public:
    using CallbackType =
            std::shared_ptr<::aidl::android::hardware::automotive::vehicle::IVehicleCallback>;

    explicit DefaultVehicleHal(std::unique_ptr<IVehicleHardware> hardware);

    ::ndk::ScopedAStatus getAllPropConfigs(
            ::aidl::android::hardware::automotive::vehicle::VehiclePropConfigs* returnConfigs)
            override;
    ::ndk::ScopedAStatus getValues(
            const CallbackType& callback,
            const ::aidl::android::hardware::automotive::vehicle::GetValueRequests& requests)
            override;
    ::ndk::ScopedAStatus setValues(
            const CallbackType& callback,
            const ::aidl::android::hardware::automotive::vehicle::SetValueRequests& requests)
            override;
    ::ndk::ScopedAStatus getPropConfigs(
            const std::vector<int32_t>& props,
            ::aidl::android::hardware::automotive::vehicle::VehiclePropConfigs* returnConfigs)
            override;
    ::ndk::ScopedAStatus subscribe(
            const CallbackType& callback,
            const std::vector<::aidl::android::hardware::automotive::vehicle::SubscribeOptions>&
                    options,
            int32_t maxSharedMemoryFileCount) override;
    ::ndk::ScopedAStatus unsubscribe(const CallbackType& callback,
                                     const std::vector<int32_t>& propIds) override;
    ::ndk::ScopedAStatus returnSharedMemory(const CallbackType& callback,
                                            int64_t sharedMemoryId) override;

    IVehicleHardware* getHardware();

  private:
    // friend class for unit testing.
    friend class DefaultVehicleHalTest;

    using GetValuesClient =
            GetSetValuesClient<::aidl::android::hardware::automotive::vehicle::GetValueResult,
                               ::aidl::android::hardware::automotive::vehicle::GetValueResults>;
    using SetValuesClient =
            GetSetValuesClient<::aidl::android::hardware::automotive::vehicle::SetValueResult,
                               ::aidl::android::hardware::automotive::vehicle::SetValueResults>;

    // The default timeout of get or set value requests is 30s.
    // TODO(b/214605968): define TIMEOUT_IN_NANO in IVehicle and allow getValues/setValues/subscribe
    // to specify custom timeouts.
    static constexpr int64_t TIMEOUT_IN_NANO = 30'000'000'000;
    const std::unique_ptr<IVehicleHardware> mVehicleHardware;

    // mConfigsByPropId and mConfigFile are only modified during initialization, so no need to
    // lock guard them.
    std::unordered_map<int32_t, ::aidl::android::hardware::automotive::vehicle::VehiclePropConfig>
            mConfigsByPropId;
    // Only modified in constructor, so thread-safe.
    std::unique_ptr<::ndk::ScopedFileDescriptor> mConfigFile;
    // PendingRequestPool is thread-safe.
    std::shared_ptr<PendingRequestPool> mPendingRequestPool;

    std::mutex mLock;
    std::unordered_map<CallbackType, std::shared_ptr<GetValuesClient>> mGetValuesClients
            GUARDED_BY(mLock);
    std::unordered_map<CallbackType, std::shared_ptr<SetValuesClient>> mSetValuesClients
            GUARDED_BY(mLock);

    template <class T>
    std::shared_ptr<T> getOrCreateClient(
            std::unordered_map<CallbackType, std::shared_ptr<T>>* clients,
            const CallbackType& callback) REQUIRES(mLock);

    ::android::base::Result<void> checkProperty(
            const ::aidl::android::hardware::automotive::vehicle::VehiclePropValue& propValue);

    ::android::base::Result<std::vector<int64_t>> checkDuplicateRequests(
            const std::vector<::aidl::android::hardware::automotive::vehicle::GetValueRequest>&
                    requests);

    ::android::base::Result<std::vector<int64_t>> checkDuplicateRequests(
            const std::vector<::aidl::android::hardware::automotive::vehicle::SetValueRequest>&
                    requests);

    // Test-only
    // Set the default timeout for pending requests.
    void setTimeout(int64_t timeoutInNano);
};

}  // namespace vehicle
}  // namespace automotive
}  // namespace hardware
}  // namespace android

#endif  // android_hardware_automotive_vehicle_aidl_impl_vhal_include_DefaultVehicleHal_H_
