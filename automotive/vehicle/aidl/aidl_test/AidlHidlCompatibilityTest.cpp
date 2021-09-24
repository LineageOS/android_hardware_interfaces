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

#include <VehicleHalTypes.h>
#include <VehicleUtils.h>

#include <android/binder_enums.h>
#include <android/hardware/automotive/vehicle/2.0/types.h>
#include <gtest/gtest.h>
#include <hidl/HidlSupport.h>

namespace hidl_vehicle = ::android::hardware::automotive::vehicle::V2_0;
namespace aidl_vehicle = ::aidl::android::hardware::automotive::vehicle;
using ::android::hardware::hidl_enum_range;
using ::ndk::enum_range;

TEST(AidlHidlCompatibilityTest, testHidlPropertiesDefinedInAidl) {
    for (const auto prop : hidl_enum_range<hidl_vehicle::VehicleProperty>()) {
        int propInt = ::android::hardware::automotive::vehicle::toInt(prop);
        auto aidlProperties = enum_range<aidl_vehicle::VehicleProperty>();

        ASSERT_NE(std::find(aidlProperties.begin(), aidlProperties.end(),
                            static_cast<aidl_vehicle::VehicleProperty>(propInt)),
                  aidlProperties.end())
                << "property: " << propInt << " defined in HIDL, but not defined in AIDL";
    }
}
