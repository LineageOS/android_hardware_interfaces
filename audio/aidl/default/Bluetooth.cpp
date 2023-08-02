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

#define LOG_TAG "AHAL_Bluetooth"
#include <android-base/logging.h>

#include "core-impl/Bluetooth.h"

using aidl::android::hardware::audio::core::VendorParameter;
using aidl::android::media::audio::common::Boolean;
using aidl::android::media::audio::common::Float;
using aidl::android::media::audio::common::Int;

namespace aidl::android::hardware::audio::core {

Bluetooth::Bluetooth() {
    mScoConfig.isEnabled = Boolean{false};
    mScoConfig.isNrecEnabled = Boolean{false};
    mScoConfig.mode = ScoConfig::Mode::SCO;
    mHfpConfig.isEnabled = Boolean{false};
    mHfpConfig.sampleRate = Int{8000};
    mHfpConfig.volume = Float{HfpConfig::VOLUME_MAX};
}

ndk::ScopedAStatus Bluetooth::setScoConfig(const ScoConfig& in_config, ScoConfig* _aidl_return) {
    if (in_config.isEnabled.has_value()) {
        mScoConfig.isEnabled = in_config.isEnabled;
    }
    if (in_config.isNrecEnabled.has_value()) {
        mScoConfig.isNrecEnabled = in_config.isNrecEnabled;
    }
    if (in_config.mode != ScoConfig::Mode::UNSPECIFIED) {
        mScoConfig.mode = in_config.mode;
    }
    if (in_config.debugName.has_value()) {
        mScoConfig.debugName = in_config.debugName;
    }
    *_aidl_return = mScoConfig;
    LOG(DEBUG) << __func__ << ": received " << in_config.toString() << ", returning "
               << _aidl_return->toString();
    return ndk::ScopedAStatus::ok();
}

ndk::ScopedAStatus Bluetooth::setHfpConfig(const HfpConfig& in_config, HfpConfig* _aidl_return) {
    if (in_config.sampleRate.has_value() && in_config.sampleRate.value().value <= 0) {
        LOG(ERROR) << __func__ << ": invalid sample rate: " << in_config.sampleRate.value().value;
        return ndk::ScopedAStatus::fromExceptionCode(EX_ILLEGAL_ARGUMENT);
    }
    if (in_config.volume.has_value() && (in_config.volume.value().value < HfpConfig::VOLUME_MIN ||
                                         in_config.volume.value().value > HfpConfig::VOLUME_MAX)) {
        LOG(ERROR) << __func__ << ": invalid volume: " << in_config.volume.value().value;
        return ndk::ScopedAStatus::fromExceptionCode(EX_ILLEGAL_ARGUMENT);
    }

    if (in_config.isEnabled.has_value()) {
        mHfpConfig.isEnabled = in_config.isEnabled;
    }
    if (in_config.sampleRate.has_value()) {
        mHfpConfig.sampleRate = in_config.sampleRate;
    }
    if (in_config.volume.has_value()) {
        mHfpConfig.volume = in_config.volume;
    }
    *_aidl_return = mHfpConfig;
    LOG(DEBUG) << __func__ << ": received " << in_config.toString() << ", returning "
               << _aidl_return->toString();
    return ndk::ScopedAStatus::ok();
}

ndk::ScopedAStatus BluetoothA2dp::isEnabled(bool* _aidl_return) {
    *_aidl_return = mEnabled;
    return ndk::ScopedAStatus::ok();
}

ndk::ScopedAStatus BluetoothA2dp::setEnabled(bool in_enabled) {
    mEnabled = in_enabled;
    LOG(DEBUG) << __func__ << ": " << mEnabled;
    if (mHandler) return mHandler();
    return ndk::ScopedAStatus::ok();
}

ndk::ScopedAStatus BluetoothA2dp::supportsOffloadReconfiguration(bool* _aidl_return) {
    *_aidl_return = false;
    return ndk::ScopedAStatus::ok();
}

ndk::ScopedAStatus BluetoothA2dp::reconfigureOffload(
        const std::vector<::aidl::android::hardware::audio::core::VendorParameter>& in_parameters
                __unused) {
    LOG(DEBUG) << __func__ << ": " << ::android::internal::ToString(in_parameters);
    return ndk::ScopedAStatus::fromExceptionCode(EX_UNSUPPORTED_OPERATION);
}

ndk::ScopedAStatus BluetoothLe::isEnabled(bool* _aidl_return) {
    *_aidl_return = mEnabled;
    return ndk::ScopedAStatus::ok();
}

ndk::ScopedAStatus BluetoothLe::setEnabled(bool in_enabled) {
    mEnabled = in_enabled;
    if (mHandler) return mHandler();
    return ndk::ScopedAStatus::ok();
}

ndk::ScopedAStatus BluetoothLe::supportsOffloadReconfiguration(bool* _aidl_return) {
    *_aidl_return = false;
    return ndk::ScopedAStatus::ok();
}

ndk::ScopedAStatus BluetoothLe::reconfigureOffload(
        const std::vector<::aidl::android::hardware::audio::core::VendorParameter>& in_parameters
                __unused) {
    LOG(DEBUG) << __func__ << ": " << ::android::internal::ToString(in_parameters);
    return ndk::ScopedAStatus::fromExceptionCode(EX_UNSUPPORTED_OPERATION);
}

}  // namespace aidl::android::hardware::audio::core
