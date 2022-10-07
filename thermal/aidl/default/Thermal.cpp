/*
 * Copyright (C) 2022 The Android Open Source Project
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

#include "Thermal.h"

#include <android-base/logging.h>

namespace aidl::android::hardware::thermal::impl::example {

using ndk::ScopedAStatus;

ScopedAStatus Thermal::getCoolingDevices(std::vector<CoolingDevice>* /* out_devices */) {
    LOG(VERBOSE) << __func__;
    return ScopedAStatus::ok();
}

ScopedAStatus Thermal::getCoolingDevicesWithType(CoolingType in_type,
                                                 std::vector<CoolingDevice>* /* out_devices */) {
    LOG(VERBOSE) << __func__ << " CoolingType: " << static_cast<int32_t>(in_type);
    return ScopedAStatus::ok();
}

ScopedAStatus Thermal::getTemperatures(std::vector<Temperature>* /* out_temperatures */) {
    LOG(VERBOSE) << __func__;
    return ScopedAStatus::ok();
}

ScopedAStatus Thermal::getTemperaturesWithType(TemperatureType in_type,
                                               std::vector<Temperature>* /* out_temperatures */) {
    LOG(VERBOSE) << __func__ << " TemperatureType: " << static_cast<int32_t>(in_type);
    return ScopedAStatus::ok();
}

ScopedAStatus Thermal::getTemperatureThresholds(
        std::vector<TemperatureThreshold>* /* out_temperatureThresholds */) {
    LOG(VERBOSE) << __func__;
    return ScopedAStatus::ok();
}

ScopedAStatus Thermal::getTemperatureThresholdsWithType(
        TemperatureType in_type,
        std::vector<TemperatureThreshold>* /* out_temperatureThresholds */) {
    LOG(VERBOSE) << __func__ << " TemperatureType: " << static_cast<int32_t>(in_type);
    return ScopedAStatus::ok();
}

ScopedAStatus Thermal::registerThermalChangedCallback(
        const std::shared_ptr<IThermalChangedCallback>& in_callback) {
    LOG(VERBOSE) << __func__ << " IThermalChangedCallback: " << in_callback;
    if (in_callback == nullptr) {
        return ScopedAStatus::fromStatus(STATUS_BAD_VALUE);
    }
    if (mCallbacks.find(in_callback) != mCallbacks.end()) {
        return ScopedAStatus::fromStatus(STATUS_INVALID_OPERATION);
    }
    mCallbacks.insert(in_callback);
    return ScopedAStatus::ok();
}

ScopedAStatus Thermal::registerThermalChangedCallbackWithType(
        const std::shared_ptr<IThermalChangedCallback>& in_callback, TemperatureType in_type) {
    LOG(VERBOSE) << __func__ << " IThermalChangedCallback: " << in_callback
                 << ", TemperatureType: " << static_cast<int32_t>(in_type);
    if (in_callback == nullptr) {
        return ScopedAStatus::fromStatus(STATUS_BAD_VALUE);
    }
    if (mCallbacks.find(in_callback) != mCallbacks.end()) {
        return ScopedAStatus::fromStatus(STATUS_INVALID_OPERATION);
    }
    mCallbacks.insert(in_callback);
    return ScopedAStatus::ok();
}

ScopedAStatus Thermal::unregisterThermalChangedCallback(
        const std::shared_ptr<IThermalChangedCallback>& in_callback) {
    LOG(VERBOSE) << __func__ << " IThermalChangedCallback: " << in_callback;
    bool found = false;
    if (in_callback == nullptr) {
        return ScopedAStatus::fromStatus(STATUS_BAD_VALUE);
    }
    if (mCallbacks.find(in_callback) == mCallbacks.end()) {
        return ScopedAStatus::fromStatus(STATUS_INVALID_OPERATION);
    }
    mCallbacks.erase(in_callback);
    return ScopedAStatus::ok();
}

}  // namespace aidl::android::hardware::thermal::impl::example
