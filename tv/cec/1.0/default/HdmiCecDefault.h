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

#include <android/hardware/tv/cec/1.0/IHdmiCec.h>
#include <hardware/hdmi_cec.h>
#include <linux/cec.h>
#include <thread>

namespace android {
namespace hardware {
namespace tv {
namespace cec {
namespace V1_0 {
namespace implementation {

using std::thread;

class HdmiCecDefault : public IHdmiCec, public hidl_death_recipient {
  public:
    HdmiCecDefault();
    ~HdmiCecDefault();
    // Methods from ::android::hardware::tv::cec::V1_0::IHdmiCec follow.
    Return<Result> addLogicalAddress(CecLogicalAddress addr) override;
    Return<void> clearLogicalAddress() override;
    Return<void> getPhysicalAddress(getPhysicalAddress_cb _hidl_cb) override;
    Return<SendMessageResult> sendMessage(const CecMessage& message) override;
    Return<void> setCallback(const sp<IHdmiCecCallback>& callback) override;
    Return<int32_t> getCecVersion() override;
    Return<uint32_t> getVendorId() override;
    Return<void> getPortInfo(getPortInfo_cb _hidl_cb) override;
    Return<void> setOption(OptionKey key, bool value) override;
    Return<void> setLanguage(const hidl_string& language) override;
    Return<void> enableAudioReturnChannel(int32_t portId, bool enable) override;
    Return<bool> isConnected(int32_t portId) override;

    virtual void serviceDied(uint64_t, const wp<::android::hidl::base::V1_0::IBase>&) {
        setCallback(nullptr);
    }

    Return<Result> init();
    Return<void> release();

  private:
    void event_thread();
    static int getOpcode(cec_msg message);
    static int getFirstParam(cec_msg message);
    static bool isWakeupMessage(cec_msg message);
    static bool isTransferableInSleep(cec_msg message);
    static bool isPowerUICommand(cec_msg message);

    thread mEventThread;

    // When set to false, all the CEC commands are discarded. True by default after initialization.
    bool mCecEnabled;
    /*
     * When set to false, HAL does not wake up the system upon receiving <Image View On> or
     * <Text View On>. True by default after initialization.
     */
    bool mWakeupEnabled;
    /*
     * Updated when system goes into or comes out of standby mode.
     * When set to true, Android system is handling CEC commands.
     * When set to false, microprocessor is handling CEC commands.
     * True by default after initialization.
     */
    bool mCecControlEnabled;
    sp<IHdmiCecCallback> mCallback;

    int mCecFd;
    int mExitFd;
};
}  // namespace implementation
}  // namespace V1_0
}  // namespace cec
}  // namespace tv
}  // namespace hardware
}  // namespace android
