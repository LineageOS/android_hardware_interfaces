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

#include <fingerprint.sysprop.h>
#include "Session.h"

#include <android-base/logging.h>

using namespace ::android::fingerprint::virt;

namespace aidl::android::hardware::biometrics::fingerprint {
namespace {
constexpr size_t MAX_WORKER_QUEUE_SIZE = 5;
constexpr int SENSOR_ID = 5;
constexpr common::SensorStrength SENSOR_STRENGTH = common::SensorStrength::STRONG;
constexpr int MAX_ENROLLMENTS_PER_USER = 5;
constexpr bool SUPPORTS_NAVIGATION_GESTURES = true;
constexpr char HW_COMPONENT_ID[] = "fingerprintSensor";
constexpr char HW_VERSION[] = "vendor/model/revision";
constexpr char FW_VERSION[] = "1.01";
constexpr char SERIAL_NUMBER[] = "00000001";
constexpr char SW_COMPONENT_ID[] = "matchingAlgorithm";
constexpr char SW_VERSION[] = "vendor/version/revision";

}  // namespace

Fingerprint::Fingerprint() : mWorker(MAX_WORKER_QUEUE_SIZE) {
    std::string sensorTypeProp = FingerprintHalProperties::type().value_or("");
    if (sensorTypeProp == "" || sensorTypeProp == "default" || sensorTypeProp == "rear") {
        mSensorType = FingerprintSensorType::REAR;
        mEngine = std::make_unique<FakeFingerprintEngineRear>();
    } else if (sensorTypeProp == "udfps") {
        mSensorType = FingerprintSensorType::UNDER_DISPLAY_OPTICAL;
        mEngine = std::make_unique<FakeFingerprintEngineUdfps>();
    } else if (sensorTypeProp == "side") {
        mSensorType = FingerprintSensorType::POWER_BUTTON;
        mEngine = std::make_unique<FakeFingerprintEngineSide>();
    } else {
        mSensorType = FingerprintSensorType::UNKNOWN;
        mEngine = std::make_unique<FakeFingerprintEngineRear>();
        UNIMPLEMENTED(FATAL) << "unrecognized or unimplemented fingerprint behavior: "
                             << sensorTypeProp;
    }
    LOG(INFO) << "sensorTypeProp:" << sensorTypeProp;
}

ndk::ScopedAStatus Fingerprint::getSensorProps(std::vector<SensorProps>* out) {
    std::vector<common::ComponentInfo> componentInfo = {
            {HW_COMPONENT_ID, HW_VERSION, FW_VERSION, SERIAL_NUMBER, "" /* softwareVersion */},
            {SW_COMPONENT_ID, "" /* hardwareVersion */, "" /* firmwareVersion */,
             "" /* serialNumber */, SW_VERSION}};

    auto sensorId = FingerprintHalProperties::sensor_id().value_or(SENSOR_ID);
    auto sensorStrength =
            FingerprintHalProperties::sensor_strength().value_or((int)SENSOR_STRENGTH);
    auto maxEnrollments =
            FingerprintHalProperties::max_enrollments().value_or(MAX_ENROLLMENTS_PER_USER);
    auto navigationGuesture = FingerprintHalProperties::navigation_guesture().value_or(false);
    auto detectInteraction = FingerprintHalProperties::detect_interaction().value_or(false);
    auto displayTouch = FingerprintHalProperties::display_touch().value_or(true);
    auto controlIllumination = FingerprintHalProperties::control_illumination().value_or(false);

    common::CommonProps commonProps = {sensorId, (common::SensorStrength)sensorStrength,
                                       maxEnrollments, componentInfo};

    SensorLocation sensorLocation = mEngine->getSensorLocation();

    LOG(INFO) << "sensor type:" << (int)mSensorType << " location:" << sensorLocation.toString();

    *out = {{commonProps,
             mSensorType,
             {sensorLocation},
             navigationGuesture,
             detectInteraction,
             displayTouch,
             controlIllumination}};
    return ndk::ScopedAStatus::ok();
}

ndk::ScopedAStatus Fingerprint::createSession(int32_t sensorId, int32_t userId,
                                              const std::shared_ptr<ISessionCallback>& cb,
                                              std::shared_ptr<ISession>* out) {
    CHECK(mSession == nullptr || mSession->isClosed()) << "Open session already exists!";

    mSession = SharedRefBase::make<Session>(sensorId, userId, cb, mEngine.get(), &mWorker);
    *out = mSession;

    LOG(INFO) << "createSession: sensorId:" << sensorId << " userId:" << userId;
    return ndk::ScopedAStatus::ok();
}

}  // namespace aidl::android::hardware::biometrics::fingerprint
