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

template <class T1, class T2>
::ndk::ScopedAStatus vectorToStableLargeParcelable(std::vector<T1>&& values, T2* output) {
    auto result = ::android::automotive::car_binder_lib::LargeParcelableBase::
            parcelableVectorToStableLargeParcelable(values);
    if (!result.ok()) {
        return toScopedAStatus(
                result, ::aidl::android::hardware::automotive::vehicle::StatusCode::INTERNAL_ERROR);
    }
    auto& fd = result.value();
    if (fd == nullptr) {
        // If we no longer needs values, move it inside the payloads to avoid copying.
        output->payloads = std::move(values);
    } else {
        // Move the returned ScopedFileDescriptor pointer to ScopedFileDescriptor value in
        // 'sharedMemoryFd' field.
        output->sharedMemoryFd = std::move(*fd);
    }
    return ::ndk::ScopedAStatus::ok();
}

template <class T1, class T2>
::ndk::ScopedAStatus vectorToStableLargeParcelable(const std::vector<T1>& values, T2* output) {
    // Because 'values' is passed in as const reference, we have to do a copy here.
    std::vector<T1> valuesCopy = values;

    return vectorToStableLargeParcelable(std::move(valuesCopy), output);
}

template <class T1, class T2>
::android::base::expected<std::vector<T1>, ::ndk::ScopedAStatus> stableLargeParcelableToVector(
        const T2& largeParcelable) {
    ::android::base::Result<std::optional<std::vector<T1>>> result =
            ::android::automotive::car_binder_lib::LargeParcelableBase::
                    stableLargeParcelableToParcelableVector<T1>(largeParcelable.sharedMemoryFd);

    if (!result.ok()) {
        return ::android::base::unexpected(toScopedAStatus(
                result, ::aidl::android::hardware::automotive::vehicle::StatusCode::INVALID_ARG,
                "failed to parse large parcelable"));
    }

    if (!result.value().has_value()) {
        return ::android::base::unexpected(
                ::ndk::ScopedAStatus::fromServiceSpecificErrorWithMessage(
                        toInt(::aidl::android::hardware::automotive::vehicle::StatusCode::
                                      INVALID_ARG),
                        "empty request"));
    }

    return std::move(result.value().value());
}

}  // namespace vehicle
}  // namespace automotive
}  // namespace hardware
}  // namespace android

#endif  // android_hardware_automotive_vehicle_aidl_impl_vhal_include_ParcelableUtils_H_
