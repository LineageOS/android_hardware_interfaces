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

#include <android-base/properties.h>
#include <fingerprint.sysprop.h>

#include <android-base/file.h>
#include <android-base/logging.h>
#include <android-base/stringprintf.h>

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
    LOG(INFO) << "ro.product.name=" << ::android::base::GetProperty("ro.product.name", "UNKNOWN");
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

    LOG(INFO) << "sensor type:" << ::android::internal::ToString(mSensorType)
              << " location:" << sensorLocation.toString();

    *out = {{commonProps,
             mSensorType,
             {sensorLocation},
             navigationGuesture,
             detectInteraction,
             displayTouch,
             controlIllumination,
             std::nullopt}};
    return ndk::ScopedAStatus::ok();
}

ndk::ScopedAStatus Fingerprint::createSession(int32_t sensorId, int32_t userId,
                                              const std::shared_ptr<ISessionCallback>& cb,
                                              std::shared_ptr<ISession>* out) {
    CHECK(mSession == nullptr || mSession->isClosed()) << "Open session already exists!";

    mSession = SharedRefBase::make<Session>(sensorId, userId, cb, mEngine.get(), &mWorker);
    *out = mSession;

    mSession->linkToDeath(cb->asBinder().get());

    LOG(INFO) << __func__ << ": sensorId:" << sensorId << " userId:" << userId;
    return ndk::ScopedAStatus::ok();
}

binder_status_t Fingerprint::dump(int fd, const char** /*args*/, uint32_t numArgs) {
    if (fd < 0) {
        LOG(ERROR) << __func__ << "fd invalid: " << fd;
        return STATUS_BAD_VALUE;
    } else {
        LOG(INFO) << __func__ << " fd:" << fd << "numArgs:" << numArgs;
    }

    dprintf(fd, "----- FingerprintVirtualHal::dump -----\n");
    std::vector<SensorProps> sps(1);
    getSensorProps(&sps);
    for (auto& sp : sps) {
        ::android::base::WriteStringToFd(sp.toString(), fd);
    }
    ::android::base::WriteStringToFd(mEngine->toString(), fd);

    fsync(fd);
    return STATUS_OK;
}

binder_status_t Fingerprint::handleShellCommand(int in, int out, int err, const char** args,
                                                uint32_t numArgs) {
    LOG(INFO) << __func__ << " in:" << in << " out:" << out << " err:" << err
              << " numArgs:" << numArgs;

    if (numArgs == 0) {
        LOG(INFO) << __func__ << ": available commands";
        onHelp(out);
        return STATUS_OK;
    }

    for (auto&& str : std::vector<std::string_view>(args, args + numArgs)) {
        std::string option = str.data();
        if (option.find("clearconfig") != std::string::npos ||
            option.find("resetconfig") != std::string::npos) {
            resetConfigToDefault();
        }
        if (option.find("help") != std::string::npos) {
            onHelp(out);
        }
    }

    return STATUS_OK;
}

void Fingerprint::onHelp(int fd) {
    dprintf(fd, "Virtual HAL commands:\n");
    dprintf(fd, "         help: print this help\n");
    dprintf(fd, "  resetconfig: reset all configuration to default\n");
    dprintf(fd, "\n");
    fsync(fd);
}

void Fingerprint::resetConfigToDefault() {
    LOG(INFO) << __func__ << ": reset virtual HAL configuration to default";
#define RESET_CONFIG_O(__NAME__) \
    if (FingerprintHalProperties::__NAME__()) FingerprintHalProperties::__NAME__(std::nullopt)
#define RESET_CONFIG_V(__NAME__)                       \
    if (!FingerprintHalProperties::__NAME__().empty()) \
    FingerprintHalProperties::__NAME__({std::nullopt})

    RESET_CONFIG_O(type);
    RESET_CONFIG_V(enrollments);
    RESET_CONFIG_O(enrollment_hit);
    RESET_CONFIG_O(authenticator_id);
    RESET_CONFIG_O(challenge);
    RESET_CONFIG_O(lockout);
    RESET_CONFIG_O(operation_authenticate_fails);
    RESET_CONFIG_O(operation_detect_interaction_error);
    RESET_CONFIG_O(operation_enroll_error);
    RESET_CONFIG_V(operation_authenticate_latency);
    RESET_CONFIG_V(operation_detect_interaction_latency);
    RESET_CONFIG_V(operation_enroll_latency);
    RESET_CONFIG_O(operation_authenticate_duration);
    RESET_CONFIG_O(operation_authenticate_error);
    RESET_CONFIG_O(sensor_location);
    RESET_CONFIG_O(operation_authenticate_acquired);
    RESET_CONFIG_O(operation_detect_interaction_duration);
    RESET_CONFIG_O(operation_detect_interaction_acquired);
    RESET_CONFIG_O(sensor_id);
    RESET_CONFIG_O(sensor_strength);
    RESET_CONFIG_O(max_enrollments);
    RESET_CONFIG_O(navigation_guesture);
    RESET_CONFIG_O(detect_interaction);
    RESET_CONFIG_O(display_touch);
    RESET_CONFIG_O(control_illumination);
    RESET_CONFIG_O(lockout_enable);
    RESET_CONFIG_O(lockout_timed_threshold);
    RESET_CONFIG_O(lockout_timed_duration);
    RESET_CONFIG_O(lockout_permanent_threshold);
}

}  // namespace aidl::android::hardware::biometrics::fingerprint
