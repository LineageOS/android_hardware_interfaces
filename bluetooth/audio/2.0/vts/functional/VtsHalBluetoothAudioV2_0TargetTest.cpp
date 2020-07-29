/*
 * Copyright 2018 The Android Open Source Project
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

#define LOG_TAG "bluetooth_audio_hidl_hal_test"

#include <android-base/logging.h>
#include <android/hardware/bluetooth/audio/2.0/IBluetoothAudioPort.h>
#include <android/hardware/bluetooth/audio/2.0/IBluetoothAudioProvider.h>
#include <android/hardware/bluetooth/audio/2.0/IBluetoothAudioProvidersFactory.h>
#include <fmq/MessageQueue.h>
#include <gtest/gtest.h>
#include <hidl/GtestPrinter.h>
#include <hidl/MQDescriptor.h>
#include <hidl/ServiceManagement.h>
#include <utils/Log.h>

#include <VtsHalHidlTargetCallbackBase.h>

using ::android::sp;
using ::android::hardware::hidl_vec;
using ::android::hardware::kSynchronizedReadWrite;
using ::android::hardware::MessageQueue;
using ::android::hardware::Return;
using ::android::hardware::Void;
using ::android::hardware::audio::common::V5_0::SourceMetadata;
using ::android::hardware::bluetooth::audio::V2_0::AacObjectType;
using ::android::hardware::bluetooth::audio::V2_0::AacParameters;
using ::android::hardware::bluetooth::audio::V2_0::AacVariableBitRate;
using ::android::hardware::bluetooth::audio::V2_0::AptxParameters;
using ::android::hardware::bluetooth::audio::V2_0::AudioCapabilities;
using ::android::hardware::bluetooth::audio::V2_0::AudioConfiguration;
using ::android::hardware::bluetooth::audio::V2_0::BitsPerSample;
using ::android::hardware::bluetooth::audio::V2_0::ChannelMode;
using ::android::hardware::bluetooth::audio::V2_0::CodecCapabilities;
using ::android::hardware::bluetooth::audio::V2_0::CodecConfiguration;
using ::android::hardware::bluetooth::audio::V2_0::CodecType;
using ::android::hardware::bluetooth::audio::V2_0::IBluetoothAudioPort;
using ::android::hardware::bluetooth::audio::V2_0::IBluetoothAudioProvider;
using ::android::hardware::bluetooth::audio::V2_0::
    IBluetoothAudioProvidersFactory;
using ::android::hardware::bluetooth::audio::V2_0::LdacChannelMode;
using ::android::hardware::bluetooth::audio::V2_0::LdacParameters;
using ::android::hardware::bluetooth::audio::V2_0::LdacQualityIndex;
using ::android::hardware::bluetooth::audio::V2_0::PcmParameters;
using ::android::hardware::bluetooth::audio::V2_0::SampleRate;
using ::android::hardware::bluetooth::audio::V2_0::SbcAllocMethod;
using ::android::hardware::bluetooth::audio::V2_0::SbcBlockLength;
using ::android::hardware::bluetooth::audio::V2_0::SbcChannelMode;
using ::android::hardware::bluetooth::audio::V2_0::SbcNumSubbands;
using ::android::hardware::bluetooth::audio::V2_0::SbcParameters;
using ::android::hardware::bluetooth::audio::V2_0::SessionType;

using DataMQ = MessageQueue<uint8_t, kSynchronizedReadWrite>;
using BluetoothAudioStatus =
    ::android::hardware::bluetooth::audio::V2_0::Status;
using CodecSpecificConfig = ::android::hardware::bluetooth::audio::V2_0::
    CodecConfiguration::CodecSpecific;

namespace {
constexpr SampleRate a2dp_sample_rates[5] = {
    SampleRate::RATE_UNKNOWN, SampleRate::RATE_44100, SampleRate::RATE_48000,
    SampleRate::RATE_88200, SampleRate::RATE_96000};
constexpr BitsPerSample a2dp_bits_per_samples[4] = {
    BitsPerSample::BITS_UNKNOWN, BitsPerSample::BITS_16, BitsPerSample::BITS_24,
    BitsPerSample::BITS_32};
constexpr ChannelMode a2dp_channel_modes[3] = {
    ChannelMode::UNKNOWN, ChannelMode::MONO, ChannelMode::STEREO};
constexpr CodecType a2dp_codec_types[6] = {CodecType::UNKNOWN, CodecType::SBC,
                                           CodecType::AAC,     CodecType::APTX,
                                           CodecType::APTX_HD, CodecType::LDAC};

template <typename T>
std::vector<T> ExtractValuesFromBitmask(T bitmasks, uint32_t bitfield,
                                        bool supported) {
  std::vector<T> retval;
  if (!supported) {
    retval.push_back(static_cast<T>(bitfield));
  }
  uint32_t test_bit = 0x00000001;
  while (test_bit <= static_cast<uint32_t>(bitmasks) && test_bit <= bitfield) {
    if ((bitfield & test_bit)) {
      if ((!(bitmasks & test_bit) && !supported) ||
          ((bitmasks & test_bit) && supported)) {
        retval.push_back(static_cast<T>(test_bit));
      }
    }
    if (test_bit == 0x80000000) {
      break;
    }
    test_bit <<= 1;
  }
  return retval;
}
}  // namespace

// The base test class for Bluetooth Audio HAL.
class BluetoothAudioProvidersFactoryHidlTest
    : public ::testing::TestWithParam<std::string> {
 public:
  virtual void SetUp() override {
    providers_factory_ =
        IBluetoothAudioProvidersFactory::getService(GetParam());
    ASSERT_NE(providers_factory_, nullptr);
  }

  virtual void TearDown() override { providers_factory_ = nullptr; }

  // A simple test implementation of IBluetoothAudioPort.
  class BluetoothAudioPort : public ::testing::VtsHalHidlTargetCallbackBase<
                                 BluetoothAudioProvidersFactoryHidlTest>,
                             public IBluetoothAudioPort {
    BluetoothAudioProvidersFactoryHidlTest& parent_;

   public:
    BluetoothAudioPort(BluetoothAudioProvidersFactoryHidlTest& parent)
        : parent_(parent) {}
    virtual ~BluetoothAudioPort() = default;

    Return<void> startStream() override {
      parent_.audio_provider_->streamStarted(BluetoothAudioStatus::SUCCESS);
      return Void();
    }

    Return<void> suspendStream() override {
      parent_.audio_provider_->streamSuspended(BluetoothAudioStatus::SUCCESS);
      return Void();
    }

    Return<void> stopStream() override { return Void(); }

    Return<void> getPresentationPosition(getPresentationPosition_cb _hidl_cb) {
      _hidl_cb(BluetoothAudioStatus::SUCCESS, 0, 0, {.tvSec = 0, .tvNSec = 0});
      return Void();
    }

    Return<void> updateMetadata(const SourceMetadata& sourceMetadata __unused) {
      return Void();
    }
  };

  void GetProviderCapabilitiesHelper(const SessionType& session_type) {
    temp_provider_capabilities_.clear();
    auto hidl_cb = [& temp_capabilities = this->temp_provider_capabilities_](
                       const hidl_vec<AudioCapabilities>& audioCapabilities) {
      for (auto audioCapability : audioCapabilities)
        temp_capabilities.push_back(audioCapability);
    };
    auto hidl_retval =
        providers_factory_->getProviderCapabilities(session_type, hidl_cb);
    // HIDL calls should not be failed and callback has to be executed
    ASSERT_TRUE(hidl_retval.isOk());
    if (session_type == SessionType::UNKNOWN) {
      ASSERT_TRUE(temp_provider_capabilities_.empty());
    } else if (session_type != SessionType::A2DP_HARDWARE_OFFLOAD_DATAPATH) {
      // All software paths are mandatory and must have exact 1 "PcmParameters"
      ASSERT_EQ(temp_provider_capabilities_.size(), 1);
      ASSERT_EQ(temp_provider_capabilities_[0].getDiscriminator(),
                AudioCapabilities::hidl_discriminator::pcmCapabilities);
    } else {
      uint32_t codec_type_bitmask = 0x00000000;
      // empty capability means offload is unsupported
      for (auto audio_capability : temp_provider_capabilities_) {
        ASSERT_EQ(audio_capability.getDiscriminator(),
                  AudioCapabilities::hidl_discriminator::codecCapabilities);
        const CodecCapabilities& codec_capabilities =
            audio_capability.codecCapabilities();
        // Every codec can present once at most
        ASSERT_EQ(codec_type_bitmask &
                      static_cast<uint32_t>(codec_capabilities.codecType),
                  0);
        switch (codec_capabilities.codecType) {
          case CodecType::SBC:
            ASSERT_EQ(codec_capabilities.capabilities.getDiscriminator(),
                      CodecCapabilities::Capabilities::hidl_discriminator::
                          sbcCapabilities);
            break;
          case CodecType::AAC:
            ASSERT_EQ(codec_capabilities.capabilities.getDiscriminator(),
                      CodecCapabilities::Capabilities::hidl_discriminator::
                          aacCapabilities);
            break;
          case CodecType::APTX:
            FALLTHROUGH_INTENDED;
          case CodecType::APTX_HD:
            ASSERT_EQ(codec_capabilities.capabilities.getDiscriminator(),
                      CodecCapabilities::Capabilities::hidl_discriminator::
                          aptxCapabilities);
            break;
          case CodecType::LDAC:
            ASSERT_EQ(codec_capabilities.capabilities.getDiscriminator(),
                      CodecCapabilities::Capabilities::hidl_discriminator::
                          ldacCapabilities);
            break;
          case CodecType::UNKNOWN:
            break;
        }
        codec_type_bitmask |= codec_capabilities.codecType;
      }
    }
  }

  // This helps to open the specified provider and check the openProvider()
  // has corruct return values. BUT, to keep it simple, it does not consider
  // the capability, and please do so at the SetUp of each session's test.
  void OpenProviderHelper(const SessionType& session_type) {
    BluetoothAudioStatus cb_status;
    auto hidl_cb = [&cb_status, &local_provider = this->audio_provider_](
                       BluetoothAudioStatus status,
                       const sp<IBluetoothAudioProvider>& provider) {
      cb_status = status;
      local_provider = provider;
    };
    auto hidl_retval = providers_factory_->openProvider(session_type, hidl_cb);
    // HIDL calls should not be failed and callback has to be executed
    ASSERT_TRUE(hidl_retval.isOk());
    if (cb_status == BluetoothAudioStatus::SUCCESS) {
      ASSERT_NE(session_type, SessionType::UNKNOWN);
      ASSERT_NE(audio_provider_, nullptr);
      audio_port_ = new BluetoothAudioPort(*this);
    } else {
      // A2DP_HARDWARE_OFFLOAD_DATAPATH is optional
      ASSERT_TRUE(session_type == SessionType::UNKNOWN ||
                  session_type == SessionType::A2DP_HARDWARE_OFFLOAD_DATAPATH);
      ASSERT_EQ(cb_status, BluetoothAudioStatus::FAILURE);
      ASSERT_EQ(audio_provider_, nullptr);
    }
  }

  bool IsPcmParametersSupported(const PcmParameters& pcm_parameters) {
    if (temp_provider_capabilities_.size() != 1 ||
        temp_provider_capabilities_[0].getDiscriminator() !=
            AudioCapabilities::hidl_discriminator::pcmCapabilities) {
      return false;
    }
    auto pcm_capability = temp_provider_capabilities_[0].pcmCapabilities();
    bool is_parameter_valid =
        (pcm_parameters.sampleRate != SampleRate::RATE_UNKNOWN &&
         pcm_parameters.channelMode != ChannelMode::UNKNOWN &&
         pcm_parameters.bitsPerSample != BitsPerSample::BITS_UNKNOWN);
    bool is_parameter_in_capability =
        (pcm_capability.sampleRate & pcm_parameters.sampleRate &&
         pcm_capability.channelMode & pcm_parameters.channelMode &&
         pcm_capability.bitsPerSample & pcm_parameters.bitsPerSample);
    return is_parameter_valid && is_parameter_in_capability;
  }

  sp<IBluetoothAudioProvidersFactory> providers_factory_;

  // temp storage saves the specified provider capability by
  // GetProviderCapabilitiesHelper()
  std::vector<AudioCapabilities> temp_provider_capabilities_;

  // audio_provider_ is for the Bluetooth stack to report session started/ended
  // and handled audio stream started / suspended
  sp<IBluetoothAudioProvider> audio_provider_;

  // audio_port_ is for the Audio HAL to send stream start/suspend/stop commands
  // to Bluetooth stack
  sp<IBluetoothAudioPort> audio_port_;

  static constexpr SessionType session_types_[4] = {
      SessionType::UNKNOWN, SessionType::A2DP_SOFTWARE_ENCODING_DATAPATH,
      SessionType::A2DP_HARDWARE_OFFLOAD_DATAPATH,
      SessionType::HEARING_AID_SOFTWARE_ENCODING_DATAPATH};
};

/**
 * Test whether we can get the FactoryService from HIDL
 */
