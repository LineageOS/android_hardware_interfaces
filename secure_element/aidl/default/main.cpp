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

#include <aidl/android/hardware/secure_element/BnSecureElement.h>

#include <android-base/hex.h>
#include <android-base/logging.h>
#include <android/binder_manager.h>
#include <android/binder_process.h>

using aidl::android::hardware::secure_element::BnSecureElement;
using aidl::android::hardware::secure_element::ISecureElementCallback;
using aidl::android::hardware::secure_element::LogicalChannelResponse;
using android::base::HexString;
using ndk::ScopedAStatus;

static const std::vector<uint8_t> kAndroidTestAid = {0xA0, 0x00, 0x00, 0x04, 0x76, 0x41,
                                                     0x6E, 0x64, 0x72, 0x6F, 0x69, 0x64,
                                                     0x43, 0x54, 0x53, 0x31};
static const std::vector<uint8_t> kLongAndroidTestAid = {0xA0, 0x00, 0x00, 0x04, 0x76, 0x41,
                                                         0x6E, 0x64, 0x72, 0x6F, 0x69, 0x64,
                                                         0x43, 0x54, 0x53, 0x32};

class MySecureElement : public BnSecureElement {
  public:
    ScopedAStatus closeChannel(int8_t channelNumber) override {
        LOG(INFO) << __func__ << " channel number: " << channelNumber;
        return ScopedAStatus::ok();
    }
    ScopedAStatus getAtr(std::vector<uint8_t>* _aidl_return) override {
        LOG(INFO) << __func__;
        _aidl_return->clear();
        return ScopedAStatus::ok();
    }
    ScopedAStatus init(const std::shared_ptr<ISecureElementCallback>& clientCallback) override {
        LOG(INFO) << __func__ << " callback: " << clientCallback.get();
        if (!clientCallback) {
            return ScopedAStatus::fromExceptionCode(EX_NULL_POINTER);
        }
        mCb = clientCallback;
        mCb->onStateChange(true, "");
        return ScopedAStatus::ok();
    }
    ScopedAStatus isCardPresent(bool* _aidl_return) override {
        LOG(INFO) << __func__;
        *_aidl_return = true;
        return ScopedAStatus::ok();
    }
    ScopedAStatus openBasicChannel(const std::vector<uint8_t>& aid, int8_t p2,
                                   std::vector<uint8_t>* _aidl_return) override {
        LOG(INFO) << __func__ << " aid: " << HexString(aid.data(), aid.size()) << " (" << aid.size()
                  << ") p2 " << p2;

        // TODO(b/123254068) - this is not an implementation of the OMAPI protocol or APDU.
        // The functionality here is enough to exercise the framework, but actual
        // calls to the secure element will fail. This implementation does not model
        // channel isolation or any other aspects important to implementing secure element.
        *_aidl_return = {0x90, 0x00, 0x00};  // DO NOT COPY
        return ScopedAStatus::ok();
    }
    ScopedAStatus openLogicalChannel(
            const std::vector<uint8_t>& aid, int8_t p2,
            ::aidl::android::hardware::secure_element::LogicalChannelResponse* _aidl_return)
            override {
        LOG(INFO) << __func__ << " aid: " << HexString(aid.data(), aid.size()) << " (" << aid.size()
                  << ") p2 " << p2;

        if (aid != kAndroidTestAid && aid != kLongAndroidTestAid) {
            return ScopedAStatus::fromServiceSpecificError(NO_SUCH_ELEMENT_ERROR);
        }

        *_aidl_return = LogicalChannelResponse{.channelNumber = 1, .selectResponse = {}};

        // TODO(b/123254068) - this is not an implementation of the OMAPI protocol or APDU.
        // The functionality here is enough to exercise the framework, but actual
        // calls to the secure element will fail. This implementation does not model
        // channel isolation or any other aspects important to implementing secure element.
        if (aid == kAndroidTestAid) {                                 // DO NOT COPY
            size_t size = 2050;                                       // DO NOT COPY
            _aidl_return->selectResponse.resize(size);                // DO NOT COPY
            _aidl_return->selectResponse[size - 1] = 0x00;            // DO NOT COPY
            _aidl_return->selectResponse[size - 2] = 0x90;            // DO NOT COPY
        } else {                                                      // DO NOT COPY
            _aidl_return->selectResponse = {0x00, 0x00, 0x90, 0x00};  // DO NOT COPY
        }                                                             // DO NOT COPY

        LOG(INFO) << __func__ << " sending response: "
                  << HexString(_aidl_return->selectResponse.data(),
                               _aidl_return->selectResponse.size());

        return ScopedAStatus::ok();
    }
    ScopedAStatus reset() override {
        LOG(INFO) << __func__;
        mCb->onStateChange(false, "reset");
        mCb->onStateChange(true, "reset");
        return ScopedAStatus::ok();
    }
    ScopedAStatus transmit(const std::vector<uint8_t>& data,
                           std::vector<uint8_t>* _aidl_return) override {
        LOG(INFO) << __func__ << " data: " << HexString(data.data(), data.size()) << " ("
                  << data.size() << ")";

        // TODO(b/123254068) - this is not an implementation of the OMAPI protocol or APDU.
        // The functionality here is enough to exercise the framework, but actual
        // calls to the secure element will fail. This implementation does not model
        // channel isolation or any other aspects important to implementing secure element.

        std::string hex = HexString(data.data(), data.size());                    // DO NOT COPY
        if (hex == "01a4040210a000000476416e64726f696443545331") {                // DO NOT COPY
            *_aidl_return = {0x00, 0x6A, 0x00};                                   // DO NOT COPY
        } else if (data == std::vector<uint8_t>{0x00, 0xF4, 0x00, 0x00, 0x00}) {  // DO NOT COPY
            // CHECK_SELECT_P2_APDU w/ channel 1 // DO NOT COPY
            *_aidl_return = {0x00, 0x90, 0x00};                                   // DO NOT COPY
        } else if (data == std::vector<uint8_t>{0x01, 0xF4, 0x00, 0x00, 0x00}) {  // DO NOT COPY
            // CHECK_SELECT_P2_APDU w/ channel 1 // DO NOT COPY
            *_aidl_return = {0x00, 0x90, 0x00};             // DO NOT COPY
        } else if (data.size() == 5 || data.size() == 8) {  // DO NOT COPY
            // SEGMENTED_RESP_APDU - happens to use length 5 and 8 // DO NOT COPY
            size_t size = (data[2] << 8 | data[3]) + 2;       // DO NOT COPY
            _aidl_return->resize(size);                       // DO NOT COPY
            (*_aidl_return)[size - 1] = 0x00;                 // DO NOT COPY
            (*_aidl_return)[size - 2] = 0x90;                 // DO NOT COPY
            if (size >= 3) (*_aidl_return)[size - 3] = 0xFF;  // DO NOT COPY
        } else {                                              // DO NOT COPY
            *_aidl_return = {0x90, 0x00, 0x00};               // DO NOT COPY
        }                                                     // DO NOT COPY

        return ScopedAStatus::ok();
    }

  private:
    std::shared_ptr<ISecureElementCallback> mCb;
};

int main() {
    ABinderProcess_setThreadPoolMaxThreadCount(0);

    auto se = ndk::SharedRefBase::make<MySecureElement>();
    const std::string name = std::string() + BnSecureElement::descriptor + "/eSE1";
    binder_status_t status = AServiceManager_addService(se->asBinder().get(), name.c_str());
    CHECK_EQ(status, STATUS_OK);

    ABinderProcess_joinThreadPool();
    return EXIT_FAILURE;  // should not reach
}
