/*
 * Copyright (C) 2023 The Android Open Source Project
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

#include <aidl/android/hardware/thermal/BnThermalChangedCallback.h>
#include <aidl/android/hardware/thermal/IThermal.h>
#include <android/hardware/thermal/2.0/IThermal.h>
#include <android/hardware/thermal/2.0/IThermalChangedCallback.h>
#include <android/hardware/thermal/2.0/types.h>
#include <hidl/Status.h>

#include <utility>

namespace aidl {
namespace android {
namespace hardware {
namespace thermal {

using ::android::sp;
using ::android::hardware::hidl_array;
using ::android::hardware::hidl_handle;
using ::android::hardware::hidl_string;
using ::android::hardware::hidl_vec;
using ::android::hardware::Return;

using IThermal_Aidl = ::aidl::android::hardware::thermal::IThermal;
using ::android::hardware::thermal::V1_0::CpuUsage;
using CoolingType_2_0 = ::android::hardware::thermal::V2_0::CoolingType;
using CoolingDevice_1_0 = ::android::hardware::thermal::V1_0::CoolingDevice;
using CoolingDevice_2_0 = ::android::hardware::thermal::V2_0::CoolingDevice;
using IThermal_2_0 = ::android::hardware::thermal::V2_0::IThermal;
using IThermalChangedCallback_2_0 = ::android::hardware::thermal::V2_0::IThermalChangedCallback;
using Temperature_1_0 = ::android::hardware::thermal::V1_0::Temperature;
using Temperature_2_0 = ::android::hardware::thermal::V2_0::Temperature;
using TemperatureType_2_0 = ::android::hardware::thermal::V2_0::TemperatureType;

using ::android::hardware::thermal::V1_0::ThermalStatus;
using ::android::hardware::thermal::V1_0::ThermalStatusCode;

using TemperatureThreshold_2_0 = ::android::hardware::thermal::V2_0::TemperatureThreshold;
using ThrottlingSeverity_2_0 = ::android::hardware::thermal::V2_0::ThrottlingSeverity;

// This wrapper converts all Thermal HIDL 2.0 calls to AIDL calls and converts AIDL response to
// HIDL 2.0 response.
//
// For Thermal HIDL 1.0 calls, it returns unsupported error.
class ThermalHidlWrapper : public IThermal_2_0 {
  public:
    explicit ThermalHidlWrapper(::std::shared_ptr<IThermal_Aidl> thermal_service)
        : thermal_service_(std::move(thermal_service)) {}

    // Methods from ::android::hardware::thermal::V1_0::IThermal follow.
    Return<void> getTemperatures(getTemperatures_cb _hidl_cb) override;
    Return<void> getCpuUsages(getCpuUsages_cb _hidl_cb) override;
    Return<void> getCoolingDevices(getCoolingDevices_cb _hidl_cb) override;

    // Methods from ::android::hardware::thermal::V2_0::IThermal follow.
    Return<void> getCurrentTemperatures(bool filterType, TemperatureType_2_0 type,
                                        getCurrentTemperatures_cb _hidl_cb) override;
    Return<void> getTemperatureThresholds(bool filterType, TemperatureType_2_0 type,
                                          getTemperatureThresholds_cb _hidl_cb) override;
    Return<void> registerThermalChangedCallback(
            const sp<IThermalChangedCallback_2_0>& callback, bool filterType,
            TemperatureType_2_0 type, registerThermalChangedCallback_cb _hidl_cb) override;
    Return<void> unregisterThermalChangedCallback(
            const sp<IThermalChangedCallback_2_0>& callback,
            unregisterThermalChangedCallback_cb _hidl_cb) override;
    Return<void> getCurrentCoolingDevices(bool filterType, CoolingType_2_0 type,
                                          getCurrentCoolingDevices_cb _hidl_cb) override;

    // Methods from ::android::hidl::base::V1_0::IBase follow.
    Return<void> debug(const hidl_handle& handle, const hidl_vec<hidl_string>& args) override;

  private:
    class IThermalChangedCallbackWrapper : public BnThermalChangedCallback {
      public:
        explicit IThermalChangedCallbackWrapper(const sp<IThermalChangedCallback_2_0>& callback_2_0)
            : callback_2_0_(callback_2_0) {}
        ::ndk::ScopedAStatus notifyThrottling(const Temperature& temperature) override;
        sp<IThermalChangedCallback_2_0> callback_2_0_;
    };

    // Reference to thermal service.
    ::std::shared_ptr<IThermal_Aidl> thermal_service_;
    // Mutex lock for read/write on callback wrappers.
    std::mutex callback_wrappers_mutex_;
    // All thermal changed callback wrappers registered.
    ::std::vector<std::shared_ptr<IThermalChangedCallbackWrapper>> callback_wrappers_;
};

}  // namespace thermal
}  // namespace hardware
}  // namespace android
}  // namespace aidl
