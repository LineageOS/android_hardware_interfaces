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

#include <gmock/gmock.h>
#include <gtest/gtest.h>
#include <vector>

#include "vhal_v2_0/DefaultConfig.h"

namespace android {
namespace hardware {
namespace automotive {
namespace vehicle {
namespace V2_0 {
namespace impl {

using ::testing::ElementsAreArray;

// Test that VHAL_SUPPORTED_PROPERTY_IDS contains all supported property IDs.
TEST(DefaultConfigSupportedPropertyIdsTest, testIncludeAllSupportedIds) {
    const int32_t vhalSupportedPropertyIdsPropId = 289476424;

    std::vector<int32_t> allSupportedIds;
    std::vector<int32_t> configuredSupportedIds;

    for (const auto& property : impl::kVehicleProperties) {
        int propId = property.config.prop;
        allSupportedIds.push_back(propId);

        if (propId == vhalSupportedPropertyIdsPropId) {
            configuredSupportedIds = property.initialValue.int32Values;
        }
    }

    ASSERT_THAT(allSupportedIds, ElementsAreArray(configuredSupportedIds));
}

}  // namespace impl
}  // namespace V2_0
}  // namespace vehicle
}  // namespace automotive
}  // namespace hardware
}  // namespace android
