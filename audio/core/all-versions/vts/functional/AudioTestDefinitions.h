/*
 * Copyright (C) 2021 The Android Open Source Project
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

#pragma once

#include <string>
#include <tuple>
#include <variant>
#include <vector>

// clang-format off
#include PATH(android/hardware/audio/FILE_VERSION/types.h)
#include PATH(android/hardware/audio/common/FILE_VERSION/types.h)
// clang-format on

enum { PARAM_FACTORY_NAME, PARAM_DEVICE_NAME };
using DeviceParameter = std::tuple<std::string, std::string>;

// Nesting a tuple in another tuple allows to use GTest Combine function to generate
// all combinations of devices and configs.
#if MAJOR_VERSION <= 6
enum { PARAM_DEVICE, PARAM_CONFIG, PARAM_FLAGS };
enum { INDEX_INPUT, INDEX_OUTPUT };
using DeviceConfigParameter =
        std::tuple<DeviceParameter, android::hardware::audio::common::CPP_VERSION::AudioConfig,
                   std::variant<android::hardware::audio::common::CPP_VERSION::AudioInputFlag,
                                android::hardware::audio::common::CPP_VERSION::AudioOutputFlag>>;
#elif MAJOR_VERSION >= 7
enum { PARAM_DEVICE, PARAM_PORT_NAME, PARAM_CONFIG, PARAM_FLAGS };
using DeviceConfigParameter =
        std::tuple<DeviceParameter, std::string,
                   android::hardware::audio::common::CPP_VERSION::AudioConfig,
                   std::vector<android::hardware::audio::CPP_VERSION::AudioInOutFlag>>;
#endif
