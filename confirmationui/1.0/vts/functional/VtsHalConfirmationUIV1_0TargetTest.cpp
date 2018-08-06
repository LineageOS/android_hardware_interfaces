/*
 * Copyright (C) 2018 The Android Open Source Project
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

#define LOG_TAG "ConfirmationIOHidlHalTest"
#include <cutils/log.h>

#include <algorithm>
#include <iostream>
#include <memory>

#include <android/hardware/confirmationui/1.0/IConfirmationResultCallback.h>
#include <android/hardware/confirmationui/1.0/IConfirmationUI.h>
#include <android/hardware/confirmationui/1.0/types.h>
#include <android/hardware/confirmationui/support/confirmationui_utils.h>

#include <VtsHalHidlTargetCallbackBase.h>
#include <VtsHalHidlTargetTestBase.h>

#include <openssl/hmac.h>
#include <openssl/sha.h>

#include <cn-cbor/cn-cbor.h>

using ::android::sp;

using ::std::string;

namespace android {
namespace hardware {

namespace confirmationui {
namespace V1_0 {

namespace test {
namespace {
const support::auth_token_key_t testKey(static_cast<uint8_t>(TestKeyBits::BYTE));

class HMacImplementation {
   public:
    static support::NullOr<support::hmac_t> hmac256(
        const support::auth_token_key_t& key,
        std::initializer_list<support::ByteBufferProxy> buffers) {
        HMAC_CTX hmacCtx;
        HMAC_CTX_init(&hmacCtx);
        if (!HMAC_Init_ex(&hmacCtx, key.data(), key.size(), EVP_sha256(), nullptr)) {
            return {};
        }
        for (auto& buffer : buffers) {
            if (!HMAC_Update(&hmacCtx, buffer.data(), buffer.size())) {
                return {};
            }
        }
        support::hmac_t result;
        if (!HMAC_Final(&hmacCtx, result.data(), nullptr)) {
            return {};
        }
        return result;
    }
};

using HMacer = support::HMac<HMacImplementation>;

template <typename... Data>
hidl_vec<uint8_t> testHMAC(const Data&... data) {
    auto hmac = HMacer::hmac256(testKey, data...);
    if (!hmac.isOk()) {
        EXPECT_TRUE(false) << "Failed to compute test hmac.  This is a self-test error.";
        return {};
    }
    hidl_vec<uint8_t> result(hmac.value().size());
    copy(hmac.value().data(), hmac.value().data() + hmac.value().size(), result.data());
    return result;
}

using ::android::hardware::keymaster::V4_0::HardwareAuthToken;
using ::android::hardware::keymaster::V4_0::HardwareAuthenticatorType;

template <typename T>
auto toBytes(const T& v) -> const uint8_t (&)[sizeof(T)] {
    return *reinterpret_cast<const uint8_t(*)[sizeof(T)]>(&v);
}

HardwareAuthToken makeTestToken(const TestModeCommands command, uint64_t timestamp = 0) {
    HardwareAuthToken auth_token;
    auth_token.challenge = static_cast<uint64_t>(command);
    auth_token.userId = 0;
    auth_token.authenticatorId = 0;
    auth_token.authenticatorType = HardwareAuthenticatorType::NONE;
    auth_token.timestamp = timestamp;

    // Canonical form  of auth-token v0
    // version (1 byte)
    // challenge (8 bytes)
    // user_id (8 bytes)
    // authenticator_id (8 bytes)
    // authenticator_type (4 bytes)
    // timestamp (8 bytes)
    // total 37 bytes
    auth_token.mac = testHMAC("\0",
                              toBytes(auth_token.challenge),                         //
                              toBytes(auth_token.userId),                            //
                              toBytes(auth_token.authenticatorId),                   //
                              toBytes(support::hton(auth_token.authenticatorType)),  //
                              toBytes(support::hton(auth_token.timestamp)));         //

    return auth_token;
}

#define DEBUG_CONFRIMATIONUI_UTILS_TEST

#ifdef DEBUG_CONFRIMATIONUI_UTILS_TEST
std::ostream& hexdump(std::ostream& out, const uint8_t* data, size_t size) {
    for (size_t i = 0; i < size; ++i) {
        uint8_t byte = data[i];
        out << std::hex << std::setw(2) << std::setfill('0') << (unsigned)byte;
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
#endif

constexpr char hex_value[256] = {0, 0,  0,  0,  0,  0,  0,  0, 0, 0, 0, 0, 0, 0, 0, 0,  //
                                 0, 0,  0,  0,  0,  0,  0,  0, 0, 0, 0, 0, 0, 0, 0, 0,  //
                                 0, 0,  0,  0,  0,  0,  0,  0, 0, 0, 0, 0, 0, 0, 0, 0,  //
                                 0, 1,  2,  3,  4,  5,  6,  7, 8, 9, 0, 0, 0, 0, 0, 0,  // '0'..'9'
                                 0, 10, 11, 12, 13, 14, 15, 0, 0, 0, 0, 0, 0, 0, 0, 0,  // 'A'..'F'
                                 0, 0,  0,  0,  0,  0,  0,  0, 0, 0, 0, 0, 0, 0, 0, 0,  //
                                 0, 10, 11, 12, 13, 14, 15, 0, 0, 0, 0, 0, 0, 0, 0, 0,  // 'a'..'f'
                                 0, 0,  0,  0,  0,  0,  0,  0, 0, 0, 0, 0, 0, 0, 0, 0,  //
                                 0, 0,  0,  0,  0,  0,  0,  0, 0, 0, 0, 0, 0, 0, 0, 0,  //
                                 0, 0,  0,  0,  0,  0,  0,  0, 0, 0, 0, 0, 0, 0, 0, 0,  //
                                 0, 0,  0,  0,  0,  0,  0,  0, 0, 0, 0, 0, 0, 0, 0, 0,  //
                                 0, 0,  0,  0,  0,  0,  0,  0, 0, 0, 0, 0, 0, 0, 0, 0,  //
                                 0, 0,  0,  0,  0,  0,  0,  0, 0, 0, 0, 0, 0, 0, 0, 0,  //
                                 0, 0,  0,  0,  0,  0,  0,  0, 0, 0, 0, 0, 0, 0, 0, 0,  //
                                 0, 0,  0,  0,  0,  0,  0,  0, 0, 0, 0, 0, 0, 0, 0, 0,  //
                                 0, 0,  0,  0,  0,  0,  0,  0, 0, 0, 0, 0, 0, 0, 0, 0};

std::string hex2str(std::string a) {
    std::string b;
    size_t num = a.size() / 2;
    b.resize(num);
    for (size_t i = 0; i < num; i++) {
        b[i] = (hex_value[a[i * 2] & 0xFF] << 4) + (hex_value[a[i * 2 + 1] & 0xFF]);
    }
    return b;
}

}  // namespace

class ConfirmationArgs {
   public:
    ResponseCode error_;
    hidl_vec<uint8_t> formattedMessage_;
    hidl_vec<uint8_t> confirmationToken_;
    bool verifyConfirmationToken() {
        static constexpr char confirmationPrefix[] = "confirmation token";
        EXPECT_EQ(32U, confirmationToken_.size());
        return 32U == confirmationToken_.size() &&
               !memcmp(confirmationToken_.data(),
                       testHMAC(confirmationPrefix, formattedMessage_).data(), 32);
    }
};

class ConfirmationTestCallback : public ::testing::VtsHalHidlTargetCallbackBase<ConfirmationArgs>,
                                 public IConfirmationResultCallback {
   public:
    Return<void> result(ResponseCode error, const hidl_vec<uint8_t>& formattedMessage,
                        const hidl_vec<uint8_t>& confirmationToken) override {
        ConfirmationArgs args;
        args.error_ = error;
        args.formattedMessage_ = formattedMessage;
        args.confirmationToken_ = confirmationToken;
        NotifyFromCallback(args);
        return Void();
    }
};

class ConfirmationUIHidlEnvironment : public ::testing::VtsHalHidlTargetTestEnvBase {
   public:
    // get the test environment singleton
    static ConfirmationUIHidlEnvironment* Instance() {
        static ConfirmationUIHidlEnvironment* instance = new ConfirmationUIHidlEnvironment;
        return instance;
    }

    void registerTestServices() override { registerTestService<IConfirmationUI>(); }

   private:
    ConfirmationUIHidlEnvironment(){};

    GTEST_DISALLOW_COPY_AND_ASSIGN_(ConfirmationUIHidlEnvironment);
};

class ConfirmationUIHidlTest : public ::testing::VtsHalHidlTargetTestBase {
   public:
    void TearDown() override { confirmator().abort(); }

    static void SetUpTestCase() {
        string service_name =
            ConfirmationUIHidlEnvironment::Instance()->getServiceName<IConfirmationUI>();
        confirmator_ = IConfirmationUI::getService(service_name);
        ASSERT_NE(nullptr, confirmator_.get());
    }

    static void TearDownTestCase() { confirmator_.clear(); }

    static IConfirmationUI& confirmator() { return *confirmator_; }

   private:
    static sp<IConfirmationUI> confirmator_;
};

sp<IConfirmationUI> ConfirmationUIHidlTest::confirmator_;

#define ASSERT_HAL_CALL(expected, call)                               \
    {                                                                 \
        auto result = call;                                           \
        ASSERT_TRUE(result.isOk());                                   \
        ASSERT_EQ(expected, static_cast<decltype(expected)>(result)); \
    }

struct CnCborDeleter {
    void operator()(cn_cbor* ptr) { cn_cbor_free(ptr); }
};

typedef std::unique_ptr<cn_cbor, CnCborDeleter> CnCborPtr;

// Simulates the User taping Ok
TEST_F(ConfirmationUIHidlTest, UserOkTest) {
    static constexpr char test_prompt[] = "Me first, gimme gimme!";
    static constexpr uint8_t test_extra[] = {0x1, 0x2, 0x3};
    sp<ConfirmationTestCallback> conf_cb = new ConfirmationTestCallback;
    hidl_string prompt_text(test_prompt);
    hidl_vec<uint8_t> extra(test_extra, test_extra + 3);
    ASSERT_HAL_CALL(ResponseCode::OK,
                    confirmator().promptUserConfirmation(conf_cb, prompt_text, extra, "en", {}));

    ASSERT_HAL_CALL(ResponseCode::OK, confirmator().deliverSecureInputEvent(
                                          makeTestToken(TestModeCommands::OK_EVENT)));

    auto result = conf_cb->WaitForCallback();
    ASSERT_EQ(ResponseCode::OK, result.args->error_);

    ASSERT_TRUE(result.args->verifyConfirmationToken());

    cn_cbor_errback cn_cbor_error;
    auto parsed_message =
        CnCborPtr(cn_cbor_decode(result.args->formattedMessage_.data(),
                                 result.args->formattedMessage_.size(), &cn_cbor_error));
    // is parsable CBOR
    ASSERT_TRUE(parsed_message.get());
    // is a map
    ASSERT_EQ(CN_CBOR_MAP, parsed_message->type);

    // the message must have exactly 2 key value pairs.
    // cn_cbor holds 2*<no_of_pairs> in the length field
    ASSERT_EQ(4, parsed_message->length);
    // map has key "prompt"
    auto prompt = cn_cbor_mapget_string(parsed_message.get(), "prompt");
    ASSERT_TRUE(prompt);
    ASSERT_EQ(CN_CBOR_TEXT, prompt->type);
    ASSERT_EQ(22, prompt->length);
    ASSERT_EQ(0, memcmp(test_prompt, prompt->v.str, 22));
    // map has key "extra"
    auto extra_out = cn_cbor_mapget_string(parsed_message.get(), "extra");
    ASSERT_TRUE(extra_out);
    ASSERT_EQ(CN_CBOR_BYTES, extra_out->type);
    ASSERT_EQ(3, extra_out->length);
    ASSERT_EQ(0, memcmp(test_extra, extra_out->v.bytes, 3));
}

// Initiates a confirmation prompt with a message that is too long
TEST_F(ConfirmationUIHidlTest, MessageTooLongTest) {
    static constexpr uint8_t test_extra[static_cast<uint32_t>(MessageSize::MAX)] = {};
    static constexpr char test_prompt[] = "D\'oh!";
    sp<ConfirmationTestCallback> conf_cb = new ConfirmationTestCallback;
    hidl_string prompt_text(test_prompt);
    hidl_vec<uint8_t> extra(test_extra, test_extra + sizeof(test_extra));
    ASSERT_HAL_CALL(ResponseCode::UIErrorMessageTooLong,
                    confirmator().promptUserConfirmation(conf_cb, prompt_text, extra, "en", {}));
}

// If the message gets very long some HAL implementations might fail even before the message
// reaches the trusted app implementation. But the HAL must still diagnose the correct error.
TEST_F(ConfirmationUIHidlTest, MessageWayTooLongTest) {
    static constexpr uint8_t test_extra[static_cast<uint32_t>(MessageSize::MAX) * 10] = {};
    static constexpr char test_prompt[] = "D\'oh!";
    sp<ConfirmationTestCallback> conf_cb = new ConfirmationTestCallback;
    hidl_string prompt_text(test_prompt);
    hidl_vec<uint8_t> extra(test_extra, test_extra + sizeof(test_extra));
    ASSERT_HAL_CALL(ResponseCode::UIErrorMessageTooLong,
                    confirmator().promptUserConfirmation(conf_cb, prompt_text, extra, "en", {}));
}

// Simulates the User tapping the Cancel
TEST_F(ConfirmationUIHidlTest, UserCancelTest) {
    static constexpr char test_prompt[] = "Me first, gimme gimme!";
    static constexpr uint8_t test_extra[] = {0x1, 0x2, 0x3};
    sp<ConfirmationTestCallback> conf_cb = new ConfirmationTestCallback;
    hidl_string prompt_text(test_prompt);
    hidl_vec<uint8_t> extra(test_extra, test_extra + 3);
    ASSERT_HAL_CALL(ResponseCode::OK,
                    confirmator().promptUserConfirmation(conf_cb, prompt_text, extra, "en", {}));

    ASSERT_HAL_CALL(ResponseCode::OK, confirmator().deliverSecureInputEvent(
                                          makeTestToken(TestModeCommands::CANCEL_EVENT)));

    auto result = conf_cb->WaitForCallback();
    ASSERT_EQ(ResponseCode::Canceled, result.args->error_);

    ASSERT_EQ(0U, result.args->confirmationToken_.size());
    ASSERT_EQ(0U, result.args->formattedMessage_.size());
}

// Simulates the framework candelling an ongoing prompt
TEST_F(ConfirmationUIHidlTest, AbortTest) {
    static constexpr char test_prompt[] = "Me first, gimme gimme!";
    static constexpr uint8_t test_extra[] = {0x1, 0x2, 0x3};
    sp<ConfirmationTestCallback> conf_cb = new ConfirmationTestCallback;
    hidl_string prompt_text(test_prompt);
    hidl_vec<uint8_t> extra(test_extra, test_extra + 3);
    ASSERT_HAL_CALL(ResponseCode::OK,
                    confirmator().promptUserConfirmation(conf_cb, prompt_text, extra, "en", {}));

    confirmator().abort();

    auto result = conf_cb->WaitForCallback();
    ASSERT_EQ(ResponseCode::Aborted, result.args->error_);
    ASSERT_EQ(0U, result.args->confirmationToken_.size());
    ASSERT_EQ(0U, result.args->formattedMessage_.size());
}

// Passing malformed UTF-8 to the confirmation UI
// This test passes a string that ends in the middle of a multibyte character
TEST_F(ConfirmationUIHidlTest, MalformedUTF8Test1) {
    static constexpr char test_prompt[] = {char(0xc0), 0};
    static constexpr uint8_t test_extra[] = {0x1, 0x2, 0x3};
    sp<ConfirmationTestCallback> conf_cb = new ConfirmationTestCallback;
    hidl_string prompt_text(test_prompt);
    hidl_vec<uint8_t> extra(test_extra, test_extra + 3);
    ASSERT_HAL_CALL(ResponseCode::UIErrorMalformedUTF8Encoding,
                    confirmator().promptUserConfirmation(conf_cb, prompt_text, extra, "en", {}));
}

// Passing malformed UTF-8 to the confirmation UI
// This test passes a string with a 5-byte character.
TEST_F(ConfirmationUIHidlTest, MalformedUTF8Test2) {
    static constexpr char test_prompt[] = {char(0xf8), char(0x82), char(0x82),
                                           char(0x82), char(0x82), 0};
    static constexpr uint8_t test_extra[] = {0x1, 0x2, 0x3};
    sp<ConfirmationTestCallback> conf_cb = new ConfirmationTestCallback;
    hidl_string prompt_text(test_prompt);
    hidl_vec<uint8_t> extra(test_extra, test_extra + 3);
    ASSERT_HAL_CALL(ResponseCode::UIErrorMalformedUTF8Encoding,
                    confirmator().promptUserConfirmation(conf_cb, prompt_text, extra, "en", {}));
}

// Passing malformed UTF-8 to the confirmation UI
// This test passes a string with a 2-byte character followed by a stray non UTF-8 character.
TEST_F(ConfirmationUIHidlTest, MalformedUTF8Test3) {
    static constexpr char test_prompt[] = {char(0xc0), char(0x82), char(0x83), 0};
    static constexpr uint8_t test_extra[] = {0x1, 0x2, 0x3};
    sp<ConfirmationTestCallback> conf_cb = new ConfirmationTestCallback;
    hidl_string prompt_text(test_prompt);
    hidl_vec<uint8_t> extra(test_extra, test_extra + 3);
    ASSERT_HAL_CALL(ResponseCode::UIErrorMalformedUTF8Encoding,
                    confirmator().promptUserConfirmation(conf_cb, prompt_text, extra, "en", {}));
}

// Test the implementation of HMAC SHA 256 against a golden blob.
TEST(ConfirmationUITestSelfTest, HMAC256SelfTest) {
    const char key_str[32] = "keykeykeykeykeykeykeykeykeykeyk";
    const uint8_t(&key)[32] = *reinterpret_cast<const uint8_t(*)[32]>(key_str);
    auto expected = hex2str("2377fbcaa7fb3f6c20cfa1d9ebc60e9922cf58c909e25e300f3cb57f7805c886");
    auto result = HMacer::hmac256(key, "value1", "value2", "value3");

#ifdef DEBUG_CONFRIMATIONUI_UTILS_TEST
    hexdump(std::cout, reinterpret_cast<const uint8_t*>(expected.data()), 32) << std::endl;
    hexdump(std::cout, result.value().data(), 32) << std::endl;
#endif

    support::ByteBufferProxy expected_bytes(expected);
    ASSERT_TRUE(result.isOk());
    ASSERT_EQ(expected, result.value());
}

}  // namespace test
}  // namespace V1_0
}  // namespace confirmationui
}  // namespace hardware
}  // namespace android

int main(int argc, char** argv) {
    ::testing::InitGoogleTest(&argc, argv);
    std::vector<std::string> positional_args;
    int status = RUN_ALL_TESTS();
    ALOGI("Test result = %d", status);
    return status;
}
