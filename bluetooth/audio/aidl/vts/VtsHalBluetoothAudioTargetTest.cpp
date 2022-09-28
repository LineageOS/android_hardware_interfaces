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
#include <aidl/Gtest.h>
#include <aidl/Vintf.h>
#include <aidl/android/hardware/bluetooth/audio/BnBluetoothAudioPort.h>
#include <aidl/android/hardware/bluetooth/audio/IBluetoothAudioPort.h>
#include <aidl/android/hardware/bluetooth/audio/IBluetoothAudioProviderFactory.h>
#include <android/binder_auto_utils.h>
#include <android/binder_manager.h>
#include <android/binder_process.h>
#include <binder/IServiceManager.h>
#include <binder/ProcessState.h>
#include <cutils/properties.h>
#include <fmq/AidlMessageQueue.h>

#include <cstdint>
#include <future>
#include <unordered_set>
#include <vector>

using aidl::android::hardware::audio::common::SinkMetadata;
using aidl::android::hardware::audio::common::SourceMetadata;
using aidl::android::hardware::bluetooth::audio::AacCapabilities;
using aidl::android::hardware::bluetooth::audio::AacConfiguration;
using aidl::android::hardware::bluetooth::audio::AptxCapabilities;
using aidl::android::hardware::bluetooth::audio::AptxConfiguration;
using aidl::android::hardware::bluetooth::audio::AudioCapabilities;
using aidl::android::hardware::bluetooth::audio::AudioConfiguration;
using aidl::android::hardware::bluetooth::audio::BnBluetoothAudioPort;
using aidl::android::hardware::bluetooth::audio::BroadcastCapability;
using aidl::android::hardware::bluetooth::audio::ChannelMode;
using aidl::android::hardware::bluetooth::audio::CodecCapabilities;
using aidl::android::hardware::bluetooth::audio::CodecConfiguration;
using aidl::android::hardware::bluetooth::audio::CodecType;
using aidl::android::hardware::bluetooth::audio::IBluetoothAudioPort;
using aidl::android::hardware::bluetooth::audio::IBluetoothAudioProvider;
using aidl::android::hardware::bluetooth::audio::IBluetoothAudioProviderFactory;
using aidl::android::hardware::bluetooth::audio::LatencyMode;
using aidl::android::hardware::bluetooth::audio::Lc3Capabilities;
using aidl::android::hardware::bluetooth::audio::Lc3Configuration;
using aidl::android::hardware::bluetooth::audio::LdacCapabilities;
using aidl::android::hardware::bluetooth::audio::LdacConfiguration;
using aidl::android::hardware::bluetooth::audio::LeAudioBroadcastConfiguration;
using aidl::android::hardware::bluetooth::audio::
    LeAudioCodecCapabilitiesSetting;
using aidl::android::hardware::bluetooth::audio::LeAudioCodecConfiguration;
using aidl::android::hardware::bluetooth::audio::LeAudioConfiguration;
using aidl::android::hardware::bluetooth::audio::OpusCapabilities;
using aidl::android::hardware::bluetooth::audio::OpusConfiguration;
using aidl::android::hardware::bluetooth::audio::PcmConfiguration;
using aidl::android::hardware::bluetooth::audio::PresentationPosition;
using aidl::android::hardware::bluetooth::audio::SbcAllocMethod;
using aidl::android::hardware::bluetooth::audio::SbcCapabilities;
using aidl::android::hardware::bluetooth::audio::SbcChannelMode;
using aidl::android::hardware::bluetooth::audio::SbcConfiguration;
using aidl::android::hardware::bluetooth::audio::SessionType;
using aidl::android::hardware::bluetooth::audio::UnicastCapability;
using aidl::android::hardware::common::fmq::MQDescriptor;
using aidl::android::hardware::common::fmq::SynchronizedReadWrite;
using android::AidlMessageQueue;
using android::ProcessState;
using android::String16;
using ndk::ScopedAStatus;
using ndk::SpAIBinder;

using MqDataType = int8_t;
using MqDataMode = SynchronizedReadWrite;
using DataMQ = AidlMessageQueue<MqDataType, MqDataMode>;
using DataMQDesc = MQDescriptor<MqDataType, MqDataMode>;

// Constants

static constexpr int32_t a2dp_sample_rates[] = {0, 44100, 48000, 88200, 96000};
static constexpr int8_t a2dp_bits_per_samples[] = {0, 16, 24, 32};
static constexpr ChannelMode a2dp_channel_modes[] = {
    ChannelMode::UNKNOWN, ChannelMode::MONO, ChannelMode::STEREO};
static constexpr CodecType a2dp_codec_types[] = {
    CodecType::UNKNOWN, CodecType::SBC,          CodecType::AAC,
    CodecType::APTX,    CodecType::APTX_HD,      CodecType::LDAC,
    CodecType::LC3,     CodecType::APTX_ADAPTIVE};
static std::vector<LatencyMode> latency_modes = {LatencyMode::FREE};
// Helpers

template <typename T>
struct identity {
  typedef T type;
};

template <class T>
bool contained_in_vector(const std::vector<T>& vector,
                         const typename identity<T>::type& target) {
  return std::find(vector.begin(), vector.end(), target) != vector.end();
}

void copy_codec_specific(CodecConfiguration::CodecSpecific& dst,
                         const CodecConfiguration::CodecSpecific& src) {
  switch (src.getTag()) {
    case CodecConfiguration::CodecSpecific::sbcConfig:
      dst.set<CodecConfiguration::CodecSpecific::sbcConfig>(
          src.get<CodecConfiguration::CodecSpecific::sbcConfig>());
      break;
    case CodecConfiguration::CodecSpecific::aacConfig:
      dst.set<CodecConfiguration::CodecSpecific::aacConfig>(
          src.get<CodecConfiguration::CodecSpecific::aacConfig>());
      break;
    case CodecConfiguration::CodecSpecific::ldacConfig:
      dst.set<CodecConfiguration::CodecSpecific::ldacConfig>(
          src.get<CodecConfiguration::CodecSpecific::ldacConfig>());
      break;
    case CodecConfiguration::CodecSpecific::aptxConfig:
      dst.set<CodecConfiguration::CodecSpecific::aptxConfig>(
          src.get<CodecConfiguration::CodecSpecific::aptxConfig>());
      break;
    case CodecConfiguration::CodecSpecific::opusConfig:
      dst.set<CodecConfiguration::CodecSpecific::opusConfig>(
          src.get<CodecConfiguration::CodecSpecific::opusConfig>());
      break;
    case CodecConfiguration::CodecSpecific::aptxAdaptiveConfig:
      dst.set<CodecConfiguration::CodecSpecific::aptxAdaptiveConfig>(
          src.get<CodecConfiguration::CodecSpecific::aptxAdaptiveConfig>());
      break;
    default:
      break;
  }
}

class BluetoothAudioPort : public BnBluetoothAudioPort {
 public:
  BluetoothAudioPort() {}

  ndk::ScopedAStatus startStream(bool) { return ScopedAStatus::ok(); }

  ndk::ScopedAStatus suspendStream() { return ScopedAStatus::ok(); }

  ndk::ScopedAStatus stopStream() { return ScopedAStatus::ok(); }

  ndk::ScopedAStatus getPresentationPosition(PresentationPosition*) {
    return ScopedAStatus::ok();
  }

  ndk::ScopedAStatus updateSourceMetadata(const SourceMetadata&) {
    return ScopedAStatus::ok();
  }

  ndk::ScopedAStatus updateSinkMetadata(const SinkMetadata&) {
    return ScopedAStatus::ok();
  }

  ndk::ScopedAStatus setLatencyMode(const LatencyMode) {
    return ScopedAStatus::ok();
  }

  ndk::ScopedAStatus setCodecType(const CodecType) {
    return ScopedAStatus::ok();
  }

 protected:
  virtual ~BluetoothAudioPort() = default;
};

