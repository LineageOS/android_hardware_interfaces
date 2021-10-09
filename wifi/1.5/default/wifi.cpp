/*
 * Copyright (C) 2016 The Android Open Source Project
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

#include <android-base/logging.h>

#include "hidl_return_util.h"
#include "wifi.h"
#include "wifi_status_util.h"

namespace {
// Starting Chip ID, will be assigned to primary chip
static constexpr android::hardware::wifi::V1_0::ChipId kPrimaryChipId = 0;
}  // namespace

namespace android {
namespace hardware {
namespace wifi {
namespace V1_5 {
namespace implementation {
using hidl_return_util::validateAndCall;
using hidl_return_util::validateAndCallWithLock;

Wifi::Wifi(
    const std::shared_ptr<wifi_system::InterfaceTool> iface_tool,
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

Return<void> Wifi::registerEventCallback(
    const sp<V1_0::IWifiEventCallback>& event_callback,
    registerEventCallback_cb hidl_status_cb) {
    return validateAndCall(this, WifiStatusCode::ERROR_UNKNOWN,
                           &Wifi::registerEventCallbackInternal, hidl_status_cb,
                           event_callback);
}

Return<void> Wifi::registerEventCallback_1_5(
    const sp<V1_5::IWifiEventCallback>& event_callback,
    registerEventCallback_1_5_cb hidl_status_cb) {
    return validateAndCall(this, WifiStatusCode::ERROR_UNKNOWN,
                           &Wifi::registerEventCallbackInternal_1_5,
                           hidl_status_cb, event_callback);
}

Return<bool> Wifi::isStarted() { return run_state_ != RunState::STOPPED; }

Return<void> Wifi::start(start_cb hidl_status_cb) {
    return validateAndCall(this, WifiStatusCode::ERROR_UNKNOWN,
                           &Wifi::startInternal, hidl_status_cb);
}

Return<void> Wifi::stop(stop_cb hidl_status_cb) {
    return validateAndCallWithLock(this, WifiStatusCode::ERROR_UNKNOWN,
                                   &Wifi::stopInternal, hidl_status_cb);
}

Return<void> Wifi::getChipIds(getChipIds_cb hidl_status_cb) {
    return validateAndCall(this, WifiStatusCode::ERROR_UNKNOWN,
                           &Wifi::getChipIdsInternal, hidl_status_cb);
}

Return<void> Wifi::getChip(ChipId chip_id, getChip_cb hidl_status_cb) {
    return validateAndCall(this, WifiStatusCode::ERROR_UNKNOWN,
                           &Wifi::getChipInternal, hidl_status_cb, chip_id);
}

Return<void> Wifi::debug(const hidl_handle& handle,
                         const hidl_vec<hidl_string>&) {
    LOG(INFO) << "-----------Debug is called----------------";
    if (chips_.size() == 0) {
        return Void();
    }

    for (sp<WifiChip> chip : chips_) {
        if (!chip.get()) continue;

        chip->debug(handle, {});
    }
    return Void();
}

WifiStatus Wifi::registerEventCallbackInternal(
    const sp<V1_0::IWifiEventCallback>& event_callback __unused) {
    // Deprecated support for this callback.
    return createWifiStatus(WifiStatusCode::ERROR_NOT_SUPPORTED);
}

WifiStatus Wifi::registerEventCallbackInternal_1_5(
    const sp<V1_5::IWifiEventCallback>& event_callback) {
    if (!event_cb_handler_.addCallback(event_callback)) {
        return createWifiStatus(WifiStatusCode::ERROR_UNKNOWN);
    }
    return createWifiStatus(WifiStatusCode::SUCCESS);
}

WifiStatus Wifi::startInternal() {
    if (run_state_ == RunState::STARTED) {
        return createWifiStatus(WifiStatusCode::SUCCESS);
    } else if (run_state_ == RunState::STOPPING) {
        return createWifiStatus(WifiStatusCode::ERROR_NOT_AVAILABLE,
                                "HAL is stopping");
    }
    WifiStatus wifi_status = initializeModeControllerAndLegacyHal();
    if (wifi_status.code == WifiStatusCode::SUCCESS) {
        // Register the callback for subsystem restart
        const auto& on_subsystem_restart_callback =
            [this](const std::string& error) {
                WifiStatus wifi_status =
                    createWifiStatus(WifiStatusCode::ERROR_UNKNOWN, error);
                for (const auto& callback : event_cb_handler_.getCallbacks()) {
                    LOG(INFO) << "Attempting to invoke onSubsystemRestart "
                                 "callback";
                    if (!callback->onSubsystemRestart(wifi_status).isOk()) {
                        LOG(ERROR)
                            << "Failed to invoke onSubsystemRestart callback";
                    } else {
                        LOG(INFO) << "Succeeded to invoke onSubsystemRestart "
                                     "callback";
                    }
                }
            };

        // Create the chip instance once the HAL is started.
        android::hardware::wifi::V1_0::ChipId chipId = kPrimaryChipId;
        for (auto& hal : legacy_hals_) {
            chips_.push_back(new WifiChip(
                chipId, chipId == kPrimaryChipId, hal, mode_controller_,
                std::make_shared<iface_util::WifiIfaceUtil>(iface_tool_, hal),
                feature_flags_, on_subsystem_restart_callback));
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
            if (!callback->onFailure(wifi_status).isOk()) {
                LOG(ERROR) << "Failed to invoke onFailure callback";
            }
        }
        LOG(ERROR) << "Wifi HAL start failed";
        // Clear the event callback objects since the HAL start failed.
        event_cb_handler_.invalidate();
    }
    return wifi_status;
}

WifiStatus Wifi::stopInternal(
    /* NONNULL */ std::unique_lock<std::recursive_mutex>* lock) {
    if (run_state_ == RunState::STOPPED) {
        return createWifiStatus(WifiStatusCode::SUCCESS);
    } else if (run_state_ == RunState::STOPPING) {
        return createWifiStatus(WifiStatusCode::ERROR_NOT_AVAILABLE,
                                "HAL is stopping");
    }
    // Clear the chip object and its child objects since the HAL is now
    // stopped.
    for (auto& chip : chips_) {
        if (chip.get()) {
            chip->invalidate();
            chip.clear();
        }
    }
    chips_.clear();
    WifiStatus wifi_status = stopLegacyHalAndDeinitializeModeController(lock);
    if (wifi_status.code == WifiStatusCode::SUCCESS) {
        for (const auto& callback : event_cb_handler_.getCallbacks()) {
            if (!callback->onStop().isOk()) {
                LOG(ERROR) << "Failed to invoke onStop callback";
            };
        }
        LOG(INFO) << "Wifi HAL stopped";
    } else {
        for (const auto& callback : event_cb_handler_.getCallbacks()) {
            if (!callback->onFailure(wifi_status).isOk()) {
                LOG(ERROR) << "Failed to invoke onFailure callback";
            }
        }
        LOG(ERROR) << "Wifi HAL stop failed";
    }
    // Clear the event callback objects since the HAL is now stopped.
    event_cb_handler_.invalidate();
    return wifi_status;
}

