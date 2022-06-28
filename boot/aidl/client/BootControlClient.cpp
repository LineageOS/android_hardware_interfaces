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

#include <BootControlClient.h>

#include <aidl/android/hardware/boot/IBootControl.h>
#include <android-base/logging.h>
#include <android/binder_manager.h>
#include <android/hardware/boot/1.0/IBootControl.h>
#include <android/hardware/boot/1.1/IBootControl.h>
#include <android/hardware/boot/1.2/IBootControl.h>
#include "utils/StrongPointer.h"

#define CONCAT(x, y) x##y

#define LOG_NDK_STATUS(x)                                                                   \
    do {                                                                                    \
        const auto CONCAT(status, __COUNTER__) = x;                                         \
        if (!CONCAT(status, __COUNTER__).isOk()) {                                          \
            LOG(ERROR) << #x << " failed " << CONCAT(status, __COUNTER__).getDescription(); \
        }                                                                                   \
    } while (0)

using aidl::android::hardware::boot::MergeStatus;

std::ostream& operator<<(std::ostream& os, MergeStatus status) {
    switch (status) {
        case MergeStatus::NONE:
            os << "MergeStatus::NONE";
            break;
        case MergeStatus::UNKNOWN:
            os << "MergeStatus::UNKNOWN";
            break;
        case MergeStatus::SNAPSHOTTED:
            os << "MergeStatus::SNAPSHOTTED";
            break;
        case MergeStatus::MERGING:
            os << "MergeStatus::MERGING";
            break;
        case MergeStatus::CANCELLED:
            os << "MergeStatus::CANCELLED";
            break;
        default:
            os << static_cast<int>(status);
            break;
    }
    return os;
}

namespace android::hal {
class BootControlClientAidl final : public BootControlClient {
    using IBootControl = ::aidl::android::hardware::boot::IBootControl;

  public:
    BootControlClientAidl(std::shared_ptr<IBootControl> module) : module_(module) {}

    BootControlVersion GetVersion() const override { return BootControlVersion::BOOTCTL_AIDL; }

    ~BootControlClientAidl() = default;
    virtual int32_t GetNumSlots() const {
        int32_t ret = -1;
        LOG_NDK_STATUS(module_->getNumberSlots(&ret));
        return ret;
    }

    int32_t GetCurrentSlot() const {
        int32_t ret = -1;
        LOG_NDK_STATUS(module_->getCurrentSlot(&ret));
        return ret;
    }
    MergeStatus getSnapshotMergeStatus() const {
        MergeStatus status = MergeStatus::UNKNOWN;
        LOG_NDK_STATUS(module_->getSnapshotMergeStatus(&status));
        return status;
    }
    std::string GetSuffix(int32_t slot) const {
        std::string ret;
        const auto status = module_->getSuffix(slot, &ret);
        if (!status.isOk()) {
            LOG(ERROR) << __FUNCTION__ << "(" << slot << ")"
                       << " failed " << status.getDescription();
            return {};
        }
        return ret;
    }

    std::optional<bool> IsSlotBootable(int32_t slot) const {
        bool ret = false;
        const auto status = module_->isSlotBootable(slot, &ret);
        if (!status.isOk()) {
            LOG(ERROR) << __FUNCTION__ << "(" << slot << ")"
                       << " failed " << status.getDescription();
            return {};
        }
        return ret;
    }

    CommandResult MarkSlotUnbootable(int32_t slot) {
        const auto status = module_->setSlotAsUnbootable(slot);
        if (!status.isOk()) {
            LOG(ERROR) << __FUNCTION__ << "(" << slot << ")"
                       << " failed " << status.getDescription();
        }
        return {.success = status.isOk(), .errMsg = status.getDescription()};
    }

    CommandResult SetActiveBootSlot(int slot) {
        const auto status = module_->setActiveBootSlot(slot);
        if (!status.isOk()) {
            LOG(ERROR) << __FUNCTION__ << "(" << slot << ")"
                       << " failed " << status.getDescription();
        }
        return {.success = status.isOk(), .errMsg = status.getDescription()};
    }
    int GetActiveBootSlot() const {
        int ret = -1;
        LOG_NDK_STATUS(module_->getActiveBootSlot(&ret));
        return ret;
    }

    // Check if |slot| is marked boot successfully.
    std::optional<bool> IsSlotMarkedSuccessful(int slot) const {
        bool ret = false;
        const auto status = module_->isSlotMarkedSuccessful(slot, &ret);
        if (!status.isOk()) {
            LOG(ERROR) << __FUNCTION__ << "(" << slot << ")"
                       << " failed " << status.getDescription();
            return {};
        }
        return ret;
    }

