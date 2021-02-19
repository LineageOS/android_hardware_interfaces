/*
 * Copyright (C) 2020 The Android Open Source Project
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

#include "Face.h"
#include "Session.h"

namespace aidl::android::hardware::biometrics::face {

const int kSensorId = 4;
const common::SensorStrength kSensorStrength = common::SensorStrength::STRONG;
const int kMaxEnrollmentsPerUser = 5;
const FaceSensorType kSensorType = FaceSensorType::RGB;
const bool kHalControlsPreview = true;
const std::string kHwDeviceName = "faceSensor";
const std::string kHardwareVersion = "vendor/model/revision";
const std::string kFirmwareVersion = "1.01";
const std::string kSerialNumber = "00000001";

ndk::ScopedAStatus Face::getSensorProps(std::vector<SensorProps>* return_val) {
    common::HardwareInfo hardware_info;
    hardware_info.deviceName = kHwDeviceName;
    hardware_info.hardwareVersion = kHardwareVersion;
    hardware_info.firmwareVersion = kFirmwareVersion;
    hardware_info.serialNumber = kSerialNumber;

    common::CommonProps commonProps;
    commonProps.sensorId = kSensorId;
    commonProps.sensorStrength = kSensorStrength;
    commonProps.maxEnrollmentsPerUser = kMaxEnrollmentsPerUser;
    commonProps.hardwareInfo = {std::move(hardware_info)};

    SensorProps props;
    props.commonProps = std::move(commonProps);
    props.sensorType = kSensorType;
    props.halControlsPreview = kHalControlsPreview;
    props.enrollPreviewWidth = 1080;
    props.enrollPreviewHeight = 1920;
    props.enrollTranslationX = 100.f;
    props.enrollTranslationY = 50.f;
    props.enrollPreviewScale = 1.f;

    *return_val = {std::move(props)};
    return ndk::ScopedAStatus::ok();
}

ndk::ScopedAStatus Face::createSession(int32_t /*sensorId*/, int32_t /*userId*/,
                                       const std::shared_ptr<ISessionCallback>& cb,
                                       std::shared_ptr<ISession>* return_val) {
    *return_val = SharedRefBase::make<Session>(cb);
    return ndk::ScopedAStatus::ok();
}

ndk::ScopedAStatus Face::reset() {
    return ndk::ScopedAStatus::ok();
}

}  // namespace aidl::android::hardware::biometrics::face