class BluetoothAudioProviderFactoryAidl
    : public testing::TestWithParam<std::string> {
 public:
  virtual void SetUp() override {
    provider_factory_ = IBluetoothAudioProviderFactory::fromBinder(
        SpAIBinder(AServiceManager_getService(GetParam().c_str())));
    audio_provider_ = nullptr;
    ASSERT_NE(provider_factory_, nullptr);
  }

  virtual void TearDown() override { provider_factory_ = nullptr; }

  void GetProviderCapabilitiesHelper(const SessionType& session_type) {
    temp_provider_capabilities_.clear();
    auto aidl_retval = provider_factory_->getProviderCapabilities(
        session_type, &temp_provider_capabilities_);
    // AIDL calls should not be failed and callback has to be executed
    ASSERT_TRUE(aidl_retval.isOk());
    switch (session_type) {
      case SessionType::UNKNOWN: {
        ASSERT_TRUE(temp_provider_capabilities_.empty());
      } break;
      case SessionType::A2DP_SOFTWARE_ENCODING_DATAPATH:
      case SessionType::HEARING_AID_SOFTWARE_ENCODING_DATAPATH:
      case SessionType::LE_AUDIO_SOFTWARE_ENCODING_DATAPATH:
      case SessionType::LE_AUDIO_SOFTWARE_DECODING_DATAPATH:
      case SessionType::LE_AUDIO_BROADCAST_SOFTWARE_ENCODING_DATAPATH: {
        // All software paths are mandatory and must have exact 1
        // "PcmParameters"
        ASSERT_EQ(temp_provider_capabilities_.size(), 1);
        ASSERT_EQ(temp_provider_capabilities_[0].getTag(),
                  AudioCapabilities::pcmCapabilities);
      } break;
      case SessionType::A2DP_HARDWARE_OFFLOAD_ENCODING_DATAPATH:
      case SessionType::A2DP_HARDWARE_OFFLOAD_DECODING_DATAPATH: {
        std::unordered_set<CodecType> codec_types;
        // empty capability means offload is unsupported
        for (auto& audio_capability : temp_provider_capabilities_) {
          ASSERT_EQ(audio_capability.getTag(),
                    AudioCapabilities::a2dpCapabilities);
          const auto& codec_capabilities =
              audio_capability.get<AudioCapabilities::a2dpCapabilities>();
          // Every codec can present once at most
          ASSERT_EQ(codec_types.count(codec_capabilities.codecType), 0);
          switch (codec_capabilities.codecType) {
            case CodecType::SBC:
              ASSERT_EQ(codec_capabilities.capabilities.getTag(),
                        CodecCapabilities::Capabilities::sbcCapabilities);
              break;
            case CodecType::AAC:
              ASSERT_EQ(codec_capabilities.capabilities.getTag(),
                        CodecCapabilities::Capabilities::aacCapabilities);
              break;
            case CodecType::APTX:
            case CodecType::APTX_HD:
              ASSERT_EQ(codec_capabilities.capabilities.getTag(),
                        CodecCapabilities::Capabilities::aptxCapabilities);
              break;
            case CodecType::LDAC:
              ASSERT_EQ(codec_capabilities.capabilities.getTag(),
                        CodecCapabilities::Capabilities::ldacCapabilities);
              break;
            case CodecType::OPUS:
              ASSERT_EQ(codec_capabilities.capabilities.getTag(),
                        CodecCapabilities::Capabilities::opusCapabilities);
              break;
            case CodecType::APTX_ADAPTIVE:
            case CodecType::LC3:
            case CodecType::VENDOR:
            case CodecType::UNKNOWN:
              break;
          }
          codec_types.insert(codec_capabilities.codecType);
        }
      } break;
      case SessionType::LE_AUDIO_HARDWARE_OFFLOAD_ENCODING_DATAPATH:
      case SessionType::LE_AUDIO_HARDWARE_OFFLOAD_DECODING_DATAPATH:
      case SessionType::LE_AUDIO_BROADCAST_HARDWARE_OFFLOAD_ENCODING_DATAPATH: {
        // empty capability means offload is unsupported since capabilities are
        // not hardcoded
        for (auto audio_capability : temp_provider_capabilities_) {
          ASSERT_EQ(audio_capability.getTag(),
                    AudioCapabilities::leAudioCapabilities);
        }
      } break;
      case SessionType::A2DP_SOFTWARE_DECODING_DATAPATH: {
        if (!temp_provider_capabilities_.empty()) {
          ASSERT_EQ(temp_provider_capabilities_.size(), 1);
          ASSERT_EQ(temp_provider_capabilities_[0].getTag(),
                    AudioCapabilities::pcmCapabilities);
        }
      } break;
      default: {
        ASSERT_TRUE(temp_provider_capabilities_.empty());
      }
    }
  }

  /***
   * This helps to open the specified provider and check the openProvider()
   * has corruct return values. BUT, to keep it simple, it does not consider
   * the capability, and please do so at the SetUp of each session's test.
   ***/
  void OpenProviderHelper(const SessionType& session_type) {
    auto aidl_retval =
        provider_factory_->openProvider(session_type, &audio_provider_);
    if (aidl_retval.isOk()) {
      ASSERT_NE(session_type, SessionType::UNKNOWN);
      ASSERT_NE(audio_provider_, nullptr);
      audio_port_ = ndk::SharedRefBase::make<BluetoothAudioPort>();
    } else {
      // optional session type
      ASSERT_TRUE(
          session_type == SessionType::UNKNOWN ||
          session_type ==
              SessionType::A2DP_HARDWARE_OFFLOAD_ENCODING_DATAPATH ||
          session_type ==
              SessionType::LE_AUDIO_HARDWARE_OFFLOAD_DECODING_DATAPATH ||
          session_type ==
              SessionType::LE_AUDIO_HARDWARE_OFFLOAD_ENCODING_DATAPATH ||
          session_type ==
              SessionType::
                  LE_AUDIO_BROADCAST_HARDWARE_OFFLOAD_ENCODING_DATAPATH ||
          session_type ==
              SessionType::A2DP_HARDWARE_OFFLOAD_DECODING_DATAPATH ||
          session_type == SessionType::A2DP_SOFTWARE_DECODING_DATAPATH);
      ASSERT_EQ(audio_provider_, nullptr);
    }
  }

  void GetA2dpOffloadCapabilityHelper(const CodecType& codec_type) {
    temp_codec_capabilities_ = nullptr;
    for (auto& codec_capability : temp_provider_capabilities_) {
      auto& a2dp_capabilities =
          codec_capability.get<AudioCapabilities::a2dpCapabilities>();
      if (a2dp_capabilities.codecType != codec_type) {
        continue;
      }
      temp_codec_capabilities_ = &a2dp_capabilities;
    }
  }

  std::vector<CodecConfiguration::CodecSpecific>
  GetSbcCodecSpecificSupportedList(bool supported) {
    std::vector<CodecConfiguration::CodecSpecific> sbc_codec_specifics;
    if (!supported) {
      SbcConfiguration sbc_config{.sampleRateHz = 0, .bitsPerSample = 0};
      sbc_codec_specifics.push_back(
          CodecConfiguration::CodecSpecific(sbc_config));
      return sbc_codec_specifics;
    }
    GetA2dpOffloadCapabilityHelper(CodecType::SBC);
    if (temp_codec_capabilities_ == nullptr ||
        temp_codec_capabilities_->codecType != CodecType::SBC) {
      return sbc_codec_specifics;
    }
    // parse the capability
    auto& sbc_capability =
        temp_codec_capabilities_->capabilities
            .get<CodecCapabilities::Capabilities::sbcCapabilities>();
    if (sbc_capability.minBitpool > sbc_capability.maxBitpool) {
      return sbc_codec_specifics;
    }

    // combine those parameters into one list of
    // CodecConfiguration::CodecSpecific
    for (int32_t sample_rate : sbc_capability.sampleRateHz) {
      for (int8_t block_length : sbc_capability.blockLength) {
        for (int8_t num_subbands : sbc_capability.numSubbands) {
          for (int8_t bits_per_sample : sbc_capability.bitsPerSample) {
            for (auto channel_mode : sbc_capability.channelMode) {
              for (auto alloc_method : sbc_capability.allocMethod) {
                SbcConfiguration sbc_data = {
                    .sampleRateHz = sample_rate,
                    .channelMode = channel_mode,
                    .blockLength = block_length,
                    .numSubbands = num_subbands,
                    .allocMethod = alloc_method,
                    .bitsPerSample = bits_per_sample,
                    .minBitpool = sbc_capability.minBitpool,
                    .maxBitpool = sbc_capability.maxBitpool};
                sbc_codec_specifics.push_back(
                    CodecConfiguration::CodecSpecific(sbc_data));
              }
            }
          }
        }
      }
    }
    return sbc_codec_specifics;
  }

  std::vector<CodecConfiguration::CodecSpecific>
  GetAacCodecSpecificSupportedList(bool supported) {
    std::vector<CodecConfiguration::CodecSpecific> aac_codec_specifics;
    if (!supported) {
      AacConfiguration aac_config{.sampleRateHz = 0, .bitsPerSample = 0};
      aac_codec_specifics.push_back(
          CodecConfiguration::CodecSpecific(aac_config));
      return aac_codec_specifics;
    }
    GetA2dpOffloadCapabilityHelper(CodecType::AAC);
    if (temp_codec_capabilities_ == nullptr ||
        temp_codec_capabilities_->codecType != CodecType::AAC) {
      return aac_codec_specifics;
    }
    // parse the capability
    auto& aac_capability =
        temp_codec_capabilities_->capabilities
            .get<CodecCapabilities::Capabilities::aacCapabilities>();

    std::vector<bool> variable_bit_rate_enableds = {false};
    if (aac_capability.variableBitRateSupported) {
      variable_bit_rate_enableds.push_back(true);
    }

    // combine those parameters into one list of
    // CodecConfiguration::CodecSpecific
    for (auto object_type : aac_capability.objectType) {
      for (int32_t sample_rate : aac_capability.sampleRateHz) {
        for (auto channel_mode : aac_capability.channelMode) {
          for (int8_t bits_per_sample : aac_capability.bitsPerSample) {
            for (auto variable_bit_rate_enabled : variable_bit_rate_enableds) {
              AacConfiguration aac_data{
                  .objectType = object_type,
                  .sampleRateHz = sample_rate,
                  .channelMode = channel_mode,
                  .variableBitRateEnabled = variable_bit_rate_enabled,
                  .bitsPerSample = bits_per_sample};
              aac_codec_specifics.push_back(
                  CodecConfiguration::CodecSpecific(aac_data));
            }
          }
        }
      }
    }
    return aac_codec_specifics;
  }

  std::vector<CodecConfiguration::CodecSpecific>
  GetLdacCodecSpecificSupportedList(bool supported) {
    std::vector<CodecConfiguration::CodecSpecific> ldac_codec_specifics;
    if (!supported) {
      LdacConfiguration ldac_config{.sampleRateHz = 0, .bitsPerSample = 0};
      ldac_codec_specifics.push_back(
          CodecConfiguration::CodecSpecific(ldac_config));
      return ldac_codec_specifics;
    }
    GetA2dpOffloadCapabilityHelper(CodecType::LDAC);
    if (temp_codec_capabilities_ == nullptr ||
        temp_codec_capabilities_->codecType != CodecType::LDAC) {
      return ldac_codec_specifics;
    }
    // parse the capability
    auto& ldac_capability =
        temp_codec_capabilities_->capabilities
            .get<CodecCapabilities::Capabilities::ldacCapabilities>();

    // combine those parameters into one list of
    // CodecConfiguration::CodecSpecific
    for (int32_t sample_rate : ldac_capability.sampleRateHz) {
      for (int8_t bits_per_sample : ldac_capability.bitsPerSample) {
        for (auto channel_mode : ldac_capability.channelMode) {
          for (auto quality_index : ldac_capability.qualityIndex) {
            LdacConfiguration ldac_data{.sampleRateHz = sample_rate,
                                        .channelMode = channel_mode,
                                        .qualityIndex = quality_index,
                                        .bitsPerSample = bits_per_sample};
            ldac_codec_specifics.push_back(
                CodecConfiguration::CodecSpecific(ldac_data));
          }
        }
      }
    }
    return ldac_codec_specifics;
  }

  std::vector<CodecConfiguration::CodecSpecific>
  GetAptxCodecSpecificSupportedList(bool is_hd, bool supported) {
    std::vector<CodecConfiguration::CodecSpecific> aptx_codec_specifics;
    if (!supported) {
      AptxConfiguration aptx_config{.sampleRateHz = 0, .bitsPerSample = 0};
      aptx_codec_specifics.push_back(
          CodecConfiguration::CodecSpecific(aptx_config));
      return aptx_codec_specifics;
    }
    GetA2dpOffloadCapabilityHelper(
        (is_hd ? CodecType::APTX_HD : CodecType::APTX));
    if (temp_codec_capabilities_ == nullptr) {
      return aptx_codec_specifics;
    }
    if ((is_hd && temp_codec_capabilities_->codecType != CodecType::APTX_HD) ||
        (!is_hd && temp_codec_capabilities_->codecType != CodecType::APTX)) {
      return aptx_codec_specifics;
    }

    // parse the capability
    auto& aptx_capability =
        temp_codec_capabilities_->capabilities
            .get<CodecCapabilities::Capabilities::aptxCapabilities>();

    // combine those parameters into one list of
    // CodecConfiguration::CodecSpecific
    for (int8_t bits_per_sample : aptx_capability.bitsPerSample) {
      for (int32_t sample_rate : aptx_capability.sampleRateHz) {
        for (auto channel_mode : aptx_capability.channelMode) {
          AptxConfiguration aptx_data{.sampleRateHz = sample_rate,
                                      .channelMode = channel_mode,
                                      .bitsPerSample = bits_per_sample};
          aptx_codec_specifics.push_back(
              CodecConfiguration::CodecSpecific(aptx_data));
        }
      }
    }
    return aptx_codec_specifics;
  }

  std::vector<CodecConfiguration::CodecSpecific>
  GetOpusCodecSpecificSupportedList(bool supported) {
    std::vector<CodecConfiguration::CodecSpecific> opus_codec_specifics;
    if (!supported) {
      OpusConfiguration opus_config{.samplingFrequencyHz = 0,
                                    .frameDurationUs = 0};
      opus_codec_specifics.push_back(
          CodecConfiguration::CodecSpecific(opus_config));
      return opus_codec_specifics;
    }
    GetA2dpOffloadCapabilityHelper(CodecType::OPUS);
    if (temp_codec_capabilities_ == nullptr ||
        temp_codec_capabilities_->codecType != CodecType::OPUS) {
      return opus_codec_specifics;
    }
    // parse the capability
    auto& opus_capability =
        temp_codec_capabilities_->capabilities
            .get<CodecCapabilities::Capabilities::opusCapabilities>();

    // combine those parameters into one list of
    // CodecConfiguration::CodecSpecific
    for (int32_t samplingFrequencyHz : opus_capability->samplingFrequencyHz) {
      for (int32_t frameDurationUs : opus_capability->frameDurationUs) {
        for (auto channel_mode : opus_capability->channelMode) {
          OpusConfiguration opus_data{
              .samplingFrequencyHz = samplingFrequencyHz,
              .frameDurationUs = frameDurationUs,
              .channelMode = channel_mode,
          };
          opus_codec_specifics.push_back(
              CodecConfiguration::CodecSpecific(opus_data));
        }
      }
    }
    return opus_codec_specifics;
  }

  bool IsPcmConfigSupported(const PcmConfiguration& pcm_config) {
    if (temp_provider_capabilities_.size() != 1 ||
        temp_provider_capabilities_[0].getTag() !=
            AudioCapabilities::pcmCapabilities) {
      return false;
    }
    auto pcm_capability = temp_provider_capabilities_[0]
                              .get<AudioCapabilities::pcmCapabilities>();
    return (contained_in_vector(pcm_capability.channelMode,
                                pcm_config.channelMode) &&
            contained_in_vector(pcm_capability.sampleRateHz,
                                pcm_config.sampleRateHz) &&
            contained_in_vector(pcm_capability.bitsPerSample,
                                pcm_config.bitsPerSample));
  }

  std::shared_ptr<IBluetoothAudioProviderFactory> provider_factory_;
  std::shared_ptr<IBluetoothAudioProvider> audio_provider_;
  std::shared_ptr<IBluetoothAudioPort> audio_port_;
  std::vector<AudioCapabilities> temp_provider_capabilities_;

  // temp storage saves the specified codec capability by
  // GetOffloadCodecCapabilityHelper()
  CodecCapabilities* temp_codec_capabilities_;

  static constexpr SessionType kSessionTypes[] = {
      SessionType::UNKNOWN,
      SessionType::A2DP_SOFTWARE_ENCODING_DATAPATH,
      SessionType::A2DP_HARDWARE_OFFLOAD_ENCODING_DATAPATH,
      SessionType::HEARING_AID_SOFTWARE_ENCODING_DATAPATH,
      SessionType::LE_AUDIO_SOFTWARE_ENCODING_DATAPATH,
      SessionType::LE_AUDIO_SOFTWARE_DECODING_DATAPATH,
      SessionType::LE_AUDIO_HARDWARE_OFFLOAD_ENCODING_DATAPATH,
      SessionType::LE_AUDIO_HARDWARE_OFFLOAD_DECODING_DATAPATH,
      SessionType::LE_AUDIO_BROADCAST_SOFTWARE_ENCODING_DATAPATH,
      SessionType::LE_AUDIO_BROADCAST_HARDWARE_OFFLOAD_ENCODING_DATAPATH,
      SessionType::A2DP_SOFTWARE_DECODING_DATAPATH,
      SessionType::A2DP_HARDWARE_OFFLOAD_DECODING_DATAPATH,
  };
};