TEST_P(BluetoothAudioProvidersFactoryHidlTest, GetProvidersFactoryService) {}

/**
 * Test whether we can open a provider for each provider returned by
 * getProviderCapabilities() with non-empty capabalities
 */
TEST_P(BluetoothAudioProvidersFactoryHidlTest,
       OpenProviderAndCheckCapabilitiesBySession) {
  for (auto session_type : session_types_) {
    GetProviderCapabilitiesHelper(session_type);
    OpenProviderHelper(session_type);
    // We must be able to open a provider if its getProviderCapabilities()
    // returns non-empty list.
    EXPECT_TRUE(temp_provider_capabilities_.empty() ||
                audio_provider_ != nullptr);
  }
}

/**
 * openProvider A2DP_SOFTWARE_ENCODING_DATAPATH
 */
class BluetoothAudioProviderA2dpSoftwareHidlTest
    : public BluetoothAudioProvidersFactoryHidlTest {
 public:
  virtual void SetUp() override {
    BluetoothAudioProvidersFactoryHidlTest::SetUp();
    GetProviderCapabilitiesHelper(SessionType::A2DP_SOFTWARE_ENCODING_DATAPATH);
    OpenProviderHelper(SessionType::A2DP_SOFTWARE_ENCODING_DATAPATH);
    ASSERT_NE(audio_provider_, nullptr);
  }

  virtual void TearDown() override {
    audio_port_ = nullptr;
    audio_provider_ = nullptr;
    BluetoothAudioProvidersFactoryHidlTest::TearDown();
  }
};

