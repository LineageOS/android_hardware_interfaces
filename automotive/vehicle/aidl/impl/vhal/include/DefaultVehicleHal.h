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

#include <IVehicleHardware.h>
#include <LargeParcelableBase.h>
#include <VehicleUtils.h>
#include <aidl/android/hardware/automotive/vehicle/BnVehicle.h>
#include <android/binder_auto_utils.h>

#include <memory>
#include <unordered_map>
#include <vector>

namespace android {
namespace hardware {
namespace automotive {
namespace vehicle {

// private namespace
namespace defaultvehiclehal_impl {

constexpr int INVALID_MEMORY_FD = -1;

template <class T>
::ndk::ScopedAStatus toScopedAStatus(
        const ::android::base::Result<T>& result,
        ::aidl::android::hardware::automotive::vehicle::StatusCode status) {
    if (result.ok()) {
        return ::ndk::ScopedAStatus::ok();
    }
    return ::ndk::ScopedAStatus::fromServiceSpecificErrorWithMessage(toInt(status),
                                                                     getErrorMsg(result).c_str());
}

template <class T>
::ndk::ScopedAStatus toScopedAStatus(const ::android::base::Result<T>& result) {
    return toScopedAStatus(result, getErrorCode(result));
}

template <class T1, class T2>
::ndk::ScopedAStatus vectorToStableLargeParcelable(std::vector<T1>& values, T2* output) {
    auto result = ::android::automotive::car_binder_lib::LargeParcelableBase::
            parcelableVectorToStableLargeParcelable(values);
    if (!result.ok()) {
        return toScopedAStatus(
                result, ::aidl::android::hardware::automotive::vehicle::StatusCode::INTERNAL_ERROR);
    }
    auto& fd = result.value();
    if (fd == nullptr) {
        output->payloads = values;
    } else {
        // Move the returned ScopedFileDescriptor pointer to ScopedFileDescriptor value in
        // 'sharedMemoryFd' field.
        output->sharedMemoryFd.set(fd->get());
        *(fd->getR()) = INVALID_MEMORY_FD;
    }
    return ::ndk::ScopedAStatus::ok();
}

}  // namespace defaultvehiclehal_impl

class DefaultVehicleHal final : public ::aidl::android::hardware::automotive::vehicle::BnVehicle {
  public:
    explicit DefaultVehicleHal(std::unique_ptr<IVehicleHardware> hardware);

    ::ndk::ScopedAStatus getAllPropConfigs(
            ::aidl::android::hardware::automotive::vehicle::VehiclePropConfigs* returnConfigs)
            override;
    ::ndk::ScopedAStatus getValues(
            const std::shared_ptr<::aidl::android::hardware::automotive::vehicle::IVehicleCallback>&
                    callback,
            const ::aidl::android::hardware::automotive::vehicle::GetValueRequests& requests)
            override;
    ::ndk::ScopedAStatus setValues(
            const std::shared_ptr<::aidl::android::hardware::automotive::vehicle::IVehicleCallback>&
                    callback,
            const ::aidl::android::hardware::automotive::vehicle::SetValueRequests& requests)
            override;
    ::ndk::ScopedAStatus getPropConfigs(
            const std::vector<int32_t>& props,
            ::aidl::android::hardware::automotive::vehicle::VehiclePropConfigs* returnConfigs)
            override;
    ::ndk::ScopedAStatus subscribe(
            const std::shared_ptr<::aidl::android::hardware::automotive::vehicle::IVehicleCallback>&
                    callback,
            const std::vector<::aidl::android::hardware::automotive::vehicle::SubscribeOptions>&
                    options,
            int32_t maxSharedMemoryFileCount) override;
    ::ndk::ScopedAStatus unsubscribe(
            const std::shared_ptr<::aidl::android::hardware::automotive::vehicle::IVehicleCallback>&
                    callback,
            const std::vector<int32_t>& propIds) override;
    ::ndk::ScopedAStatus returnSharedMemory(
            const std::shared_ptr<::aidl::android::hardware::automotive::vehicle::IVehicleCallback>&
                    callback,
            int64_t sharedMemoryId) override;

    IVehicleHardware* getHardware();

  private:
    const std::unique_ptr<IVehicleHardware> mVehicleHardware;
    std::unordered_map<int32_t, ::aidl::android::hardware::automotive::vehicle::VehiclePropConfig>
            mConfigsByPropId;
    std::unique_ptr<::ndk::ScopedFileDescriptor> mConfigFile;
};

}  // namespace vehicle
}  // namespace automotive
}  // namespace hardware
}  // namespace android

#endif  // android_hardware_automotive_vehicle_aidl_impl_vhal_include_DefaultVehicleHal_H_
