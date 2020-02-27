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

#include "VtsHalSensorsV2_XTargetTest.h"

TEST_P(SensorsHidlTest, SensorListDoesntContainInvalidType) {
    getSensors()->getSensorsList([&](const auto& list) {
        const size_t count = list.size();
        for (size_t i = 0; i < count; ++i) {
            const auto& s = list[i];
            EXPECT_FALSE(s.type == ::android::hardware::sensors::V2_1::SensorType::HINGE_ANGLE);
        }
    });
}

INSTANTIATE_TEST_SUITE_P(PerInstance, SensorsHidlTest,
                         testing::ValuesIn(android::hardware::getAllHalInstanceNames(
                                 android::hardware::sensors::V2_0::ISensors::descriptor)),
                         android::hardware::PrintInstanceNameToString);