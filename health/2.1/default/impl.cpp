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

#include <memory>
#include <string_view>

#include <health/utils.h>
#include <health2impl/Health.h>

using ::android::sp;
using ::android::hardware::health::InitHealthdConfig;
using ::android::hardware::health::V2_1::IHealth;
using ::android::hardware::health::V2_1::implementation::Health;

using namespace std::literals;

// Passthrough implementation of the health service. Use default configuration.
// It does not invoke callbacks unless update() is called explicitly. No
// background thread is spawned to handle callbacks.
//
// The passthrough implementation is only allowed in recovery mode, charger, and
// opened by the hwbinder service.
// If Android is booted normally, the hwbinder service is used instead.
//
// This implementation only implements the "default" instance. It rejects
// other instance names.
// Note that the Android framework only reads values from the "default"
// health HAL 2.1 instance.
extern "C" IHealth* HIDL_FETCH_IHealth(const char* instance) {
    if (instance != "default"sv) {
        return nullptr;
    }
    auto config = std::make_unique<healthd_config>();
    InitHealthdConfig(config.get());

    // This implementation uses default config. If you want to customize it
    // (e.g. with healthd_board_init), do it here.

    return new Health(std::move(config));
}
