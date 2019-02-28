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

using testing::Test;

namespace {
constexpr uint8_t kValidUnicastLocallyAssignedMacAddressMask = 0x02;

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
class WifiIfaceUtilTest : public Test {
   protected:
    iface_util::WifiIfaceUtil iface_util_;
};

TEST_F(WifiIfaceUtilTest, GetOrCreateRandomMacAddress) {
    auto mac_address = iface_util_.getOrCreateRandomMacAddress();
    ASSERT_TRUE(isValidUnicastLocallyAssignedMacAddress(mac_address));

    // All further calls should return the same MAC address.
    ASSERT_EQ(mac_address, iface_util_.getOrCreateRandomMacAddress());
    ASSERT_EQ(mac_address, iface_util_.getOrCreateRandomMacAddress());
}
}  // namespace implementation
}  // namespace V1_3
}  // namespace wifi
}  // namespace hardware
}  // namespace android
