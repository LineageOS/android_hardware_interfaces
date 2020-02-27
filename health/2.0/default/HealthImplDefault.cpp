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

void healthd_mode_default_impl_init(struct healthd_config*) {
    // noop
}

int healthd_mode_default_impl_preparetowait(void) {
    return -1;
}

void healthd_mode_default_impl_heartbeat(void) {
    // noop
}

void healthd_mode_default_impl_battery_update(struct android::BatteryProperties*) {
    // noop
}

static struct healthd_mode_ops healthd_mode_default_impl_ops = {
    .init = healthd_mode_default_impl_init,
    .preparetowait = healthd_mode_default_impl_preparetowait,
    .heartbeat = healthd_mode_default_impl_heartbeat,
    .battery_update = healthd_mode_default_impl_battery_update,
};

extern "C" IHealth* HIDL_FETCH_IHealth(const char* name) {
    const static std::string providedInstance{"backup"};
    healthd_mode_ops = &healthd_mode_default_impl_ops;
    if (providedInstance == name) {
        // use defaults
        // Health class stores static instance
        return Health::initInstance(&gHealthdConfig).get();
    }
    return nullptr;
}