std::pair<WifiStatus, std::vector<ChipId>> Wifi::getChipIdsInternal() {
    std::vector<ChipId> chip_ids;

    for (auto& chip : chips_) {
        ChipId chip_id = getChipIdFromWifiChip(chip);
        if (chip_id != UINT32_MAX) chip_ids.emplace_back(chip_id);
    }
    return {createWifiStatus(WifiStatusCode::SUCCESS), std::move(chip_ids)};
}

std::pair<WifiStatus, sp<V1_4::IWifiChip>> Wifi::getChipInternal(
    ChipId chip_id) {
    for (auto& chip : chips_) {
        ChipId cand_id = getChipIdFromWifiChip(chip);
        if ((cand_id != UINT32_MAX) && (cand_id == chip_id))
            return {createWifiStatus(WifiStatusCode::SUCCESS), chip};
    }

    return {createWifiStatus(WifiStatusCode::ERROR_INVALID_ARGS), nullptr};
}

WifiStatus Wifi::initializeModeControllerAndLegacyHal() {
    if (!mode_controller_->initialize()) {
        LOG(ERROR) << "Failed to initialize firmware mode controller";
        return createWifiStatus(WifiStatusCode::ERROR_UNKNOWN);
    }

    legacy_hals_ = legacy_hal_factory_->getHals();
    if (legacy_hals_.empty())
        return createWifiStatus(WifiStatusCode::ERROR_UNKNOWN);
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
    return createWifiStatus(WifiStatusCode::SUCCESS);
}

WifiStatus Wifi::stopLegacyHalAndDeinitializeModeController(
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
    return createWifiStatus(WifiStatusCode::SUCCESS);
}

ChipId Wifi::getChipIdFromWifiChip(sp<WifiChip>& chip) {
    ChipId chip_id = UINT32_MAX;
    if (chip.get()) {
        chip->getId([&](WifiStatus status, uint32_t id) {
            if (status.code == WifiStatusCode::SUCCESS) {
                chip_id = id;
            }
        });
    }

    return chip_id;
}
}  // namespace implementation
}  // namespace V1_5
}  // namespace wifi
}  // namespace hardware
}  // namespace android
