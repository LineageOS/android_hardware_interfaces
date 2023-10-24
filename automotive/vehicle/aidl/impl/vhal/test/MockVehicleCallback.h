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

#ifndef android_hardware_automotive_vehicle_aidl_impl_vhal_test_MockVehicleCallback_H_
#define android_hardware_automotive_vehicle_aidl_impl_vhal_test_MockVehicleCallback_H_

#include <VehicleHalTypes.h>

#include <aidl/android/hardware/automotive/vehicle/BnVehicleCallback.h>
#include <android-base/thread_annotations.h>

#include <condition_variable>
#include <list>
#include <mutex>
#include <optional>

namespace android {
namespace hardware {
namespace automotive {
namespace vehicle {

template <class T>
std::optional<T> pop(std::list<T>& items) {
    if (items.size() > 0) {
        auto item = std::move(items.front());
        items.pop_front();
        return item;
    }
    return std::nullopt;
}

// MockVehicleCallback is a mock VehicleCallback implementation that simply stores the results.
class MockVehicleCallback final
    : public aidl::android::hardware::automotive::vehicle::BnVehicleCallback {
  public:
    ndk::ScopedAStatus onGetValues(
            const aidl::android::hardware::automotive::vehicle::GetValueResults& results) override;
    ndk::ScopedAStatus onSetValues(
            const aidl::android::hardware::automotive::vehicle::SetValueResults& results) override;
    ndk::ScopedAStatus onPropertyEvent(
            const aidl::android::hardware::automotive::vehicle::VehiclePropValues&,
            int32_t) override;
    ndk::ScopedAStatus onPropertySetError(
            const aidl::android::hardware::automotive::vehicle::VehiclePropErrors&) override;

    // Test functions
    std::optional<aidl::android::hardware::automotive::vehicle::GetValueResults>
    nextGetValueResults();
    std::optional<aidl::android::hardware::automotive::vehicle::SetValueResults>
    nextSetValueResults();
    std::optional<aidl::android::hardware::automotive::vehicle::VehiclePropValues>
    nextOnPropertyEventResults();
    size_t countOnPropertySetErrorResults();
    std::optional<aidl::android::hardware::automotive::vehicle::VehiclePropErrors>
    nextOnPropertySetErrorResults();
    size_t countOnPropertyEventResults();
    bool waitForSetValueResults(size_t size, size_t timeoutInNano);
    bool waitForGetValueResults(size_t size, size_t timeoutInNano);
    bool waitForOnPropertyEventResults(size_t size, size_t timeoutInNano);

  private:
    std::mutex mLock;
    std::condition_variable mCond;
    std::list<aidl::android::hardware::automotive::vehicle::GetValueResults> mGetValueResults
            GUARDED_BY(mLock);
    std::list<aidl::android::hardware::automotive::vehicle::SetValueResults> mSetValueResults
            GUARDED_BY(mLock);
    std::list<aidl::android::hardware::automotive::vehicle::VehiclePropValues>
            mOnPropertyEventResults GUARDED_BY(mLock);
    int32_t mSharedMemoryFileCount GUARDED_BY(mLock);
    std::list<aidl::android::hardware::automotive::vehicle::VehiclePropErrors>
            mOnPropertySetErrorResults GUARDED_BY(mLock);
};

}  // namespace vehicle
}  // namespace automotive
}  // namespace hardware
}  // namespace android

#endif  // android_hardware_automotive_vehicle_aidl_impl_vhal_test_MockVehicleCallback_H_
