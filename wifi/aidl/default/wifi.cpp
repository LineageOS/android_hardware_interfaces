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

#include "wifi.h"

#include <android-base/logging.h>

#include "aidl_return_util.h"
#include "aidl_sync_util.h"
#include "wifi_status_util.h"

namespace {
// Starting Chip ID, will be assigned to primary chip
static constexpr int32_t kPrimaryChipId = 0;
}  // namespace

namespace aidl {
namespace android {
namespace hardware {
namespace wifi {
using aidl_return_util::validateAndCall;
using aidl_return_util::validateAndCallWithLock;
using aidl_sync_util::acquireGlobalLock;

Wifi::Wifi(const std::shared_ptr<::android::wifi_system::InterfaceTool> iface_tool,
           const std::shared_ptr<legacy_hal::WifiLegacyHalFactory> legacy_hal_factory,
           const std::shared_ptr<mode_controller::WifiModeController> mode_controller,
           const std::shared_ptr<feature_flags::WifiFeatureFlags> feature_flags)
    : iface_tool_(iface_tool),
      legacy_hal_factory_(legacy_hal_factory),
      mode_controller_(mode_controller),
      feature_flags_(feature_flags),
      run_state_(RunState::STOPPED) {}

bool Wifi::isValid() {
    // This object is always valid.
    return true;
}

ndk::ScopedAStatus Wifi::registerEventCallback(
        const std::shared_ptr<IWifiEventCallback>& in_callback) {
    return validateAndCall(this, WifiStatusCode::ERROR_UNKNOWN,
                           &Wifi::registerEventCallbackInternal, in_callback);
}

ndk::ScopedAStatus Wifi::isStarted(bool* _aidl_return) {
    *_aidl_return = (run_state_ != RunState::STOPPED);
    return ndk::ScopedAStatus::ok();
}

ndk::ScopedAStatus Wifi::start() {
    return validateAndCall(this, WifiStatusCode::ERROR_UNKNOWN, &Wifi::startInternal);
}

ndk::ScopedAStatus Wifi::stop() {
    return validateAndCallWithLock(this, WifiStatusCode::ERROR_UNKNOWN, &Wifi::stopInternal);
}

ndk::ScopedAStatus Wifi::getChipIds(std::vector<int32_t>* _aidl_return) {
    return validateAndCall(this, WifiStatusCode::ERROR_UNKNOWN, &Wifi::getChipIdsInternal,
                           _aidl_return);
}

ndk::ScopedAStatus Wifi::getChip(int32_t in_chipId, std::shared_ptr<IWifiChip>* _aidl_return) {
    return validateAndCall(this, WifiStatusCode::ERROR_UNKNOWN, &Wifi::getChipInternal,
                           _aidl_return, in_chipId);
}

binder_status_t Wifi::dump(int fd, const char** args, uint32_t numArgs) {
    const auto lock = acquireGlobalLock();
    LOG(INFO) << "-----------Debug was called----------------";
    if (chips_.size() == 0) {
        LOG(INFO) << "No chips to display.";
        return STATUS_OK;
    }

    for (std::shared_ptr<WifiChip> chip : chips_) {
        if (!chip.get()) continue;
        chip->dump(fd, args, numArgs);
    }
    return STATUS_OK;
}

ndk::ScopedAStatus Wifi::registerEventCallbackInternal(
        const std::shared_ptr<IWifiEventCallback>& event_callback) {
    if (!event_cb_handler_.addCallback(event_callback)) {
        return createWifiStatus(WifiStatusCode::ERROR_UNKNOWN);
    }
    return ndk::ScopedAStatus::ok();
}

ndk::ScopedAStatus Wifi::startInternal() {
    if (run_state_ == RunState::STARTED) {
        return ndk::ScopedAStatus::ok();
    } else if (run_state_ == RunState::STOPPING) {
        return createWifiStatus(WifiStatusCode::ERROR_NOT_AVAILABLE, "HAL is stopping");
    }
    ndk::ScopedAStatus wifi_status = initializeModeControllerAndLegacyHal();
    if (wifi_status.isOk()) {
        // Register the callback for subsystem restart
        const auto& on_subsystem_restart_callback = [this](const std::string& error) {
            ndk::ScopedAStatus wifi_status = createWifiStatus(WifiStatusCode::ERROR_UNKNOWN, error);
            for (const auto& callback : event_cb_handler_.getCallbacks()) {
                LOG(INFO) << "Attempting to invoke onSubsystemRestart "
                             "callback";
                WifiStatusCode errorCode =
                        static_cast<WifiStatusCode>(wifi_status.getServiceSpecificError());
                if (!callback->onSubsystemRestart(errorCode).isOk()) {
                    LOG(ERROR) << "Failed to invoke onSubsystemRestart callback";
                } else {
                    LOG(INFO) << "Succeeded to invoke onSubsystemRestart "
                                 "callback";
                }
            }
        };

        // Create the chip instance once the HAL is started.
        int32_t chipId = kPrimaryChipId;
        for (auto& hal : legacy_hals_) {
            chips_.push_back(
                    WifiChip::create(chipId, chipId == kPrimaryChipId, hal, mode_controller_,
                                     std::make_shared<iface_util::WifiIfaceUtil>(iface_tool_, hal),
                                     feature_flags_, on_subsystem_restart_callback, false));
            chipId++;
        }
        run_state_ = RunState::STARTED;
        for (const auto& callback : event_cb_handler_.getCallbacks()) {
            if (!callback->onStart().isOk()) {
                LOG(ERROR) << "Failed to invoke onStart callback";
            };
        }
        LOG(INFO) << "Wifi HAL started";
    } else {
        for (const auto& callback : event_cb_handler_.getCallbacks()) {
            WifiStatusCode errorCode =
                    static_cast<WifiStatusCode>(wifi_status.getServiceSpecificError());
            if (!callback->onFailure(errorCode).isOk()) {
                LOG(ERROR) << "Failed to invoke onFailure callback";
            }
        }
        LOG(ERROR) << "Wifi HAL start failed";
        // Clear the event callback objects since the HAL start failed.
        event_cb_handler_.invalidate();
    }
    return wifi_status;
}

ndk::ScopedAStatus Wifi::stopInternal(
        /* NONNULL */ std::unique_lock<std::recursive_mutex>* lock) {
    if (run_state_ == RunState::STOPPED) {
        return ndk::ScopedAStatus::ok();
    } else if (run_state_ == RunState::STOPPING) {
        return createWifiStatus(WifiStatusCode::ERROR_NOT_AVAILABLE, "HAL is stopping");
    }
    // Clear the chip object and its child objects since the HAL is now
    // stopped.
    for (auto& chip : chips_) {
        if (chip.get()) {
            chip->invalidate();
            chip.reset();
        }
    }
    chips_.clear();
    ndk::ScopedAStatus wifi_status = stopLegacyHalAndDeinitializeModeController(lock);
    if (wifi_status.isOk()) {
        for (const auto& callback : event_cb_handler_.getCallbacks()) {
            if (!callback->onStop().isOk()) {
                LOG(ERROR) << "Failed to invoke onStop callback";
            };
        }
        LOG(INFO) << "Wifi HAL stopped";
    } else {
        for (const auto& callback : event_cb_handler_.getCallbacks()) {
            WifiStatusCode errorCode =
                    static_cast<WifiStatusCode>(wifi_status.getServiceSpecificError());
            if (!callback->onFailure(errorCode).isOk()) {
                LOG(ERROR) << "Failed to invoke onFailure callback";
            }
        }
        LOG(ERROR) << "Wifi HAL stop failed";
    }
    // Clear the event callback objects since the HAL is now stopped.
    event_cb_handler_.invalidate();
    return wifi_status;
}

std::pair<std::vector<int32_t>, ndk::ScopedAStatus> Wifi::getChipIdsInternal() {
    std::vector<int32_t> chip_ids;

    for (auto& chip : chips_) {
        int32_t chip_id = getChipIdFromWifiChip(chip);
        if (chip_id != INT32_MAX) chip_ids.emplace_back(chip_id);
    }
    return {std::move(chip_ids), ndk::ScopedAStatus::ok()};
}

std::pair<std::shared_ptr<IWifiChip>, ndk::ScopedAStatus> Wifi::getChipInternal(int32_t chip_id) {
    for (auto& chip : chips_) {
        int32_t cand_id = getChipIdFromWifiChip(chip);
        if ((cand_id != INT32_MAX) && (cand_id == chip_id)) return {chip, ndk::ScopedAStatus::ok()};
    }

    return {nullptr, createWifiStatus(WifiStatusCode::ERROR_INVALID_ARGS)};
}

ndk::ScopedAStatus Wifi::initializeModeControllerAndLegacyHal() {
    if (!mode_controller_->initialize()) {
        LOG(ERROR) << "Failed to initialize firmware mode controller";
        return createWifiStatus(WifiStatusCode::ERROR_UNKNOWN);
    }

    legacy_hals_ = legacy_hal_factory_->getHals();
    if (legacy_hals_.empty()) return createWifiStatus(WifiStatusCode::ERROR_UNKNOWN);
    int index = 0;  // for failure log
    for (auto& hal : legacy_hals_) {
        legacy_hal::wifi_error legacy_status = hal->initialize();
        if (legacy_status != legacy_hal::WIFI_SUCCESS) {
            // Currently WifiLegacyHal::initialize does not allocate extra mem,
            // only initializes the function table. If this changes, need to
            // implement WifiLegacyHal::deinitialize and deinitalize the
            // HALs already initialized
            LOG(ERROR) << "Failed to initialize legacy HAL index: " << index
                       << " error: " << legacyErrorToString(legacy_status);
            return createWifiStatusFromLegacyError(legacy_status);
        }
        index++;
    }
    return ndk::ScopedAStatus::ok();
}

ndk::ScopedAStatus Wifi::stopLegacyHalAndDeinitializeModeController(
        /* NONNULL */ std::unique_lock<std::recursive_mutex>* lock) {
    legacy_hal::wifi_error legacy_status = legacy_hal::WIFI_SUCCESS;
    int index = 0;

    run_state_ = RunState::STOPPING;
    for (auto& hal : legacy_hals_) {
        legacy_hal::wifi_error tmp = hal->stop(lock, [&]() {});
        if (tmp != legacy_hal::WIFI_SUCCESS) {
            LOG(ERROR) << "Failed to stop legacy HAL index: " << index
                       << " error: " << legacyErrorToString(legacy_status);
            legacy_status = tmp;
        }
        index++;
    }
    run_state_ = RunState::STOPPED;

    if (legacy_status != legacy_hal::WIFI_SUCCESS) {
        LOG(ERROR) << "One or more legacy HALs failed to stop";
        return createWifiStatusFromLegacyError(legacy_status);
    }
    if (!mode_controller_->deinitialize()) {
        LOG(ERROR) << "Failed to deinitialize firmware mode controller";
        return createWifiStatus(WifiStatusCode::ERROR_UNKNOWN);
    }
    return ndk::ScopedAStatus::ok();
}

int32_t Wifi::getChipIdFromWifiChip(std::shared_ptr<WifiChip>& chip) {
    int32_t chip_id = INT32_MAX;
    if (chip.get()) {
        ndk::ScopedAStatus status = chip->getId(&chip_id);
        if (!status.isOk()) {
            // Reset value if operation failed.
            chip_id = INT32_MAX;
        }
    }
    return chip_id;
}

}  // namespace wifi
}  // namespace hardware
}  // namespace android
}  // namespace aidl
