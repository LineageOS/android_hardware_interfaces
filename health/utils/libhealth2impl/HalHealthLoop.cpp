/*
 * Copyright (C) 2019 The Android Open Source Project
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

#include <health2impl/HalHealthLoop.h>

#include <android-base/logging.h>
#include <hal_conversion.h>
#include <hidl/HidlTransportSupport.h>
#include <hwbinder/IPCThreadState.h>

#include <health2impl/Health.h>

using android::hardware::configureRpcThreadpool;
using android::hardware::handleTransportPoll;
using android::hardware::IPCThreadState;
using android::hardware::setupTransportPolling;

using android::hardware::health::V1_0::hal_conversion::convertFromHealthConfig;
using android::hardware::health::V2_0::Result;

namespace android {
namespace hardware {
namespace health {
namespace V2_1 {
namespace implementation {

void HalHealthLoop::Init(struct healthd_config* config) {
    // Retrieve healthd_config from the HAL.
    service_->getHealthConfig([config](auto res, const auto& health_config) {
        CHECK(res == Result::SUCCESS);

        convertFromHealthConfig(health_config.battery, config);
        config->boot_min_cap = health_config.bootMinCap;

        // Leave screen_on empty because it is handled in GetScreenOn below.

        // Leave ignorePowerSupplyNames empty because it isn't
        // used by clients of health HAL.
    });
}

void HalHealthLoop::Heartbeat(void) {
    // noop
}

void HalHealthLoop::ScheduleBatteryUpdate() {
    // ignore errors. impl may not be able to handle any callbacks, so
    // update() may return errors.
    Result res = service_->update();
    if (res != Result::SUCCESS) {
        LOG(WARNING) << "update() on the health HAL implementation failed with " << toString(res);
    }

    service_->getHealthInfo_2_1([this](auto res, const auto& health_info) {
        CHECK(res == Result::SUCCESS)
                << "getHealthInfo_2_1() on the health HAL implementation failed with "
                << toString(res);
        this->OnHealthInfoChanged(health_info);
    });
}

int HalHealthLoop::PrepareToWait() {
    return -1;
}

void HalHealthLoop::OnHealthInfoChanged(const HealthInfo& health_info) {
    set_charger_online(health_info);
    AdjustWakealarmPeriods(charger_online());
}

void HalHealthLoop::set_charger_online(const HealthInfo& health_info) {
    const auto& props = health_info.legacy.legacy;
    charger_online_ =
            props.chargerAcOnline || props.chargerUsbOnline || props.chargerWirelessOnline;
}

}  // namespace implementation
}  // namespace V2_1
}  // namespace health
}  // namespace hardware
}  // namespace android
