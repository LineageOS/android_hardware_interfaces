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

#include <aidl/android/hardware/tv/hdmi/cec/BnHdmiCec.h>
#include <algorithm>
#include <vector>

using namespace std;

namespace android {
namespace hardware {
namespace tv {
namespace hdmi {
namespace cec {
namespace implementation {

using ::aidl::android::hardware::tv::hdmi::cec::BnHdmiCec;
using ::aidl::android::hardware::tv::hdmi::cec::CecLogicalAddress;
using ::aidl::android::hardware::tv::hdmi::cec::CecMessage;
using ::aidl::android::hardware::tv::hdmi::cec::IHdmiCec;
using ::aidl::android::hardware::tv::hdmi::cec::IHdmiCecCallback;
using ::aidl::android::hardware::tv::hdmi::cec::Result;
using ::aidl::android::hardware::tv::hdmi::cec::SendMessageResult;

#define CEC_MSG_IN_FIFO "/dev/cec_aidl_in_pipe"
#define CEC_MSG_OUT_FIFO "/dev/cec_aidl_out_pipe"

struct HdmiCecMock : public BnHdmiCec {
    HdmiCecMock();
    ~HdmiCecMock();
    ::ndk::ScopedAStatus addLogicalAddress(CecLogicalAddress addr, Result* _aidl_return) override;
    ::ndk::ScopedAStatus clearLogicalAddress() override;
    ::ndk::ScopedAStatus enableAudioReturnChannel(int32_t portId, bool enable) override;
    ::ndk::ScopedAStatus getCecVersion(int32_t* _aidl_return) override;
    ::ndk::ScopedAStatus getPhysicalAddress(int32_t* _aidl_return) override;
    ::ndk::ScopedAStatus getVendorId(int32_t* _aidl_return) override;
    ::ndk::ScopedAStatus sendMessage(const CecMessage& message,
                                     SendMessageResult* _aidl_return) override;
    ::ndk::ScopedAStatus setCallback(const std::shared_ptr<IHdmiCecCallback>& callback) override;
    ::ndk::ScopedAStatus setLanguage(const std::string& language) override;
    ::ndk::ScopedAStatus enableWakeupByOtp(bool value) override;
    ::ndk::ScopedAStatus enableCec(bool value) override;
    ::ndk::ScopedAStatus enableSystemCecControl(bool value) override;
    void printCecMsgBuf(const char* msg_buf, int len);

  private:
    static void* __threadLoop(void* data);
    void threadLoop();
    int readMessageFromFifo(unsigned char* buf, int msgCount);
    int sendMessageToFifo(const CecMessage& message);
    void handleCecMessage(unsigned char* msgBuf, int length);

  private:
    static void serviceDied(void* cookie);
    std::shared_ptr<IHdmiCecCallback> mCallback;

    // Variables for the virtual cec hal impl
    uint16_t mPhysicalAddress = 0xFFFF;
    vector<CecLogicalAddress> mLogicalAddresses;
    int32_t mCecVersion = 0x06;
    uint32_t mCecVendorId = 0x01;

    // CEC Option value
    bool mOptionWakeUp = 0;
    bool mOptionEnableCec = 0;
    bool mOptionSystemCecControl = 0;
    int mOptionLanguage;

    // Testing variables
    // Input file descriptor
    int mInputFile;
    // Output file descriptor
    int mOutputFile;
    bool mCecThreadRun = true;
    pthread_t mThreadId = 0;

    ::ndk::ScopedAIBinder_DeathRecipient mDeathRecipient;
};
}  // namespace implementation
}  // namespace cec
}  // namespace hdmi
}  // namespace tv
}  // namespace hardware
}  // namespace android