/**
 * Test whether we can get the FactoryService from HIDL
 */
TEST_P(BluetoothAudioProviderFactoryAidl, GetProviderFactoryService) {}

/**
 * Test whether we can open a provider for each provider returned by
 * getProviderCapabilities() with non-empty capabalities
 */
TEST_P(BluetoothAudioProviderFactoryAidl,
       OpenProviderAndCheckCapabilitiesBySession) {
  for (auto session_type : kSessionTypes) {
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
class BluetoothAudioProviderA2dpEncodingSoftwareAidl
    : public BluetoothAudioProviderFactoryAidl {
 public:
  virtual void SetUp() override {
    BluetoothAudioProviderFactoryAidl::SetUp();
    GetProviderCapabilitiesHelper(SessionType::A2DP_SOFTWARE_ENCODING_DATAPATH);
    OpenProviderHelper(SessionType::A2DP_SOFTWARE_ENCODING_DATAPATH);
    ASSERT_NE(audio_provider_, nullptr);
  }

  virtual void TearDown() override {
    audio_port_ = nullptr;
    audio_provider_ = nullptr;
    BluetoothAudioProviderFactoryAidl::TearDown();
  }
};

/**
 * Test whether we can open a provider of type
 */
TEST_P(BluetoothAudioProviderA2dpEncodingSoftwareAidl,
       OpenA2dpEncodingSoftwareProvider) {}

/**
 * Test whether each provider of type
 * SessionType::A2DP_SOFTWARE_ENCODING_DATAPATH can be started and stopped with
 * different PCM config
 */
TEST_P(BluetoothAudioProviderA2dpEncodingSoftwareAidl,
       StartAndEndA2dpEncodingSoftwareSessionWithPossiblePcmConfig) {
  for (auto sample_rate : a2dp_sample_rates) {
    for (auto bits_per_sample : a2dp_bits_per_samples) {
      for (auto channel_mode : a2dp_channel_modes) {
        PcmConfiguration pcm_config{
            .sampleRateHz = sample_rate,
            .channelMode = channel_mode,
            .bitsPerSample = bits_per_sample,
        };
        bool is_codec_config_valid = IsPcmConfigSupported(pcm_config);
        DataMQDesc mq_desc;
        auto aidl_retval = audio_provider_->startSession(
            audio_port_, AudioConfiguration(pcm_config), latency_modes,
            &mq_desc);
        DataMQ data_mq(mq_desc);

        EXPECT_EQ(aidl_retval.isOk(), is_codec_config_valid);
        if (is_codec_config_valid) {
          EXPECT_TRUE(data_mq.isValid());
        }
        EXPECT_TRUE(audio_provider_->endSession().isOk());
      }
    }
  }
}

/**
 * openProvider A2DP_HARDWARE_OFFLOAD_ENCODING_DATAPATH
 */
class BluetoothAudioProviderA2dpEncodingHardwareAidl
    : public BluetoothAudioProviderFactoryAidl {
 public:
  virtual void SetUp() override {
    BluetoothAudioProviderFactoryAidl::SetUp();
    GetProviderCapabilitiesHelper(
        SessionType::A2DP_HARDWARE_OFFLOAD_ENCODING_DATAPATH);
    OpenProviderHelper(SessionType::A2DP_HARDWARE_OFFLOAD_ENCODING_DATAPATH);
    ASSERT_TRUE(temp_provider_capabilities_.empty() ||
                audio_provider_ != nullptr);
  }

  virtual void TearDown() override {
    audio_port_ = nullptr;
    audio_provider_ = nullptr;
    BluetoothAudioProviderFactoryAidl::TearDown();
  }

  bool IsOffloadSupported() { return (temp_provider_capabilities_.size() > 0); }
};

/**
 * Test whether we can open a provider of type
 */
TEST_P(BluetoothAudioProviderA2dpEncodingHardwareAidl,
       OpenA2dpEncodingHardwareProvider) {}

/**
 * Test whether each provider of type
 * SessionType::A2DP_HARDWARE_ENCODING_DATAPATH can be started and stopped with
 * SBC hardware encoding config
 */
TEST_P(BluetoothAudioProviderA2dpEncodingHardwareAidl,
       StartAndEndA2dpSbcEncodingHardwareSession) {
  if (!IsOffloadSupported()) {
    return;
  }

  CodecConfiguration codec_config = {
      .codecType = CodecType::SBC,
      .encodedAudioBitrate = 328000,
      .peerMtu = 1005,
      .isScmstEnabled = false,
  };
  auto sbc_codec_specifics = GetSbcCodecSpecificSupportedList(true);

  for (auto& codec_specific : sbc_codec_specifics) {
    copy_codec_specific(codec_config.config, codec_specific);
    DataMQDesc mq_desc;
    auto aidl_retval = audio_provider_->startSession(
        audio_port_, AudioConfiguration(codec_config), latency_modes, &mq_desc);

    ASSERT_TRUE(aidl_retval.isOk());
    EXPECT_TRUE(audio_provider_->endSession().isOk());
  }
}

/**
 * Test whether each provider of type
 * SessionType::A2DP_HARDWARE_ENCODING_DATAPATH can be started and stopped with
 * AAC hardware encoding config
 */
TEST_P(BluetoothAudioProviderA2dpEncodingHardwareAidl,
       StartAndEndA2dpAacEncodingHardwareSession) {
  if (!IsOffloadSupported()) {
    return;
  }

  CodecConfiguration codec_config = {
      .codecType = CodecType::AAC,
      .encodedAudioBitrate = 320000,
      .peerMtu = 1005,
      .isScmstEnabled = false,
  };
  auto aac_codec_specifics = GetAacCodecSpecificSupportedList(true);

  for (auto& codec_specific : aac_codec_specifics) {
    copy_codec_specific(codec_config.config, codec_specific);
    DataMQDesc mq_desc;
    auto aidl_retval = audio_provider_->startSession(
        audio_port_, AudioConfiguration(codec_config), latency_modes, &mq_desc);

    ASSERT_TRUE(aidl_retval.isOk());
    EXPECT_TRUE(audio_provider_->endSession().isOk());
  }
}

/**
 * Test whether each provider of type
 * SessionType::A2DP_HARDWARE_ENCODING_DATAPATH can be started and stopped with
 * LDAC hardware encoding config
 */
TEST_P(BluetoothAudioProviderA2dpEncodingHardwareAidl,
       StartAndEndA2dpLdacEncodingHardwareSession) {
  if (!IsOffloadSupported()) {
    return;
  }

  CodecConfiguration codec_config = {
      .codecType = CodecType::LDAC,
      .encodedAudioBitrate = 990000,
      .peerMtu = 1005,
      .isScmstEnabled = false,
  };
  auto ldac_codec_specifics = GetLdacCodecSpecificSupportedList(true);

  for (auto& codec_specific : ldac_codec_specifics) {
    copy_codec_specific(codec_config.config, codec_specific);
    DataMQDesc mq_desc;
    auto aidl_retval = audio_provider_->startSession(
        audio_port_, AudioConfiguration(codec_config), latency_modes, &mq_desc);

    ASSERT_TRUE(aidl_retval.isOk());
    EXPECT_TRUE(audio_provider_->endSession().isOk());
  }
}

/**
 * Test whether each provider of type
 * SessionType::A2DP_HARDWARE_ENCODING_DATAPATH can be started and stopped with
 * Opus hardware encoding config
 */
TEST_P(BluetoothAudioProviderA2dpEncodingHardwareAidl,
       StartAndEndA2dpOpusEncodingHardwareSession) {
  if (!IsOffloadSupported()) {
    return;
  }

  CodecConfiguration codec_config = {
      .codecType = CodecType::OPUS,
      .encodedAudioBitrate = 990000,
      .peerMtu = 1005,
      .isScmstEnabled = false,
  };
  auto opus_codec_specifics = GetOpusCodecSpecificSupportedList(true);

  for (auto& codec_specific : opus_codec_specifics) {
    copy_codec_specific(codec_config.config, codec_specific);
    DataMQDesc mq_desc;
    auto aidl_retval = audio_provider_->startSession(
        audio_port_, AudioConfiguration(codec_config), latency_modes, &mq_desc);

    ASSERT_TRUE(aidl_retval.isOk());
    EXPECT_TRUE(audio_provider_->endSession().isOk());
  }
}

/**
 * Test whether each provider of type
 * SessionType::A2DP_HARDWARE_ENCODING_DATAPATH can be started and stopped with
 * AptX hardware encoding config
 */
TEST_P(BluetoothAudioProviderA2dpEncodingHardwareAidl,
       StartAndEndA2dpAptxEncodingHardwareSession) {
  if (!IsOffloadSupported()) {
    return;
  }

  for (auto codec_type : {CodecType::APTX, CodecType::APTX_HD}) {
    CodecConfiguration codec_config = {
        .codecType = codec_type,
        .encodedAudioBitrate =
            (codec_type == CodecType::APTX ? 352000 : 576000),
        .peerMtu = 1005,
        .isScmstEnabled = false,
    };

    auto aptx_codec_specifics = GetAptxCodecSpecificSupportedList(
        (codec_type == CodecType::APTX_HD ? true : false), true);

    for (auto& codec_specific : aptx_codec_specifics) {
      copy_codec_specific(codec_config.config, codec_specific);
      DataMQDesc mq_desc;
      auto aidl_retval = audio_provider_->startSession(
          audio_port_, AudioConfiguration(codec_config), latency_modes,
          &mq_desc);

      ASSERT_TRUE(aidl_retval.isOk());
      EXPECT_TRUE(audio_provider_->endSession().isOk());
    }
  }
}

/**
 * Test whether each provider of type
 * SessionType::A2DP_HARDWARE_ENCODING_DATAPATH can be started and stopped with
 * an invalid codec config
 */
TEST_P(BluetoothAudioProviderA2dpEncodingHardwareAidl,
       StartAndEndA2dpEncodingHardwareSessionInvalidCodecConfig) {
  if (!IsOffloadSupported()) {
    return;
  }
  ASSERT_NE(audio_provider_, nullptr);

  std::vector<CodecConfiguration::CodecSpecific> codec_specifics;
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
      case CodecType::OPUS:
        codec_specifics = GetOpusCodecSpecificSupportedList(false);
        continue;
      case CodecType::APTX_ADAPTIVE:
      case CodecType::LC3:
      case CodecType::VENDOR:
      case CodecType::UNKNOWN:
        codec_specifics.clear();
        break;
    }
    if (codec_specifics.empty()) {
      continue;
    }

    CodecConfiguration codec_config = {
        .codecType = codec_type,
        .encodedAudioBitrate = 328000,
        .peerMtu = 1005,
        .isScmstEnabled = false,
    };
    for (auto codec_specific : codec_specifics) {
      copy_codec_specific(codec_config.config, codec_specific);
      DataMQDesc mq_desc;
      auto aidl_retval = audio_provider_->startSession(
          audio_port_, AudioConfiguration(codec_config), latency_modes,
          &mq_desc);

      // AIDL call should fail on invalid codec
      ASSERT_FALSE(aidl_retval.isOk());
      EXPECT_TRUE(audio_provider_->endSession().isOk());
    }
  }
}

