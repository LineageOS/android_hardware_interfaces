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

#include <fastbootshim.h>

using ::android::sp;
using ::android::hardware::hidl_string;
using ::android::hardware::Void;
using ::android::hardware::fastboot::V1_0::FileSystemType;
using ::android::hardware::fastboot::V1_0::Result;
using ::android::hardware::fastboot::V1_0::Status;

using ndk::ScopedAStatus;

namespace aidl {
namespace android {
namespace hardware {
namespace fastboot {
ScopedAStatus ResultToAStatus(Result result) {
    switch (result.status) {
        case Status::SUCCESS:
            return ScopedAStatus::ok();
        case Status::NOT_SUPPORTED:
            return ScopedAStatus::fromExceptionCode(EX_UNSUPPORTED_OPERATION);
        case Status::INVALID_ARGUMENT:
            return ScopedAStatus::fromExceptionCode(EX_ILLEGAL_ARGUMENT);
        case Status::FAILURE_UNKNOWN:
            return ScopedAStatus::fromServiceSpecificErrorWithMessage(
                    BnFastboot::FAILURE_UNKNOWN, ("Error " + std::string(result.message)).c_str());
    }
    return ScopedAStatus::fromServiceSpecificErrorWithMessage(
            BnFastboot::FAILURE_UNKNOWN,
            ("Unrecognized status value " + toString(result.status)).c_str());
}
FastbootShim::FastbootShim(const sp<HidlFastboot>& service) : service_(service) {}

ScopedAStatus FastbootShim::getPartitionType(const std::string& in_partitionName,
                                             FileSystemType* _aidl_return) {
    Result out_result = {Status::FAILURE_UNKNOWN, ""};
    if (in_partitionName.empty()) {
        return ScopedAStatus::fromExceptionCodeWithMessage(EX_ILLEGAL_ARGUMENT,
                                                           "Invalid partition name");
    }
    const hidl_string partition = in_partitionName;
    auto ret = service_->getPartitionType(partition, [&](auto type, auto& result) {
        out_result = result;
        if (out_result.status != Status::SUCCESS) return;
        *_aidl_return = static_cast<aidl::android::hardware::fastboot::FileSystemType>(type);
    });
    return ResultToAStatus(out_result);
}

ScopedAStatus FastbootShim::doOemCommand(const std::string& in_oemCmd, std::string* _aidl_return) {
    Result out_result = {Status::FAILURE_UNKNOWN, ""};
    *_aidl_return = "";
    if (in_oemCmd.empty()) {
        return ScopedAStatus::fromExceptionCodeWithMessage(EX_ILLEGAL_ARGUMENT, "Invalid command");
    }
    const hidl_string oemCmdArgs = in_oemCmd;
    auto ret = service_->doOemCommand(oemCmdArgs, [&](auto& result) {
        out_result = result;
        if (out_result.status != Status::SUCCESS) return;
        *_aidl_return = std::string(result.message.c_str());
    });
    return ResultToAStatus(out_result);
}

ScopedAStatus FastbootShim::getVariant(std::string* _aidl_return) {
    Result out_result = {Status::FAILURE_UNKNOWN, ""};
    *_aidl_return = "";
    auto ret = service_->getVariant([&](auto& variant, auto& result) {
        out_result = result;
        if (out_result.status != Status::SUCCESS) return;
        *_aidl_return = std::string(variant.c_str());
    });
    return ResultToAStatus(out_result);
}

ScopedAStatus FastbootShim::getOffModeChargeState(bool* _aidl_return) {
    Result out_result = {Status::FAILURE_UNKNOWN, ""};
    *_aidl_return = false;
    auto ret = service_->getOffModeChargeState([&](auto state, auto& result) {
        out_result = result;
        if (out_result.status != Status::SUCCESS) return;
        *_aidl_return = state;
    });
    return ResultToAStatus(out_result);
}

ScopedAStatus FastbootShim::getBatteryVoltageFlashingThreshold(int32_t* _aidl_return) {
    Result out_result = {Status::FAILURE_UNKNOWN, ""};
    *_aidl_return = 0;
    auto ret = service_->getBatteryVoltageFlashingThreshold([&](auto batteryVoltage, auto& result) {
        out_result = result;
        if (out_result.status != Status::SUCCESS) return;
        *_aidl_return = batteryVoltage;
    });
    return ResultToAStatus(out_result);
}

ScopedAStatus FastbootShim::doOemSpecificErase() {
    Result out_result = {Status::FAILURE_UNKNOWN, ""};
    auto ret = service_->doOemSpecificErase([&](auto& result) { out_result = result; });
    return ResultToAStatus(out_result);
}

}  // namespace fastboot
}  // namespace hardware
}  // namespace android
}  // namespace aidl
