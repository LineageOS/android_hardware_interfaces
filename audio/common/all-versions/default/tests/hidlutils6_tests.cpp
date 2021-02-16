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

#include <gtest/gtest.h>

#define LOG_TAG "HidlUtils_Test"
#include <log/log.h>

#include <HidlUtils.h>
#include <system/audio.h>

using namespace android;
using namespace ::android::hardware::audio::common::CPP_VERSION;
using ::android::hardware::audio::common::CPP_VERSION::implementation::HidlUtils;

// Not generated automatically because DeviceAddress contains
// an union.
//
// operator== must be defined in the same namespace as the data type.
namespace android::hardware::audio::common::CPP_VERSION {

inline bool operator==(const DeviceAddress& lhs, const DeviceAddress& rhs) {
    if (lhs.device != rhs.device) return false;
    audio_devices_t halDeviceType = static_cast<audio_devices_t>(lhs.device);
    if (audio_is_a2dp_out_device(halDeviceType) || audio_is_a2dp_in_device(halDeviceType)) {
        return lhs.address.mac == rhs.address.mac;
    } else if (halDeviceType == AUDIO_DEVICE_OUT_IP || halDeviceType == AUDIO_DEVICE_IN_IP) {
        return lhs.address.ipv4 == rhs.address.ipv4;
    } else if (audio_is_usb_out_device(halDeviceType) || audio_is_usb_in_device(halDeviceType)) {
        return lhs.address.alsa == rhs.address.alsa;
    } else if (halDeviceType == AUDIO_DEVICE_OUT_REMOTE_SUBMIX ||
               halDeviceType == AUDIO_DEVICE_IN_REMOTE_SUBMIX) {
        return lhs.rSubmixAddress == rhs.rSubmixAddress;
    }
    // busAddress field can be used for types other than bus, e.g. for microphones.
    return lhs.busAddress == rhs.busAddress;
}

}  // namespace android::hardware::audio::common::CPP_VERSION

static void ConvertDeviceAddress(const DeviceAddress& device) {
    audio_devices_t halDeviceType;
    char halDeviceAddress[AUDIO_DEVICE_MAX_ADDRESS_LEN] = {};
    EXPECT_EQ(NO_ERROR, HidlUtils::deviceAddressToHal(device, &halDeviceType, halDeviceAddress));
    DeviceAddress deviceBack;
    EXPECT_EQ(NO_ERROR,
              HidlUtils::deviceAddressFromHal(halDeviceType, halDeviceAddress, &deviceBack));
    EXPECT_EQ(device, deviceBack);
}

TEST(HidlUtils6, ConvertUniqueDeviceAddress) {
    DeviceAddress speaker;
    speaker.device = AudioDevice::OUT_SPEAKER;
    ConvertDeviceAddress(speaker);

    DeviceAddress micWithAddress;
    micWithAddress.device = AudioDevice::IN_BUILTIN_MIC;
    micWithAddress.busAddress = "bottom";
    ConvertDeviceAddress(micWithAddress);
}

TEST(HidlUtils6, ConvertA2dpDeviceAddress) {
    DeviceAddress a2dpSpeaker;
    a2dpSpeaker.device = AudioDevice::OUT_BLUETOOTH_A2DP_SPEAKER;
    a2dpSpeaker.address.mac = std::array<uint8_t, 6>{1, 2, 3, 4, 5, 6};
    ConvertDeviceAddress(a2dpSpeaker);
}

TEST(HidlUtils6, ConvertIpv4DeviceAddress) {
    DeviceAddress ipv4;
    ipv4.device = AudioDevice::OUT_IP;
    ipv4.address.ipv4 = std::array<uint8_t, 4>{1, 2, 3, 4};
    ConvertDeviceAddress(ipv4);
}

TEST(HidlUtils6, ConvertUsbDeviceAddress) {
    DeviceAddress usbHeadset;
    usbHeadset.device = AudioDevice::OUT_USB_HEADSET;
    usbHeadset.address.alsa = {1, 2};
    ConvertDeviceAddress(usbHeadset);
}

TEST(HidlUtils6, ConvertBusDeviceAddress) {
    DeviceAddress bus;
    bus.device = AudioDevice::OUT_BUS;
    bus.busAddress = "bus_device";
    ConvertDeviceAddress(bus);
}

TEST(HidlUtils6, ConvertRSubmixDeviceAddress) {
    DeviceAddress rSubmix;
    rSubmix.device = AudioDevice::OUT_REMOTE_SUBMIX;
    rSubmix.rSubmixAddress = AUDIO_REMOTE_SUBMIX_DEVICE_ADDRESS;
    ConvertDeviceAddress(rSubmix);
}