/**
 * openProvider HEARING_AID_SOFTWARE_ENCODING_DATAPATH
 */
class BluetoothAudioProviderHearingAidSoftwareAidl
    : public BluetoothAudioProviderFactoryAidl {
 public:
  virtual void SetUp() override {
    BluetoothAudioProviderFactoryAidl::SetUp();
    GetProviderCapabilitiesHelper(
        SessionType::HEARING_AID_SOFTWARE_ENCODING_DATAPATH);
    OpenProviderHelper(SessionType::HEARING_AID_SOFTWARE_ENCODING_DATAPATH);
    ASSERT_NE(audio_provider_, nullptr);
  }

  virtual void TearDown() override {
    audio_port_ = nullptr;
    audio_provider_ = nullptr;
    BluetoothAudioProviderFactoryAidl::TearDown();
  }

  static constexpr int32_t hearing_aid_sample_rates_[] = {0, 16000, 24000};
  static constexpr int8_t hearing_aid_bits_per_samples_[] = {0, 16, 24};
  static constexpr ChannelMode hearing_aid_channel_modes_[] = {
      ChannelMode::UNKNOWN, ChannelMode::MONO, ChannelMode::STEREO};
};

/**
 * Test whether we can open a provider of type
 */
TEST_P(BluetoothAudioProviderHearingAidSoftwareAidl,
       OpenHearingAidSoftwareProvider) {}

/**
 * Test whether each provider of type
 * SessionType::HEARING_AID_SOFTWARE_ENCODING_DATAPATH can be started and
 * stopped with different PCM config
 */
TEST_P(BluetoothAudioProviderHearingAidSoftwareAidl,
       StartAndEndHearingAidSessionWithPossiblePcmConfig) {
  for (int32_t sample_rate : hearing_aid_sample_rates_) {
    for (int8_t bits_per_sample : hearing_aid_bits_per_samples_) {
      for (auto channel_mode : hearing_aid_channel_modes_) {
        PcmConfiguration pcm_config{
            .sampleRateHz = sample_rate,
            .channelMode = channel_mode,
            .bitsPerSample = bits_per_sample,
        };
        bool is_codec_config_valid = IsPcmConfigSupported(pcm_config);
        DataMQDesc mq_desc;
        auto aidl_retval = audio_provider_->startSession(
            audio_port_, AudioConfiguration(pcm_config), latency_modes,
            &mq_desc);
        DataMQ data_mq(mq_desc);

        EXPECT_EQ(aidl_retval.isOk(), is_codec_config_valid);
        if (is_codec_config_valid) {
          EXPECT_TRUE(data_mq.isValid());
        }
        EXPECT_TRUE(audio_provider_->endSession().isOk());
      }
    }
  }
}

/**
 * openProvider LE_AUDIO_SOFTWARE_ENCODING_DATAPATH
 */
class BluetoothAudioProviderLeAudioOutputSoftwareAidl
    : public BluetoothAudioProviderFactoryAidl {
 public:
  virtual void SetUp() override {
    BluetoothAudioProviderFactoryAidl::SetUp();
    GetProviderCapabilitiesHelper(
        SessionType::LE_AUDIO_SOFTWARE_ENCODING_DATAPATH);
    OpenProviderHelper(SessionType::LE_AUDIO_SOFTWARE_ENCODING_DATAPATH);
    ASSERT_NE(audio_provider_, nullptr);
  }

  virtual void TearDown() override {
    audio_port_ = nullptr;
    audio_provider_ = nullptr;
    BluetoothAudioProviderFactoryAidl::TearDown();
  }

  static constexpr int32_t le_audio_output_sample_rates_[] = {
      0, 8000, 16000, 24000, 32000, 44100, 48000,
  };
  static constexpr int8_t le_audio_output_bits_per_samples_[] = {0, 16, 24};
  static constexpr ChannelMode le_audio_output_channel_modes_[] = {
      ChannelMode::UNKNOWN, ChannelMode::MONO, ChannelMode::STEREO};
  static constexpr int32_t le_audio_output_data_interval_us_[] = {
      0 /* Invalid */, 10000 /* Valid 10ms */};
};

/**
 * Test whether each provider of type
 * SessionType::LE_AUDIO_SOFTWARE_ENCODING_DATAPATH can be started and
 * stopped
 */
TEST_P(BluetoothAudioProviderLeAudioOutputSoftwareAidl,
       OpenLeAudioOutputSoftwareProvider) {}

/**
 * Test whether each provider of type
 * SessionType::LE_AUDIO_SOFTWARE_ENCODING_DATAPATH can be started and
 * stopped with different PCM config
 */
