/*
 * Copyright (C) 2023 The Android Open Source Project
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

#include "IvnAndroidDeviceService.h"

#include <aidl/android/hardware/automotive/ivn/EndpointInfo.h>
#include <aidl/android/hardware/automotive/ivn/OccupantType.h>
#include <aidl/android/hardware/automotive/ivn/OccupantZoneInfo.h>
#include <android-base/file.h>
#include <gtest/gtest.h>

namespace android {
namespace hardware {
namespace automotive {
namespace ivn {

using ::aidl::android::hardware::automotive::ivn::ConnectProtocol;
using ::aidl::android::hardware::automotive::ivn::EndpointInfo;
using ::aidl::android::hardware::automotive::ivn::OccupantType;
using ::aidl::android::hardware::automotive::ivn::OccupantZoneInfo;
using ::ndk::ScopedAStatus;

class IvnAndroidDeviceServiceUnitTest : public ::testing::Test {
  public:
    virtual void SetUp() override {
        mService = ndk::SharedRefBase::make<IvnAndroidDeviceService>(
                android::base::GetExecutableDirectory() + "/DefaultConfig.json");
        mService->init();
    }

    std::shared_ptr<IvnAndroidDeviceService> mService;
};

TEST_F(IvnAndroidDeviceServiceUnitTest, TestGetMyDeviceId) {
    int deviceId = -1;

    ScopedAStatus status = mService->getMyDeviceId(&deviceId);

    ASSERT_TRUE(status.isOk());
    ASSERT_EQ(deviceId, 0);
}

TEST_F(IvnAndroidDeviceServiceUnitTest, TestGetOtherDeviceIds) {
    std::vector<int> deviceIds;

    ScopedAStatus status = mService->getOtherDeviceIds(&deviceIds);

    ASSERT_TRUE(status.isOk());
    ASSERT_EQ(deviceIds, std::vector<int>({1}));
}

TEST_F(IvnAndroidDeviceServiceUnitTest, TestGetDeviceIdForOccupantZone) {
    int deviceId = -1;

    ScopedAStatus status = mService->getDeviceIdForOccupantZone(/*zoneId=*/0, &deviceId);

    ASSERT_TRUE(status.isOk());
    EXPECT_EQ(deviceId, 0);

    status = mService->getDeviceIdForOccupantZone(/*zoneId=*/1, &deviceId);

    ASSERT_TRUE(status.isOk());
    EXPECT_EQ(deviceId, 0);

    status = mService->getDeviceIdForOccupantZone(/*zoneId=*/2, &deviceId);

    ASSERT_TRUE(status.isOk());
    EXPECT_EQ(deviceId, 1);

    status = mService->getDeviceIdForOccupantZone(/*zoneId=*/3, &deviceId);

    ASSERT_TRUE(status.isOk());
    EXPECT_EQ(deviceId, 1);

    status = mService->getDeviceIdForOccupantZone(/*zoneId=*/4, &deviceId);

    ASSERT_FALSE(status.isOk());
}

TEST_F(IvnAndroidDeviceServiceUnitTest, TestGetOccupantZonesForDevice) {
    std::vector<OccupantZoneInfo> occupantZones;

    ScopedAStatus status =
            mService->getOccupantZonesForDevice(/*androidDeviceId=*/0, &occupantZones);

    ASSERT_TRUE(status.isOk());
    EXPECT_EQ(occupantZones.size(), 2);
    if (occupantZones.size() == 2) {
        EXPECT_EQ(occupantZones[0].zoneId, 0);
        EXPECT_EQ(occupantZones[0].occupantType, OccupantType::DRIVER);
        EXPECT_EQ(occupantZones[0].seat, 1);
        EXPECT_EQ(occupantZones[1].zoneId, 1);
        EXPECT_EQ(occupantZones[1].occupantType, OccupantType::FRONT_PASSENGER);
        EXPECT_EQ(occupantZones[1].seat, 4);
    }
}

TEST_F(IvnAndroidDeviceServiceUnitTest, TestGetMyEndpointInfo) {
    EndpointInfo endpointInfo;

    ScopedAStatus status = mService->getMyEndpointInfo(&endpointInfo);

    ASSERT_TRUE(status.isOk());
    EXPECT_EQ(endpointInfo.connectProtocol, ConnectProtocol::TCP_IP);
    EXPECT_EQ(endpointInfo.ipAddress, "10.10.10.1");
    EXPECT_EQ(endpointInfo.portNumber, 1234);
    EXPECT_EQ(endpointInfo.hardwareId.brandName, "MyBrand");
    EXPECT_EQ(endpointInfo.hardwareId.deviceName, "MyDevice");
    EXPECT_EQ(endpointInfo.hardwareId.productName, "MyProduct");
    EXPECT_EQ(endpointInfo.hardwareId.manufacturerName, "MyCompany");
    EXPECT_EQ(endpointInfo.hardwareId.modelName, "MyModel");
    EXPECT_EQ(endpointInfo.hardwareId.serialNumber, "Serial1234");
}

TEST_F(IvnAndroidDeviceServiceUnitTest, TestGetEndpointInfoForDevice) {
    EndpointInfo endpointInfo;

    ScopedAStatus status = mService->getEndpointInfoForDevice(/*androidDeviceId=*/0, &endpointInfo);

    ASSERT_TRUE(status.isOk());
    EXPECT_EQ(endpointInfo.connectProtocol, ConnectProtocol::TCP_IP);
    EXPECT_EQ(endpointInfo.ipAddress, "10.10.10.1");
    EXPECT_EQ(endpointInfo.portNumber, 1234);

    status = mService->getEndpointInfoForDevice(/*androidDeviceId=*/1, &endpointInfo);

    ASSERT_TRUE(status.isOk());
    EXPECT_EQ(endpointInfo.connectProtocol, ConnectProtocol::TCP_IP);
    EXPECT_EQ(endpointInfo.ipAddress, "10.10.10.2");
    EXPECT_EQ(endpointInfo.portNumber, 2345);
}

}  // namespace ivn
}  // namespace automotive
}  // namespace hardware
}  // namespace android
