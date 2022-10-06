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

#define LOG_TAG "ConfirmationIOAidlHalTest"

#include <algorithm>
#include <condition_variable>
#include <future>
#include <iostream>
#include <memory>
#include <mutex>

#include <aidl/Gtest.h>
#include <aidl/Vintf.h>
#include <aidl/android/hardware/confirmationui/BnConfirmationResultCallback.h>
#include <aidl/android/hardware/confirmationui/IConfirmationUI.h>
#include <aidl/android/hardware/confirmationui/TestModeCommands.h>
#include <aidl/android/hardware/confirmationui/UIOption.h>
#include <android-base/thread_annotations.h>
#include <android/hardware/confirmationui/support/confirmationui_utils.h>
#include <cutils/log.h>

#include <openssl/hmac.h>
#include <openssl/sha.h>

#include <cn-cbor/cn-cbor.h>

#include <android/binder_manager.h>
#include <android/binder_process.h>
#include <android/binder_status.h>

static constexpr int TIMEOUT_PERIOD = 10;

namespace aidl::android::hardware::confirmationui::test {
using ::aidl::android::hardware::security::keymint::HardwareAuthenticatorType;
using ::aidl::android::hardware::security::keymint::HardwareAuthToken;
using ::android::hardware::confirmationui::support::auth_token_key_t;
using ::android::hardware::confirmationui::support::ByteBufferProxy;
using ::android::hardware::confirmationui::support::HMac;
using ::android::hardware::confirmationui::support::hmac_t;
using ::android::hardware::confirmationui::support::hton;
using ::android::hardware::confirmationui::support::NullOr;
using std::shared_ptr;
using std::string;
using std::vector;

namespace {
const auth_token_key_t testKey(static_cast<uint8_t>(IConfirmationUI::TEST_KEY_BYTE));

class HMacImplementation {
  public:
    static NullOr<hmac_t> hmac256(const auth_token_key_t& key,
                                  std::initializer_list<ByteBufferProxy> buffers) {
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
        hmac_t result;
        if (!HMAC_Final(&hmacCtx, result.data(), nullptr)) {
            return {};
        }
        return result;
    }
};

using HMacer = HMac<HMacImplementation>;

template <typename... Data>
vector<uint8_t> testHMAC(const Data&... data) {
    auto hmac = HMacer::hmac256(testKey, data...);
    if (!hmac.isOk()) {
        ADD_FAILURE() << "Failed to compute test hmac.  This is a self-test error.";
        return {};
    }
    vector<uint8_t> result(hmac.value().size());
    std::copy(hmac.value().data(), hmac.value().data() + hmac.value().size(), result.data());
    return result;
}

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
    auth_token.timestamp = {static_cast<int64_t>(timestamp)};

    // Canonical form  of auth-token v0
    // version (1 byte)
    // challenge (8 bytes)
    // user_id (8 bytes)
    // authenticator_id (8 bytes)
    // authenticator_type (4 bytes)
    // timestamp (8 bytes)
    // total 37 bytes
    auth_token.mac = testHMAC("\0",
                              toBytes(auth_token.challenge),                //
                              toBytes(auth_token.userId),                   //
                              toBytes(auth_token.authenticatorId),          //
                              toBytes(hton(auth_token.authenticatorType)),  //
                              toBytes(hton(auth_token.timestamp)));         //

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

int getReturnCode(const ::ndk::ScopedAStatus& result) {
    if (result.isOk()) return IConfirmationUI::OK;

    if (result.getExceptionCode() == EX_SERVICE_SPECIFIC) {
        return static_cast<int>(result.getServiceSpecificError());
    }
    return result.getStatus();
}

}  // namespace

class ConfirmationUIAidlTest : public ::testing::TestWithParam<std::string> {
  public:
    void TearDown() override { confirmator_->abort(); }
    void SetUp() override {
        std::string name = GetParam();
        ASSERT_TRUE(AServiceManager_isDeclared(name.c_str())) << name;
        ndk::SpAIBinder binder(AServiceManager_waitForService(name.c_str()));
        ASSERT_NE(binder, nullptr);
        confirmator_ = IConfirmationUI::fromBinder(binder);
        ASSERT_NE(confirmator_, nullptr);
    }

