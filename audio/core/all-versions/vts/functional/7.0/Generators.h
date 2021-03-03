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

#include <vector>

#include "AudioTestDefinitions.h"

const std::vector<DeviceConfigParameter>& getOutputDeviceConfigParameters();
const std::vector<DeviceConfigParameter>& getOutputDeviceSingleConfigParameters();
const std::vector<DeviceConfigParameter>& getOutputDeviceInvalidConfigParameters(
        bool generateInvalidFlags = true);
const std::vector<DeviceConfigParameter>& getInputDeviceConfigParameters();
const std::vector<DeviceConfigParameter>& getInputDeviceSingleConfigParameters();
const std::vector<DeviceConfigParameter>& getInputDeviceInvalidConfigParameters(
        bool generateInvalidFlags = true);

// For unit tests
std::vector<DeviceConfigParameter> generateOutputDeviceConfigParameters(bool oneProfilePerDevice);
std::vector<DeviceConfigParameter> generateInputDeviceConfigParameters(bool oneProfilePerDevice);