TEST_P(BluetoothAudioProviderLeAudioOutputSoftwareAidl,
       StartAndEndLeAudioOutputSessionWithPossiblePcmConfig) {
  for (auto sample_rate : le_audio_output_sample_rates_) {
    for (auto bits_per_sample : le_audio_output_bits_per_samples_) {
      for (auto channel_mode : le_audio_output_channel_modes_) {
        for (auto data_interval_us : le_audio_output_data_interval_us_) {
          PcmConfiguration pcm_config{
              .sampleRateHz = sample_rate,
              .channelMode = channel_mode,
              .bitsPerSample = bits_per_sample,
              .dataIntervalUs = data_interval_us,
          };
          bool is_codec_config_valid =
              IsPcmConfigSupported(pcm_config) && pcm_config.dataIntervalUs > 0;
          DataMQDesc mq_desc;
          auto aidl_retval = audio_provider_->startSession(
              audio_port_, AudioConfiguration(pcm_config), latency_modes,
              &mq_desc);
          DataMQ data_mq(mq_desc);

          EXPECT_EQ(aidl_retval.isOk(), is_codec_config_valid);
          if (is_codec_config_valid) {
            EXPECT_TRUE(data_mq.isValid());
          }
          EXPECT_TRUE(audio_provider_->endSession().isOk());
        }
      }
    }
  }
}

/**
 * openProvider LE_AUDIO_SOFTWARE_DECODING_DATAPATH
 */
class BluetoothAudioProviderLeAudioInputSoftwareAidl
    : public BluetoothAudioProviderFactoryAidl {
 public:
  virtual void SetUp() override {
    BluetoothAudioProviderFactoryAidl::SetUp();
    GetProviderCapabilitiesHelper(
        SessionType::LE_AUDIO_SOFTWARE_DECODING_DATAPATH);
    OpenProviderHelper(SessionType::LE_AUDIO_SOFTWARE_DECODING_DATAPATH);
    ASSERT_NE(audio_provider_, nullptr);
  }

  virtual void TearDown() override {
    audio_port_ = nullptr;
    audio_provider_ = nullptr;
    BluetoothAudioProviderFactoryAidl::TearDown();
  }

  static constexpr int32_t le_audio_input_sample_rates_[] = {
      0, 8000, 16000, 24000, 32000, 44100, 48000};
  static constexpr int8_t le_audio_input_bits_per_samples_[] = {0, 16, 24};
  static constexpr ChannelMode le_audio_input_channel_modes_[] = {
      ChannelMode::UNKNOWN, ChannelMode::MONO, ChannelMode::STEREO};
  static constexpr int32_t le_audio_input_data_interval_us_[] = {
      0 /* Invalid */, 10000 /* Valid 10ms */};
};

/**
 * Test whether each provider of type
 * SessionType::LE_AUDIO_SOFTWARE_DECODING_DATAPATH can be started and
 * stopped
 */
TEST_P(BluetoothAudioProviderLeAudioInputSoftwareAidl,
       OpenLeAudioInputSoftwareProvider) {}

/**
 * Test whether each provider of type
 * SessionType::LE_AUDIO_SOFTWARE_DECODING_DATAPATH can be started and
 * stopped with different PCM config
 */
TEST_P(BluetoothAudioProviderLeAudioInputSoftwareAidl,
       StartAndEndLeAudioInputSessionWithPossiblePcmConfig) {
  for (auto sample_rate : le_audio_input_sample_rates_) {
    for (auto bits_per_sample : le_audio_input_bits_per_samples_) {
      for (auto channel_mode : le_audio_input_channel_modes_) {
        for (auto data_interval_us : le_audio_input_data_interval_us_) {
          PcmConfiguration pcm_config{
              .sampleRateHz = sample_rate,
              .channelMode = channel_mode,
              .bitsPerSample = bits_per_sample,
              .dataIntervalUs = data_interval_us,
          };
          bool is_codec_config_valid =
              IsPcmConfigSupported(pcm_config) && pcm_config.dataIntervalUs > 0;
          DataMQDesc mq_desc;
          auto aidl_retval = audio_provider_->startSession(
              audio_port_, AudioConfiguration(pcm_config), latency_modes,
              &mq_desc);
          DataMQ data_mq(mq_desc);

          EXPECT_EQ(aidl_retval.isOk(), is_codec_config_valid);
          if (is_codec_config_valid) {
            EXPECT_TRUE(data_mq.isValid());
          }
          EXPECT_TRUE(audio_provider_->endSession().isOk());
        }
      }
    }
  }
}

/**
 * openProvider LE_AUDIO_HARDWARE_OFFLOAD_ENCODING_DATAPATH
 */
class BluetoothAudioProviderLeAudioOutputHardwareAidl
    : public BluetoothAudioProviderFactoryAidl {
 public:
  virtual void SetUp() override {
    BluetoothAudioProviderFactoryAidl::SetUp();
    GetProviderCapabilitiesHelper(
        SessionType::LE_AUDIO_HARDWARE_OFFLOAD_ENCODING_DATAPATH);
    OpenProviderHelper(
        SessionType::LE_AUDIO_HARDWARE_OFFLOAD_ENCODING_DATAPATH);
    ASSERT_TRUE(temp_provider_capabilities_.empty() ||
                audio_provider_ != nullptr);
  }

  virtual void TearDown() override {
    audio_port_ = nullptr;
    audio_provider_ = nullptr;
    BluetoothAudioProviderFactoryAidl::TearDown();
  }

  bool IsOffloadOutputSupported() {
    for (auto& capability : temp_provider_capabilities_) {
      if (capability.getTag() != AudioCapabilities::leAudioCapabilities) {
        continue;
      }
      auto& le_audio_capability =
          capability.get<AudioCapabilities::leAudioCapabilities>();
      if (le_audio_capability.unicastEncodeCapability.codecType !=
          CodecType::UNKNOWN)
        return true;
    }
    return false;
  }

  std::vector<Lc3Configuration> GetUnicastLc3SupportedList(bool decoding,
                                                           bool supported) {
    std::vector<Lc3Configuration> le_audio_codec_configs;
    if (!supported) {
      Lc3Configuration lc3_config{.pcmBitDepth = 0, .samplingFrequencyHz = 0};
      le_audio_codec_configs.push_back(lc3_config);
      return le_audio_codec_configs;
    }

    // There might be more than one LeAudioCodecCapabilitiesSetting
    std::vector<Lc3Capabilities> lc3_capabilities;
    for (auto& capability : temp_provider_capabilities_) {
      if (capability.getTag() != AudioCapabilities::leAudioCapabilities) {
        continue;
      }
      auto& le_audio_capability =
          capability.get<AudioCapabilities::leAudioCapabilities>();
      auto& unicast_capability =
          decoding ? le_audio_capability.unicastDecodeCapability
                   : le_audio_capability.unicastEncodeCapability;
      if (unicast_capability.codecType != CodecType::LC3) {
        continue;
      }
      auto& lc3_capability = unicast_capability.leAudioCodecCapabilities.get<
          UnicastCapability::LeAudioCodecCapabilities::lc3Capabilities>();
      lc3_capabilities.push_back(lc3_capability);
    }

    // Combine those parameters into one list of LeAudioCodecConfiguration
    // This seems horrible, but usually each Lc3Capability only contains a
    // single Lc3Configuration, which means every array has a length of 1.
    for (auto& lc3_capability : lc3_capabilities) {
      for (int32_t samplingFrequencyHz : lc3_capability.samplingFrequencyHz) {
        for (int32_t frameDurationUs : lc3_capability.frameDurationUs) {
          for (int32_t octetsPerFrame : lc3_capability.octetsPerFrame) {
            Lc3Configuration lc3_config = {
                .samplingFrequencyHz = samplingFrequencyHz,
                .frameDurationUs = frameDurationUs,
                .octetsPerFrame = octetsPerFrame,
            };
            le_audio_codec_configs.push_back(lc3_config);
          }
        }
      }
    }

    return le_audio_codec_configs;
  }

  LeAudioCodecCapabilitiesSetting temp_le_audio_capabilities_;
};

/**
 * Test whether each provider of type
 * SessionType::LE_AUDIO_HARDWARE_OFFLOAD_ENCODING_DATAPATH can be started and
 * stopped
 */
TEST_P(BluetoothAudioProviderLeAudioOutputHardwareAidl,
       OpenLeAudioOutputHardwareProvider) {}

/**
 * Test whether each provider of type
 * SessionType::LE_AUDIO_HARDWARE_OFFLOAD_ENCODING_DATAPATH can be started and
 * stopped with Unicast hardware encoding config
 */
TEST_P(BluetoothAudioProviderLeAudioOutputHardwareAidl,
       StartAndEndLeAudioOutputSessionWithPossibleUnicastConfig) {
  if (!IsOffloadOutputSupported()) {
    return;
  }

  auto lc3_codec_configs =
      GetUnicastLc3SupportedList(false /* decoding */, true /* supported */);
  LeAudioConfiguration le_audio_config = {
      .codecType = CodecType::LC3,
      .peerDelayUs = 0,
  };

  for (auto& lc3_config : lc3_codec_configs) {
    le_audio_config.leAudioCodecConfig
        .set<LeAudioCodecConfiguration::lc3Config>(lc3_config);
    DataMQDesc mq_desc;
    auto aidl_retval = audio_provider_->startSession(
        audio_port_, AudioConfiguration(le_audio_config), latency_modes,
        &mq_desc);

    ASSERT_TRUE(aidl_retval.isOk());
    EXPECT_TRUE(audio_provider_->endSession().isOk());
  }
}

/**
 * Test whether each provider of type
 * SessionType::LE_AUDIO_HARDWARE_OFFLOAD_ENCODING_DATAPATH can be started and
 * stopped with Unicast hardware encoding config
 *
 * Disabled since offload codec checking is not ready
 */
TEST_P(BluetoothAudioProviderLeAudioOutputHardwareAidl,
       DISABLED_StartAndEndLeAudioOutputSessionWithInvalidAudioConfiguration) {
  if (!IsOffloadOutputSupported()) {
    return;
  }

  auto lc3_codec_configs =
      GetUnicastLc3SupportedList(false /* decoding */, false /* supported */);
  LeAudioConfiguration le_audio_config = {
      .codecType = CodecType::LC3,
      .peerDelayUs = 0,
  };

  for (auto& lc3_config : lc3_codec_configs) {
    le_audio_config.leAudioCodecConfig
        .set<LeAudioCodecConfiguration::lc3Config>(lc3_config);
    DataMQDesc mq_desc;
    auto aidl_retval = audio_provider_->startSession(
        audio_port_, AudioConfiguration(le_audio_config), latency_modes,
        &mq_desc);

    // AIDL call should fail on invalid codec
    ASSERT_FALSE(aidl_retval.isOk());
    EXPECT_TRUE(audio_provider_->endSession().isOk());
  }
}