/**
 * Test whether we can open a provider of type
 */
TEST_P(BluetoothAudioProviderA2dpSoftwareHidlTest, OpenA2dpSoftwareProvider) {}

/**
 * Test whether each provider of type
 * SessionType::A2DP_SOFTWARE_ENCODING_DATAPATH can be started and stopped with
 * different PCM config
 */
TEST_P(BluetoothAudioProviderA2dpSoftwareHidlTest,
       StartAndEndA2dpSoftwareSessionWithPossiblePcmConfig) {
  bool is_codec_config_valid;
  std::unique_ptr<DataMQ> tempDataMQ;
  auto hidl_cb = [&is_codec_config_valid, &tempDataMQ](
                     BluetoothAudioStatus status,
                     const DataMQ::Descriptor& dataMQ) {
    if (is_codec_config_valid) {
      ASSERT_EQ(status, BluetoothAudioStatus::SUCCESS);
      ASSERT_TRUE(dataMQ.isHandleValid());
      tempDataMQ.reset(new DataMQ(dataMQ));
    } else {
      EXPECT_EQ(status, BluetoothAudioStatus::UNSUPPORTED_CODEC_CONFIGURATION);
      EXPECT_FALSE(dataMQ.isHandleValid());
    }
  };
  AudioConfiguration audio_config = {};
  PcmParameters pcm_parameters = {};
  for (auto sample_rate : a2dp_sample_rates) {
    pcm_parameters.sampleRate = sample_rate;
    for (auto bits_per_sample : a2dp_bits_per_samples) {
      pcm_parameters.bitsPerSample = bits_per_sample;
      for (auto channel_mode : a2dp_channel_modes) {
        pcm_parameters.channelMode = channel_mode;
        is_codec_config_valid = IsPcmParametersSupported(pcm_parameters);
        audio_config.pcmConfig(pcm_parameters);
        auto hidl_retval =
            audio_provider_->startSession(audio_port_, audio_config, hidl_cb);
        // HIDL calls should not be failed and callback has to be executed
        ASSERT_TRUE(hidl_retval.isOk());
        if (is_codec_config_valid) {
          EXPECT_TRUE(tempDataMQ != nullptr && tempDataMQ->isValid());
        }
        EXPECT_TRUE(audio_provider_->endSession().isOk());
      }  // ChannelMode
    }    // BitsPerSampple
  }      // SampleRate
}

