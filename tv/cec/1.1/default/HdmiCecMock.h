/*
 * Copyright (C) 2021 The Android Open Source Project
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

#include <android/hardware/tv/cec/1.1/IHdmiCec.h>
#include <hidl/Status.h>
#include <algorithm>
#include <vector>

using namespace std;

namespace android {
namespace hardware {
namespace tv {
namespace cec {
namespace V1_1 {
namespace implementation {

using ::android::sp;
using ::android::hardware::hidl_string;
using ::android::hardware::hidl_vec;
using ::android::hardware::Return;
using ::android::hardware::Void;
using ::android::hardware::tv::cec::V1_0::CecLogicalAddress;
using ::android::hardware::tv::cec::V1_0::CecMessage;
using ::android::hardware::tv::cec::V1_0::HdmiPortInfo;
using ::android::hardware::tv::cec::V1_0::HdmiPortType;
using ::android::hardware::tv::cec::V1_0::HotplugEvent;
using ::android::hardware::tv::cec::V1_0::IHdmiCecCallback;
using ::android::hardware::tv::cec::V1_0::MaxLength;
using ::android::hardware::tv::cec::V1_0::OptionKey;
using ::android::hardware::tv::cec::V1_0::Result;
using ::android::hardware::tv::cec::V1_0::SendMessageResult;
using ::android::hardware::tv::cec::V1_1::IHdmiCec;

#define CEC_MSG_IN_FIFO "/dev/cec_in_pipe"
#define CEC_MSG_OUT_FIFO "/dev/cec_out_pipe"

struct HdmiCecMock : public IHdmiCec, public hidl_death_recipient {
    HdmiCecMock();
    // Methods from ::android::hardware::tv::cec::V1_0::IHdmiCec follow.
    Return<Result> addLogicalAddress(CecLogicalAddress addr) override;
    Return<void> clearLogicalAddress() override;
    Return<void> getPhysicalAddress(getPhysicalAddress_cb _hidl_cb) override;
    Return<SendMessageResult> sendMessage(const CecMessage& message) override;
    Return<void> setCallback(
            const sp<::android::hardware::tv::cec::V1_0::IHdmiCecCallback>& callback) override;
    Return<int32_t> getCecVersion() override;
    Return<uint32_t> getVendorId() override;
    Return<void> getPortInfo(getPortInfo_cb _hidl_cb) override;
    Return<void> setOption(OptionKey key, bool value) override;
    Return<void> setLanguage(const hidl_string& language) override;
    Return<void> enableAudioReturnChannel(int32_t portId, bool enable) override;
    Return<bool> isConnected(int32_t portId) override;

    // Methods from ::android::hardware::tv::cec::V1_1::IHdmiCec follow.
    Return<Result> addLogicalAddress_1_1(
            ::android::hardware::tv::cec::V1_1::CecLogicalAddress addr) override;
    Return<SendMessageResult> sendMessage_1_1(
            const ::android::hardware::tv::cec::V1_1::CecMessage& message) override;
    Return<void> setCallback_1_1(
            const sp<::android::hardware::tv::cec::V1_1::IHdmiCecCallback>& callback) override;

    virtual void serviceDied(uint64_t /*cookie*/,
                             const wp<::android::hidl::base::V1_0::IBase>& /*who*/) {
        setCallback(nullptr);
    }

    void cec_set_option(int flag, int value);
    void printCecMsgBuf(const char* msg_buf, int len);

  private:
    static void* __threadLoop(void* data);
    void threadLoop();
    int readMessageFromFifo(unsigned char* buf, int msgCount);
    int sendMessageToFifo(const ::android::hardware::tv::cec::V1_1::CecMessage& message);
    void handleHotplugMessage(unsigned char* msgBuf);
    void handleCecMessage(unsigned char* msgBuf, int length);

  private:
    sp<::android::hardware::tv::cec::V1_1::IHdmiCecCallback> mCallback;

    // Variables for the virtual cec hal impl
    uint16_t mPhysicalAddress = 0xFFFF;
    vector<::android::hardware::tv::cec::V1_1::CecLogicalAddress> mLogicalAddresses;
    int32_t mCecVersion = 0x06;
    uint32_t mCecVendorId = 0x01;

    // Port configuration
    int mTotalPorts = 1;
    hidl_vec<HdmiPortInfo> mPortInfo;
    hidl_vec<bool> mPortConnectionStatus;

    // CEC Option value
    int mOptionWakeUp = 0;
    int mOptionEnableCec = 0;
    int mOptionSystemCecControl = 0;
    int mOptionLanguage = 0;

    // Testing variables
    // Input file descriptor
    int mInputFile;
    // Output file descriptor
    int mOutputFile;
    bool mCecThreadRun = true;
    pthread_t mThreadId = 0;
};
}  // namespace implementation
}  // namespace V1_1
}  // namespace cec
}  // namespace tv
}  // namespace hardware
}  // namespace android