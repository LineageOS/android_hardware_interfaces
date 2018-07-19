/*
 * Copyright (C) 2018 The Android Open Source Project
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

#include <health2/Health.h>
#include <healthd/healthd.h>

using android::hardware::health::V2_0::IHealth;
using android::hardware::health::V2_0::implementation::Health;

static struct healthd_config gHealthdConfig = {
    .batteryStatusPath = android::String8(android::String8::kEmptyString),
    .batteryHealthPath = android::String8(android::String8::kEmptyString),
    .batteryPresentPath = android::String8(android::String8::kEmptyString),
    .batteryCapacityPath = android::String8(android::String8::kEmptyString),
    .batteryVoltagePath = android::String8(android::String8::kEmptyString),
    .batteryTemperaturePath = android::String8(android::String8::kEmptyString),
    .batteryTechnologyPath = android::String8(android::String8::kEmptyString),
    .batteryCurrentNowPath = android::String8(android::String8::kEmptyString),
    .batteryCurrentAvgPath = android::String8(android::String8::kEmptyString),
    .batteryChargeCounterPath = android::String8(android::String8::kEmptyString),
    .batteryFullChargePath = android::String8(android::String8::kEmptyString),
    .batteryCycleCountPath = android::String8(android::String8::kEmptyString),
    .energyCounter = nullptr,
    .boot_min_cap = 0,
    .screen_on = nullptr};

void healthd_board_init(struct healthd_config*) {
    // use defaults
}

int healthd_board_battery_update(struct android::BatteryProperties*) {
    // return 0 to log periodic polled battery status to kernel log
    return 0;
}

extern "C" IHealth* HIDL_FETCH_IHealth(const char* name) {
    const static std::string providedInstance{"backup"};

    if (providedInstance == name) {
        // use defaults
        // Health class stores static instance
        return Health::initInstance(&gHealthdConfig).get();
    }
    return nullptr;
}
