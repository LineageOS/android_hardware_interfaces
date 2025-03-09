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

#ifndef android_hardware_automotive_vehicle_aidl_impl_vhal_include_ParcelableUtils_H_
#define android_hardware_automotive_vehicle_aidl_impl_vhal_include_ParcelableUtils_H_

#include <LargeParcelableBase.h>
#include <VehicleHalTypes.h>
#include <VehicleUtils.h>

#include <memory>
#include <vector>

namespace android {
namespace hardware {
namespace automotive {
namespace vehicle {

// Turns the values into a stable large parcelable that could be sent via binder.
// If values is small enough, it would be put into output.payloads, otherwise a shared memory file
// would be created and output.sharedMemoryFd would be filled in.
template <class T1, class T2>
ndk::ScopedAStatus vectorToStableLargeParcelable(std::vector<T1>&& values, T2* output) {
    output->payloads = std::move(values);
    auto result = android::automotive::car_binder_lib::LargeParcelableBase::
            parcelableToStableLargeParcelable(*output);
    if (!result.ok()) {
        return toScopedAStatus(
                result, aidl::android::hardware::automotive::vehicle::StatusCode::INTERNAL_ERROR);
    }
    auto& fd = result.value();
    if (fd != nullptr) {
        // Move the returned ScopedFileDescriptor pointer to ScopedFileDescriptor value in
        // 'sharedMemoryFd' field.
        output->payloads.clear();
        output->sharedMemoryFd = std::move(*fd);
    } else {
        output->sharedMemoryFd = ndk::ScopedFileDescriptor();
        // Do not modify payloads.
    }
    return ndk::ScopedAStatus::ok();
}

template <class T1, class T2>
ndk::ScopedAStatus vectorToStableLargeParcelable(const std::vector<T1>& values, T2* output) {
    // Because 'values' is passed in as const reference, we have to do a copy here.
    std::vector<T1> valuesCopy = values;

    return vectorToStableLargeParcelable(std::move(valuesCopy), output);
}

template <class T>
android::base::expected<
        android::automotive::car_binder_lib::LargeParcelableBase::BorrowedOwnedObject<T>,
        ndk::ScopedAStatus>
fromStableLargeParcelable(const T& largeParcelable) {
    auto result = android::automotive::car_binder_lib::LargeParcelableBase::
            stableLargeParcelableToParcelable(largeParcelable);

    if (!result.ok()) {
        return android::base::unexpected(toScopedAStatus(
                result, aidl::android::hardware::automotive::vehicle::StatusCode::INVALID_ARG,
                "failed to parse large parcelable"));
    }

    return std::move(result.value());
}

}  // namespace vehicle
}  // namespace automotive
}  // namespace hardware
}  // namespace android

#endif  // android_hardware_automotive_vehicle_aidl_impl_vhal_include_ParcelableUtils_H_