/**
 * openProvider A2DP_HARDWARE_OFFLOAD_DATAPATH
 */
class BluetoothAudioProviderA2dpHardwareHidlTest
    : public BluetoothAudioProvidersFactoryHidlTest {
 public:
  virtual void SetUp() override {
    BluetoothAudioProvidersFactoryHidlTest::SetUp();
    GetProviderCapabilitiesHelper(SessionType::A2DP_HARDWARE_OFFLOAD_DATAPATH);
    OpenProviderHelper(SessionType::A2DP_HARDWARE_OFFLOAD_DATAPATH);
    ASSERT_TRUE(temp_provider_capabilities_.empty() ||
                audio_provider_ != nullptr);
  }

  virtual void TearDown() override {
    audio_port_ = nullptr;
    audio_provider_ = nullptr;
    BluetoothAudioProvidersFactoryHidlTest::TearDown();
  }

  bool IsOffloadSupported() { return (temp_provider_capabilities_.size() > 0); }

  void GetOffloadCodecCapabilityHelper(const CodecType& codec_type) {
    temp_codec_capabilities_ = {};
    for (auto codec_capability : temp_provider_capabilities_) {
      if (codec_capability.codecCapabilities().codecType != codec_type) {
        continue;
      }
      temp_codec_capabilities_ = codec_capability.codecCapabilities();
    }
  }

  std::vector<CodecSpecificConfig> GetSbcCodecSpecificSupportedList(
      bool supported) {
    std::vector<CodecSpecificConfig> sbc_codec_specifics;
    GetOffloadCodecCapabilityHelper(CodecType::SBC);
    if (temp_codec_capabilities_.codecType != CodecType::SBC) {
      return sbc_codec_specifics;
    }
    // parse the capability
    SbcParameters sbc_capability =
        temp_codec_capabilities_.capabilities.sbcCapabilities();
    if (sbc_capability.minBitpool > sbc_capability.maxBitpool) {
      return sbc_codec_specifics;
    }
    std::vector<SampleRate> sample_rates = ExtractValuesFromBitmask<SampleRate>(
        sbc_capability.sampleRate, 0xff, supported);
    std::vector<SbcChannelMode> channel_modes =
        ExtractValuesFromBitmask<SbcChannelMode>(sbc_capability.channelMode,
                                                 0x0f, supported);
    std::vector<SbcBlockLength> block_lengths =
        ExtractValuesFromBitmask<SbcBlockLength>(sbc_capability.blockLength,
                                                 0xf0, supported);
    std::vector<SbcNumSubbands> num_subbandss =
        ExtractValuesFromBitmask<SbcNumSubbands>(sbc_capability.numSubbands,
                                                 0x0c, supported);
    std::vector<SbcAllocMethod> alloc_methods =
        ExtractValuesFromBitmask<SbcAllocMethod>(sbc_capability.allocMethod,
                                                 0x03, supported);
    std::vector<BitsPerSample> bits_per_samples =
        ExtractValuesFromBitmask<BitsPerSample>(sbc_capability.bitsPerSample,
                                                0x07, supported);
    // combine those parameters into one list of
    // CodecConfiguration::CodecSpecific
    CodecSpecificConfig codec_specific = {};
    SbcParameters sbc_data;
    for (auto sample_rate : sample_rates) {
      for (auto channel_mode : channel_modes) {
        for (auto block_length : block_lengths) {
          for (auto num_subbands : num_subbandss) {
            for (auto alloc_method : alloc_methods) {
              for (auto bits_per_sample : bits_per_samples) {
                sbc_data = {.sampleRate = sample_rate,
                            .channelMode = channel_mode,
                            .blockLength = block_length,
                            .numSubbands = num_subbands,
                            .allocMethod = alloc_method,
                            .bitsPerSample = bits_per_sample,
                            .minBitpool = sbc_capability.minBitpool,
                            .maxBitpool = sbc_capability.maxBitpool};
                codec_specific.sbcConfig(sbc_data);
                sbc_codec_specifics.push_back(codec_specific);
              }
            }
          }
        }
      }
    }
    return sbc_codec_specifics;
  }

  std::vector<CodecSpecificConfig> GetAacCodecSpecificSupportedList(
      bool supported) {
    std::vector<CodecSpecificConfig> aac_codec_specifics;
    GetOffloadCodecCapabilityHelper(CodecType::AAC);
    if (temp_codec_capabilities_.codecType != CodecType::AAC) {
      return aac_codec_specifics;
    }
    // parse the capability
    AacParameters aac_capability =
        temp_codec_capabilities_.capabilities.aacCapabilities();
    std::vector<AacObjectType> object_types =
        ExtractValuesFromBitmask<AacObjectType>(aac_capability.objectType, 0xf0,
                                                supported);
    std::vector<SampleRate> sample_rates = ExtractValuesFromBitmask<SampleRate>(
        aac_capability.sampleRate, 0xff, supported);
    std::vector<ChannelMode> channel_modes =
        ExtractValuesFromBitmask<ChannelMode>(aac_capability.channelMode, 0x03,
                                              supported);
    std::vector<AacVariableBitRate> variable_bit_rate_enableds = {
        AacVariableBitRate::DISABLED};
    if (aac_capability.variableBitRateEnabled == AacVariableBitRate::ENABLED) {
      variable_bit_rate_enableds.push_back(AacVariableBitRate::ENABLED);
    }
    std::vector<BitsPerSample> bits_per_samples =
        ExtractValuesFromBitmask<BitsPerSample>(aac_capability.bitsPerSample,
                                                0x07, supported);
    // combine those parameters into one list of
    // CodecConfiguration::CodecSpecific
    CodecSpecificConfig codec_specific = {};
    AacParameters aac_data;
    for (auto object_type : object_types) {
      for (auto sample_rate : sample_rates) {
        for (auto channel_mode : channel_modes) {
          for (auto variable_bit_rate_enabled : variable_bit_rate_enableds) {
            for (auto bits_per_sample : bits_per_samples) {
              aac_data = {.objectType = object_type,
                          .sampleRate = sample_rate,
                          .channelMode = channel_mode,
                          .variableBitRateEnabled = variable_bit_rate_enabled,
                          .bitsPerSample = bits_per_sample};
              codec_specific.aacConfig(aac_data);
              aac_codec_specifics.push_back(codec_specific);
            }
          }
        }
      }
    }
    return aac_codec_specifics;
  }

  std::vector<CodecSpecificConfig> GetLdacCodecSpecificSupportedList(
      bool supported) {
    std::vector<CodecSpecificConfig> ldac_codec_specifics;
    GetOffloadCodecCapabilityHelper(CodecType::LDAC);
    if (temp_codec_capabilities_.codecType != CodecType::LDAC) {
      return ldac_codec_specifics;
    }
    // parse the capability
    LdacParameters ldac_capability =
        temp_codec_capabilities_.capabilities.ldacCapabilities();
    std::vector<SampleRate> sample_rates = ExtractValuesFromBitmask<SampleRate>(
        ldac_capability.sampleRate, 0xff, supported);
    std::vector<LdacChannelMode> channel_modes =
        ExtractValuesFromBitmask<LdacChannelMode>(ldac_capability.channelMode,
                                                  0x07, supported);
    std::vector<LdacQualityIndex> quality_indexes = {
        LdacQualityIndex::QUALITY_HIGH, LdacQualityIndex::QUALITY_MID,
        LdacQualityIndex::QUALITY_LOW, LdacQualityIndex::QUALITY_ABR};
    std::vector<BitsPerSample> bits_per_samples =
        ExtractValuesFromBitmask<BitsPerSample>(ldac_capability.bitsPerSample,
                                                0x07, supported);
    // combine those parameters into one list of
    // CodecConfiguration::CodecSpecific
    CodecSpecificConfig codec_specific = {};
    LdacParameters ldac_data;
    for (auto sample_rate : sample_rates) {
      for (auto channel_mode : channel_modes) {
        for (auto quality_index : quality_indexes) {
          for (auto bits_per_sample : bits_per_samples) {
            ldac_data = {.sampleRate = sample_rate,
                         .channelMode = channel_mode,
                         .qualityIndex = quality_index,
                         .bitsPerSample = bits_per_sample};
            codec_specific.ldacConfig(ldac_data);
            ldac_codec_specifics.push_back(codec_specific);
          }
        }
      }
    }
    return ldac_codec_specifics;
  }

  std::vector<CodecSpecificConfig> GetAptxCodecSpecificSupportedList(
      bool is_hd, bool supported) {
    std::vector<CodecSpecificConfig> aptx_codec_specifics;
    GetOffloadCodecCapabilityHelper(
        (is_hd ? CodecType::APTX_HD : CodecType::APTX));
    if ((is_hd && temp_codec_capabilities_.codecType != CodecType::APTX_HD) ||
        (!is_hd && temp_codec_capabilities_.codecType != CodecType::APTX)) {
      return aptx_codec_specifics;
    }
    // parse the capability
    AptxParameters aptx_capability =
        temp_codec_capabilities_.capabilities.aptxCapabilities();
    std::vector<SampleRate> sample_rates = ExtractValuesFromBitmask<SampleRate>(
        aptx_capability.sampleRate, 0xff, supported);
    std::vector<ChannelMode> channel_modes =
        ExtractValuesFromBitmask<ChannelMode>(aptx_capability.channelMode, 0x03,
                                              supported);
    std::vector<BitsPerSample> bits_per_samples =
        ExtractValuesFromBitmask<BitsPerSample>(aptx_capability.bitsPerSample,
                                                0x07, supported);
    // combine those parameters into one list of
    // CodecConfiguration::CodecSpecific
    CodecSpecificConfig codec_specific = {};
    AptxParameters aptx_data;
    for (auto sample_rate : sample_rates) {
      for (auto channel_mode : channel_modes) {
        for (auto bits_per_sample : bits_per_samples) {
          aptx_data = {.sampleRate = sample_rate,
                       .channelMode = channel_mode,
                       .bitsPerSample = bits_per_sample};
          codec_specific.aptxConfig(aptx_data);
          aptx_codec_specifics.push_back(codec_specific);
        }
      }
    }
    return aptx_codec_specifics;
  }

  // temp storage saves the specified codec capability by
  // GetOffloadCodecCapabilityHelper()
  CodecCapabilities temp_codec_capabilities_;
};

