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
const std::string kHwComponentId = "faceSensor";
const std::string kHardwareVersion = "vendor/model/revision";
const std::string kFirmwareVersion = "1.01";
const std::string kSerialNumber = "00000001";
const std::string kSwComponentId = "matchingAlgorithm";
const std::string kSoftwareVersion = "vendor/version/revision";

ndk::ScopedAStatus Face::getSensorProps(std::vector<SensorProps>* return_val) {
    common::ComponentInfo hw_component_info;
    hw_component_info.componentId = kHwComponentId;
    hw_component_info.hardwareVersion = kHardwareVersion;
    hw_component_info.firmwareVersion = kFirmwareVersion;
    hw_component_info.serialNumber = kSerialNumber;
    hw_component_info.softwareVersion = "";

    common::ComponentInfo sw_component_info;
    sw_component_info.componentId = kSwComponentId;
    sw_component_info.hardwareVersion = "";
    sw_component_info.firmwareVersion = "";
    sw_component_info.serialNumber = "";
    sw_component_info.softwareVersion = kSoftwareVersion;

    common::CommonProps commonProps;
    commonProps.sensorId = kSensorId;
    commonProps.sensorStrength = kSensorStrength;
    commonProps.maxEnrollmentsPerUser = kMaxEnrollmentsPerUser;
    commonProps.componentInfo = {std::move(hw_component_info), std::move(sw_component_info)};

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

}  // namespace aidl::android::hardware::biometrics::face
