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

#include <aidl/Gtest.h>
#include <aidl/Vintf.h>
#include <aidl/android/hardware/automotive/ivn/ConnectProtocol.h>
#include <aidl/android/hardware/automotive/ivn/EndpointInfo.h>
#include <aidl/android/hardware/automotive/ivn/IIvnAndroidDevice.h>
#include <android/binder_ibinder.h>
#include <android/binder_manager.h>
#include <gmock/gmock.h>
#include <unordered_set>

namespace aidl::android::hardware::automotive::ivn {

using ::ndk::ScopedAStatus;
using ::ndk::SpAIBinder;

using ::testing::Contains;
using ::testing::Not;

class VtsHalIvnTargetTest : public ::testing::TestWithParam<std::string> {
  public:
    void SetUp() override {
        std::string descriptor = GetParam();
        AIBinder* binder = AServiceManager_checkService(descriptor.c_str());
        ASSERT_NE(binder, nullptr) << "Failed to connect to IVN HAL";
        mIvnHal = IIvnAndroidDevice::fromBinder(SpAIBinder(binder));
    }

    std::shared_ptr<IIvnAndroidDevice> getHal() { return mIvnHal; }

  private:
    std::shared_ptr<IIvnAndroidDevice> mIvnHal;

  protected:
    ScopedAStatus getAllDeviceIds(std::unordered_set<int>* deviceIds);
};

TEST_P(VtsHalIvnTargetTest, testDeviceIdIsUnique) {
    std::unordered_set<int> foundDeviceIds;
    int myDeviceId = 0;

    ScopedAStatus status = getHal()->getMyDeviceId(&myDeviceId);

    ASSERT_TRUE(status.isOk()) << "Failed to call getMyDeviceId, status: " << status;
    foundDeviceIds.insert(myDeviceId);

    std::vector<int> otherDeviceIds;

    status = getHal()->getOtherDeviceIds(&otherDeviceIds);

    ASSERT_TRUE(status.isOk()) << "Failed to call getOtherDeviceIds, status: " << status;

    for (int deviceId : otherDeviceIds) {
        EXPECT_THAT(foundDeviceIds, Not(Contains(deviceId))) << "Duplicate device ID: " << deviceId;
        foundDeviceIds.insert(deviceId);
    }
}

ScopedAStatus VtsHalIvnTargetTest::getAllDeviceIds(std::unordered_set<int>* deviceIds) {
    int myDeviceId = 0;
    ScopedAStatus status = getHal()->getMyDeviceId(&myDeviceId);

    if (!status.isOk()) {
        return status;
    }
    deviceIds->insert(myDeviceId);
    std::vector<int> otherDeviceIds;
    status = getHal()->getOtherDeviceIds(&otherDeviceIds);
    if (!status.isOk()) {
        return status;
    }
    for (int otherDeviceId : otherDeviceIds) {
        deviceIds->insert(otherDeviceId);
    }
    return ScopedAStatus::ok();
}

TEST_P(VtsHalIvnTargetTest, testDeviceIdOccupantZoneMapping) {
    std::unordered_set<int> allDeviceIds;

    ScopedAStatus status = getAllDeviceIds(&allDeviceIds);

    ASSERT_FALSE(allDeviceIds.empty());
    ASSERT_TRUE(status.isOk()) << "Failed to get all device IDs, status: " << status;

    std::unordered_set<int> foundOccupantZoneIds;

    for (int deviceId : allDeviceIds) {
        std::vector<OccupantZoneInfo> occupantZones;
        status = getHal()->getOccupantZonesForDevice(deviceId, &occupantZones);

        ASSERT_TRUE(status.isOk())
                << "Failed to call getOccupantZonesForDevice, status: " << status;
        ASSERT_FALSE(occupantZones.empty()) << "No occupant zones for device: " << deviceId;

        for (const OccupantZoneInfo& occupantZone : occupantZones) {
            int zoneId = occupantZone.zoneId;

            EXPECT_THAT(foundOccupantZoneIds, Not(Contains(zoneId)))
                    << "Duplicate zone ID: " << zoneId;

            foundOccupantZoneIds.insert(zoneId);

            int gotDeviceId = 0;
            status = getHal()->getDeviceIdForOccupantZone(zoneId, &gotDeviceId);

            ASSERT_TRUE(status.isOk())
                    << "Failed to call getDeviceIdForOccupantZone, status: " << status;
            EXPECT_EQ(deviceId, gotDeviceId);
        }
    }
}

TEST_P(VtsHalIvnTargetTest, testGetEndpointInfo) {
    EndpointInfo endpointInfo;
    std::vector<EndpointInfo> foundEndpointInfo;

    ScopedAStatus status = getHal()->getMyEndpointInfo(&endpointInfo);

    foundEndpointInfo.push_back(endpointInfo);

    ASSERT_TRUE(status.isOk()) << "Failed to call getMyEndpointInfo, status: " << status;
    EXPECT_EQ(endpointInfo.connectProtocol, ConnectProtocol::TCP_IP);

    std::vector<int> otherDeviceIds;
    status = getHal()->getOtherDeviceIds(&otherDeviceIds);

    ASSERT_TRUE(status.isOk()) << "Failed to call getOtherDeviceIds, status: " << status;

    for (int deviceId : otherDeviceIds) {
        status = getHal()->getEndpointInfoForDevice(deviceId, &endpointInfo);

        ASSERT_TRUE(status.isOk()) << "Failed to call getEndpointInfoForDevice, status: " << status;
        EXPECT_EQ(endpointInfo.connectProtocol, ConnectProtocol::TCP_IP);

        for (EndpointInfo foundInfo : foundEndpointInfo) {
            ASSERT_NE(foundInfo, endpointInfo)
                    << "Found duplicate endpoint info" << endpointInfo.toString();
        }

        foundEndpointInfo.push_back(endpointInfo);
    }
}

GTEST_ALLOW_UNINSTANTIATED_PARAMETERIZED_TEST(VtsHalIvnTargetTest);

INSTANTIATE_TEST_SUITE_P(
        PerInstance, VtsHalIvnTargetTest,
        testing::ValuesIn(::android::getAidlHalInstanceNames(IIvnAndroidDevice::descriptor)),
        ::android::PrintInstanceNameToString);

}  // namespace aidl::android::hardware::automotive::ivn
