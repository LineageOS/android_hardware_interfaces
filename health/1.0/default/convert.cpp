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

#include "include/hal_conversion.h"

namespace android {
namespace hardware {
namespace health {
namespace V1_0 {
namespace hal_conversion {

void convertToHealthConfig(const struct healthd_config *hc, HealthConfig& config) {
    config.periodicChoresIntervalFast = hc->periodic_chores_interval_fast;
    config.periodicChoresIntervalSlow = hc->periodic_chores_interval_slow;

    config.batteryStatusPath = hc->batteryStatusPath.c_str();
    config.batteryHealthPath = hc->batteryHealthPath.c_str();
    config.batteryPresentPath = hc->batteryPresentPath.c_str();
    config.batteryCapacityPath = hc->batteryCapacityPath.c_str();
    config.batteryVoltagePath = hc->batteryVoltagePath.c_str();
    config.batteryTemperaturePath = hc->batteryTemperaturePath.c_str();
    config.batteryTechnologyPath = hc->batteryTechnologyPath.c_str();
    config.batteryCurrentNowPath = hc->batteryCurrentNowPath.c_str();
    config.batteryCurrentAvgPath = hc->batteryCurrentAvgPath.c_str();
    config.batteryChargeCounterPath = hc->batteryChargeCounterPath.c_str();
    config.batteryFullChargePath = hc->batteryFullChargePath.c_str();
    config.batteryCycleCountPath = hc->batteryCycleCountPath.c_str();
}

void convertFromHealthConfig(const HealthConfig& c, struct healthd_config *hc) {
    hc->periodic_chores_interval_fast = c.periodicChoresIntervalFast;
    hc->periodic_chores_interval_slow = c.periodicChoresIntervalSlow;

    hc->batteryStatusPath =
        android::String8(c.batteryStatusPath.c_str(),
                         c.batteryStatusPath.size());

    hc->batteryHealthPath =
        android::String8(c.batteryHealthPath.c_str(),
                         c.batteryHealthPath.size());

    hc->batteryPresentPath =
        android::String8(c.batteryPresentPath.c_str(),
                         c.batteryPresentPath.size());

    hc->batteryCapacityPath =
        android::String8(c.batteryCapacityPath.c_str(),
                         c.batteryCapacityPath.size());

    hc->batteryVoltagePath =
        android::String8(c.batteryVoltagePath.c_str(),
                         c.batteryVoltagePath.size());

    hc->batteryTemperaturePath =
        android::String8(c.batteryTemperaturePath.c_str(),
                         c.batteryTemperaturePath.size());

    hc->batteryTechnologyPath =
        android::String8(c.batteryTechnologyPath.c_str(),
                         c.batteryTechnologyPath.size());

    hc->batteryCurrentNowPath =
        android::String8(c.batteryCurrentNowPath.c_str(),
                         c.batteryCurrentNowPath.size());

    hc->batteryCurrentAvgPath =
        android::String8(c.batteryCurrentAvgPath.c_str(),
                         c.batteryCurrentAvgPath.size());

    hc->batteryChargeCounterPath =
        android::String8(c.batteryChargeCounterPath.c_str(),
                         c.batteryChargeCounterPath.size());

    hc->batteryFullChargePath =
        android::String8(c.batteryFullChargePath.c_str(),
                         c.batteryFullChargePath.size());

    hc->batteryCycleCountPath =
        android::String8(c.batteryCycleCountPath.c_str(),
                         c.batteryCycleCountPath.size());

    // energyCounter is handled through special means so all calls to
    // the function go across the HALs

    // boot_min_cap - never used in Android (only in charger-mode).

    // screen_on - never used in Android (only in charger mode).
}

void convertToHealthInfo(const struct android::BatteryProperties *p,
                                 HealthInfo& info) {
    info.chargerAcOnline        = p->chargerAcOnline;
    info.chargerUsbOnline       = p->chargerUsbOnline;
    info.chargerWirelessOnline  = p->chargerWirelessOnline;
    info.maxChargingCurrent     = p->maxChargingCurrent;
    info.maxChargingVoltage     = p->maxChargingVoltage;
    info.batteryStatus          = static_cast<BatteryStatus>(p->batteryStatus);
    info.batteryHealth          = static_cast<BatteryHealth>(p->batteryHealth);
    info.batteryPresent         = p->batteryPresent;
    info.batteryLevel           = p->batteryLevel;
    info.batteryVoltage         = p->batteryVoltage;
    info.batteryTemperature     = p->batteryTemperature;
    info.batteryCurrent         = p->batteryCurrent;
    info.batteryCycleCount      = p->batteryCycleCount;
    info.batteryFullCharge      = p->batteryFullCharge;
    info.batteryChargeCounter   = p->batteryChargeCounter;
    info.batteryTechnology      = p->batteryTechnology.c_str();
}

void convertFromHealthInfo(const HealthInfo& info,
                                   struct android::BatteryProperties *p) {
    p->chargerAcOnline          = info.chargerAcOnline;
    p->chargerUsbOnline         = info.chargerUsbOnline;
    p->chargerWirelessOnline    = info.chargerWirelessOnline;
    p->maxChargingCurrent       = info.maxChargingCurrent;
    p->maxChargingVoltage       = info.maxChargingVoltage;
    p->batteryStatus            = static_cast<int>(info.batteryStatus);
    p->batteryHealth            = static_cast<int>(info.batteryHealth);
    p->batteryPresent           = info.batteryPresent;
    p->batteryLevel             = info.batteryLevel;
    p->batteryVoltage           = info.batteryVoltage;
    p->batteryTemperature       = info.batteryTemperature;
    p->batteryCurrent           = info.batteryCurrent;
    p->batteryCycleCount        = info.batteryCycleCount;
    p->batteryFullCharge        = info.batteryFullCharge;
    p->batteryChargeCounter     = info.batteryChargeCounter;
    p->batteryTechnology        = android::String8(info.batteryTechnology.c_str());
}

} // namespace hal_conversion
} // namespace V1_0
} // namespace health
} // namespace hardware
} // namespace android
