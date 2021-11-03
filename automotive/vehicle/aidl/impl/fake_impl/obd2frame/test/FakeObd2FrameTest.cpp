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

#include "FakeObd2Frame.h"

#include <PropertyUtils.h>
#include <VehicleObjectPool.h>
#include <VehiclePropertyStore.h>
#include <gtest/gtest.h>
#include <utils/SystemClock.h>

#include <set>

namespace android {
namespace hardware {
namespace automotive {
namespace vehicle {
namespace fake {
namespace obd2frame {

using ::aidl::android::hardware::automotive::vehicle::VehiclePropConfig;
using ::aidl::android::hardware::automotive::vehicle::VehicleProperty;
using ::aidl::android::hardware::automotive::vehicle::VehiclePropertyAccess;
using ::aidl::android::hardware::automotive::vehicle::VehiclePropertyChangeMode;
using ::aidl::android::hardware::automotive::vehicle::VehiclePropValue;

class FakeObd2FrameTest : public ::testing::Test {
  protected:
    void SetUp() override {
        std::shared_ptr<VehiclePropValuePool> valuePool = std::make_shared<VehiclePropValuePool>();
        mPropertyStore = std::make_shared<VehiclePropertyStore>(valuePool);
        mObd2Frame = std::make_unique<FakeObd2Frame>(mPropertyStore);

        mPropertyStore->registerProperty(getObd2LiveFrameConfig());
        mPropertyStore->registerProperty(
                getObd2FreezeFrameConfig(),
                [](const VehiclePropValue& propValue) { return propValue.timestamp; });
        mPropertyStore->registerProperty(getObd2FreezeFrameInfoConfig());
    }

    VehiclePropConfig getObd2LiveFrameConfig() {
        return VehiclePropConfig{.prop = OBD2_LIVE_FRAME,
                                 .access = VehiclePropertyAccess::READ,
                                 .changeMode = VehiclePropertyChangeMode::ON_CHANGE,
                                 .configArray = {1, 1}};
    }

    VehiclePropConfig getObd2FreezeFrameConfig() {
        return VehiclePropConfig{.prop = OBD2_FREEZE_FRAME,
                                 .access = VehiclePropertyAccess::READ,
                                 .changeMode = VehiclePropertyChangeMode::ON_CHANGE,
                                 .configArray = {0, 0}};
    }

    VehiclePropConfig getObd2FreezeFrameInfoConfig() {
        return VehiclePropConfig{.prop = OBD2_FREEZE_FRAME_INFO,
                                 .access = VehiclePropertyAccess::READ,
                                 .changeMode = VehiclePropertyChangeMode::ON_CHANGE};
    }

    FakeObd2Frame* getFakeObd2Frame() { return mObd2Frame.get(); }

    VehiclePropertyStore* getPropertyStore() { return mPropertyStore.get(); }

