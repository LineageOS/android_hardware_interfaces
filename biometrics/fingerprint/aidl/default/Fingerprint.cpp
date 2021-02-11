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

#include "Fingerprint.h"
#include "Session.h"

namespace aidl::android::hardware::biometrics::fingerprint {

const int kSensorId = 1;
const common::SensorStrength kSensorStrength = common::SensorStrength::STRONG;
const int kMaxEnrollmentsPerUser = 5;
const FingerprintSensorType kSensorType = FingerprintSensorType::REAR;
const bool kSupportsNavigationGestures = true;
const std::string kHwDeviceName = "fingerprintSensor";
const std::string kHardwareVersion = "vendor/model/revision";
const std::string kFirmwareVersion = "1.01";
const std::string kSerialNumber = "00000001";

ndk::ScopedAStatus Fingerprint::getSensorProps(std::vector<SensorProps>* return_val) {
    *return_val = std::vector<SensorProps>();

    std::vector<common::HardwareInfo> hardwareInfos = std::vector<common::HardwareInfo>();
    common::HardwareInfo sensorInfo = {kHwDeviceName,
            kHardwareVersion,
            kFirmwareVersion,
            kSerialNumber
    };
    hardwareInfos.push_back(sensorInfo);
    common::CommonProps commonProps = {kSensorId,
            kSensorStrength,
            kMaxEnrollmentsPerUser,
            hardwareInfos};
    SensorLocation sensorLocation = {
            0 /* displayId */,
            0 /* sensorLocationX */,
            0 /* sensorLocationY */,
            0 /* sensorRadius */
    };
    SensorProps props = {commonProps,
            kSensorType,
            {sensorLocation},
            kSupportsNavigationGestures,
            false /* supportsDetectInteraction */};
    return_val->push_back(props);
    return ndk::ScopedAStatus::ok();
}

ndk::ScopedAStatus Fingerprint::createSession(int32_t /*sensorId*/, int32_t /*userId*/,
                                              const std::shared_ptr<ISessionCallback>& cb,
                                              std::shared_ptr<ISession>* return_val) {
    *return_val = SharedRefBase::make<Session>(cb);
    return ndk::ScopedAStatus::ok();
}
}  // namespace aidl::android::hardware::biometrics::fingerprint
