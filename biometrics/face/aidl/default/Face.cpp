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

#undef LOG_TAG
#define LOG_TAG "FaceVirtualHal"

#include "Face.h"
#include "Session.h"

#include "FakeFaceEngine.h"

#include <android-base/properties.h>
#include <face.sysprop.h>

#include <android-base/file.h>
#include <android-base/logging.h>
#include <android-base/stringprintf.h>

using namespace ::android::face::virt;

namespace aidl::android::hardware::biometrics::face {

const int kSensorId = 4;
const common::SensorStrength kSensorStrength = FakeFaceEngine::GetSensorStrength();
const int kMaxEnrollmentsPerUser = 5;
const FaceSensorType kSensorType = FakeFaceEngine::GetSensorType();
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

ndk::ScopedAStatus Face::createSession(int32_t sensorId, int32_t userId,
                                       const std::shared_ptr<ISessionCallback>& cb,
                                       std::shared_ptr<ISession>* return_val) {
    mSession = SharedRefBase::make<Session>(std::make_unique<FakeFaceEngine>(), cb);
    *return_val = mSession;

    mSession->linkToDeath(cb->asBinder().get());

    LOG(INFO) << __func__ << ": sensorId:" << sensorId << " userId:" << userId;
    return ndk::ScopedAStatus::ok();
}

binder_status_t Face::dump(int fd, const char** /*args*/, uint32_t numArgs) {
    if (fd < 0) {
        LOG(ERROR) << __func__ << "fd invalid: " << fd;
        return STATUS_BAD_VALUE;
    } else {
        LOG(INFO) << __func__ << " fd:" << fd << "numArgs:" << numArgs;
    }

    dprintf(fd, "----- FaceVirtualHal::dump -----\n");
    std::vector<SensorProps> sps(1);
    getSensorProps(&sps);
    for (auto& sp : sps) {
        ::android::base::WriteStringToFd(sp.toString(), fd);
    }
    if (mSession != nullptr) {
        ::android::base::WriteStringToFd(mSession->toString(), fd);
    } else {
        dprintf(fd, "\nWARNING: no ISession found\n");
    }

    fsync(fd);
    return STATUS_OK;
}

binder_status_t Face::handleShellCommand(int in, int out, int err, const char** args,
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

void Face::onHelp(int fd) {
    dprintf(fd, "Virtual Face HAL commands:\n");
    dprintf(fd, "         help: print this help\n");
    dprintf(fd, "  resetconfig: reset all configuration to default\n");
    dprintf(fd, "\n");
    fsync(fd);
}

void Face::resetConfigToDefault() {
    LOG(INFO) << __func__ << ": reset virtual Face HAL configuration to default";
#define RESET_CONFIG_O(__NAME__) \
    if (FaceHalProperties::__NAME__()) FaceHalProperties::__NAME__(std::nullopt)
#define RESET_CONFIG_V(__NAME__) \
    if (!FaceHalProperties::__NAME__().empty()) FaceHalProperties::__NAME__({std::nullopt})

    RESET_CONFIG_O(type);
    RESET_CONFIG_O(strength);
    RESET_CONFIG_V(enrollments);
    RESET_CONFIG_O(enrollment_hit);
    RESET_CONFIG_V(features);
    RESET_CONFIG_O(next_enrollment);
    RESET_CONFIG_O(authenticator_id);
    RESET_CONFIG_O(challenge);
    RESET_CONFIG_O(lockout);
    RESET_CONFIG_O(operation_authenticate_fails);
    RESET_CONFIG_O(operation_detect_interaction_fails);
    RESET_CONFIG_O(operation_enroll_fails);
    RESET_CONFIG_V(operation_authenticate_latency);
    RESET_CONFIG_V(operation_detect_interaction_latency);
    RESET_CONFIG_V(operation_enroll_latency);
    RESET_CONFIG_O(operation_authenticate_duration);
    RESET_CONFIG_O(operation_authenticate_error);
    RESET_CONFIG_O(operation_authenticate_acquired);
    RESET_CONFIG_O(lockout_enable);
    RESET_CONFIG_O(lockout_timed_enable);
    RESET_CONFIG_O(lockout_timed_threshold);
    RESET_CONFIG_O(lockout_timed_duration);
    RESET_CONFIG_O(lockout_permanent_threshold);
}

}  // namespace aidl::android::hardware::biometrics::face
