/*
**
** Copyright 2017, The Android Open Source Project
**
** Licensed under the Apache License, Version 2.0 (the "License");
** you may not use this file except in compliance with the License.
** You may obtain a copy of the License at
**
**     http://www.apache.org/licenses/LICENSE-2.0
**
** Unless required by applicable law or agreed to in writing, software
** distributed under the License is distributed on an "AS IS" BASIS,
** WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
** See the License for the specific language governing permissions and
** limitations under the License.
*/

#include <stddef.h>
#include <stdint.h>
#include <iomanip>
#include <iostream>
#include <string>

#include <android/hardware/confirmationui/support/msg_formatting.h>
#include <gtest/gtest.h>

using android::hardware::confirmationui::support::Message;
using android::hardware::confirmationui::support::WriteStream;
using android::hardware::confirmationui::support::ReadStream;
using android::hardware::confirmationui::support::PromptUserConfirmationMsg;
using android::hardware::confirmationui::support::write;
using ::android::hardware::confirmationui::V1_0::UIOption;
using ::android::hardware::keymaster::V4_0::HardwareAuthToken;
using ::android::hardware::keymaster::V4_0::HardwareAuthenticatorType;
using ::android::hardware::hidl_string;
using ::android::hardware::hidl_vec;

#ifdef DEBUG_MSG_FORMATTING
namespace {

char nibble2hex[16] = {'0', '1', '2', '3', '4', '5', '6', '7',
                       '8', '9', 'a', 'b', 'c', 'd', 'e', 'f'};

std::ostream& hexdump(std::ostream& out, const uint8_t* data, size_t size) {
    for (size_t i = 0; i < size; ++i) {
        uint8_t byte = data[i];
        out << (nibble2hex[0x0F & (byte >> 4)]);
        out << (nibble2hex[0x0F & byte]);
        switch (i & 0xf) {
            case 0xf:
                out << "\n";
                break;
            case 7:
                out << "  ";
                break;
            default:
                out << " ";
                break;
        }
    }
    return out;
}

}  // namespace
#endif

TEST(MsgFormattingTest, FeatureTest) {
    uint8_t buffer[0x1000];

    WriteStream out(buffer);
    out = unalign(out);
    out += 4;
    auto begin = out.pos();
    out = write(
        PromptUserConfirmationMsg(), out, hidl_string("Do you?"),
        hidl_vec<uint8_t>{0x01, 0x02, 0x03}, hidl_string("en"),
        hidl_vec<UIOption>{UIOption::AccessibilityInverted, UIOption::AccessibilityMagnified});

    ReadStream in(buffer);
    in = unalign(in);
    in += 4;
    hidl_string prompt;
    hidl_vec<uint8_t> extra;
    hidl_string locale;
    hidl_vec<UIOption> uiOpts;
    bool command_matches;
    std::tie(in, command_matches, prompt, extra, locale, uiOpts) =
        read(PromptUserConfirmationMsg(), in);
    ASSERT_TRUE(in);
    ASSERT_TRUE(command_matches);
    ASSERT_EQ(hidl_string("Do you?"), prompt);
    ASSERT_EQ((hidl_vec<uint8_t>{0x01, 0x02, 0x03}), extra);
    ASSERT_EQ(hidl_string("en"), locale);
    ASSERT_EQ(
        (hidl_vec<UIOption>{UIOption::AccessibilityInverted, UIOption::AccessibilityMagnified}),
        uiOpts);

#ifdef DEBUG_MSG_FORMATTING
    hexdump(std::cout, buffer, 100) << std::endl;
#endif

    // The following assertions check that the hidl_[vec|string] types are in fact read in place,
    // and no copying occurs. Copying results in heap allocation which we intend to avoid.
    ASSERT_EQ(8, const_cast<uint8_t*>(reinterpret_cast<const uint8_t*>(prompt.c_str())) - begin);
    ASSERT_EQ(20, extra.data() - begin);
    ASSERT_EQ(27, const_cast<uint8_t*>(reinterpret_cast<const uint8_t*>(locale.c_str())) - begin);
    ASSERT_EQ(40, reinterpret_cast<uint8_t*>(uiOpts.data()) - begin);
}

TEST(MsgFormattingTest, HardwareAuthTokenTest) {
    uint8_t buffer[0x1000];

    HardwareAuthToken expected, actual;
    expected.authenticatorId = 0xa1a3a4a5a6a7a8;
    expected.authenticatorType = HardwareAuthenticatorType::NONE;
    expected.challenge = 0xb1b2b3b4b5b6b7b8;
    expected.userId = 0x1122334455667788;
    expected.timestamp = 0xf1f2f3f4f5f6f7f8;
    expected.mac =
        hidl_vec<uint8_t>{1,  2,  3,  4,  5,  6,  7,  8,  9,  10, 11, 12, 13, 14, 15, 16,
                          17, 18, 19, 20, 21, 22, 23, 24, 25, 26, 27, 28, 29, 30, 31, 32};

    WriteStream out(buffer);
    out = write(Message<HardwareAuthToken>(), out, expected);
    ReadStream in(buffer);
    std::tie(in, actual) = read(Message<HardwareAuthToken>(), in);
    ASSERT_EQ(expected, actual);
}
