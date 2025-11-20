/*
 * Copyright 2025 The Android Open Source Project
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

#include "bluetooth_hal/config/cs_config_loader.h"

#include <string_view>
#include <vector>

#include "bluetooth_hal/hal_packet.h"
#include "bluetooth_hal/test/mock/mock_android_base_wrapper.h"
#include "gtest/gtest.h"

namespace bluetooth_hal {
namespace config {
namespace {

using ::bluetooth_hal::hci::HalPacket;
using ::bluetooth_hal::util::MockAndroidBaseWrapper;

using ::testing::Test;

constexpr std::string_view kValidContent = R"({
  "commands": [
      {
          "packet_type": 1,
          "opcode": 4660,
          "payload_length": 8,
          "sub_opcode": [1, 2, 3],
          "data": [4, 5, 6, 7, 8]
      },
      {
          "packet_type": 1,
          "opcode": 4660,
          "payload_length": 5,
          "sub_opcode": [10, 11],
          "data": [12, 13, 14]
      }
  ]
})";

// Content with invalid filed `payload_lengths`.
constexpr std::string_view kEmptyContent = R"({
  "commands": [

  ]
})";

class CsConfigLoaderTest : public Test {
 protected:
  void SetUp() override {
    MockAndroidBaseWrapper::SetMockWrapper(&mock_android_base_wrapper_);
  }

  MockAndroidBaseWrapper mock_android_base_wrapper_;
};

TEST_F(CsConfigLoaderTest, ParseValidContentAndGetCsCalibrationCommands) {
  EXPECT_TRUE(CsConfigLoader::GetLoader().LoadConfigFromString(kValidContent));

  const std::vector<HalPacket>& commands =
      CsConfigLoader::GetLoader().GetCsCalibrationCommands();

  EXPECT_EQ(commands.size(), 2);

  EXPECT_EQ(commands[0],
            std::vector<uint8_t>({0x01, 0x34, 0x12, 0x08, 0x01, 0x02, 0x03,
                                  0x04, 0x05, 0x06, 0x07, 0x08}));

  EXPECT_EQ(commands[1], std::vector<uint8_t>({0x01, 0x34, 0x12, 0x05, 0x0A,
                                               0x0B, 0x0C, 0x0D, 0x0E}));
}

TEST_F(CsConfigLoaderTest, ParseEmptyContent) {
  EXPECT_TRUE(CsConfigLoader::GetLoader().LoadConfigFromString(kEmptyContent));

  const std::vector<HalPacket>& commands =
      CsConfigLoader::GetLoader().GetCsCalibrationCommands();

  EXPECT_TRUE(commands.empty());
}

}  // namespace
}  // namespace config
}  // namespace bluetooth_hal