    CommandResult MarkBootSuccessful() {
        const auto status = module_->markBootSuccessful();
        if (!status.isOk()) {
            LOG(ERROR) << __FUNCTION__ << " failed " << status.getDescription();
        }
        return {.success = status.isOk(), .errMsg = status.getDescription()};
    }

    CommandResult SetSnapshotMergeStatus(aidl::android::hardware::boot::MergeStatus merge_status) {
        const auto status = module_->setSnapshotMergeStatus(merge_status);
        if (!status.isOk()) {
            LOG(ERROR) << __FUNCTION__ << "(" << merge_status << ")"
                       << " failed " << status.getDescription();
        }
        return {.success = status.isOk(), .errMsg = status.getDescription()};
    }

  private:
    const std::shared_ptr<IBootControl> module_;
};

using namespace android::hardware::boot;

class BootControlClientHIDL final : public BootControlClient {
  public:
    BootControlClientHIDL(android::sp<V1_0::IBootControl> module_v1,
                          android::sp<V1_1::IBootControl> module_v1_1,
                          android::sp<V1_2::IBootControl> module_v1_2)
        : module_v1_(module_v1), module_v1_1_(module_v1_1), module_v1_2_(module_v1_2) {
        CHECK(module_v1_ != nullptr);
    }
    BootControlVersion GetVersion() const override {
        if (module_v1_2_ != nullptr) {
            return BootControlVersion::BOOTCTL_V1_2;
        } else if (module_v1_1_ != nullptr) {
            return BootControlVersion::BOOTCTL_V1_1;
        } else {
            return BootControlVersion::BOOTCTL_V1_0;
        }
    }
    int32_t GetNumSlots() const {
        const auto ret = module_v1_->getNumberSlots();
        if (!ret.isOk()) {
            LOG(ERROR) << __FUNCTION__ << " failed " << ret.description();
        }
        return ret.withDefault(-1);
    }

    int32_t GetCurrentSlot() const {
        const auto ret = module_v1_->getCurrentSlot();
        if (!ret.isOk()) {
            LOG(ERROR) << __FUNCTION__ << " failed " << ret.description();
        }
        return ret.withDefault(-1);
    }

    std::string GetSuffix(int32_t slot) const {
        std::string suffix;
        const auto ret = module_v1_->getSuffix(
                slot,
                [&](const ::android::hardware::hidl_string& slotSuffix) { suffix = slotSuffix; });
        if (!ret.isOk()) {
            LOG(ERROR) << __FUNCTION__ << "(" << slot << ")"
                       << " failed " << ret.description();
        }
        return suffix;
    }

    std::optional<bool> IsSlotBootable(int32_t slot) const {
        const auto ret = module_v1_->isSlotBootable(slot);
        if (!ret.isOk()) {
            LOG(ERROR) << __FUNCTION__ << "(" << slot << ")"
                       << " failed " << ret.description();
            return {};
        }
        const auto bool_result = ret.withDefault(V1_0::BoolResult::INVALID_SLOT);
        if (bool_result == V1_0::BoolResult::INVALID_SLOT) {
            return {};
        }
        return bool_result == V1_0::BoolResult::TRUE;
    }

    CommandResult MarkSlotUnbootable(int32_t slot) {
        CommandResult result;
        const auto ret =
                module_v1_->setSlotAsUnbootable(slot, [&](const V1_0::CommandResult& error) {
                    result.success = error.success;
                    result.errMsg = error.errMsg;
                });
        if (!ret.isOk()) {
            LOG(ERROR) << __FUNCTION__ << "(" << slot << ")"
                       << " failed " << ret.description();
        }
        return result;
    }

    CommandResult SetActiveBootSlot(int32_t slot) {
        CommandResult result;
        const auto ret = module_v1_->setActiveBootSlot(slot, [&](const V1_0::CommandResult& error) {
            result.success = error.success;
            result.errMsg = error.errMsg;
        });
        if (!ret.isOk()) {
            LOG(ERROR) << __FUNCTION__ << "(" << slot << ")"
                       << " failed " << ret.description();
        }
        return result;
    }

    CommandResult MarkBootSuccessful() {
        CommandResult result;
        const auto ret = module_v1_->markBootSuccessful([&](const V1_0::CommandResult& error) {
            result.success = error.success;
            result.errMsg = error.errMsg;
        });
        if (!ret.isOk()) {
            LOG(ERROR) << __FUNCTION__ << " failed " << ret.description();
        }
        return result;
    }