/**
 * Test whether we can open a provider of type
 */
TEST_P(BluetoothAudioProviderA2dpHardwareHidlTest, OpenA2dpHardwareProvider) {}

/**
 * Test whether each provider of type
 * SessionType::A2DP_HARDWARE_ENCODING_DATAPATH can be started and stopped with
 * SBC hardware encoding config
 */
TEST_P(BluetoothAudioProviderA2dpHardwareHidlTest,
       StartAndEndA2dpSbcHardwareSession) {
  if (!IsOffloadSupported()) {
    return;
  }

  CodecConfiguration codec_config = {};
  codec_config.codecType = CodecType::SBC;
  codec_config.encodedAudioBitrate = 328000;
  codec_config.peerMtu = 1005;
  codec_config.isScmstEnabled = false;
  AudioConfiguration audio_config = {};
  std::vector<CodecSpecificConfig> sbc_codec_specifics =
      GetSbcCodecSpecificSupportedList(true);
  auto hidl_cb = [](BluetoothAudioStatus status,
                    const DataMQ::Descriptor& dataMQ) {
    EXPECT_EQ(status, BluetoothAudioStatus::SUCCESS);
    EXPECT_FALSE(dataMQ.isHandleValid());
  };
  for (auto codec_specific : sbc_codec_specifics) {
    codec_config.config = codec_specific;
    audio_config.codecConfig(codec_config);
    auto hidl_retval =
        audio_provider_->startSession(audio_port_, audio_config, hidl_cb);
    // HIDL calls should not be failed and callback has to be executed
    ASSERT_TRUE(hidl_retval.isOk());
    EXPECT_TRUE(audio_provider_->endSession().isOk());
  }
}

