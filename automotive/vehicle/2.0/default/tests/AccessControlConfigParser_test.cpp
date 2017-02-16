/*
 * Copyright (C) 2016 The Android Open Source Project
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

#include <gtest/gtest.h>
#include <memory>
#include <fstream>
#include <unordered_set>

#include "vhal_v2_0/AccessControlConfigParser.h"
#include "vhal_v2_0/VehicleUtils.h"

namespace android {
namespace hardware {
namespace automotive {
namespace vehicle {
namespace V2_0 {

namespace {

class AccessControlConfigParserTest : public ::testing::Test {
protected:
    void SetUp() override {
        std::vector<int32_t> supportedProperties {
            toInt(VehicleProperty::HVAC_FAN_SPEED),
            toInt(VehicleProperty::HVAC_FAN_DIRECTION),
        };
        parser.reset(new AccessControlConfigParser(supportedProperties));
    }
public:
    PropertyAclMap aclMap;
    std::unique_ptr<AccessControlConfigParser> parser;
};

TEST_F(AccessControlConfigParserTest, basicParsing) {
    std::stringstream file;
    file << "S:0x0500 1000 RW" << std::endl;

    ASSERT_TRUE(parser->parseFromStream(&file, &aclMap));

    ASSERT_EQ(1u, aclMap.size());
    auto it = aclMap.find(toInt(VehicleProperty::HVAC_FAN_SPEED));
    ASSERT_NE(aclMap.end(), it);
    ASSERT_EQ(VehiclePropertyAccess::READ_WRITE, it->second.access);
    ASSERT_EQ(toInt(VehicleProperty::HVAC_FAN_SPEED), it->second.propId);
    ASSERT_EQ(1000u, it->second.uid);
}

TEST_F(AccessControlConfigParserTest, multipleUids) {
    std::stringstream file;
    file << "Set AID_AUDIO 1004" << std::endl
            << "Set AID_SYSTEM 1000" << std::endl
            << "S:0x0500 AID_SYSTEM RW" << std::endl
            << "S:0x0500 AID_AUDIO RW" << std::endl
            << "S:0x0500 0xbeef R" << std::endl;  // Read-only.

    std::unordered_set<unsigned> expectedUids {1000, 1004, 0xbeef};

    ASSERT_TRUE(parser->parseFromStream(&file, &aclMap));

    auto range = aclMap.equal_range(toInt(VehicleProperty::HVAC_FAN_SPEED));
    for (auto it = range.first; it != range.second; ++it) {
        auto& acl = it->second;

        ASSERT_EQ(1u, expectedUids.count(acl.uid))
                << " uid: " << std::hex << acl.uid;

        if (acl.uid == 0xbeef) {
            ASSERT_EQ(VehiclePropertyAccess::READ, acl.access);
        } else {
            ASSERT_EQ(VehiclePropertyAccess::READ_WRITE, acl.access);
        }
    }
}

TEST_F(AccessControlConfigParserTest, fileContainsJunk) {
    std::stringstream file;
    file << "This string will be ignored with warning in the log" << std::endl
         << "# However comments are quit legitimate" << std::endl
         << "S:0x0500 0xbeef R # YAY" << std::endl;

    ASSERT_FALSE(parser->parseFromStream(&file, &aclMap));

    ASSERT_EQ(1u, aclMap.size());
    auto it = aclMap.find(toInt(VehicleProperty::HVAC_FAN_SPEED));
    ASSERT_NE(aclMap.end(), it);
    ASSERT_EQ(VehiclePropertyAccess::READ, it->second.access);
    ASSERT_EQ(toInt(VehicleProperty::HVAC_FAN_SPEED), it->second.propId);
    ASSERT_EQ(0xBEEFu, it->second.uid);
}

TEST_F(AccessControlConfigParserTest, badIntegerFormat) {
    std::stringstream file;
    file << "S:0x0500 A12 RW " << std::endl;

    ASSERT_FALSE(parser->parseFromStream(&file, &aclMap));
    ASSERT_EQ(0u, aclMap.size());
}

TEST_F(AccessControlConfigParserTest, ignoreNotSupportedProperties) {
    std::stringstream file;
    file << "S:0x0666 1000 RW " << std::endl;

    ASSERT_FALSE(parser->parseFromStream(&file, &aclMap));
    ASSERT_EQ(0u, aclMap.size());
}

TEST_F(AccessControlConfigParserTest, multipleCalls) {
    std::stringstream configFile;
    configFile << "S:0x0500 1000 RW" << std::endl;

    ASSERT_TRUE(parser->parseFromStream(&configFile, &aclMap));
    ASSERT_EQ(1u, aclMap.size());

    std::stringstream configFile2;
    configFile2 << "S:0x0501 1004 RW" << std::endl;
    ASSERT_TRUE(parser->parseFromStream(&configFile2, &aclMap));
    ASSERT_EQ(2u, aclMap.size());

    auto it = aclMap.find(toInt(VehicleProperty::HVAC_FAN_SPEED));
    ASSERT_NE(aclMap.end(), it);
    ASSERT_EQ(VehiclePropertyAccess::READ_WRITE, it->second.access);
    ASSERT_EQ(toInt(VehicleProperty::HVAC_FAN_SPEED), it->second.propId);
    ASSERT_EQ(1000u, it->second.uid);

    it = aclMap.find(toInt(VehicleProperty::HVAC_FAN_DIRECTION));
    ASSERT_NE(aclMap.end(), it);
    ASSERT_EQ(VehiclePropertyAccess::READ_WRITE, it->second.access);
    ASSERT_EQ(toInt(VehicleProperty::HVAC_FAN_DIRECTION), it->second.propId);
    ASSERT_EQ(1004u, it->second.uid);
}


}  // namespace anonymous

}  // namespace V2_0
}  // namespace vehicle
}  // namespace automotive
}  // namespace hardware
}  // namespace android
