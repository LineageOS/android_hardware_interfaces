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

#include <aidl/android/hardware/tv/hdmi/BnHdmi.h>
#include <aidl/android/hardware/tv/hdmi/Result.h>
#include <algorithm>
#include <vector>

using namespace std;

namespace android {
namespace hardware {
namespace tv {
namespace hdmi {
namespace implementation {

using ::aidl::android::hardware::tv::hdmi::BnHdmi;
using ::aidl::android::hardware::tv::hdmi::HdmiPortInfo;
using ::aidl::android::hardware::tv::hdmi::HdmiPortType;
using ::aidl::android::hardware::tv::hdmi::HpdSignal;
using ::aidl::android::hardware::tv::hdmi::IHdmi;
using ::aidl::android::hardware::tv::hdmi::IHdmiCallback;
using ::aidl::android::hardware::tv::hdmi::Result;

#define HDMI_MSG_IN_FIFO "/dev/hdmi_in_pipe"
#define MESSAGE_BODY_MAX_LENGTH 4

struct HdmiMock : public BnHdmi {
    HdmiMock();

    ::ndk::ScopedAStatus getPortInfo(std::vector<HdmiPortInfo>* _aidl_return) override;
    ::ndk::ScopedAStatus isConnected(int32_t portId, bool* _aidl_return) override;
    ::ndk::ScopedAStatus setCallback(const std::shared_ptr<IHdmiCallback>& callback) override;
    ::ndk::ScopedAStatus setHpdSignal(HpdSignal signal) override;
    ::ndk::ScopedAStatus getHpdSignal(HpdSignal* _aidl_return) override;

    void printEventBuf(const char* msg_buf, int len);

  private:
    static void* __threadLoop(void* data);
    void threadLoop();
    int readMessageFromFifo(unsigned char* buf, int msgCount);
    void handleHotplugMessage(unsigned char* msgBuf);

  private:
    static void serviceDied(void* cookie);
    std::shared_ptr<IHdmiCallback> mCallback;

    // Variables for the virtual HDMI hal impl
    std::vector<HdmiPortInfo> mPortInfos;
    std::vector<bool> mPortConnectionStatus;

    // Port configuration
    uint16_t mPhysicalAddress = 0xFFFF;
    int mTotalPorts = 1;

    // HPD Signal being used
    HpdSignal mHpdSignal = HpdSignal::HDMI_HPD_PHYSICAL;

    // Testing variables
    // Input file descriptor
    int mInputFile;
    bool mHdmiThreadRun = true;
    pthread_t mThreadId = 0;

    ::ndk::ScopedAIBinder_DeathRecipient mDeathRecipient;
};
}  // namespace implementation
}  // namespace hdmi
}  // Namespace tv
}  // namespace hardware
}  // namespace android