/**
 * Test whether each provider of type
 * SessionType::A2DP_HARDWARE_ENCODING_DATAPATH can be started and stopped with
 * AAC hardware encoding config
 */
TEST_P(BluetoothAudioProviderA2dpHardwareHidlTest,
       StartAndEndA2dpAacHardwareSession) {
  if (!IsOffloadSupported()) {
    return;
  }

  CodecConfiguration codec_config = {};
  codec_config.codecType = CodecType::AAC;
  codec_config.encodedAudioBitrate = 320000;
  codec_config.peerMtu = 1005;
  codec_config.isScmstEnabled = false;
  AudioConfiguration audio_config = {};
  std::vector<CodecSpecificConfig> aac_codec_specifics =
      GetAacCodecSpecificSupportedList(true);
  auto hidl_cb = [](BluetoothAudioStatus status,
                    const DataMQ::Descriptor& dataMQ) {
    EXPECT_EQ(status, BluetoothAudioStatus::SUCCESS);
    EXPECT_FALSE(dataMQ.isHandleValid());
  };
  for (auto codec_specific : aac_codec_specifics) {
    codec_config.config = codec_specific;
    audio_config.codecConfig(codec_config);
    auto hidl_retval =
        audio_provider_->startSession(audio_port_, audio_config, hidl_cb);
    // HIDL calls should not be failed and callback has to be executed
    ASSERT_TRUE(hidl_retval.isOk());
    EXPECT_TRUE(audio_provider_->endSession().isOk());
  }
}

/**
 * Test whether each provider of type
 * SessionType::A2DP_HARDWARE_ENCODING_DATAPATH can be started and stopped with
 * LDAC hardware encoding config
 */
