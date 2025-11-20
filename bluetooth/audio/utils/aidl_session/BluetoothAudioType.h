#include <sys/types.h>

#include <cstdint>
#include <map>

#include "aidl/android/hardware/bluetooth/audio/CodecId.h"
#include "aidl/android/hardware/bluetooth/audio/CodecSpecificConfigurationLtv.h"

namespace aidl {
namespace android {
namespace hardware {
namespace bluetooth {
namespace audio {

/* Datapath */
constexpr uint8_t kIsoDataPathHci = 0x00;
constexpr uint8_t kIsoDataPathPlatformDefault = 0x01;
constexpr uint8_t kIsoDataPathHciLinkFeedback = 0x19;
constexpr uint8_t kIsoDataPathDisabled = 0xFF;

/* Direction */
constexpr uint8_t kLeAudioDirectionSink = 0x01;
constexpr uint8_t kLeAudioDirectionSource = 0x02;
constexpr uint8_t kLeAudioDirectionBoth =
    kLeAudioDirectionSink | kLeAudioDirectionSource;

/* Sampling Frequencies */
constexpr uint8_t kLeAudioSamplingFreq8000Hz = 0x01;
constexpr uint8_t kLeAudioSamplingFreq11025Hz = 0x02;
constexpr uint8_t kLeAudioSamplingFreq16000Hz = 0x03;
constexpr uint8_t kLeAudioSamplingFreq22050Hz = 0x04;
constexpr uint8_t kLeAudioSamplingFreq24000Hz = 0x05;
constexpr uint8_t kLeAudioSamplingFreq32000Hz = 0x06;
constexpr uint8_t kLeAudioSamplingFreq44100Hz = 0x07;
constexpr uint8_t kLeAudioSamplingFreq48000Hz = 0x08;
constexpr uint8_t kLeAudioSamplingFreq88200Hz = 0x09;
constexpr uint8_t kLeAudioSamplingFreq96000Hz = 0x0A;
constexpr uint8_t kLeAudioSamplingFreq176400Hz = 0x0B;
constexpr uint8_t kLeAudioSamplingFreq192000Hz = 0x0C;
constexpr uint8_t kLeAudioSamplingFreq384000Hz = 0x0D;

/* Frame Durations */
constexpr uint8_t kLeAudioCodecFrameDur7500us = 0x00;
constexpr uint8_t kLeAudioCodecFrameDur10000us = 0x01;
constexpr uint8_t kLeAudioCodecFrameDur20000us = 0x02;

/* Audio Allocations */
constexpr uint32_t kLeAudioLocationMonoAudio = 0x00000000;
constexpr uint32_t kLeAudioLocationFrontLeft = 0x00000001;
constexpr uint32_t kLeAudioLocationFrontRight = 0x00000002;
constexpr uint32_t kLeAudioLocationFrontCenter = 0x00000004;
constexpr uint32_t kLeAudioLocationLowFreqEffects1 = 0x00000008;
constexpr uint32_t kLeAudioLocationBackLeft = 0x00000010;
constexpr uint32_t kLeAudioLocationBackRight = 0x00000020;
constexpr uint32_t kLeAudioLocationFrontLeftOfCenter = 0x00000040;
constexpr uint32_t kLeAudioLocationFrontRightOfCenter = 0x00000080;
constexpr uint32_t kLeAudioLocationBackCenter = 0x00000100;
constexpr uint32_t kLeAudioLocationLowFreqEffects2 = 0x00000200;
constexpr uint32_t kLeAudioLocationSideLeft = 0x00000400;
constexpr uint32_t kLeAudioLocationSideRight = 0x00000800;
constexpr uint32_t kLeAudioLocationTopFrontLeft = 0x00001000;
constexpr uint32_t kLeAudioLocationTopFrontRight = 0x00002000;
constexpr uint32_t kLeAudioLocationTopFrontCenter = 0x00004000;
constexpr uint32_t kLeAudioLocationTopCenter = 0x00008000;
constexpr uint32_t kLeAudioLocationTopBackLeft = 0x00010000;
constexpr uint32_t kLeAudioLocationTopBackRight = 0x00020000;
constexpr uint32_t kLeAudioLocationTopSideLeft = 0x00040000;
constexpr uint32_t kLeAudioLocationTopSideRight = 0x00080000;
constexpr uint32_t kLeAudioLocationTopBackCenter = 0x00100000;
constexpr uint32_t kLeAudioLocationBottomFrontCenter = 0x00200000;
constexpr uint32_t kLeAudioLocationBottomFrontLeft = 0x00400000;
constexpr uint32_t kLeAudioLocationBottomFrontRight = 0x00800000;
constexpr uint32_t kLeAudioLocationFrontLeftWide = 0x01000000;
constexpr uint32_t kLeAudioLocationFrontRightWide = 0x02000000;
constexpr uint32_t kLeAudioLocationLeftSurround = 0x04000000;
constexpr uint32_t kLeAudioLocationRightSurround = 0x08000000;

constexpr uint32_t kLeAudioLocationAnyLeft =
    kLeAudioLocationFrontLeft | kLeAudioLocationBackLeft |
    kLeAudioLocationFrontLeftOfCenter | kLeAudioLocationSideLeft |
    kLeAudioLocationTopFrontLeft | kLeAudioLocationTopBackLeft |
    kLeAudioLocationTopSideLeft | kLeAudioLocationBottomFrontLeft |
    kLeAudioLocationFrontLeftWide | kLeAudioLocationLeftSurround;

constexpr uint32_t kLeAudioLocationAnyRight =
    kLeAudioLocationFrontRight | kLeAudioLocationBackRight |
    kLeAudioLocationFrontRightOfCenter | kLeAudioLocationSideRight |
    kLeAudioLocationTopFrontRight | kLeAudioLocationTopBackRight |
    kLeAudioLocationTopSideRight | kLeAudioLocationBottomFrontRight |
    kLeAudioLocationFrontRightWide | kLeAudioLocationRightSurround;

constexpr uint32_t kLeAudioLocationStereo =
    kLeAudioLocationFrontLeft | kLeAudioLocationFrontRight;

/* Octets Per Frame */
constexpr uint16_t kLeAudioCodecFrameLen30 = 30;
constexpr uint16_t kLeAudioCodecFrameLen40 = 40;
constexpr uint16_t kLeAudioCodecFrameLen60 = 60;
constexpr uint16_t kLeAudioCodecFrameLen80 = 80;
constexpr uint16_t kLeAudioCodecFrameLen100 = 100;
constexpr uint16_t kLeAudioCodecFrameLen120 = 120;

constexpr uint8_t kCodecConfigOpcode = 0x02;
constexpr uint8_t kAudioChannelAllocationOpcode = 0x05;
constexpr uint8_t kOctetsPerCodecFrameOpcode = 0x03;
constexpr uint8_t kFrameBlocksPerSDUSubOpcode = 0x05;

constexpr uint8_t kFrameDurationSubOpcode = 0x02;
constexpr uint8_t kSamplingFrequencySubOpcode = 0x01;
constexpr uint8_t kAudioChannelAllocationSubOpcode = 0x03;
constexpr uint8_t kOctetsPerCodecFrameSubOpcode = 0x04;

/* Vendor codec ID */
constexpr uint16_t kLeAudioVendorCompanyIdGoogle = 0x00E0;
constexpr uint16_t kLeAudioVendorCodecIdOpus = 0x0001;

const CodecId::Vendor opus_codec{
    .codecId = kLeAudioVendorCodecIdOpus,
    .id = kLeAudioVendorCompanyIdGoogle,
};

/* Opus Hi-res */
constexpr uint32_t kOpusHiresSamplingFrequency = 96000;
constexpr uint32_t kOpusHiresBitPerSample = 24;
constexpr uint32_t kOpusHiresComplexity = 10;
constexpr bool kOpusHiresVbr = false;

/* Utility conversion */
const std::map<CodecSpecificConfigurationLtv::SamplingFrequency, uint32_t>
    sampling_rate_ltv_map = {
        {CodecSpecificConfigurationLtv::SamplingFrequency::HZ8000, 8000},
        {CodecSpecificConfigurationLtv::SamplingFrequency::HZ11025, 11025},
        {CodecSpecificConfigurationLtv::SamplingFrequency::HZ16000, 16000},
        {CodecSpecificConfigurationLtv::SamplingFrequency::HZ22050, 22050},
        {CodecSpecificConfigurationLtv::SamplingFrequency::HZ24000, 24000},
        {CodecSpecificConfigurationLtv::SamplingFrequency::HZ32000, 32000},
        {CodecSpecificConfigurationLtv::SamplingFrequency::HZ48000, 48000},
        {CodecSpecificConfigurationLtv::SamplingFrequency::HZ88200, 88200},
        {CodecSpecificConfigurationLtv::SamplingFrequency::HZ96000, 96000},
        {CodecSpecificConfigurationLtv::SamplingFrequency::HZ176400, 176400},
        {CodecSpecificConfigurationLtv::SamplingFrequency::HZ192000, 192000},
        {CodecSpecificConfigurationLtv::SamplingFrequency::HZ384000, 384000},
};

const std::map<CodecSpecificConfigurationLtv::FrameDuration, int32_t>
    frame_duration_ltv_map = {
        {CodecSpecificConfigurationLtv::FrameDuration::US7500, 7500},
        {CodecSpecificConfigurationLtv::FrameDuration::US10000, 10000},
        {CodecSpecificConfigurationLtv::FrameDuration::US20000, 20000},
};

const std::map<uint32_t, CodecSpecificConfigurationLtv::SamplingFrequency>
    codec_cfg_map_to_sampling_rate_ltv = {
        {0x01, CodecSpecificConfigurationLtv::SamplingFrequency::HZ8000},
        {0x02, CodecSpecificConfigurationLtv::SamplingFrequency::HZ11025},
        {0x03, CodecSpecificConfigurationLtv::SamplingFrequency::HZ16000},
        {0x04, CodecSpecificConfigurationLtv::SamplingFrequency::HZ22050},
        {0x05, CodecSpecificConfigurationLtv::SamplingFrequency::HZ24000},
        {0x06, CodecSpecificConfigurationLtv::SamplingFrequency::HZ32000},
        {0x07, CodecSpecificConfigurationLtv::SamplingFrequency::HZ44100},
        {0x08, CodecSpecificConfigurationLtv::SamplingFrequency::HZ48000},
        {0x09, CodecSpecificConfigurationLtv::SamplingFrequency::HZ88200},
        {0x0A, CodecSpecificConfigurationLtv::SamplingFrequency::HZ96000},
        {0x0B, CodecSpecificConfigurationLtv::SamplingFrequency::HZ176400},
        {0x0C, CodecSpecificConfigurationLtv::SamplingFrequency::HZ192000},
        {0x0D, CodecSpecificConfigurationLtv::SamplingFrequency::HZ384000},
};

const std::map<uint32_t, CodecSpecificConfigurationLtv::FrameDuration>
    codec_cfg_map_to_frame_duration_ltv = {
        {0x00, CodecSpecificConfigurationLtv::FrameDuration::US7500},
        {0x01, CodecSpecificConfigurationLtv::FrameDuration::US10000},
        {0x02, CodecSpecificConfigurationLtv::FrameDuration::US20000},
};

const std::map<uint8_t, CodecSpecificConfigurationLtv::SamplingFrequency>
    sampling_freq_map = {
        {kLeAudioSamplingFreq8000Hz,
         CodecSpecificConfigurationLtv::SamplingFrequency::HZ8000},
        {kLeAudioSamplingFreq16000Hz,
         CodecSpecificConfigurationLtv::SamplingFrequency::HZ16000},
        {kLeAudioSamplingFreq24000Hz,
         CodecSpecificConfigurationLtv::SamplingFrequency::HZ24000},
        {kLeAudioSamplingFreq32000Hz,
         CodecSpecificConfigurationLtv::SamplingFrequency::HZ32000},
        {kLeAudioSamplingFreq44100Hz,
         CodecSpecificConfigurationLtv::SamplingFrequency::HZ44100},
        {kLeAudioSamplingFreq48000Hz,
         CodecSpecificConfigurationLtv::SamplingFrequency::HZ48000},
        {kLeAudioSamplingFreq96000Hz,
         CodecSpecificConfigurationLtv::SamplingFrequency::HZ96000}};

/* Helper map for matching various frame durations notations */
const std::map<uint8_t, CodecSpecificConfigurationLtv::FrameDuration>
    frame_duration_map = {
        {kLeAudioCodecFrameDur7500us,
         CodecSpecificConfigurationLtv::FrameDuration::US7500},
        {kLeAudioCodecFrameDur10000us,
         CodecSpecificConfigurationLtv::FrameDuration::US10000},
        {kLeAudioCodecFrameDur20000us,
         CodecSpecificConfigurationLtv::FrameDuration::US20000}};

/* Helper map for matching various audio channel allocation notations */
const std::map<uint32_t, uint32_t> audio_channel_allocation_map = {
    {kLeAudioLocationMonoAudio,
     CodecSpecificConfigurationLtv::AudioChannelAllocation::MONO},
    {kLeAudioLocationFrontLeft,
     CodecSpecificConfigurationLtv::AudioChannelAllocation::FRONT_LEFT},
    {kLeAudioLocationFrontRight,
     CodecSpecificConfigurationLtv::AudioChannelAllocation::FRONT_RIGHT},
    {kLeAudioLocationFrontCenter,
     CodecSpecificConfigurationLtv::AudioChannelAllocation::FRONT_CENTER},
    {kLeAudioLocationLowFreqEffects1,
     CodecSpecificConfigurationLtv::AudioChannelAllocation::
         LOW_FREQUENCY_EFFECTS_1},
    {kLeAudioLocationBackLeft,
     CodecSpecificConfigurationLtv::AudioChannelAllocation::BACK_LEFT},
    {kLeAudioLocationBackRight,
     CodecSpecificConfigurationLtv::AudioChannelAllocation::BACK_RIGHT},
    {kLeAudioLocationFrontLeftOfCenter,
     CodecSpecificConfigurationLtv::AudioChannelAllocation::
         FRONT_LEFT_OF_CENTER},
    {kLeAudioLocationFrontRightOfCenter,
     CodecSpecificConfigurationLtv::AudioChannelAllocation::
         FRONT_RIGHT_OF_CENTER},
    {kLeAudioLocationBackCenter,
     CodecSpecificConfigurationLtv::AudioChannelAllocation::BACK_CENTER},
    {kLeAudioLocationLowFreqEffects2,
     CodecSpecificConfigurationLtv::AudioChannelAllocation::
         LOW_FREQUENCY_EFFECTS_2},
    {kLeAudioLocationSideLeft,
     CodecSpecificConfigurationLtv::AudioChannelAllocation::SIDE_LEFT},
    {kLeAudioLocationSideRight,
     CodecSpecificConfigurationLtv::AudioChannelAllocation::SIDE_RIGHT},
    {kLeAudioLocationTopFrontLeft,
     CodecSpecificConfigurationLtv::AudioChannelAllocation::TOP_FRONT_LEFT},
    {kLeAudioLocationTopFrontRight,
     CodecSpecificConfigurationLtv::AudioChannelAllocation::TOP_FRONT_RIGHT},
    {kLeAudioLocationTopFrontCenter,
     CodecSpecificConfigurationLtv::AudioChannelAllocation::TOP_FRONT_CENTER},
    {kLeAudioLocationTopCenter,
     CodecSpecificConfigurationLtv::AudioChannelAllocation::TOP_CENTER},
    {kLeAudioLocationTopBackLeft,
     CodecSpecificConfigurationLtv::AudioChannelAllocation::TOP_BACK_LEFT},
    {kLeAudioLocationTopBackRight,
     CodecSpecificConfigurationLtv::AudioChannelAllocation::TOP_BACK_RIGHT},
    {kLeAudioLocationTopSideLeft,
     CodecSpecificConfigurationLtv::AudioChannelAllocation::TOP_SIDE_LEFT},
    {kLeAudioLocationTopSideRight,
     CodecSpecificConfigurationLtv::AudioChannelAllocation::TOP_SIDE_RIGHT},
    {kLeAudioLocationTopBackCenter,
     CodecSpecificConfigurationLtv::AudioChannelAllocation::TOP_BACK_CENTER},
    {kLeAudioLocationBottomFrontCenter,
     CodecSpecificConfigurationLtv::AudioChannelAllocation::
         BOTTOM_FRONT_CENTER},
    {kLeAudioLocationBottomFrontLeft,
     CodecSpecificConfigurationLtv::AudioChannelAllocation::BOTTOM_FRONT_LEFT},
    {kLeAudioLocationBottomFrontRight,
     CodecSpecificConfigurationLtv::AudioChannelAllocation::BOTTOM_FRONT_RIGHT},
    {kLeAudioLocationFrontLeftWide,
     CodecSpecificConfigurationLtv::AudioChannelAllocation::FRONT_LEFT_WIDE},
    {kLeAudioLocationFrontRightWide,
     CodecSpecificConfigurationLtv::AudioChannelAllocation::FRONT_RIGHT_WIDE},
    {kLeAudioLocationLeftSurround,
     CodecSpecificConfigurationLtv::AudioChannelAllocation::LEFT_SURROUND},
    {kLeAudioLocationRightSurround,
     CodecSpecificConfigurationLtv::AudioChannelAllocation::RIGHT_SURROUND},
};

const std::map<CodecSpecificConfigurationLtv::SamplingFrequency, uint32_t>
    sampling_rate_ltv_to_codec_cfg_map = {
        {CodecSpecificConfigurationLtv::SamplingFrequency::HZ8000, 0x01},
        {CodecSpecificConfigurationLtv::SamplingFrequency::HZ11025, 0x02},
        {CodecSpecificConfigurationLtv::SamplingFrequency::HZ16000, 0x03},
        {CodecSpecificConfigurationLtv::SamplingFrequency::HZ22050, 0x04},
        {CodecSpecificConfigurationLtv::SamplingFrequency::HZ24000, 0x05},
        {CodecSpecificConfigurationLtv::SamplingFrequency::HZ32000, 0x06},
        {CodecSpecificConfigurationLtv::SamplingFrequency::HZ44100, 0x07},
        {CodecSpecificConfigurationLtv::SamplingFrequency::HZ48000, 0x08},
        {CodecSpecificConfigurationLtv::SamplingFrequency::HZ88200, 0x09},
        {CodecSpecificConfigurationLtv::SamplingFrequency::HZ96000, 0x0A},
        {CodecSpecificConfigurationLtv::SamplingFrequency::HZ176400, 0x0B},
        {CodecSpecificConfigurationLtv::SamplingFrequency::HZ192000, 0x0C},
        {CodecSpecificConfigurationLtv::SamplingFrequency::HZ384000, 0x0D},
};

const std::map<CodecSpecificConfigurationLtv::FrameDuration, uint32_t>
    frame_duration_ltv_to_codec_cfg_map = {
        {CodecSpecificConfigurationLtv::FrameDuration::US7500, 0x00},
        {CodecSpecificConfigurationLtv::FrameDuration::US10000, 0x01},
        {CodecSpecificConfigurationLtv::FrameDuration::US20000, 0x02},
};

}  // namespace audio
}  // namespace bluetooth
}  // namespace hardware
}  // namespace android
}  // namespace aidl