/**
 * openProvider LE_AUDIO_HARDWARE_OFFLOAD_DECODING_DATAPATH
 */
class BluetoothAudioProviderLeAudioInputHardwareAidl
    : public BluetoothAudioProviderLeAudioOutputHardwareAidl {
 public:
  virtual void SetUp() override {
    BluetoothAudioProviderFactoryAidl::SetUp();
    GetProviderCapabilitiesHelper(
        SessionType::LE_AUDIO_HARDWARE_OFFLOAD_DECODING_DATAPATH);
    OpenProviderHelper(
        SessionType::LE_AUDIO_HARDWARE_OFFLOAD_DECODING_DATAPATH);
    ASSERT_TRUE(temp_provider_capabilities_.empty() ||
                audio_provider_ != nullptr);
  }

  bool IsOffloadInputSupported() {
    for (auto& capability : temp_provider_capabilities_) {
      if (capability.getTag() != AudioCapabilities::leAudioCapabilities) {
        continue;
      }
      auto& le_audio_capability =
          capability.get<AudioCapabilities::leAudioCapabilities>();
      if (le_audio_capability.unicastDecodeCapability.codecType !=
          CodecType::UNKNOWN)
        return true;
    }
    return false;
  }

  virtual void TearDown() override {
    audio_port_ = nullptr;
    audio_provider_ = nullptr;
    BluetoothAudioProviderFactoryAidl::TearDown();
  }
};

/**
 * Test whether each provider of type
 * SessionType::LE_AUDIO_HARDWARE_OFFLOAD_ENCODING_DATAPATH can be started and
 * stopped
 */
TEST_P(BluetoothAudioProviderLeAudioInputHardwareAidl,
       OpenLeAudioInputHardwareProvider) {}

/**
 * Test whether each provider of type
 * SessionType::LE_AUDIO_HARDWARE_OFFLOAD_ENCODING_DATAPATH can be started and
 * stopped with Unicast hardware encoding config
 */
TEST_P(BluetoothAudioProviderLeAudioInputHardwareAidl,
       StartAndEndLeAudioInputSessionWithPossibleUnicastConfig) {
  if (!IsOffloadInputSupported()) {
    return;
  }

  auto lc3_codec_configs =
      GetUnicastLc3SupportedList(true /* decoding */, true /* supported */);
  LeAudioConfiguration le_audio_config = {
      .codecType = CodecType::LC3,
      .peerDelayUs = 0,
  };

  for (auto& lc3_config : lc3_codec_configs) {
    le_audio_config.leAudioCodecConfig
        .set<LeAudioCodecConfiguration::lc3Config>(lc3_config);
    DataMQDesc mq_desc;
    auto aidl_retval = audio_provider_->startSession(
        audio_port_, AudioConfiguration(le_audio_config), latency_modes,
        &mq_desc);

    ASSERT_TRUE(aidl_retval.isOk());
    EXPECT_TRUE(audio_provider_->endSession().isOk());
  }
}

/**
 * Test whether each provider of type
 * SessionType::LE_AUDIO_HARDWARE_OFFLOAD_ENCODING_DATAPATH can be started and
 * stopped with Unicast hardware encoding config
 *
 * Disabled since offload codec checking is not ready
 */
TEST_P(BluetoothAudioProviderLeAudioInputHardwareAidl,
       DISABLED_StartAndEndLeAudioInputSessionWithInvalidAudioConfiguration) {
  if (!IsOffloadInputSupported()) {
    return;
  }

  auto lc3_codec_configs =
      GetUnicastLc3SupportedList(true /* decoding */, false /* supported */);
  LeAudioConfiguration le_audio_config = {
      .codecType = CodecType::LC3,
      .peerDelayUs = 0,
  };

  for (auto& lc3_config : lc3_codec_configs) {
    le_audio_config.leAudioCodecConfig
        .set<LeAudioCodecConfiguration::lc3Config>(lc3_config);

    DataMQDesc mq_desc;
    auto aidl_retval = audio_provider_->startSession(
        audio_port_, AudioConfiguration(le_audio_config), latency_modes,
        &mq_desc);

    // AIDL call should fail on invalid codec
    ASSERT_FALSE(aidl_retval.isOk());
    EXPECT_TRUE(audio_provider_->endSession().isOk());
  }
}

/**
 * openProvider LE_AUDIO_BROADCAST_SOFTWARE_ENCODING_DATAPATH
 */
class BluetoothAudioProviderLeAudioBroadcastSoftwareAidl
    : public BluetoothAudioProviderFactoryAidl {
 public:
  virtual void SetUp() override {
    BluetoothAudioProviderFactoryAidl::SetUp();
    GetProviderCapabilitiesHelper(
        SessionType::LE_AUDIO_BROADCAST_SOFTWARE_ENCODING_DATAPATH);
    OpenProviderHelper(
        SessionType::LE_AUDIO_BROADCAST_SOFTWARE_ENCODING_DATAPATH);
    ASSERT_NE(audio_provider_, nullptr);
  }

  virtual void TearDown() override {
    audio_port_ = nullptr;
    audio_provider_ = nullptr;
    BluetoothAudioProviderFactoryAidl::TearDown();
  }

  static constexpr int32_t le_audio_output_sample_rates_[] = {
      0, 8000, 16000, 24000, 32000, 44100, 48000,
  };
  static constexpr int8_t le_audio_output_bits_per_samples_[] = {0, 16, 24};
  static constexpr ChannelMode le_audio_output_channel_modes_[] = {
      ChannelMode::UNKNOWN, ChannelMode::MONO, ChannelMode::STEREO};
  static constexpr int32_t le_audio_output_data_interval_us_[] = {
      0 /* Invalid */, 10000 /* Valid 10ms */};
};

/**
 * Test whether each provider of type
 * SessionType::LE_AUDIO_BROADCAST_SOFTWARE_ENCODING_DATAPATH can be started and
 * stopped
 */
TEST_P(BluetoothAudioProviderLeAudioBroadcastSoftwareAidl,
       OpenLeAudioOutputSoftwareProvider) {}

/**
 * Test whether each provider of type
 * SessionType::LE_AUDIO_BROADCAST_SOFTWARE_ENCODING_DATAPATH can be started and
 * stopped with different PCM config
 */
TEST_P(BluetoothAudioProviderLeAudioBroadcastSoftwareAidl,
       StartAndEndLeAudioOutputSessionWithPossiblePcmConfig) {
  for (auto sample_rate : le_audio_output_sample_rates_) {
    for (auto bits_per_sample : le_audio_output_bits_per_samples_) {
      for (auto channel_mode : le_audio_output_channel_modes_) {
        for (auto data_interval_us : le_audio_output_data_interval_us_) {
          PcmConfiguration pcm_config{
              .sampleRateHz = sample_rate,
              .channelMode = channel_mode,
              .bitsPerSample = bits_per_sample,
              .dataIntervalUs = data_interval_us,
          };
          bool is_codec_config_valid =
              IsPcmConfigSupported(pcm_config) && pcm_config.dataIntervalUs > 0;
          DataMQDesc mq_desc;
          auto aidl_retval = audio_provider_->startSession(
              audio_port_, AudioConfiguration(pcm_config), latency_modes,
              &mq_desc);
          DataMQ data_mq(mq_desc);

          EXPECT_EQ(aidl_retval.isOk(), is_codec_config_valid);
          if (is_codec_config_valid) {
            EXPECT_TRUE(data_mq.isValid());
          }
          EXPECT_TRUE(audio_provider_->endSession().isOk());
        }
      }
    }
  }
}

/**
 * openProvider LE_AUDIO_BROADCAST_HARDWARE_OFFLOAD_ENCODING_DATAPATH
 */
class BluetoothAudioProviderLeAudioBroadcastHardwareAidl
    : public BluetoothAudioProviderFactoryAidl {
 public:
  virtual void SetUp() override {
    BluetoothAudioProviderFactoryAidl::SetUp();
    GetProviderCapabilitiesHelper(
        SessionType::LE_AUDIO_BROADCAST_HARDWARE_OFFLOAD_ENCODING_DATAPATH);
    OpenProviderHelper(
        SessionType::LE_AUDIO_BROADCAST_HARDWARE_OFFLOAD_ENCODING_DATAPATH);
    ASSERT_TRUE(temp_provider_capabilities_.empty() ||
                audio_provider_ != nullptr);
  }

  virtual void TearDown() override {
    audio_port_ = nullptr;
    audio_provider_ = nullptr;
    BluetoothAudioProviderFactoryAidl::TearDown();
  }

  bool IsBroadcastOffloadSupported() {
    for (auto& capability : temp_provider_capabilities_) {
      if (capability.getTag() != AudioCapabilities::leAudioCapabilities) {
        continue;
      }
      auto& le_audio_capability =
          capability.get<AudioCapabilities::leAudioCapabilities>();
      if (le_audio_capability.broadcastCapability.codecType !=
          CodecType::UNKNOWN)
        return true;
    }
    return false;
  }

  std::vector<Lc3Configuration> GetBroadcastLc3SupportedList(bool supported) {
    std::vector<Lc3Configuration> le_audio_codec_configs;
    if (!supported) {
      Lc3Configuration lc3_config{.pcmBitDepth = 0, .samplingFrequencyHz = 0};
      le_audio_codec_configs.push_back(lc3_config);
      return le_audio_codec_configs;
    }

    // There might be more than one LeAudioCodecCapabilitiesSetting
    std::vector<Lc3Capabilities> lc3_capabilities;
    for (auto& capability : temp_provider_capabilities_) {
      if (capability.getTag() != AudioCapabilities::leAudioCapabilities) {
        continue;
      }
      auto& le_audio_capability =
          capability.get<AudioCapabilities::leAudioCapabilities>();
      auto& broadcast_capability = le_audio_capability.broadcastCapability;
      if (broadcast_capability.codecType != CodecType::LC3) {
        continue;
      }
      auto& lc3_capability = broadcast_capability.leAudioCodecCapabilities.get<
          BroadcastCapability::LeAudioCodecCapabilities::lc3Capabilities>();
      for (int idx = 0; idx < lc3_capability->size(); idx++)
        lc3_capabilities.push_back(*lc3_capability->at(idx));
    }

    // Combine those parameters into one list of LeAudioCodecConfiguration
    // This seems horrible, but usually each Lc3Capability only contains a
    // single Lc3Configuration, which means every array has a length of 1.
    for (auto& lc3_capability : lc3_capabilities) {
      for (int32_t samplingFrequencyHz : lc3_capability.samplingFrequencyHz) {
        for (int32_t frameDurationUs : lc3_capability.frameDurationUs) {
          for (int32_t octetsPerFrame : lc3_capability.octetsPerFrame) {
            Lc3Configuration lc3_config = {
                .samplingFrequencyHz = samplingFrequencyHz,
                .frameDurationUs = frameDurationUs,
                .octetsPerFrame = octetsPerFrame,
            };
            le_audio_codec_configs.push_back(lc3_config);
          }
        }
      }
    }

    return le_audio_codec_configs;
  }

  LeAudioCodecCapabilitiesSetting temp_le_audio_capabilities_;
};

