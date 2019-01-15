/*
 * Copyright (C) 2019 The Android Open Source Project
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

#ifndef ANDROID_HARDWARE_TV_CEC_V2_0_HDMICEC_H
#define ANDROID_HARDWARE_TV_CEC_V2_0_HDMICEC_H

#include <algorithm>

#include <android/hardware/tv/cec/2.0/IHdmiCec.h>
#include <hardware/hardware.h>
#include <hardware/hdmi_cec.h>
#include <hidl/Status.h>

#include <hidl/MQDescriptor.h>
namespace android {
namespace hardware {
namespace tv {
namespace cec {
namespace V2_0 {
namespace implementation {

using ::android::sp;
using ::android::hardware::hidl_string;
using ::android::hardware::hidl_vec;
using ::android::hardware::Return;
using ::android::hardware::Void;
using ::android::hardware::tv::cec::V2_0::CecLogicalAddress;
using ::android::hardware::tv::cec::V2_0::CecMessage;
using ::android::hardware::tv::cec::V2_0::CecPhysicalAddress;
using ::android::hardware::tv::cec::V2_0::HdmiPortId;
using ::android::hardware::tv::cec::V2_0::HdmiPortInfo;
using ::android::hardware::tv::cec::V2_0::IHdmiCec;
using ::android::hardware::tv::cec::V2_0::IHdmiCecCallback;
using ::android::hardware::tv::cec::V2_0::MaxLength;
using ::android::hardware::tv::cec::V2_0::OptionKey;
using ::android::hardware::tv::cec::V2_0::Result;
using ::android::hardware::tv::cec::V2_0::SendMessageResult;

struct HdmiCec : public IHdmiCec, public hidl_death_recipient {
    HdmiCec(hdmi_cec_device_t* device);
    // Methods from ::android::hardware::tv::cec::V2_0::IHdmiCec follow.
    Return<Result> addDeviceType(CecDeviceType deviceType) override;
    Return<void> clearDeviceTypes() override;
    Return<void> setAllDeviceTypes(CecAllDeviceTypes allDeviceTypes) override;
    Return<void> setDeviceFeatures(CecDeviceType deviceType,
                                   CecDeviceFeatures /* deviceFeatures */) override;
    Return<void> setRcProfile(CecDeviceType deviceType,
                              const CecRcProfile& /* rcProfile */) override;
    Return<void> readDeviceInfo(CecLogicalAddress logicalAddress,
                                CecPhysicalAddress physicalAddress,
                                const readDeviceInfo_cb _hidl_cb) override;
    Return<SendMessageResult> sendMessage(const CecMessage& message) override;
    Return<void> setCallback(const sp<IHdmiCecCallback>& callback) override;
    Return<void> getPortInfo(getPortInfo_cb _hidl_cb) override;
    Return<void> setOption(OptionKey key, bool value) override;
    Return<void> setLanguage(const hidl_string& language) override;
    Return<void> enableAudioReturnChannel(HdmiPortId portId, bool enable) override;
    Return<bool> isConnected(HdmiPortId portId) override;

    static void eventCallback(const hdmi_event_t* event, void* /* arg */) {
        if (mCallback != nullptr && event != nullptr) {
            if (event->type == HDMI_EVENT_CEC_MESSAGE) {
                size_t length =
                        std::min(event->cec.length, static_cast<size_t>(MaxLength::MESSAGE_BODY));
                CecMessage cecMessage{
                        .initiator = static_cast<CecLogicalAddress>(event->cec.initiator),
                        .destination = static_cast<CecLogicalAddress>(event->cec.destination),
                };
                cecMessage.body.resize(length);
                for (size_t i = 0; i < length; ++i) {
                    cecMessage.body[i] = static_cast<uint8_t>(event->cec.body[i]);
                }
                mCallback->onCecMessage(cecMessage);
            } else if (event->type == HDMI_EVENT_HOT_PLUG) {
                HotplugEvent hotplugEvent{
                        .connected = event->hotplug.connected > 0,
                        .portId = static_cast<HdmiPortId>(event->hotplug.port_id)};
                mCallback->onHotplugEvent(hotplugEvent);
            }
        }
    }

    virtual void serviceDied(uint64_t /*cookie*/,
                             const wp<::android::hidl::base::V1_0::IBase>& /*who*/) {
        setCallback(nullptr);
    }

   private:
    static sp<IHdmiCecCallback> mCallback;
    const hdmi_cec_device_t* mDevice;
};

extern "C" IHdmiCec* HIDL_FETCH_IHdmiCec(const char* name);

}  // namespace implementation
}  // namespace V2_0
}  // namespace cec
}  // namespace tv
}  // namespace hardware
}  // namespace android

#endif  // ANDROID_HARDWARE_TV_CEC_V2_0_HDMICEC_H