TEST_P(BluetoothAudioProviderA2dpHardwareHidlTest,
       StartAndEndA2dpLdacHardwareSession) {
  if (!IsOffloadSupported()) {
    return;
  }

  CodecConfiguration codec_config = {};
  codec_config.codecType = CodecType::LDAC;
  codec_config.encodedAudioBitrate = 990000;
  codec_config.peerMtu = 1005;
  codec_config.isScmstEnabled = false;
  AudioConfiguration audio_config = {};
  std::vector<CodecSpecificConfig> ldac_codec_specifics =
      GetLdacCodecSpecificSupportedList(true);
  auto hidl_cb = [](BluetoothAudioStatus status,
                    const DataMQ::Descriptor& dataMQ) {
    EXPECT_EQ(status, BluetoothAudioStatus::SUCCESS);
    EXPECT_FALSE(dataMQ.isHandleValid());
  };
  for (auto codec_specific : ldac_codec_specifics) {
    codec_config.config = codec_specific;
    audio_config.codecConfig(codec_config);
    auto hidl_retval =
        audio_provider_->startSession(audio_port_, audio_config, hidl_cb);
    // HIDL calls should not be failed and callback has to be executed
    ASSERT_TRUE(hidl_retval.isOk());
    EXPECT_TRUE(audio_provider_->endSession().isOk());
  }
}

/**
 * Test whether each provider of type
 * SessionType::A2DP_HARDWARE_ENCODING_DATAPATH can be started and stopped with
 * AptX hardware encoding config
 */
TEST_P(BluetoothAudioProviderA2dpHardwareHidlTest,
       StartAndEndA2dpAptxHardwareSession) {
  if (!IsOffloadSupported()) {
    return;
  }

  for (auto codec_type : {CodecType::APTX, CodecType::APTX_HD}) {
    CodecConfiguration codec_config = {};
    codec_config.codecType = codec_type;
    codec_config.encodedAudioBitrate =
        (codec_type == CodecType::APTX ? 352000 : 576000);
    codec_config.peerMtu = 1005;
    codec_config.isScmstEnabled = false;
    AudioConfiguration audio_config = {};
    std::vector<CodecSpecificConfig> aptx_codec_specifics =
        GetAptxCodecSpecificSupportedList(
            (codec_type == CodecType::APTX_HD ? true : false), true);
    auto hidl_cb = [](BluetoothAudioStatus status,
                      const DataMQ::Descriptor& dataMQ) {
      EXPECT_EQ(status, BluetoothAudioStatus::SUCCESS);
      EXPECT_FALSE(dataMQ.isHandleValid());
    };
    for (auto codec_specific : aptx_codec_specifics) {
      codec_config.config = codec_specific;
      audio_config.codecConfig(codec_config);
      auto hidl_retval =
          audio_provider_->startSession(audio_port_, audio_config, hidl_cb);
      // HIDL calls should not be failed and callback has to be executed
      ASSERT_TRUE(hidl_retval.isOk());
      EXPECT_TRUE(audio_provider_->endSession().isOk());
    }
  }
}

/**
 * Test whether each provider of type
 * SessionType::A2DP_HARDWARE_ENCODING_DATAPATH can be started and stopped with
 * an invalid codec config
 */
TEST_P(BluetoothAudioProviderA2dpHardwareHidlTest,
       StartAndEndA2dpHardwareSessionInvalidCodecConfig) {
  if (!IsOffloadSupported()) {
    return;
  }
  ASSERT_NE(audio_provider_, nullptr);

  std::vector<CodecSpecificConfig> codec_specifics;
  for (auto codec_type : a2dp_codec_types) {
    switch (codec_type) {
      case CodecType::SBC:
        codec_specifics = GetSbcCodecSpecificSupportedList(false);
        break;
      case CodecType::AAC:
        codec_specifics = GetAacCodecSpecificSupportedList(false);
        break;
      case CodecType::LDAC:
        codec_specifics = GetLdacCodecSpecificSupportedList(false);
        break;
      case CodecType::APTX:
        codec_specifics = GetAptxCodecSpecificSupportedList(false, false);
        break;
      case CodecType::APTX_HD:
        codec_specifics = GetAptxCodecSpecificSupportedList(true, false);
        break;
      case CodecType::UNKNOWN:
        codec_specifics.clear();
        break;
    }
    if (codec_specifics.empty()) {
      continue;
    }

    CodecConfiguration codec_config = {};
    codec_config.codecType = codec_type;
    codec_config.encodedAudioBitrate = 328000;
    codec_config.peerMtu = 1005;
    codec_config.isScmstEnabled = false;
    AudioConfiguration audio_config = {};
    auto hidl_cb = [](BluetoothAudioStatus status,
                      const DataMQ::Descriptor& dataMQ) {
      EXPECT_EQ(status, BluetoothAudioStatus::UNSUPPORTED_CODEC_CONFIGURATION);
      EXPECT_FALSE(dataMQ.isHandleValid());
    };
    for (auto codec_specific : codec_specifics) {
      codec_config.config = codec_specific;
      audio_config.codecConfig(codec_config);
      auto hidl_retval =
          audio_provider_->startSession(audio_port_, audio_config, hidl_cb);
      // HIDL calls should not be failed and callback has to be executed
      ASSERT_TRUE(hidl_retval.isOk());
      EXPECT_TRUE(audio_provider_->endSession().isOk());
    }
  }
}

/**
 * openProvider HEARING_AID_SOFTWARE_ENCODING_DATAPATH
 */