  private:
    std::unique_ptr<FakeObd2Frame> mObd2Frame;
    std::shared_ptr<VehiclePropertyStore> mPropertyStore;
};

TEST_F(FakeObd2FrameTest, testIsDiagnosticPropertyTrue) {
    for (auto prop : std::vector<int32_t>({
                 OBD2_LIVE_FRAME,
                 OBD2_FREEZE_FRAME,
                 OBD2_FREEZE_FRAME_CLEAR,
                 OBD2_FREEZE_FRAME_INFO,
         })) {
        EXPECT_TRUE(FakeObd2Frame::isDiagnosticProperty(VehiclePropConfig{
                .prop = prop,
        }));
    }
}

TEST_F(FakeObd2FrameTest, testIsDiagnosticPropertyFalse) {
    ASSERT_FALSE(FakeObd2Frame::isDiagnosticProperty(VehiclePropConfig{
            .prop = toInt(VehicleProperty::INFO_VIN),
    }));
}

TEST_F(FakeObd2FrameTest, testInitObd2LiveFrame) {
    int64_t timestamp = elapsedRealtimeNano();

    getFakeObd2Frame()->initObd2LiveFrame(getObd2LiveFrameConfig());

    auto result = getPropertyStore()->readValue(OBD2_LIVE_FRAME);

    ASSERT_TRUE(result.ok());
    auto& value = result.value();

    EXPECT_GE(value->timestamp, timestamp);
    EXPECT_EQ(value->value.stringValue, "");
    EXPECT_EQ(value->value.int32Values.size(), static_cast<size_t>(33));
    EXPECT_EQ(value->value.floatValues.size(), static_cast<size_t>(72));
}

TEST_F(FakeObd2FrameTest, testInitFreezeFrame) {
    getFakeObd2Frame()->initObd2FreezeFrame(getObd2FreezeFrameConfig());

    auto result = getPropertyStore()->readValuesForProperty(OBD2_FREEZE_FRAME);

    ASSERT_TRUE(result.ok());
    ASSERT_EQ(result.value().size(), static_cast<size_t>(3));
}

TEST_F(FakeObd2FrameTest, testGetObd2DtcInfo) {
    getFakeObd2Frame()->initObd2FreezeFrame(getObd2FreezeFrameConfig());

    auto result = getFakeObd2Frame()->getObd2DtcInfo();

    ASSERT_TRUE(result.ok());
    EXPECT_EQ(result.value()->prop, OBD2_FREEZE_FRAME_INFO);
    EXPECT_EQ(result.value()->value.int64Values.size(), static_cast<size_t>(3));
}

TEST_F(FakeObd2FrameTest, testGetObd2FreezeFrame) {
    getFakeObd2Frame()->initObd2FreezeFrame(getObd2FreezeFrameConfig());

    auto result = getFakeObd2Frame()->getObd2DtcInfo();

    ASSERT_TRUE(result.ok());
    ASSERT_EQ(result.value()->prop, OBD2_FREEZE_FRAME_INFO);
    ASSERT_EQ(result.value()->value.int64Values.size(), static_cast<size_t>(3));

    std::set<std::string> dtcs;

    for (int64_t timestamp : result.value()->value.int64Values) {
        auto freezeFrameResult = getFakeObd2Frame()->getObd2FreezeFrame(VehiclePropValue{
                .value.int64Values = {timestamp},
        });

        ASSERT_TRUE(freezeFrameResult.ok());

        dtcs.insert(freezeFrameResult.value()->value.stringValue);
    }

    ASSERT_EQ(dtcs, std::set<std::string>({"P0070", "P0102", "P0123"}));
}

TEST_F(FakeObd2FrameTest, testClearObd2FreezeFrameAll) {
    getFakeObd2Frame()->initObd2FreezeFrame(getObd2FreezeFrameConfig());

    auto result = getFakeObd2Frame()->getObd2DtcInfo();

    ASSERT_TRUE(result.ok());
    ASSERT_EQ(result.value()->prop, OBD2_FREEZE_FRAME_INFO);
    ASSERT_EQ(result.value()->value.int64Values.size(), static_cast<size_t>(3));

    ASSERT_TRUE(getFakeObd2Frame()->clearObd2FreezeFrames(VehiclePropValue{}).ok());

    result = getFakeObd2Frame()->getObd2DtcInfo();

    ASSERT_TRUE(result.ok());
    EXPECT_EQ(result.value()->prop, OBD2_FREEZE_FRAME_INFO);
    EXPECT_EQ(result.value()->value.int64Values.size(), static_cast<size_t>(0));
}

TEST_F(FakeObd2FrameTest, testClearObd2FreezeFrameByTimestamp) {
    getFakeObd2Frame()->initObd2FreezeFrame(getObd2FreezeFrameConfig());

    auto result = getFakeObd2Frame()->getObd2DtcInfo();

    ASSERT_TRUE(result.ok());
    ASSERT_EQ(result.value()->prop, OBD2_FREEZE_FRAME_INFO);
    ASSERT_EQ(result.value()->value.int64Values.size(), static_cast<size_t>(3));

    ASSERT_TRUE(getFakeObd2Frame()
                        ->clearObd2FreezeFrames(VehiclePropValue{
                                .value.int64Values = {result.value()->value.int64Values[0],
                                                      result.value()->value.int64Values[1]}})
                        .ok());

    result = getFakeObd2Frame()->getObd2DtcInfo();

    ASSERT_TRUE(result.ok());
    EXPECT_EQ(result.value()->prop, OBD2_FREEZE_FRAME_INFO);
    EXPECT_EQ(result.value()->value.int64Values.size(), static_cast<size_t>(1));
}

}  // namespace obd2frame
}  // namespace fake
}  // namespace vehicle
}  // namespace automotive
}  // namespace hardware
}  // namespace android
