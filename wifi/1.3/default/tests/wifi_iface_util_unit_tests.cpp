/*
 * Copyright (C) 2019, The Android Open Source Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include <android-base/logging.h>
#include <android-base/macros.h>
#include <gmock/gmock.h>

#undef NAN
#include "wifi_iface_util.h"

#include "mock_interface_tool.h"

using testing::NiceMock;
using testing::Test;

namespace {
constexpr uint8_t kValidUnicastLocallyAssignedMacAddressMask = 0x02;
constexpr uint8_t kMacAddress[] = {0x02, 0x12, 0x45, 0x56, 0xab, 0xcc};
constexpr char kIfaceName[] = "test-wlan0";

bool isValidUnicastLocallyAssignedMacAddress(
    const std::array<uint8_t, 6>& mac_address) {
    uint8_t first_byte = mac_address[0];
    return (first_byte & 0x3) == kValidUnicastLocallyAssignedMacAddressMask;
}
}  // namespace

namespace android {
namespace hardware {
namespace wifi {
namespace V1_3 {
namespace implementation {
namespace iface_util {
class WifiIfaceUtilTest : public Test {
   protected:
    std::shared_ptr<NiceMock<wifi_system::MockInterfaceTool>> iface_tool_{
        new NiceMock<wifi_system::MockInterfaceTool>};
    WifiIfaceUtil* iface_util_ = new WifiIfaceUtil(iface_tool_);
};

TEST_F(WifiIfaceUtilTest, GetOrCreateRandomMacAddress) {
    auto mac_address = iface_util_->getOrCreateRandomMacAddress();
    ASSERT_TRUE(isValidUnicastLocallyAssignedMacAddress(mac_address));

    // All further calls should return the same MAC address.
    ASSERT_EQ(mac_address, iface_util_->getOrCreateRandomMacAddress());
    ASSERT_EQ(mac_address, iface_util_->getOrCreateRandomMacAddress());
}

TEST_F(WifiIfaceUtilTest, IfaceEventHandlers_SetMacAddress) {
    std::array<uint8_t, 6> mac_address = {};
    std::copy(std::begin(kMacAddress), std::end(kMacAddress),
              std::begin(mac_address));
    EXPECT_CALL(*iface_tool_, SetMacAddress(testing::_, testing::_))
        .WillRepeatedly(testing::Return(true));
    EXPECT_CALL(*iface_tool_, SetUpState(testing::_, testing::_))
        .WillRepeatedly(testing::Return(true));

    // Register for iface state toggle events.
    bool callback_invoked = false;
    iface_util::IfaceEventHandlers event_handlers = {};
    event_handlers.on_state_toggle_off_on =
        [&callback_invoked](const std::string& /* iface_name */) {
            callback_invoked = true;
        };
    iface_util_->registerIfaceEventHandlers(kIfaceName, event_handlers);
    // Invoke setMacAddress and ensure that the cb is invoked.
    ASSERT_TRUE(iface_util_->setMacAddress(kIfaceName, mac_address));
    ASSERT_TRUE(callback_invoked);

    // Unregister for iface state toggle events.
    callback_invoked = false;
    iface_util_->unregisterIfaceEventHandlers(kIfaceName);
    // Invoke setMacAddress and ensure that the cb is not invoked.
    ASSERT_TRUE(iface_util_->setMacAddress(kIfaceName, mac_address));
    ASSERT_FALSE(callback_invoked);
}
}  // namespace iface_util
}  // namespace implementation
}  // namespace V1_3
}  // namespace wifi
}  // namespace hardware
}  // namespace android