    // Used as a mechanism to inform the test about data/event callback
    inline void notify() {
        std::unique_lock<std::mutex> lock(mtx_);
        cv_.notify_one();
    }

    // Test code calls this function to wait for data/event callback
    inline std::cv_status wait() {
        std::unique_lock<std::mutex> lock(mtx_);
        auto now = std::chrono::system_clock::now();
        std::cv_status status = cv_.wait_until(lock, now + std::chrono::seconds(TIMEOUT_PERIOD));
        return status;
    }

  protected:
    shared_ptr<IConfirmationUI> confirmator_;

  private:
    // synchronization objects
    std::mutex mtx_;
    std::condition_variable cv_;
};

class ConfirmationTestCallback
    : public ::aidl::android::hardware::confirmationui::BnConfirmationResultCallback {
  public:
    ConfirmationTestCallback(ConfirmationUIAidlTest& parent) : parent_(parent){};
    virtual ~ConfirmationTestCallback() = default;

    ::ndk::ScopedAStatus result(int32_t err, const vector<uint8_t>& msg,
                                const vector<uint8_t>& confToken) override {
        error_ = err;
        formattedMessage_ = msg;
        confirmationToken_ = confToken;
        parent_.notify();
        return ndk::ScopedAStatus::ok();
    }

    bool verifyConfirmationToken() {
        static constexpr char confirmationPrefix[] = "confirmation token";
        EXPECT_EQ(32U, confirmationToken_.size());
        return 32U == confirmationToken_.size() &&
               !memcmp(confirmationToken_.data(),
                       testHMAC(confirmationPrefix, formattedMessage_).data(), 32);
    }

    int error_;
    vector<uint8_t> formattedMessage_;
    vector<uint8_t> confirmationToken_;

  private:
    ConfirmationUIAidlTest& parent_;
};

struct CnCborDeleter {
    void operator()(cn_cbor* ptr) { cn_cbor_free(ptr); }
};

typedef std::unique_ptr<cn_cbor, CnCborDeleter> CnCborPtr;

// Simulates the User taping Ok
TEST_P(ConfirmationUIAidlTest, UserOkTest) {
    static constexpr char test_prompt[] = "Me first, gimme gimme!";
    static constexpr uint8_t test_extra[] = {0x1, 0x2, 0x3};
    shared_ptr<ConfirmationTestCallback> conf_cb =
            ::ndk::SharedRefBase::make<ConfirmationTestCallback>(*this);
    vector<uint8_t> prompt_text(test_prompt, test_prompt + strlen(test_prompt));
    vector<uint8_t> extra(test_extra, test_extra + 3);
    ASSERT_TRUE(confirmator_->promptUserConfirmation(conf_cb, prompt_text, extra, "en", {}).isOk());
    // Simulate the user taping ok.
    ASSERT_TRUE(confirmator_->deliverSecureInputEvent(makeTestToken(TestModeCommands::OK_EVENT))
                        .isOk());
    // Wait for the callback.
    EXPECT_EQ(std::cv_status::no_timeout, wait());
    ASSERT_EQ(IConfirmationUI::OK, conf_cb->error_);

    ASSERT_TRUE(conf_cb->verifyConfirmationToken());

    cn_cbor_errback cn_cbor_error;
    auto parsed_message = CnCborPtr(cn_cbor_decode(
            conf_cb->formattedMessage_.data(), conf_cb->formattedMessage_.size(), &cn_cbor_error));
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
TEST_P(ConfirmationUIAidlTest, MessageTooLongTest) {
    static constexpr uint8_t test_extra[IConfirmationUI::MAX_MESSAGE_SIZE] = {};
    static constexpr char test_prompt[] = "D\'oh!";
    shared_ptr<ConfirmationTestCallback> conf_cb =
            ::ndk::SharedRefBase::make<ConfirmationTestCallback>(*this);
    vector<uint8_t> prompt_text(test_prompt, test_prompt + strlen(test_prompt));
    vector<uint8_t> extra(test_extra, test_extra + sizeof(test_extra));
    auto result = confirmator_->promptUserConfirmation(conf_cb, prompt_text, extra, "en", {});
    ASSERT_EQ(IConfirmationUI::UI_ERROR_MESSAGE_TOO_LONG, getReturnCode(result));
}

// If the message gets very long some HAL implementations might fail even before the message
// reaches the trusted app implementation. But the HAL must still diagnose the correct error.
TEST_P(ConfirmationUIAidlTest, MessageWayTooLongTest) {
    static constexpr uint8_t test_extra[(IConfirmationUI::MAX_MESSAGE_SIZE)*10] = {};
    static constexpr char test_prompt[] = "D\'oh!";
    shared_ptr<ConfirmationTestCallback> conf_cb =
            ::ndk::SharedRefBase::make<ConfirmationTestCallback>(*this);
    vector<uint8_t> prompt_text(test_prompt, test_prompt + strlen(test_prompt));
    vector<uint8_t> extra(test_extra, test_extra + sizeof(test_extra));
    auto result = confirmator_->promptUserConfirmation(conf_cb, prompt_text, extra, "en", {});
    ASSERT_EQ(IConfirmationUI::UI_ERROR_MESSAGE_TOO_LONG, getReturnCode(result));
}

// Simulates the User tapping the Cancel
TEST_P(ConfirmationUIAidlTest, UserCancelTest) {
    static constexpr char test_prompt[] = "Me first, gimme gimme!";
    static constexpr uint8_t test_extra[] = {0x1, 0x2, 0x3};
    shared_ptr<ConfirmationTestCallback> conf_cb =
            ::ndk::SharedRefBase::make<ConfirmationTestCallback>(*this);
    vector<uint8_t> prompt_text(test_prompt, test_prompt + strlen(test_prompt));
    vector<uint8_t> extra(test_extra, test_extra + 3);
    ASSERT_TRUE(confirmator_->promptUserConfirmation(conf_cb, prompt_text, extra, "en", {}).isOk());

    // Simulate the user taping ok.
    ASSERT_TRUE(confirmator_->deliverSecureInputEvent(makeTestToken(TestModeCommands::CANCEL_EVENT))
                        .isOk());
    // Wait for the callback.
    EXPECT_EQ(std::cv_status::no_timeout, wait());
    ASSERT_EQ(IConfirmationUI::CANCELED, conf_cb->error_);

    ASSERT_EQ(0U, conf_cb->confirmationToken_.size());
    ASSERT_EQ(0U, conf_cb->formattedMessage_.size());
}

// Simulates the framework cancelling an ongoing prompt
TEST_P(ConfirmationUIAidlTest, AbortTest) {
    static constexpr char test_prompt[] = "Me first, gimme gimme!";
    static constexpr uint8_t test_extra[] = {0x1, 0x2, 0x3};
    shared_ptr<ConfirmationTestCallback> conf_cb =
            ::ndk::SharedRefBase::make<ConfirmationTestCallback>(*this);
    vector<uint8_t> prompt_text(test_prompt, test_prompt + strlen(test_prompt));
    vector<uint8_t> extra(test_extra, test_extra + 3);
    ASSERT_TRUE(confirmator_->promptUserConfirmation(conf_cb, prompt_text, extra, "en", {}).isOk());

    confirmator_->abort();

    // Wait for the callback.
    EXPECT_EQ(std::cv_status::no_timeout, wait());
    ASSERT_EQ(IConfirmationUI::ABORTED, conf_cb->error_);
    ASSERT_EQ(0U, conf_cb->confirmationToken_.size());
    ASSERT_EQ(0U, conf_cb->formattedMessage_.size());
}

// Tests if the confirmation dialog can successfully render 100 'W' characters as required by
// the design guidelines.
TEST_P(ConfirmationUIAidlTest, PortableMessageTest1) {
    static constexpr char test_prompt[] =
            "WWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWW"
            "WWWWWWWWWWWWWW";
    static constexpr uint8_t test_extra[] = {0x1, 0x2, 0x3};
    shared_ptr<ConfirmationTestCallback> conf_cb =
            ::ndk::SharedRefBase::make<ConfirmationTestCallback>(*this);
    vector<uint8_t> prompt_text(test_prompt, test_prompt + strlen(test_prompt));
    vector<uint8_t> extra(test_extra, test_extra + 3);
    ASSERT_TRUE(confirmator_->promptUserConfirmation(conf_cb, prompt_text, extra, "en", {}).isOk());

    confirmator_->abort();

    // Wait for the callback.
    EXPECT_EQ(std::cv_status::no_timeout, wait());
    ASSERT_EQ(IConfirmationUI::ABORTED, conf_cb->error_);
    ASSERT_EQ(0U, conf_cb->confirmationToken_.size());
    ASSERT_EQ(0U, conf_cb->formattedMessage_.size());
}

// Tests if the confirmation dialog can successfully render 100 'W' characters as required by
// the design guidelines in magnified mode.
TEST_P(ConfirmationUIAidlTest, PortableMessageTest1Magnified) {
    static constexpr char test_prompt[] =
            "WWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWW"
            "WWWWWWWWWWWWWW";
    static constexpr uint8_t test_extra[] = {0x1, 0x2, 0x3};
    shared_ptr<ConfirmationTestCallback> conf_cb =
            ::ndk::SharedRefBase::make<ConfirmationTestCallback>(*this);
    vector<uint8_t> prompt_text(test_prompt, test_prompt + strlen(test_prompt));
    vector<uint8_t> extra(test_extra, test_extra + 3);
    ASSERT_TRUE(confirmator_
                        ->promptUserConfirmation(conf_cb, prompt_text, extra, "en",
                                                 {UIOption::ACCESSIBILITY_MAGNIFIED})
                        .isOk());

    confirmator_->abort();

    // Wait for the callback.
    EXPECT_EQ(std::cv_status::no_timeout, wait());
    ASSERT_EQ(IConfirmationUI::ABORTED, conf_cb->error_);
    ASSERT_EQ(0U, conf_cb->confirmationToken_.size());
    ASSERT_EQ(0U, conf_cb->formattedMessage_.size());
}

// Tests if the confirmation dialog can successfully render 8 groups of 12 'W' characters as
// required by the design guidelines.
TEST_P(ConfirmationUIAidlTest, PortableMessageTest2) {
    static constexpr char test_prompt[] =
            "WWWWWWWWWWWW WWWWWWWWWWWW WWWWWWWWWWWW WWWWWWWWWWWW WWWWWWWWWWWW WWWWWWWWWWWW "
            "WWWWWWWWWWWW WWWWWWWWWWWW";
    static constexpr uint8_t test_extra[] = {0x1, 0x2, 0x3};
    shared_ptr<ConfirmationTestCallback> conf_cb =
            ::ndk::SharedRefBase::make<ConfirmationTestCallback>(*this);
    vector<uint8_t> prompt_text(test_prompt, test_prompt + strlen(test_prompt));
    vector<uint8_t> extra(test_extra, test_extra + 3);
    ASSERT_TRUE(confirmator_->promptUserConfirmation(conf_cb, prompt_text, extra, "en", {}).isOk());

    confirmator_->abort();

    // Wait for the callback.
    EXPECT_EQ(std::cv_status::no_timeout, wait());
    ASSERT_EQ(IConfirmationUI::ABORTED, conf_cb->error_);
    ASSERT_EQ(0U, conf_cb->confirmationToken_.size());
    ASSERT_EQ(0U, conf_cb->formattedMessage_.size());
}

// Tests if the confirmation dialog can successfully render 8 groups of 12 'W' characters as
// required by the design guidelines in magnified mode.
TEST_P(ConfirmationUIAidlTest, PortableMessageTest2Magnified) {
    static constexpr char test_prompt[] =
            "WWWWWWWWWWWW WWWWWWWWWWWW WWWWWWWWWWWW WWWWWWWWWWWW WWWWWWWWWWWW WWWWWWWWWWWW "
            "WWWWWWWWWWWW WWWWWWWWWWWW";
    static constexpr uint8_t test_extra[] = {0x1, 0x2, 0x3};
    shared_ptr<ConfirmationTestCallback> conf_cb =
            ::ndk::SharedRefBase::make<ConfirmationTestCallback>(*this);
    vector<uint8_t> prompt_text(test_prompt, test_prompt + strlen(test_prompt));
    vector<uint8_t> extra(test_extra, test_extra + 3);
    ASSERT_TRUE(confirmator_
                        ->promptUserConfirmation(conf_cb, prompt_text, extra, "en",
                                                 {UIOption::ACCESSIBILITY_MAGNIFIED})
                        .isOk());

    confirmator_->abort();

    // Wait for the callback.
    EXPECT_EQ(std::cv_status::no_timeout, wait());
    ASSERT_EQ(IConfirmationUI::ABORTED, conf_cb->error_);
    ASSERT_EQ(0U, conf_cb->confirmationToken_.size());
    ASSERT_EQ(0U, conf_cb->formattedMessage_.size());
}

// Passing malformed UTF-8 to the confirmation UI
// This test passes a string that ends in the middle of a multibyte character
TEST_P(ConfirmationUIAidlTest, MalformedUTF8Test1) {
    static constexpr char test_prompt[] = {char(0xc0), 0};
    static constexpr uint8_t test_extra[] = {0x1, 0x2, 0x3};
    shared_ptr<ConfirmationTestCallback> conf_cb =
            ::ndk::SharedRefBase::make<ConfirmationTestCallback>(*this);
    vector<uint8_t> prompt_text(test_prompt, test_prompt + strlen(test_prompt));
    vector<uint8_t> extra(test_extra, test_extra + 3);
    auto result = confirmator_->promptUserConfirmation(conf_cb, prompt_text, extra, "en", {});
    ASSERT_EQ(IConfirmationUI::UI_ERROR_MALFORMED_UTF8ENCODING, getReturnCode(result));
}

// Passing malformed UTF-8 to the confirmation UI
// This test passes a string with a 5-byte character.
TEST_P(ConfirmationUIAidlTest, MalformedUTF8Test2) {
    static constexpr char test_prompt[] = {char(0xf8), char(0x82), char(0x82),
                                           char(0x82), char(0x82), 0};
    static constexpr uint8_t test_extra[] = {0x1, 0x2, 0x3};
    shared_ptr<ConfirmationTestCallback> conf_cb =
            ::ndk::SharedRefBase::make<ConfirmationTestCallback>(*this);
    vector<uint8_t> prompt_text(test_prompt, test_prompt + strlen(test_prompt));
    vector<uint8_t> extra(test_extra, test_extra + 3);
    auto result = confirmator_->promptUserConfirmation(conf_cb, prompt_text, extra, "en", {});
    ASSERT_EQ(IConfirmationUI::UI_ERROR_MALFORMED_UTF8ENCODING, getReturnCode(result));
}

// Passing malformed UTF-8 to the confirmation UI
// This test passes a string with a 2-byte character followed by a stray non UTF-8 character.
TEST_P(ConfirmationUIAidlTest, MalformedUTF8Test3) {
    static constexpr char test_prompt[] = {char(0xc0), char(0x82), char(0x83), 0};
    static constexpr uint8_t test_extra[] = {0x1, 0x2, 0x3};
    shared_ptr<ConfirmationTestCallback> conf_cb =
            ::ndk::SharedRefBase::make<ConfirmationTestCallback>(*this);
    vector<uint8_t> prompt_text(test_prompt, test_prompt + strlen(test_prompt));
    vector<uint8_t> extra(test_extra, test_extra + 3);
    auto result = confirmator_->promptUserConfirmation(conf_cb, prompt_text, extra, "en", {});
    ASSERT_EQ(IConfirmationUI::UI_ERROR_MALFORMED_UTF8ENCODING, getReturnCode(result));
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

    ByteBufferProxy expected_bytes(expected);
    ASSERT_TRUE(result.isOk());
    ASSERT_EQ(expected, result.value());
}

GTEST_ALLOW_UNINSTANTIATED_PARAMETERIZED_TEST(ConfirmationUIAidlTest);
INSTANTIATE_TEST_SUITE_P(
        PerInstance, ConfirmationUIAidlTest,
        testing::ValuesIn(::android::getAidlHalInstanceNames(IConfirmationUI::descriptor)),
        ::android::PrintInstanceNameToString);

}  // namespace aidl::android::hardware::confirmationui::test

int main(int argc, char** argv) {
    ::testing::InitGoogleTest(&argc, argv);
    ABinderProcess_setThreadPoolMaxThreadCount(1);
    ABinderProcess_startThreadPool();
    return RUN_ALL_TESTS();
}
