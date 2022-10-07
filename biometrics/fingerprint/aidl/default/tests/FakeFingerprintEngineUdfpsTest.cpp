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

#include <android/binder_process.h>
#include <fingerprint.sysprop.h>
#include <gtest/gtest.h>

#include <android-base/logging.h>

#include "FakeFingerprintEngine.h"
#include "FakeFingerprintEngineUdfps.h"

using namespace ::android::fingerprint::virt;
using namespace ::aidl::android::hardware::biometrics::fingerprint;
using namespace ::aidl::android::hardware::keymaster;

namespace aidl::android::hardware::biometrics::fingerprint {

class FakeFingerprintEngineUdfpsTest : public ::testing::Test {
  protected:
    void SetUp() override {}

    void TearDown() override {
        // reset to default
        FingerprintHalProperties::sensor_location("");
    }

    FakeFingerprintEngineUdfps mEngine;
};

bool isDefaultLocation(SensorLocation& sc) {
    return (sc.sensorLocationX == FakeFingerprintEngineUdfps::defaultSensorLocationX &&
            sc.sensorLocationY == FakeFingerprintEngineUdfps::defaultSensorLocationY &&
            sc.sensorRadius == FakeFingerprintEngineUdfps::defaultSensorRadius && sc.display == "");
}

TEST_F(FakeFingerprintEngineUdfpsTest, getSensorLocationOk) {
    auto loc = "100:200:30";
    FingerprintHalProperties::sensor_location(loc);
    SensorLocation sc = mEngine.getSensorLocation();
    ASSERT_TRUE(sc.sensorLocationX == 100);
    ASSERT_TRUE(sc.sensorLocationY == 200);
    ASSERT_TRUE(sc.sensorRadius == 30);

    loc = "100:200:30:screen1";
    FingerprintHalProperties::sensor_location(loc);
    sc = mEngine.getSensorLocation();
    ASSERT_TRUE(sc.sensorLocationX == 100);
    ASSERT_TRUE(sc.sensorLocationY == 200);
    ASSERT_TRUE(sc.sensorRadius == 30);
    ASSERT_TRUE(sc.display == "screen1");
}

TEST_F(FakeFingerprintEngineUdfpsTest, getSensorLocationBad) {
    FingerprintHalProperties::sensor_location("");
    SensorLocation sc = mEngine.getSensorLocation();
    ASSERT_TRUE(isDefaultLocation(sc));

    auto loc = "100";
    FingerprintHalProperties::sensor_location(loc);
    sc = mEngine.getSensorLocation();
    ASSERT_TRUE(isDefaultLocation(sc));

    loc = "10:20";
    FingerprintHalProperties::sensor_location(loc);
    sc = mEngine.getSensorLocation();
    ASSERT_TRUE(isDefaultLocation(sc));

    loc = "10,20,5";
    FingerprintHalProperties::sensor_location(loc);
    sc = mEngine.getSensorLocation();
    ASSERT_TRUE(isDefaultLocation(sc));

    loc = "a:b:c";
    FingerprintHalProperties::sensor_location(loc);
    sc = mEngine.getSensorLocation();
    ASSERT_TRUE(isDefaultLocation(sc));
}

// More
}  // namespace aidl::android::hardware::biometrics::fingerprint