/**
 * Test whether each provider of type
 * SessionType::LE_AUDIO_BROADCAST_HARDWARE_OFFLOAD_ENCODING_DATAPATH can be
 * started and stopped
 */
TEST_P(BluetoothAudioProviderLeAudioBroadcastHardwareAidl,
       OpenLeAudioOutputHardwareProvider) {}

/**
 * Test whether each provider of type
 * SessionType::LE_AUDIO_BROADCAST_HARDWARE_OFFLOAD_ENCODING_DATAPATH can be
 * started and stopped with broadcast hardware encoding config
 */
TEST_P(BluetoothAudioProviderLeAudioBroadcastHardwareAidl,
       StartAndEndLeAudioBroadcastSessionWithPossibleBroadcastConfig) {
  if (!IsBroadcastOffloadSupported()) {
    return;
  }

  auto lc3_codec_configs = GetBroadcastLc3SupportedList(true /* supported */);
  LeAudioBroadcastConfiguration le_audio_broadcast_config = {
      .codecType = CodecType::LC3,
      .streamMap = {},
  };

  for (auto& lc3_config : lc3_codec_configs) {
    le_audio_broadcast_config.streamMap[0]
        .leAudioCodecConfig.set<LeAudioCodecConfiguration::lc3Config>(
            lc3_config);
    DataMQDesc mq_desc;
    auto aidl_retval = audio_provider_->startSession(
        audio_port_, AudioConfiguration(le_audio_broadcast_config),
        latency_modes, &mq_desc);

    ASSERT_TRUE(aidl_retval.isOk());
    EXPECT_TRUE(audio_provider_->endSession().isOk());
  }
}

/**
 * Test whether each provider of type
 * SessionType::LE_AUDIO_BROADCAST_HARDWARE_OFFLOAD_ENCODING_DATAPATH can be
 * started and stopped with Broadcast hardware encoding config
 *
 * Disabled since offload codec checking is not ready
 */
TEST_P(
    BluetoothAudioProviderLeAudioBroadcastHardwareAidl,
    DISABLED_StartAndEndLeAudioBroadcastSessionWithInvalidAudioConfiguration) {
  if (!IsBroadcastOffloadSupported()) {
    return;
  }

  auto lc3_codec_configs = GetBroadcastLc3SupportedList(false /* supported */);
  LeAudioBroadcastConfiguration le_audio_broadcast_config = {
      .codecType = CodecType::LC3,
      .streamMap = {},
  };

  for (auto& lc3_config : lc3_codec_configs) {
    le_audio_broadcast_config.streamMap[0]
        .leAudioCodecConfig.set<LeAudioCodecConfiguration::lc3Config>(
            lc3_config);
    DataMQDesc mq_desc;
    auto aidl_retval = audio_provider_->startSession(
        audio_port_, AudioConfiguration(le_audio_broadcast_config),
        latency_modes, &mq_desc);

    // AIDL call should fail on invalid codec
    ASSERT_FALSE(aidl_retval.isOk());
    EXPECT_TRUE(audio_provider_->endSession().isOk());
  }
}

/**
 * openProvider A2DP_SOFTWARE_DECODING_DATAPATH
 */
class BluetoothAudioProviderA2dpDecodingSoftwareAidl
    : public BluetoothAudioProviderFactoryAidl {
 public:
  virtual void SetUp() override {
    BluetoothAudioProviderFactoryAidl::SetUp();
    GetProviderCapabilitiesHelper(SessionType::A2DP_SOFTWARE_DECODING_DATAPATH);
    OpenProviderHelper(SessionType::A2DP_SOFTWARE_DECODING_DATAPATH);
    ASSERT_TRUE(temp_provider_capabilities_.empty() ||
                audio_provider_ != nullptr);
  }

  virtual void TearDown() override {
    audio_port_ = nullptr;
    audio_provider_ = nullptr;
    BluetoothAudioProviderFactoryAidl::TearDown();
  }
};

/**
 * Test whether we can open a provider of type
 */
TEST_P(BluetoothAudioProviderA2dpDecodingSoftwareAidl,
       OpenA2dpDecodingSoftwareProvider) {}

/**
 * Test whether each provider of type
 * SessionType::A2DP_SOFTWARE_DECODING_DATAPATH can be started and stopped with
 * different PCM config
 */
TEST_P(BluetoothAudioProviderA2dpDecodingSoftwareAidl,
       StartAndEndA2dpDecodingSoftwareSessionWithPossiblePcmConfig) {
  for (auto sample_rate : a2dp_sample_rates) {
    for (auto bits_per_sample : a2dp_bits_per_samples) {
      for (auto channel_mode : a2dp_channel_modes) {
        PcmConfiguration pcm_config{
            .sampleRateHz = sample_rate,
            .channelMode = channel_mode,
            .bitsPerSample = bits_per_sample,
        };
        bool is_codec_config_valid = IsPcmConfigSupported(pcm_config);
        DataMQDesc mq_desc;
        auto aidl_retval = audio_provider_->startSession(
            audio_port_, AudioConfiguration(pcm_config), latency_modes,
            &mq_desc);
        DataMQ data_mq(mq_desc);

        EXPECT_EQ(aidl_retval.isOk(), is_codec_config_valid);
        if (is_codec_config_valid) {
          EXPECT_TRUE(data_mq.isValid());
        }
        EXPECT_TRUE(audio_provider_->endSession().isOk());
      }
    }
  }
}

/**
 * openProvider A2DP_HARDWARE_OFFLOAD_DECODING_DATAPATH
 */
class BluetoothAudioProviderA2dpDecodingHardwareAidl
    : public BluetoothAudioProviderFactoryAidl {
 public:
  virtual void SetUp() override {
    BluetoothAudioProviderFactoryAidl::SetUp();
    GetProviderCapabilitiesHelper(
        SessionType::A2DP_HARDWARE_OFFLOAD_DECODING_DATAPATH);
    OpenProviderHelper(SessionType::A2DP_HARDWARE_OFFLOAD_DECODING_DATAPATH);
    ASSERT_TRUE(temp_provider_capabilities_.empty() ||
                audio_provider_ != nullptr);
  }

  virtual void TearDown() override {
    audio_port_ = nullptr;
    audio_provider_ = nullptr;
    BluetoothAudioProviderFactoryAidl::TearDown();
  }

  bool IsOffloadSupported() { return (temp_provider_capabilities_.size() > 0); }
};

/**
 * Test whether we can open a provider of type
 */
TEST_P(BluetoothAudioProviderA2dpDecodingHardwareAidl,
       OpenA2dpDecodingHardwareProvider) {}

/**
 * Test whether each provider of type
 * SessionType::A2DP_HARDWARE_DECODING_DATAPATH can be started and stopped with
 * SBC hardware encoding config
 */
TEST_P(BluetoothAudioProviderA2dpDecodingHardwareAidl,
       StartAndEndA2dpSbcDecodingHardwareSession) {
  if (!IsOffloadSupported()) {
    return;
  }

  CodecConfiguration codec_config = {
      .codecType = CodecType::SBC,
      .encodedAudioBitrate = 328000,
      .peerMtu = 1005,
      .isScmstEnabled = false,
  };
  auto sbc_codec_specifics = GetSbcCodecSpecificSupportedList(true);

  for (auto& codec_specific : sbc_codec_specifics) {
    copy_codec_specific(codec_config.config, codec_specific);
    DataMQDesc mq_desc;
    auto aidl_retval = audio_provider_->startSession(
        audio_port_, AudioConfiguration(codec_config), latency_modes, &mq_desc);

    ASSERT_TRUE(aidl_retval.isOk());
    EXPECT_TRUE(audio_provider_->endSession().isOk());
  }
}

/**
 * Test whether each provider of type
 * SessionType::A2DP_HARDWARE_DECODING_DATAPATH can be started and stopped with
 * AAC hardware encoding config
 */
TEST_P(BluetoothAudioProviderA2dpDecodingHardwareAidl,
       StartAndEndA2dpAacDecodingHardwareSession) {
  if (!IsOffloadSupported()) {
    return;
  }

  CodecConfiguration codec_config = {
      .codecType = CodecType::AAC,
      .encodedAudioBitrate = 320000,
      .peerMtu = 1005,
      .isScmstEnabled = false,
  };
  auto aac_codec_specifics = GetAacCodecSpecificSupportedList(true);

  for (auto& codec_specific : aac_codec_specifics) {
    copy_codec_specific(codec_config.config, codec_specific);
    DataMQDesc mq_desc;
    auto aidl_retval = audio_provider_->startSession(
        audio_port_, AudioConfiguration(codec_config), latency_modes, &mq_desc);

    ASSERT_TRUE(aidl_retval.isOk());
    EXPECT_TRUE(audio_provider_->endSession().isOk());
  }
}

/**
 * Test whether each provider of type
 * SessionType::A2DP_HARDWARE_DECODING_DATAPATH can be started and stopped with
 * LDAC hardware encoding config
 */
TEST_P(BluetoothAudioProviderA2dpDecodingHardwareAidl,
       StartAndEndA2dpLdacDecodingHardwareSession) {
  if (!IsOffloadSupported()) {
    return;
  }

  CodecConfiguration codec_config = {
      .codecType = CodecType::LDAC,
      .encodedAudioBitrate = 990000,
      .peerMtu = 1005,
      .isScmstEnabled = false,
  };
  auto ldac_codec_specifics = GetLdacCodecSpecificSupportedList(true);

  for (auto& codec_specific : ldac_codec_specifics) {
    copy_codec_specific(codec_config.config, codec_specific);
    DataMQDesc mq_desc;
    auto aidl_retval = audio_provider_->startSession(
        audio_port_, AudioConfiguration(codec_config), latency_modes, &mq_desc);

    ASSERT_TRUE(aidl_retval.isOk());
    EXPECT_TRUE(audio_provider_->endSession().isOk());
  }
}

