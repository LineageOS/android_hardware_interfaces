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

#define LOG_TAG "thermal_service_example"

#include "Thermal.h"

#include <android-base/logging.h>

namespace aidl::android::hardware::thermal::impl::example {

using ndk::ScopedAStatus;

namespace {

bool interfacesEqual(const std::shared_ptr<::ndk::ICInterface>& left,
                     const std::shared_ptr<::ndk::ICInterface>& right) {
    if (left == nullptr || right == nullptr || !left->isRemote() || !right->isRemote()) {
        return left == right;
    }
    return left->asBinder() == right->asBinder();
}

}  // namespace

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
        return ndk::ScopedAStatus::fromExceptionCodeWithMessage(EX_ILLEGAL_ARGUMENT,
                                                                "Invalid nullptr callback");
    }
    {
        std::lock_guard<std::mutex> _lock(thermal_callback_mutex_);
        if (std::any_of(thermal_callbacks_.begin(), thermal_callbacks_.end(),
                        [&](const std::shared_ptr<IThermalChangedCallback>& c) {
                            return interfacesEqual(c, in_callback);
                        })) {
            return ndk::ScopedAStatus::fromExceptionCodeWithMessage(EX_ILLEGAL_ARGUMENT,
                                                                    "Callback already registered");
        }
        thermal_callbacks_.push_back(in_callback);
    }
    return ScopedAStatus::ok();
}

ScopedAStatus Thermal::registerThermalChangedCallbackWithType(
        const std::shared_ptr<IThermalChangedCallback>& in_callback, TemperatureType in_type) {
    LOG(VERBOSE) << __func__ << " IThermalChangedCallback: " << in_callback
                 << ", TemperatureType: " << static_cast<int32_t>(in_type);
    if (in_callback == nullptr) {
        return ndk::ScopedAStatus::fromExceptionCodeWithMessage(EX_ILLEGAL_ARGUMENT,
                                                                "Invalid nullptr callback");
    }
    {
        std::lock_guard<std::mutex> _lock(thermal_callback_mutex_);
        if (std::any_of(thermal_callbacks_.begin(), thermal_callbacks_.end(),
                        [&](const std::shared_ptr<IThermalChangedCallback>& c) {
                            return interfacesEqual(c, in_callback);
                        })) {
            return ndk::ScopedAStatus::fromExceptionCodeWithMessage(EX_ILLEGAL_ARGUMENT,
                                                                    "Callback already registered");
        }
        thermal_callbacks_.push_back(in_callback);
    }
    return ScopedAStatus::ok();
}

ScopedAStatus Thermal::unregisterThermalChangedCallback(
        const std::shared_ptr<IThermalChangedCallback>& in_callback) {
    LOG(VERBOSE) << __func__ << " IThermalChangedCallback: " << in_callback;
    if (in_callback == nullptr) {
        return ndk::ScopedAStatus::fromExceptionCodeWithMessage(EX_ILLEGAL_ARGUMENT,
                                                                "Invalid nullptr callback");
    }
    {
        std::lock_guard<std::mutex> _lock(thermal_callback_mutex_);
        bool removed = false;
        thermal_callbacks_.erase(
                std::remove_if(thermal_callbacks_.begin(), thermal_callbacks_.end(),
                               [&](const std::shared_ptr<IThermalChangedCallback>& c) {
                                   if (interfacesEqual(c, in_callback)) {
                                       removed = true;
                                       return true;
                                   }
                                   return false;
                               }),
                thermal_callbacks_.end());
        if (!removed) {
            return ndk::ScopedAStatus::fromExceptionCodeWithMessage(EX_ILLEGAL_ARGUMENT,
                                                                    "Callback wasn't registered");
        }
    }
    return ScopedAStatus::ok();
}

ScopedAStatus Thermal::registerCoolingDeviceChangedCallbackWithType(
        const std::shared_ptr<ICoolingDeviceChangedCallback>& in_callback, CoolingType in_type) {
    LOG(VERBOSE) << __func__ << " ICoolingDeviceChangedCallback: " << in_callback
                 << ", CoolingType: " << static_cast<int32_t>(in_type);
    if (in_callback == nullptr) {
        return ndk::ScopedAStatus::fromExceptionCodeWithMessage(EX_ILLEGAL_ARGUMENT,
                                                                "Invalid nullptr callback");
    }
    {
        std::lock_guard<std::mutex> _lock(cdev_callback_mutex_);
        if (std::any_of(cdev_callbacks_.begin(), cdev_callbacks_.end(),
                        [&](const std::shared_ptr<ICoolingDeviceChangedCallback>& c) {
                            return interfacesEqual(c, in_callback);
                        })) {
            return ndk::ScopedAStatus::fromExceptionCodeWithMessage(EX_ILLEGAL_ARGUMENT,
                                                                    "Callback already registered");
        }
        cdev_callbacks_.push_back(in_callback);
    }
    return ScopedAStatus::ok();
}

ScopedAStatus Thermal::unregisterCoolingDeviceChangedCallback(
        const std::shared_ptr<ICoolingDeviceChangedCallback>& in_callback) {
    LOG(VERBOSE) << __func__ << " ICoolingDeviceChangedCallback: " << in_callback;
    if (in_callback == nullptr) {
        return ndk::ScopedAStatus::fromExceptionCodeWithMessage(EX_ILLEGAL_ARGUMENT,
                                                                "Invalid nullptr callback");
    }
    {
        std::lock_guard<std::mutex> _lock(cdev_callback_mutex_);
        bool removed = false;
        cdev_callbacks_.erase(
                std::remove_if(cdev_callbacks_.begin(), cdev_callbacks_.end(),
                               [&](const std::shared_ptr<ICoolingDeviceChangedCallback>& c) {
                                   if (interfacesEqual(c, in_callback)) {
                                       removed = true;
                                       return true;
                                   }
                                   return false;
                               }),
                cdev_callbacks_.end());
        if (!removed) {
            return ndk::ScopedAStatus::fromExceptionCodeWithMessage(EX_ILLEGAL_ARGUMENT,
                                                                    "Callback wasn't registered");
        }
    }
    return ScopedAStatus::ok();
}
}  // namespace aidl::android::hardware::thermal::impl::example