class BluetoothAudioProviderHearingAidSoftwareHidlTest
    : public BluetoothAudioProvidersFactoryHidlTest {
 public:
  virtual void SetUp() override {
    BluetoothAudioProvidersFactoryHidlTest::SetUp();
    GetProviderCapabilitiesHelper(
        SessionType::HEARING_AID_SOFTWARE_ENCODING_DATAPATH);
    OpenProviderHelper(SessionType::HEARING_AID_SOFTWARE_ENCODING_DATAPATH);
    ASSERT_NE(audio_provider_, nullptr);
  }

  virtual void TearDown() override {
    audio_port_ = nullptr;
    audio_provider_ = nullptr;
    BluetoothAudioProvidersFactoryHidlTest::TearDown();
  }

  static constexpr SampleRate hearing_aid_sample_rates_[3] = {
      SampleRate::RATE_UNKNOWN, SampleRate::RATE_16000, SampleRate::RATE_24000};
  static constexpr BitsPerSample hearing_aid_bits_per_samples_[3] = {
      BitsPerSample::BITS_UNKNOWN, BitsPerSample::BITS_16,
      BitsPerSample::BITS_24};
  static constexpr ChannelMode hearing_aid_channel_modes_[3] = {
      ChannelMode::UNKNOWN, ChannelMode::MONO, ChannelMode::STEREO};
};

/**
 * Test whether each provider of type
 * SessionType::HEARING_AID_HARDWARE_ENCODING_DATAPATH can be started and
 * stopped with SBC hardware encoding config
 */
TEST_P(BluetoothAudioProviderHearingAidSoftwareHidlTest,
       OpenHearingAidSoftwareProvider) {}

/**
 * Test whether each provider of type
 * SessionType::HEARING_AID_SOFTWARE_ENCODING_DATAPATH can be started and
 * stopped with different PCM config
 */
TEST_P(BluetoothAudioProviderHearingAidSoftwareHidlTest,
       StartAndEndHearingAidSessionWithPossiblePcmConfig) {
  bool is_codec_config_valid;
  std::unique_ptr<DataMQ> tempDataMQ;
  auto hidl_cb = [&is_codec_config_valid, &tempDataMQ](
                     BluetoothAudioStatus status,
                     const DataMQ::Descriptor& dataMQ) {
    if (is_codec_config_valid) {
      ASSERT_EQ(status, BluetoothAudioStatus::SUCCESS);
      ASSERT_TRUE(dataMQ.isHandleValid());
      tempDataMQ.reset(new DataMQ(dataMQ));
    } else {
      EXPECT_EQ(status, BluetoothAudioStatus::UNSUPPORTED_CODEC_CONFIGURATION);
      EXPECT_FALSE(dataMQ.isHandleValid());
    }
  };
  AudioConfiguration audio_config = {};
  PcmParameters pcm_parameters = {};
  for (auto sample_rate : hearing_aid_sample_rates_) {
    pcm_parameters.sampleRate = sample_rate;
    for (auto bits_per_sample : hearing_aid_bits_per_samples_) {
      pcm_parameters.bitsPerSample = bits_per_sample;
      for (auto channel_mode : hearing_aid_channel_modes_) {
        pcm_parameters.channelMode = channel_mode;
        is_codec_config_valid = IsPcmParametersSupported(pcm_parameters);
        audio_config.pcmConfig(pcm_parameters);
        auto hidl_retval =
            audio_provider_->startSession(audio_port_, audio_config, hidl_cb);
        // HIDL calls should not be failed and callback has to be executed
        ASSERT_TRUE(hidl_retval.isOk());
        if (is_codec_config_valid) {
          EXPECT_TRUE(tempDataMQ != nullptr && tempDataMQ->isValid());
        }
        EXPECT_TRUE(audio_provider_->endSession().isOk());
      }  // ChannelMode
    }    // BitsPerSampple
  }      // SampleRate
}

static const std::vector<std::string> kAudioInstances =
    android::hardware::getAllHalInstanceNames(
        IBluetoothAudioProvidersFactory::descriptor);

GTEST_ALLOW_UNINSTANTIATED_PARAMETERIZED_TEST(
    BluetoothAudioProvidersFactoryHidlTest);
INSTANTIATE_TEST_SUITE_P(PerInstance, BluetoothAudioProvidersFactoryHidlTest,
                         testing::ValuesIn(kAudioInstances),
                         android::hardware::PrintInstanceNameToString);

GTEST_ALLOW_UNINSTANTIATED_PARAMETERIZED_TEST(
    BluetoothAudioProviderA2dpSoftwareHidlTest);
INSTANTIATE_TEST_SUITE_P(PerInstance,
                         BluetoothAudioProviderA2dpSoftwareHidlTest,
                         testing::ValuesIn(kAudioInstances),
                         android::hardware::PrintInstanceNameToString);

GTEST_ALLOW_UNINSTANTIATED_PARAMETERIZED_TEST(
    BluetoothAudioProviderA2dpHardwareHidlTest);
INSTANTIATE_TEST_SUITE_P(PerInstance,
                         BluetoothAudioProviderA2dpHardwareHidlTest,
                         testing::ValuesIn(kAudioInstances),
                         android::hardware::PrintInstanceNameToString);

GTEST_ALLOW_UNINSTANTIATED_PARAMETERIZED_TEST(
    BluetoothAudioProviderHearingAidSoftwareHidlTest);
INSTANTIATE_TEST_SUITE_P(PerInstance,
                         BluetoothAudioProviderHearingAidSoftwareHidlTest,
                         testing::ValuesIn(kAudioInstances),
                         android::hardware::PrintInstanceNameToString);