    std::optional<bool> IsSlotMarkedSuccessful(int32_t slot) const {
        const auto ret = module_v1_->isSlotMarkedSuccessful(slot);
        if (!ret.isOk()) {
            LOG(ERROR) << __FUNCTION__ << "(" << slot << ")"
                       << " failed " << ret.description();
            return {};
        }
        const auto bool_result = ret.withDefault(V1_0::BoolResult::INVALID_SLOT);
        if (bool_result == V1_0::BoolResult::INVALID_SLOT) {
            return {};
        }
        return bool_result == V1_0::BoolResult::TRUE;
    }

    MergeStatus getSnapshotMergeStatus() const {
        if (module_v1_1_ == nullptr) {
            LOG(ERROR) << __FUNCTION__ << " is unsupported, requires at least boot v1.1";
            return MergeStatus::UNKNOWN;
        }
        const auto ret = module_v1_1_->getSnapshotMergeStatus();
        if (!ret.isOk()) {
            LOG(ERROR) << __FUNCTION__ << " failed " << ret.description();
        }
        return static_cast<MergeStatus>(
                ret.withDefault(static_cast<V1_1::MergeStatus>(MergeStatus::UNKNOWN)));
    }

    CommandResult SetSnapshotMergeStatus(MergeStatus merge_status) {
        if (module_v1_1_ == nullptr) {
            return {.success = false,
                    .errMsg = "setSnapshotMergeStatus is unsupported, requires at least boot v1.1"};
        }
        const auto ret =
                module_v1_1_->setSnapshotMergeStatus(static_cast<V1_1::MergeStatus>(merge_status));
        if (!ret.isOk()) {
            LOG(ERROR) << __FUNCTION__ << "(" << merge_status << ")"
                       << " failed " << ret.description();
        }
        return {.success = ret.isOk(), .errMsg = ret.description()};
    }

    int32_t GetActiveBootSlot() const {
        if (module_v1_2_ == nullptr) {
            LOG(ERROR) << __FUNCTION__ << " is unsupported, requires at least boot v1.2";
            return -1;
        }
        const auto ret = module_v1_2_->getActiveBootSlot();
        if (!ret.isOk()) {
            LOG(ERROR) << __FUNCTION__ << " failed " << ret.description();
        }
        return ret.withDefault(-1);
    }

  private:
    android::sp<V1_0::IBootControl> module_v1_;
    android::sp<V1_1::IBootControl> module_v1_1_;
    android::sp<V1_2::IBootControl> module_v1_2_;
};

std::unique_ptr<BootControlClient> BootControlClient::WaitForService() {
    const auto instance_name =
            std::string(::aidl::android::hardware::boot::IBootControl::descriptor) + "/default";

    if (AServiceManager_isDeclared(instance_name.c_str())) {
        auto module = ::aidl::android::hardware::boot::IBootControl::fromBinder(
                ndk::SpAIBinder(AServiceManager_waitForService(instance_name.c_str())));
        if (module == nullptr) {
            LOG(ERROR) << "AIDL " << instance_name
                       << " is declared but waitForService returned nullptr.";
            return nullptr;
        }
        LOG(INFO) << "Using AIDL version of IBootControl";
        return std::make_unique<BootControlClientAidl>(module);
    }
    LOG(INFO) << "AIDL IBootControl not available, falling back to HIDL.";

    android::sp<V1_0::IBootControl> v1_0_module;
    android::sp<V1_1::IBootControl> v1_1_module;
    android::sp<V1_2::IBootControl> v1_2_module;
    v1_0_module = V1_0::IBootControl::getService();
    if (v1_0_module == nullptr) {
        LOG(ERROR) << "Error getting bootctrl v1.0 module.";
        return nullptr;
    }
    v1_1_module = V1_1::IBootControl::castFrom(v1_0_module);
    v1_2_module = V1_2::IBootControl::castFrom(v1_0_module);
    if (v1_2_module != nullptr) {
        LOG(INFO) << "Using HIDL version 1.2 of IBootControl";
    } else if (v1_1_module != nullptr) {
        LOG(INFO) << "Using HIDL version 1.1 of IBootControl";
    } else {
        LOG(INFO) << "Using HIDL version 1.0 of IBootControl";
    }

    return std::make_unique<BootControlClientHIDL>(v1_0_module, v1_1_module, v1_2_module);
}

}  // namespace android::hal
