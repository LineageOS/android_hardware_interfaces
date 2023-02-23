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

#include "include/thermalutils/ThermalHidlWrapper.h"

#include <hidl/HidlTransportSupport.h>

#include <cmath>

namespace aidl {
namespace android {
namespace hardware {
namespace thermal {

using ::android::hardware::Void;

namespace {

template <typename T, typename U>
Return<void> setFailureAndCallback(T _hidl_cb, hidl_vec<U> data, std::string_view debug_msg) {
    ThermalStatus status;
    status.code = ThermalStatusCode::FAILURE;
    status.debugMessage = debug_msg.data();
    _hidl_cb(status, data);
    return Void();
}

template <typename T>
Return<void> setFailureAndCallback(T _hidl_cb, std::string_view debug_msg) {
    ThermalStatus status;
    status.code = ThermalStatusCode::FAILURE;
    status.debugMessage = debug_msg.data();
    _hidl_cb(status);
    return Void();
}

template <typename T, typename U>
Return<void> setInitFailureAndCallback(T _hidl_cb, hidl_vec<U> data) {
    return setFailureAndCallback(
            _hidl_cb, data, "Thermal AIDL HAL client used by HIDL wrapper was not initialized");
}

template <typename T>
Return<void> setInitFailureAndCallback(T _hidl_cb) {
    return setFailureAndCallback(
            _hidl_cb, "Thermal AIDL HAL client used by HIDL wrapper was not initialized");
}

template <typename T, typename U>
Return<void> setUnsupportedFailureAndCallback(T _hidl_cb, hidl_vec<U> data) {
    return setFailureAndCallback(_hidl_cb, data, "Operation unsupported by Thermal HIDL wrapper");
}

TemperatureType_2_0 convertAidlTemperatureType(const TemperatureType& type) {
    if (type < TemperatureType::CPU || type > TemperatureType::NPU) {
        return TemperatureType_2_0::UNKNOWN;
    }
    return static_cast<TemperatureType_2_0>(type);
}

CoolingType_2_0 convertAidlCoolingType(const CoolingType& type) {
    if (type < CoolingType::FAN || type > CoolingType::COMPONENT) {
        return CoolingType_2_0::COMPONENT;
    }
    return static_cast<CoolingType_2_0>(type);
}

Temperature_2_0 convertAidlTemperature(const Temperature& temperature) {
    Temperature_2_0 t = Temperature_2_0{
            convertAidlTemperatureType(temperature.type), temperature.name, temperature.value,
            static_cast<ThrottlingSeverity_2_0>(temperature.throttlingStatus)};
    return t;
}

CoolingDevice_2_0 convertAidlCoolingDevice(const CoolingDevice& cooling_device) {
    CoolingDevice_2_0 t =
            CoolingDevice_2_0{convertAidlCoolingType(cooling_device.type), cooling_device.name,
                              static_cast<uint64_t>(cooling_device.value)};
    return t;
}
TemperatureThreshold_2_0 convertAidlTemperatureThreshold(const TemperatureThreshold& threshold) {
    TemperatureThreshold_2_0 t =
            TemperatureThreshold_2_0{convertAidlTemperatureType(threshold.type), threshold.name,
                                     threshold.hotThrottlingThresholds.data(),
                                     threshold.coldThrottlingThresholds.data(), NAN};
    return t;
}

}  // namespace

// Methods from ::android::hardware::thermal::V1_0::IThermal follow.
Return<void> ThermalHidlWrapper::getTemperatures(getTemperatures_cb _hidl_cb) {
    hidl_vec<Temperature_1_0> ret_1_0;
    setUnsupportedFailureAndCallback(_hidl_cb, ret_1_0);
    return Void();
}

Return<void> ThermalHidlWrapper::getCpuUsages(
        std::function<void(const ThermalStatus&, const hidl_vec<CpuUsage>&)> _hidl_cb) {
    hidl_vec<CpuUsage> ret_1_0;
    setUnsupportedFailureAndCallback(_hidl_cb, ret_1_0);
    return Void();
}

Return<void> ThermalHidlWrapper::getCoolingDevices(
        std::function<void(const ThermalStatus&, const hidl_vec<CoolingDevice_1_0>&)> _hidl_cb) {
    hidl_vec<CoolingDevice_1_0> ret_1_0;
    setUnsupportedFailureAndCallback(_hidl_cb, ret_1_0);
    return Void();
}

// Methods from ::android::hardware::thermal::V2_0::IThermal follow.
Return<void> ThermalHidlWrapper::getCurrentTemperatures(
        bool filterType, TemperatureType_2_0 type,
        std::function<void(const ThermalStatus&, const hidl_vec<Temperature_2_0>&)> _hidl_cb) {
    hidl_vec<Temperature_2_0> ret_2_0;
    if (!thermal_service_) {
        setInitFailureAndCallback(_hidl_cb, ret_2_0);
    }

    std::vector<Temperature> ret_aidl;
    ThermalStatus status;
    ::ndk::ScopedAStatus a_status;
    if (filterType) {
        a_status = thermal_service_->getTemperaturesWithType(static_cast<TemperatureType>(type),
                                                             &ret_aidl);
    } else {
        a_status = thermal_service_->getTemperatures(&ret_aidl);
    }
    if (a_status.isOk()) {
        std::vector<Temperature_2_0> ret;
        for (const auto& temperature : ret_aidl) {
            ret.push_back(convertAidlTemperature(temperature));
        }
        _hidl_cb(status, hidl_vec<Temperature_2_0>(ret));
    } else {
        setFailureAndCallback(_hidl_cb, ret_2_0, a_status.getMessage());
    }
    return Void();
}

Return<void> ThermalHidlWrapper::getTemperatureThresholds(
        bool filterType, TemperatureType_2_0 type,
        std::function<void(const ThermalStatus&, const hidl_vec<TemperatureThreshold_2_0>&)>
                _hidl_cb) {
    hidl_vec<TemperatureThreshold_2_0> ret_2_0;
    if (!thermal_service_) {
        setInitFailureAndCallback(_hidl_cb, ret_2_0);
    }

    std::vector<TemperatureThreshold> ret_aidl;
    ThermalStatus status;
    ::ndk::ScopedAStatus a_status;
    if (filterType) {
        a_status = thermal_service_->getTemperatureThresholdsWithType(
                static_cast<TemperatureType>(type), &ret_aidl);
    } else {
        a_status = thermal_service_->getTemperatureThresholds(&ret_aidl);
    }
    if (a_status.isOk()) {
        std::vector<TemperatureThreshold_2_0> ret;
        for (const auto& threshold : ret_aidl) {
            ret.push_back(convertAidlTemperatureThreshold(threshold));
        }
        _hidl_cb(status, hidl_vec<TemperatureThreshold_2_0>(ret));
    } else {
        setFailureAndCallback(_hidl_cb, ret_2_0, a_status.getMessage());
    }
    return Void();
}

Return<void> ThermalHidlWrapper::registerThermalChangedCallback(
        const sp<IThermalChangedCallback_2_0>& callback, bool filterType, TemperatureType_2_0 type,
        std::function<void(const ThermalStatus&)> _hidl_cb) {
    if (!thermal_service_) {
        setInitFailureAndCallback(_hidl_cb);
    }
    if (callback == nullptr) {
        setFailureAndCallback(_hidl_cb, "Invalid nullptr callback");
        return Void();
    }
    std::lock_guard<std::mutex> _lock(callback_wrappers_mutex_);
    for (const auto& callback_wrapper : callback_wrappers_) {
        if (::android::hardware::interfacesEqual(callback_wrapper->callback_2_0_.get(),
                                                 callback.get())) {
            setFailureAndCallback(_hidl_cb, "The callback was already registered through wrapper");
            return Void();
        }
    }
    std::shared_ptr<IThermalChangedCallbackWrapper> callback_wrapper =
            ndk::SharedRefBase::make<IThermalChangedCallbackWrapper>(callback);
    ::ndk::ScopedAStatus a_status;
    ThermalStatus status;
    if (filterType) {
        a_status = thermal_service_->registerThermalChangedCallbackWithType(
                callback_wrapper, static_cast<TemperatureType>(type));
    } else {
        a_status = thermal_service_->registerThermalChangedCallback(callback_wrapper);
    }
    if (a_status.isOk()) {
        callback_wrappers_.push_back(callback_wrapper);
        _hidl_cb(status);
    } else {
        setFailureAndCallback(_hidl_cb, a_status.getMessage());
    }
    return Void();
}

Return<void> ThermalHidlWrapper::unregisterThermalChangedCallback(
        const sp<IThermalChangedCallback_2_0>& callback,
        std::function<void(const ThermalStatus&)> _hidl_cb) {
    if (!thermal_service_) {
        setInitFailureAndCallback(_hidl_cb);
    }
    if (callback == nullptr) {
        setFailureAndCallback(_hidl_cb, "Invalid nullptr callback");
        return Void();
    }
    std::lock_guard<std::mutex> _lock(callback_wrappers_mutex_);
    for (auto it = callback_wrappers_.begin(); it != callback_wrappers_.end(); it++) {
        auto callback_wrapper = *it;
        if (::android::hardware::interfacesEqual(callback_wrapper->callback_2_0_.get(),
                                                 callback.get())) {
            ::ndk::ScopedAStatus a_status;
            ThermalStatus status;
            a_status = thermal_service_->unregisterThermalChangedCallback(callback_wrapper);
            if (a_status.isOk()) {
                callback_wrappers_.erase(it);
                _hidl_cb(status);
            } else {
                setFailureAndCallback(_hidl_cb, a_status.getMessage());
            }
            return Void();
        }
    }
    setFailureAndCallback(_hidl_cb, "The callback was not registered through wrapper before");
    return Void();
}

Return<void> ThermalHidlWrapper::getCurrentCoolingDevices(
        bool filterType, CoolingType_2_0 type,
        std::function<void(const ThermalStatus&, const hidl_vec<CoolingDevice_2_0>&)> _hidl_cb) {
    hidl_vec<CoolingDevice_2_0> ret_2_0;
    if (!thermal_service_) {
        setInitFailureAndCallback(_hidl_cb, ret_2_0);
    }

    std::vector<CoolingDevice> ret_aidl;
    ThermalStatus status;
    ::ndk::ScopedAStatus a_status;
    if (filterType) {
        a_status = thermal_service_->getCoolingDevicesWithType(static_cast<CoolingType>(type),
                                                               &ret_aidl);
    } else {
        a_status = thermal_service_->getCoolingDevices(&ret_aidl);
    }
    if (a_status.isOk()) {
        std::vector<CoolingDevice_2_0> ret;
        for (const auto& cooling_device : ret_aidl) {
            ret.push_back(convertAidlCoolingDevice(cooling_device));
        }
        _hidl_cb(status, hidl_vec<CoolingDevice_2_0>(ret));
    } else {
        setFailureAndCallback(_hidl_cb, ret_2_0, a_status.getMessage());
    }
    return Void();
}

// Methods from ::android::hidl::base::V1_0::IBase follow.
Return<void> ThermalHidlWrapper::debug(const hidl_handle& handle,
                                       const hidl_vec<hidl_string>& args) {
    if (handle != nullptr && handle->numFds >= 1) {
        int fd = handle->data[0];
        char** arr = new char*[args.size()];
        for (size_t i = 0; i < args.size(); i++) {
            arr[i] = strdup(args[i].c_str());
        }
        thermal_service_->dump(fd, (const char**)arr, args.size());
    }
    return Void();
}

::ndk::ScopedAStatus ThermalHidlWrapper::IThermalChangedCallbackWrapper::notifyThrottling(
        const Temperature& temperature) {
    callback_2_0_->notifyThrottling(convertAidlTemperature(temperature));
    return ::ndk::ScopedAStatus::ok();
}

}  // namespace thermal
}  // namespace hardware
}  // namespace android
}  // namespace aidl
