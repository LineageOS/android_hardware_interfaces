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
#define LOG_TAG "android.hardware.boot@1.2-impl"

#include <memory>

#include <log/log.h>

#include "BootControl.h"

namespace android {
namespace hardware {
namespace boot {
namespace V1_2 {
namespace implementation {

using ::android::hardware::boot::V1_0::CommandResult;

bool BootControl::Init() {
    return impl_.Init();
}

// Methods from ::android::hardware::boot::V1_0::IBootControl.
Return<uint32_t> BootControl::getNumberSlots() {
    return impl_.GetNumberSlots();
}

Return<uint32_t> BootControl::getCurrentSlot() {
    return impl_.GetCurrentSlot();
}

Return<void> BootControl::markBootSuccessful(markBootSuccessful_cb _hidl_cb) {
    struct CommandResult cr;
    if (impl_.MarkBootSuccessful()) {
        cr.success = true;
        cr.errMsg = "Success";
    } else {
        cr.success = false;
        cr.errMsg = "Operation failed";
    }
    _hidl_cb(cr);
    return Void();
}

Return<void> BootControl::setActiveBootSlot(uint32_t slot, setActiveBootSlot_cb _hidl_cb) {
    struct CommandResult cr;
    if (impl_.SetActiveBootSlot(slot)) {
        cr.success = true;
        cr.errMsg = "Success";
    } else {
        cr.success = false;
        cr.errMsg = "Operation failed";
    }
    _hidl_cb(cr);
    return Void();
}

Return<void> BootControl::setSlotAsUnbootable(uint32_t slot, setSlotAsUnbootable_cb _hidl_cb) {
    struct CommandResult cr;
    if (impl_.SetSlotAsUnbootable(slot)) {
        cr.success = true;
        cr.errMsg = "Success";
    } else {
        cr.success = false;
        cr.errMsg = "Operation failed";
    }
    _hidl_cb(cr);
    return Void();
}

Return<BoolResult> BootControl::isSlotBootable(uint32_t slot) {
    if (!impl_.IsValidSlot(slot)) {
        return BoolResult::INVALID_SLOT;
    }
    return impl_.IsSlotBootable(slot) ? BoolResult::TRUE : BoolResult::FALSE;
}

Return<BoolResult> BootControl::isSlotMarkedSuccessful(uint32_t slot) {
    if (!impl_.IsValidSlot(slot)) {
        return BoolResult::INVALID_SLOT;
    }
    return impl_.IsSlotMarkedSuccessful(slot) ? BoolResult::TRUE : BoolResult::FALSE;
}

Return<void> BootControl::getSuffix(uint32_t slot, getSuffix_cb _hidl_cb) {
    hidl_string ans;
    const char* suffix = impl_.GetSuffix(slot);
    if (suffix) {
        ans = suffix;
    }
    _hidl_cb(ans);
    return Void();
}

// Methods from ::android::hardware::boot::V1_1::IBootControl.
Return<bool> BootControl::setSnapshotMergeStatus(MergeStatus status) {
    return impl_.SetSnapshotMergeStatus(status);
}

Return<MergeStatus> BootControl::getSnapshotMergeStatus() {
    return impl_.GetSnapshotMergeStatus();
}

// Methods from ::android::hardware::boot::V1_2::IBootControl.
Return<uint32_t> BootControl::getActiveBootSlot() {
    return impl_.GetActiveBootSlot();
}

IBootControl* HIDL_FETCH_IBootControl(const char* /* hal */) {
    auto module = std::make_unique<BootControl>();
    if (!module->Init()) {
        ALOGE("Could not initialize BootControl module");
        return nullptr;
    }
    return module.release();
}

}  // namespace implementation
}  // namespace V1_2
}  // namespace boot
}  // namespace hardware
}  // namespace android
