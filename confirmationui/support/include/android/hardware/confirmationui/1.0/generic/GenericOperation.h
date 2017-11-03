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

#ifndef CONFIRMATIONUI_1_0_DEFAULT_GENERICOPERATION_H_
#define CONFIRMATIONUI_1_0_DEFAULT_GENERICOPERATION_H_

#include <android/hardware/confirmationui/1.0/types.h>
#include <android/hardware/confirmationui/support/cbor.h>
#include <android/hardware/confirmationui/support/confirmationui_utils.h>
#include <android/hardware/keymaster/4.0/types.h>

namespace android {
namespace hardware {
namespace confirmationui {
namespace V1_0 {
namespace generic {

namespace {
using namespace ::android::hardware::confirmationui::support;
using ::android::hardware::keymaster::V4_0::HardwareAuthToken;
using ::android::hardware::keymaster::V4_0::HardwareAuthenticatorType;

inline bool hasOption(UIOption option, const hidl_vec<UIOption>& uiOptions) {
    for (auto& o : uiOptions) {
        if (o == option) return true;
    }
    return false;
}

template <typename Callback, typename TimeStamper, typename HmacImplementation>
class Operation {
    using HMacer = support::HMac<HmacImplementation>;

   public:
    Operation() : error_(ResponseCode::Ignored), formattedMessageLength_(0) {}

    ResponseCode init(const Callback& resultCB, const hidl_string& promptText,
                      const hidl_vec<uint8_t>& extraData, const hidl_string& locale,
                      const hidl_vec<UIOption>& uiOptions) {
        (void)locale;
        (void)uiOptions;
        resultCB_ = resultCB;
        if (error_ != ResponseCode::Ignored) return ResponseCode::OperationPending;
        // TODO make copy of promptText before using it may reside in shared buffer
        auto state = write(
            WriteState(formattedMessageBuffer_),
            map(pair(text("prompt"), text(promptText)), pair(text("extra"), bytes(extraData))));
        switch (state.error_) {
            case Error::OK:
                break;
            case Error::OUT_OF_DATA:
                return ResponseCode::UIErrorMessageTooLong;
            case Error::MALFORMED_UTF8:
                return ResponseCode::UIErrorMalformedUTF8Encoding;
            case Error::MALFORMED:
            default:
                return ResponseCode::Unexpected;
        }
        formattedMessageLength_ = state.data_ - formattedMessageBuffer_;
        // setup TUI and diagnose more UI errors here.
        // on success record the start time
        startTime_ = TimeStamper::now();
        if (!startTime_.isOk()) {
            return ResponseCode::SystemError;
        }
        error_ = ResponseCode::OK;
        return ResponseCode::OK;
    }

    void setHmacKey(const uint8_t (&key)[32]) { hmacKey_ = {key}; }

    void abort() {
        // tear down TUI here
        if (isPending()) {
            resultCB_->result(ResponseCode::Aborted, {}, {});
            error_ = ResponseCode::Ignored;
        }
    }

    void userCancel() {
        // tear down TUI here
        if (isPending()) error_ = ResponseCode::Canceled;
    }

    void finalize(const uint8_t key[32]) {
        if (error_ == ResponseCode::Ignored) return;
        resultCB_->result(error_, getMessage(), userConfirm(key));
        error_ = ResponseCode::Ignored;
        resultCB_ = {};
    }

    bool isPending() const { return error_ != ResponseCode::Ignored; }

    static Operation& get() {
        static Operation operation;
        return operation;
    }

    ResponseCode deliverSecureInputEvent(const HardwareAuthToken& secureInputToken) {
        constexpr uint8_t testKeyByte = static_cast<uint8_t>(TestKeyBits::BYTE);
        constexpr uint8_t testKey[32] = {testKeyByte, testKeyByte, testKeyByte, testKeyByte,
                                         testKeyByte, testKeyByte, testKeyByte, testKeyByte,
                                         testKeyByte, testKeyByte, testKeyByte, testKeyByte,
                                         testKeyByte, testKeyByte, testKeyByte, testKeyByte};

        auto hmac = HMacer::hmac256(testKey, "\0", bytes_cast(secureInputToken.challenge),
                                    bytes_cast(secureInputToken.userId),
                                    bytes_cast(secureInputToken.authenticatorId),
                                    bytes_cast(hton(secureInputToken.authenticatorType)),
                                    bytes_cast(hton(secureInputToken.timestamp)));
        if (!hmac.isOk()) return ResponseCode::Unexpected;
        if (hmac.value() == secureInputToken.mac) {
            // okay so this is a test token
            switch (static_cast<TestModeCommands>(secureInputToken.challenge)) {
                case TestModeCommands::OK_EVENT: {
                    if (isPending()) {
                        finalize(testKey);
                        return ResponseCode::OK;
                    } else {
                        return ResponseCode::Ignored;
                    }
                }
                case TestModeCommands::CANCEL_EVENT: {
                    bool ignored = !isPending();
                    userCancel();
                    finalize(testKey);
                    return ignored ? ResponseCode::Ignored : ResponseCode::OK;
                }
                default:
                    return ResponseCode::Ignored;
            }
        }
        return ResponseCode::Ignored;
    }

   private:
    bool acceptAuthToken(const HardwareAuthToken&) { return false; }
    hidl_vec<uint8_t> getMessage() {
        hidl_vec<uint8_t> result;
        if (error_ != ResponseCode::OK) return {};
        result.setToExternal(formattedMessageBuffer_, formattedMessageLength_);
        return result;
    }
    hidl_vec<uint8_t> userConfirm(const uint8_t key[32]) {
        // tear down TUI here
        if (error_ != ResponseCode::OK) return {};
        confirmationTokenScratchpad_ = HMacer::hmac256(key, "confirmation token", getMessage());
        if (!confirmationTokenScratchpad_.isOk()) {
            error_ = ResponseCode::Unexpected;
            return {};
        }
        hidl_vec<uint8_t> result;
        result.setToExternal(confirmationTokenScratchpad_->data(),
                             confirmationTokenScratchpad_->size());
        return result;
    }

    ResponseCode error_;
    uint8_t formattedMessageBuffer_[uint32_t(MessageSize::MAX)];
    size_t formattedMessageLength_;
    NullOr<array<uint8_t, 32>> confirmationTokenScratchpad_;
    Callback resultCB_;
    typename TimeStamper::TimeStamp startTime_;
    NullOr<array<uint8_t, 32>> hmacKey_;
};

}  // namespace
}  // namespace generic
}  // namespace V1_0
}  // namespace confirmationui
}  // namespace hardware
}  // namespace android

#endif  // CONFIRMATIONUI_1_0_DEFAULT_GENERICOPERATION_H_
