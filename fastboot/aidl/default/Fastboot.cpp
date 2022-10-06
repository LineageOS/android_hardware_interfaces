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

#include "Fastboot.h"

using ndk::ScopedAStatus;

namespace aidl {
namespace android {
namespace hardware {
namespace fastboot {

ScopedAStatus Fastboot::getPartitionType(const std::string& in_partitionName,
                                         FileSystemType* _aidl_return) {
    if (in_partitionName.empty()) {
        return ScopedAStatus::fromExceptionCodeWithMessage(EX_ILLEGAL_ARGUMENT,
                                                           "Invalid partition name");
    }
    *_aidl_return = FileSystemType::RAW;
    return ScopedAStatus::ok();
}

ScopedAStatus Fastboot::doOemCommand(const std::string& in_oemCmd, std::string* _aidl_return) {
    *_aidl_return = "";
    if (in_oemCmd.empty()) {
        return ScopedAStatus::fromExceptionCodeWithMessage(EX_ILLEGAL_ARGUMENT, "Invalid command");
    }
    return ScopedAStatus::fromExceptionCodeWithMessage(
            EX_UNSUPPORTED_OPERATION, "Command not supported in default implementation");
}

ScopedAStatus Fastboot::getVariant(std::string* _aidl_return) {
    *_aidl_return = "NA";
    return ScopedAStatus::ok();
}

ScopedAStatus Fastboot::getOffModeChargeState(bool* _aidl_return) {
    *_aidl_return = false;
    return ScopedAStatus::ok();
}

ScopedAStatus Fastboot::getBatteryVoltageFlashingThreshold(int32_t* _aidl_return) {
    *_aidl_return = 0;
    return ScopedAStatus::ok();
}

ScopedAStatus Fastboot::doOemSpecificErase() {
    return ScopedAStatus::fromExceptionCodeWithMessage(
            EX_UNSUPPORTED_OPERATION, "Command not supported in default implementation");
}

}  // namespace fastboot
}  // namespace hardware
}  // namespace android
}  // namespace aidl
