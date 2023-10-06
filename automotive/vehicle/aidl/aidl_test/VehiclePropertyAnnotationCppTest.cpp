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

#include <AccessForVehicleProperty.h>
#include <ChangeModeForVehicleProperty.h>

#include <aidl/android/hardware/automotive/vehicle/VehicleProperty.h>
#include <gtest/gtest.h>
#include <unordered_set>

namespace aidl_vehicle = ::aidl::android::hardware::automotive::vehicle;
using aidl_vehicle::AccessForVehicleProperty;
using aidl_vehicle::ChangeModeForVehicleProperty;
using aidl_vehicle::VehicleProperty;

namespace {
    template<class T>
    bool doesAnnotationMapContainsAllProps(std::unordered_map<VehicleProperty, T> annotationMap) {
        for (const VehicleProperty& v : ::ndk::enum_range<VehicleProperty>()) {
            std::string name = aidl_vehicle::toString(v);
            if (name == "INVALID") {
                continue;
            }
            if (annotationMap.find(v) == annotationMap.end()) {
                return false;
            }
        }
        return true;
    }

}  // namespace

TEST(VehiclePropertyAnnotationCppTest, testChangeMode) {
    ASSERT_TRUE(doesAnnotationMapContainsAllProps(ChangeModeForVehicleProperty))
            << "Outdated annotation-generated AIDL files. Please run "
            << "generate_annotation_enums.py to update.";
}

TEST(VehiclePropertyAnnotationCppTest, testAccess) {
    ASSERT_TRUE(doesAnnotationMapContainsAllProps(AccessForVehicleProperty))
            << "Outdated annotation-generated AIDL files. Please run "
            << "generate_annotation_enums.py to update.";
}
