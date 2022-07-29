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

#include "BootControl.h"
#include <cstdint>

#include <android-base/logging.h>

using HIDLMergeStatus = ::android::bootable::BootControl::MergeStatus;
using ndk::ScopedAStatus;

namespace aidl::android::hardware::boot {

BootControl::BootControl() {
    CHECK(impl_.Init());
}

ScopedAStatus BootControl::getActiveBootSlot(int32_t* _aidl_return) {
    *_aidl_return = impl_.GetActiveBootSlot();
    return ScopedAStatus::ok();
}

ScopedAStatus BootControl::getCurrentSlot(int32_t* _aidl_return) {
    *_aidl_return = impl_.GetCurrentSlot();
    return ScopedAStatus::ok();
}

ScopedAStatus BootControl::getNumberSlots(int32_t* _aidl_return) {
    *_aidl_return = impl_.GetNumberSlots();
    return ScopedAStatus::ok();
}

namespace {

static constexpr MergeStatus ToAIDLMergeStatus(HIDLMergeStatus status) {
    switch (status) {
        case HIDLMergeStatus::NONE:
            return MergeStatus::NONE;
        case HIDLMergeStatus::UNKNOWN:
            return MergeStatus::UNKNOWN;
        case HIDLMergeStatus::SNAPSHOTTED:
            return MergeStatus::SNAPSHOTTED;
        case HIDLMergeStatus::MERGING:
            return MergeStatus::MERGING;
        case HIDLMergeStatus::CANCELLED:
            return MergeStatus::CANCELLED;
    }
}

static constexpr HIDLMergeStatus ToHIDLMergeStatus(MergeStatus status) {
    switch (status) {
        case MergeStatus::NONE:
            return HIDLMergeStatus::NONE;
        case MergeStatus::UNKNOWN:
            return HIDLMergeStatus::UNKNOWN;
        case MergeStatus::SNAPSHOTTED:
            return HIDLMergeStatus::SNAPSHOTTED;
        case MergeStatus::MERGING:
            return HIDLMergeStatus::MERGING;
        case MergeStatus::CANCELLED:
            return HIDLMergeStatus::CANCELLED;
    }
}

}

ScopedAStatus BootControl::getSnapshotMergeStatus(MergeStatus* _aidl_return) {
    *_aidl_return = ToAIDLMergeStatus(impl_.GetSnapshotMergeStatus());
    return ScopedAStatus::ok();
}

ScopedAStatus BootControl::getSuffix(int32_t in_slot, std::string* _aidl_return) {
    if (!impl_.IsValidSlot(in_slot)) {
        // Old HIDL hal returns empty string for invalid slots. We should maintain this behavior in
        // AIDL for compatibility.
        _aidl_return->clear();
    } else {
        *_aidl_return = impl_.GetSuffix(in_slot);
    }
    return ScopedAStatus::ok();
}

ScopedAStatus BootControl::isSlotBootable(int32_t in_slot, bool* _aidl_return) {
    if (!impl_.IsValidSlot(in_slot)) {
        return ScopedAStatus::fromServiceSpecificErrorWithMessage(
                INVALID_SLOT, (std::string("Invalid slot ") + std::to_string(in_slot)).c_str());
    }
    *_aidl_return = impl_.IsSlotBootable(in_slot);
    return ScopedAStatus::ok();
}

ScopedAStatus BootControl::isSlotMarkedSuccessful(int32_t in_slot, bool* _aidl_return) {
    if (!impl_.IsValidSlot(in_slot)) {
        return ScopedAStatus::fromServiceSpecificErrorWithMessage(
                INVALID_SLOT, (std::string("Invalid slot ") + std::to_string(in_slot)).c_str());
    }
    *_aidl_return = impl_.IsSlotMarkedSuccessful(in_slot);
    return ScopedAStatus::ok();
}

ScopedAStatus BootControl::markBootSuccessful() {
    if (!impl_.MarkBootSuccessful()) {
        return ScopedAStatus::fromServiceSpecificErrorWithMessage(COMMAND_FAILED,
                                                                  "Operation failed");
    }
    return ScopedAStatus::ok();
}

ScopedAStatus BootControl::setActiveBootSlot(int32_t in_slot) {
    if (!impl_.IsValidSlot(in_slot)) {
        return ScopedAStatus::fromServiceSpecificErrorWithMessage(
                INVALID_SLOT, (std::string("Invalid slot ") + std::to_string(in_slot)).c_str());
    }
    if (!impl_.SetActiveBootSlot(in_slot)) {
        return ScopedAStatus::fromServiceSpecificErrorWithMessage(COMMAND_FAILED,
                                                                  "Operation failed");
    }
    return ScopedAStatus::ok();
}

ScopedAStatus BootControl::setSlotAsUnbootable(int32_t in_slot) {
    if (!impl_.IsValidSlot(in_slot)) {
        return ScopedAStatus::fromServiceSpecificErrorWithMessage(
                INVALID_SLOT, (std::string("Invalid slot ") + std::to_string(in_slot)).c_str());
    }
    if (!impl_.SetSlotAsUnbootable(in_slot)) {
        return ScopedAStatus::fromServiceSpecificErrorWithMessage(COMMAND_FAILED,
                                                                  "Operation failed");
    }
    return ScopedAStatus::ok();
}

ScopedAStatus BootControl::setSnapshotMergeStatus(MergeStatus in_status) {
    if (!impl_.SetSnapshotMergeStatus(ToHIDLMergeStatus(in_status))) {
        return ScopedAStatus::fromServiceSpecificErrorWithMessage(COMMAND_FAILED,
                                                                  "Operation failed");
    }
    return ScopedAStatus::ok();
}

}  // namespace aidl::android::hardware::boot