/**
 * Test whether each provider of type
 * SessionType::A2DP_HARDWARE_DECODING_DATAPATH can be started and stopped with
 * Opus hardware encoding config
 */
TEST_P(BluetoothAudioProviderA2dpDecodingHardwareAidl,
       StartAndEndA2dpOpusDecodingHardwareSession) {
  if (!IsOffloadSupported()) {
    return;
  }

  CodecConfiguration codec_config = {
      .codecType = CodecType::OPUS,
      .encodedAudioBitrate = 990000,
      .peerMtu = 1005,
      .isScmstEnabled = false,
  };
  auto opus_codec_specifics = GetOpusCodecSpecificSupportedList(true);

  for (auto& codec_specific : opus_codec_specifics) {
    copy_codec_specific(codec_config.config, codec_specific);
    DataMQDesc mq_desc;
    auto aidl_retval = audio_provider_->startSession(
        audio_port_, AudioConfiguration(codec_config), latency_modes, &mq_desc);

    ASSERT_TRUE(aidl_retval.isOk());
    EXPECT_TRUE(audio_provider_->endSession().isOk());
  }
}

/**
 * Test whether each provider of type
 * SessionType::A2DP_HARDWARE_DECODING_DATAPATH can be started and stopped with
 * AptX hardware encoding config
 */
TEST_P(BluetoothAudioProviderA2dpDecodingHardwareAidl,
       StartAndEndA2dpAptxDecodingHardwareSession) {
  if (!IsOffloadSupported()) {
    return;
  }

  for (auto codec_type : {CodecType::APTX, CodecType::APTX_HD}) {
    CodecConfiguration codec_config = {
        .codecType = codec_type,
        .encodedAudioBitrate =
            (codec_type == CodecType::APTX ? 352000 : 576000),
        .peerMtu = 1005,
        .isScmstEnabled = false,
    };

    auto aptx_codec_specifics = GetAptxCodecSpecificSupportedList(
        (codec_type == CodecType::APTX_HD ? true : false), true);

    for (auto& codec_specific : aptx_codec_specifics) {
      copy_codec_specific(codec_config.config, codec_specific);
      DataMQDesc mq_desc;
      auto aidl_retval = audio_provider_->startSession(
          audio_port_, AudioConfiguration(codec_config), latency_modes,
          &mq_desc);

      ASSERT_TRUE(aidl_retval.isOk());
      EXPECT_TRUE(audio_provider_->endSession().isOk());
    }
  }
}

/**
 * Test whether each provider of type
 * SessionType::A2DP_HARDWARE_DECODING_DATAPATH can be started and stopped with
 * an invalid codec config
 */
TEST_P(BluetoothAudioProviderA2dpDecodingHardwareAidl,
       StartAndEndA2dpDecodingHardwareSessionInvalidCodecConfig) {
  if (!IsOffloadSupported()) {
    return;
  }
  ASSERT_NE(audio_provider_, nullptr);

  std::vector<CodecConfiguration::CodecSpecific> codec_specifics;
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
      case CodecType::OPUS:
        codec_specifics = GetOpusCodecSpecificSupportedList(false);
        continue;
      case CodecType::APTX_ADAPTIVE:
      case CodecType::LC3:
      case CodecType::VENDOR:
      case CodecType::UNKNOWN:
        codec_specifics.clear();
        break;
    }
    if (codec_specifics.empty()) {
      continue;
    }

    CodecConfiguration codec_config = {
        .codecType = codec_type,
        .encodedAudioBitrate = 328000,
        .peerMtu = 1005,
        .isScmstEnabled = false,
    };
    for (auto codec_specific : codec_specifics) {
      copy_codec_specific(codec_config.config, codec_specific);
      DataMQDesc mq_desc;
      auto aidl_retval = audio_provider_->startSession(
          audio_port_, AudioConfiguration(codec_config), latency_modes,
          &mq_desc);

      // AIDL call should fail on invalid codec
      ASSERT_FALSE(aidl_retval.isOk());
      EXPECT_TRUE(audio_provider_->endSession().isOk());
    }
  }
}

GTEST_ALLOW_UNINSTANTIATED_PARAMETERIZED_TEST(
    BluetoothAudioProviderFactoryAidl);
INSTANTIATE_TEST_SUITE_P(PerInstance, BluetoothAudioProviderFactoryAidl,
                         testing::ValuesIn(android::getAidlHalInstanceNames(
                             IBluetoothAudioProviderFactory::descriptor)),
                         android::PrintInstanceNameToString);

GTEST_ALLOW_UNINSTANTIATED_PARAMETERIZED_TEST(
    BluetoothAudioProviderA2dpEncodingSoftwareAidl);
INSTANTIATE_TEST_SUITE_P(PerInstance,
                         BluetoothAudioProviderA2dpEncodingSoftwareAidl,
                         testing::ValuesIn(android::getAidlHalInstanceNames(
                             IBluetoothAudioProviderFactory::descriptor)),
                         android::PrintInstanceNameToString);

GTEST_ALLOW_UNINSTANTIATED_PARAMETERIZED_TEST(
    BluetoothAudioProviderA2dpEncodingHardwareAidl);
INSTANTIATE_TEST_SUITE_P(PerInstance,
                         BluetoothAudioProviderA2dpEncodingHardwareAidl,
                         testing::ValuesIn(android::getAidlHalInstanceNames(
                             IBluetoothAudioProviderFactory::descriptor)),
                         android::PrintInstanceNameToString);

GTEST_ALLOW_UNINSTANTIATED_PARAMETERIZED_TEST(
    BluetoothAudioProviderHearingAidSoftwareAidl);
INSTANTIATE_TEST_SUITE_P(PerInstance,
                         BluetoothAudioProviderHearingAidSoftwareAidl,
                         testing::ValuesIn(android::getAidlHalInstanceNames(
                             IBluetoothAudioProviderFactory::descriptor)),
                         android::PrintInstanceNameToString);

GTEST_ALLOW_UNINSTANTIATED_PARAMETERIZED_TEST(
    BluetoothAudioProviderLeAudioOutputSoftwareAidl);
INSTANTIATE_TEST_SUITE_P(PerInstance,
                         BluetoothAudioProviderLeAudioOutputSoftwareAidl,
                         testing::ValuesIn(android::getAidlHalInstanceNames(
                             IBluetoothAudioProviderFactory::descriptor)),
                         android::PrintInstanceNameToString);

GTEST_ALLOW_UNINSTANTIATED_PARAMETERIZED_TEST(
    BluetoothAudioProviderLeAudioInputSoftwareAidl);
INSTANTIATE_TEST_SUITE_P(PerInstance,
                         BluetoothAudioProviderLeAudioInputSoftwareAidl,
                         testing::ValuesIn(android::getAidlHalInstanceNames(
                             IBluetoothAudioProviderFactory::descriptor)),
                         android::PrintInstanceNameToString);

GTEST_ALLOW_UNINSTANTIATED_PARAMETERIZED_TEST(
    BluetoothAudioProviderLeAudioOutputHardwareAidl);
INSTANTIATE_TEST_SUITE_P(PerInstance,
                         BluetoothAudioProviderLeAudioOutputHardwareAidl,
                         testing::ValuesIn(android::getAidlHalInstanceNames(
                             IBluetoothAudioProviderFactory::descriptor)),
                         android::PrintInstanceNameToString);

GTEST_ALLOW_UNINSTANTIATED_PARAMETERIZED_TEST(
    BluetoothAudioProviderLeAudioInputHardwareAidl);
INSTANTIATE_TEST_SUITE_P(PerInstance,
                         BluetoothAudioProviderLeAudioInputHardwareAidl,
                         testing::ValuesIn(android::getAidlHalInstanceNames(
                             IBluetoothAudioProviderFactory::descriptor)),
                         android::PrintInstanceNameToString);

GTEST_ALLOW_UNINSTANTIATED_PARAMETERIZED_TEST(
    BluetoothAudioProviderLeAudioBroadcastSoftwareAidl);
INSTANTIATE_TEST_SUITE_P(PerInstance,
                         BluetoothAudioProviderLeAudioBroadcastSoftwareAidl,
                         testing::ValuesIn(android::getAidlHalInstanceNames(
                             IBluetoothAudioProviderFactory::descriptor)),
                         android::PrintInstanceNameToString);

GTEST_ALLOW_UNINSTANTIATED_PARAMETERIZED_TEST(
    BluetoothAudioProviderLeAudioBroadcastHardwareAidl);
INSTANTIATE_TEST_SUITE_P(PerInstance,
                         BluetoothAudioProviderLeAudioBroadcastHardwareAidl,
                         testing::ValuesIn(android::getAidlHalInstanceNames(
                             IBluetoothAudioProviderFactory::descriptor)),
                         android::PrintInstanceNameToString);

GTEST_ALLOW_UNINSTANTIATED_PARAMETERIZED_TEST(
    BluetoothAudioProviderA2dpDecodingSoftwareAidl);
INSTANTIATE_TEST_SUITE_P(PerInstance,
                         BluetoothAudioProviderA2dpDecodingSoftwareAidl,
                         testing::ValuesIn(android::getAidlHalInstanceNames(
                             IBluetoothAudioProviderFactory::descriptor)),
                         android::PrintInstanceNameToString);

GTEST_ALLOW_UNINSTANTIATED_PARAMETERIZED_TEST(
    BluetoothAudioProviderA2dpDecodingHardwareAidl);
INSTANTIATE_TEST_SUITE_P(PerInstance,
                         BluetoothAudioProviderA2dpDecodingHardwareAidl,
                         testing::ValuesIn(android::getAidlHalInstanceNames(
                             IBluetoothAudioProviderFactory::descriptor)),
                         android::PrintInstanceNameToString);

int main(int argc, char** argv) {
  ::testing::InitGoogleTest(&argc, argv);
  ABinderProcess_setThreadPoolMaxThreadCount(1);
  ABinderProcess_startThreadPool();
  return RUN_ALL_TESTS();
}
