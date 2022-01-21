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

#define LOG_TAG "BTAudioProviderSession_2_2"

#include "BluetoothAudioSession_2_2.h"

#include <android-base/logging.h>
#include <android-base/stringprintf.h>
#include <android/hardware/bluetooth/audio/2.2/IBluetoothAudioPort.h>

#include "../aidl_session/HidlToAidlMiddleware_2_0.h"
#include "../aidl_session/HidlToAidlMiddleware_2_2.h"

namespace android {
namespace bluetooth {
namespace audio {

using ::aidl::android::hardware::bluetooth::audio::HidlToAidlMiddleware_2_0;
using ::aidl::android::hardware::bluetooth::audio::HidlToAidlMiddleware_2_2;
using ::android::hardware::audio::common::V5_0::AudioContentType;
using ::android::hardware::audio::common::V5_0::AudioSource;
using ::android::hardware::audio::common::V5_0::AudioUsage;
using ::android::hardware::audio::common::V5_0::PlaybackTrackMetadata;
using ::android::hardware::audio::common::V5_0::RecordTrackMetadata;
using ::android::hardware::audio::common::V5_0::SinkMetadata;
using ::android::hardware::audio::common::V5_0::SourceMetadata;
using ::android::hardware::bluetooth::audio::V2_0::BitsPerSample;
using ::android::hardware::bluetooth::audio::V2_0::ChannelMode;
using ::android::hardware::bluetooth::audio::V2_2::LeAudioConfiguration;
using ::android::hardware::bluetooth::audio::V2_2::LeAudioMode;
using PcmParameters_2_1 =
    ::android::hardware::bluetooth::audio::V2_1::PcmParameters;
using SampleRate_2_1 = ::android::hardware::bluetooth::audio::V2_1::SampleRate;

using SessionType_2_1 =
    ::android::hardware::bluetooth::audio::V2_1::SessionType;
using SessionType_2_0 =
    ::android::hardware::bluetooth::audio::V2_0::SessionType;

using AudioConfiguration_2_1 =
    ::android::hardware::bluetooth::audio::V2_1::AudioConfiguration;

static constexpr PcmParameters_2_1 kInvalidPcmParameters = {
    .sampleRate = SampleRate_2_1::RATE_UNKNOWN,
    .channelMode = ChannelMode::UNKNOWN,
    .bitsPerSample = BitsPerSample::BITS_UNKNOWN,
    .dataIntervalUs = 0,
};

static LeAudioConfiguration kInvalidLeAudioConfig = {
    .mode = LeAudioMode::UNKNOWN,
    .config = {},
};

::android::hardware::bluetooth::audio::V2_2::AudioConfiguration
    BluetoothAudioSession_2_2::invalidSoftwareAudioConfiguration = {};
::android::hardware::bluetooth::audio::V2_2::AudioConfiguration
    BluetoothAudioSession_2_2::invalidOffloadAudioConfiguration = {};
::android::hardware::bluetooth::audio::V2_2::AudioConfiguration
    BluetoothAudioSession_2_2::invalidLeOffloadAudioConfiguration = {};

using IBluetoothAudioPort_2_2 =
    ::android::hardware::bluetooth::audio::V2_2::IBluetoothAudioPort;

namespace {
bool is_2_0_session_type(
    const ::android::hardware::bluetooth::audio::V2_1::SessionType&
        session_type) {
  if (session_type == SessionType_2_1::A2DP_SOFTWARE_ENCODING_DATAPATH ||
      session_type == SessionType_2_1::A2DP_HARDWARE_OFFLOAD_DATAPATH ||
      session_type == SessionType_2_1::HEARING_AID_SOFTWARE_ENCODING_DATAPATH) {
    return true;
  } else {
    return false;
  }
}
}  // namespace

BluetoothAudioSession_2_2::BluetoothAudioSession_2_2(
    const ::android::hardware::bluetooth::audio::V2_1::SessionType&
        session_type)
    : audio_session(BluetoothAudioSessionInstance::GetSessionInstance(
          static_cast<SessionType_2_0>(session_type))),
      audio_session_2_1(
          BluetoothAudioSessionInstance_2_1::GetSessionInstance(session_type)) {
  if (is_2_0_session_type(session_type)) {
    session_type_2_1_ = (SessionType_2_1::UNKNOWN);
  } else {
    session_type_2_1_ = (session_type);
  }
  raw_session_type_ = session_type;
  invalidSoftwareAudioConfiguration.pcmConfig(kInvalidPcmParameters);
  invalidOffloadAudioConfiguration.codecConfig(
      audio_session->kInvalidCodecConfiguration);
  invalidLeOffloadAudioConfiguration.leAudioConfig(kInvalidLeAudioConfig);
}

bool BluetoothAudioSession_2_2::IsSessionReady() {
  if (HidlToAidlMiddleware_2_0::IsAidlAvailable())
    return HidlToAidlMiddleware_2_2::IsSessionReady(raw_session_type_);
  if (session_type_2_1_ !=
          SessionType_2_1::LE_AUDIO_HARDWARE_OFFLOAD_ENCODING_DATAPATH &&
      session_type_2_1_ !=
          SessionType_2_1::LE_AUDIO_HARDWARE_OFFLOAD_DECODING_DATAPATH) {
    return audio_session->IsSessionReady();
  }

  std::lock_guard<std::recursive_mutex> guard(audio_session->mutex_);
  return audio_session->stack_iface_ != nullptr;
}

std::shared_ptr<BluetoothAudioSession>
BluetoothAudioSession_2_2::GetAudioSession() {
  return audio_session;
}
std::shared_ptr<BluetoothAudioSession_2_1>
BluetoothAudioSession_2_2::GetAudioSession_2_1() {
  return audio_session_2_1;
}

void BluetoothAudioSession_2_2::UpdateTracksMetadata(
    const struct source_metadata* source_metadata) {
  if (HidlToAidlMiddleware_2_0::IsAidlAvailable())
    return HidlToAidlMiddleware_2_2::UpdateTracksMetadata(raw_session_type_,
                                                          source_metadata);
  std::lock_guard<std::recursive_mutex> guard(audio_session->mutex_);
  if (!IsSessionReady()) {
    LOG(DEBUG) << __func__ << " - SessionType=" << toString(session_type_2_1_)
               << " has NO session";
    return;
  }

  ssize_t track_count = source_metadata->track_count;
  LOG(INFO) << __func__ << " - SessionType=" << toString(session_type_2_1_)
            << ", " << track_count << " track(s)";

  if (session_type_2_1_ == SessionType_2_1::UNKNOWN) {
    audio_session->UpdateTracksMetadata(source_metadata);
    return;
  }

  struct playback_track_metadata* track = source_metadata->tracks;
  SourceMetadata sourceMetadata;
  PlaybackTrackMetadata* halMetadata;

  sourceMetadata.tracks.resize(track_count);
  halMetadata = sourceMetadata.tracks.data();
  while (track_count && track) {
    halMetadata->usage = static_cast<AudioUsage>(track->usage);
    halMetadata->contentType =
        static_cast<AudioContentType>(track->content_type);
    halMetadata->gain = track->gain;
    LOG(VERBOSE) << __func__ << " - SessionType=" << toString(session_type_2_1_)
                 << ", usage=" << toString(halMetadata->usage)
                 << ", content=" << toString(halMetadata->contentType)
                 << ", gain=" << halMetadata->gain;
    --track_count;
    ++track;
    ++halMetadata;
  }
  auto hal_retval = audio_session->stack_iface_->updateMetadata(sourceMetadata);
  if (!hal_retval.isOk()) {
    LOG(WARNING) << __func__ << " - IBluetoothAudioPort SessionType="
                 << toString(session_type_2_1_) << " failed";
  }
}

void BluetoothAudioSession_2_2::UpdateSinkMetadata(
    const struct sink_metadata* sink_metadata) {
  if (HidlToAidlMiddleware_2_0::IsAidlAvailable())
    return HidlToAidlMiddleware_2_2::UpdateSinkMetadata(raw_session_type_,
                                                        sink_metadata);
  std::lock_guard<std::recursive_mutex> guard(audio_session->mutex_);
  if (!IsSessionReady()) {
    LOG(DEBUG) << __func__ << " - SessionType=" << toString(session_type_2_1_)
               << " has NO session";
    return;
  }

  ssize_t track_count = sink_metadata->track_count;
  LOG(INFO) << __func__ << " - SessionType=" << toString(session_type_2_1_)
            << ", " << track_count << " track(s)";
  if (session_type_2_1_ == SessionType_2_1::A2DP_SOFTWARE_ENCODING_DATAPATH ||
      session_type_2_1_ == SessionType_2_1::A2DP_HARDWARE_OFFLOAD_DATAPATH) {
    return;
  }

  struct record_track_metadata* track = sink_metadata->tracks;
  SinkMetadata sinkMetadata;
  RecordTrackMetadata* halMetadata;

  sinkMetadata.tracks.resize(track_count);
  halMetadata = sinkMetadata.tracks.data();
  while (track_count && track) {
    halMetadata->source = static_cast<AudioSource>(track->source);
    halMetadata->gain = track->gain;
    // halMetadata->destination leave unspecified
    LOG(INFO) << __func__
              << " - SessionType=" << toString(GetAudioSession()->session_type_)
              << ", source=" << track->source
              << ", dest_device=" << track->dest_device
              << ", gain=" << track->gain
              << ", dest_device_address=" << track->dest_device_address;
    --track_count;
    ++track;
    ++halMetadata;
  }

  /* This is called just for 2.2 sessions, so it's safe to do this casting*/
  IBluetoothAudioPort_2_2* stack_iface_2_2_ =
      static_cast<IBluetoothAudioPort_2_2*>(audio_session->stack_iface_.get());
  auto hal_retval = stack_iface_2_2_->updateSinkMetadata(sinkMetadata);
  if (!hal_retval.isOk()) {
    LOG(WARNING) << __func__ << " - IBluetoothAudioPort SessionType="
                 << toString(session_type_2_1_) << " failed";
  }
}

// The control function is for the bluetooth_audio module to get the current
// AudioConfiguration
const ::android::hardware::bluetooth::audio::V2_2::AudioConfiguration
BluetoothAudioSession_2_2::GetAudioConfig() {
  if (HidlToAidlMiddleware_2_0::IsAidlAvailable())
    return HidlToAidlMiddleware_2_2::GetAudioConfig(raw_session_type_);
  std::lock_guard<std::recursive_mutex> guard(audio_session->mutex_);
  if (IsSessionReady()) {
    auto audio_config_discriminator = audio_config_2_2_.getDiscriminator();
    // If session is unknown it means it should be 2.0 type
    if (session_type_2_1_ != SessionType_2_1::UNKNOWN) {
      if ((audio_config_discriminator ==
               ::android::hardware::bluetooth::audio::V2_2::AudioConfiguration::
                   hidl_discriminator::pcmConfig &&
           audio_config_2_2_ != kInvalidSoftwareAudioConfiguration) ||
          (audio_config_discriminator ==
               ::android::hardware::bluetooth::audio::V2_2::AudioConfiguration::
                   hidl_discriminator::leAudioConfig &&
           audio_config_2_2_ != kInvalidLeOffloadAudioConfiguration))
        return audio_config_2_2_;

      ::android::hardware::bluetooth::audio::V2_2::AudioConfiguration toConf;
      const AudioConfiguration_2_1 fromConf =
          GetAudioSession_2_1()->GetAudioConfig();
      if (fromConf.getDiscriminator() ==
          AudioConfiguration_2_1::hidl_discriminator::pcmConfig) {
        toConf.pcmConfig(fromConf.pcmConfig());
        return toConf;
      }
    }

    ::android::hardware::bluetooth::audio::V2_2::AudioConfiguration toConf;
    const AudioConfiguration fromConf = GetAudioSession()->GetAudioConfig();
    // pcmConfig only differs between 2.0 and 2.1 in AudioConfiguration
    if (fromConf.getDiscriminator() ==
        AudioConfiguration::hidl_discriminator::codecConfig) {
      toConf.codecConfig(fromConf.codecConfig());
    } else {
      toConf.pcmConfig() = {
          .sampleRate = static_cast<
              ::android::hardware::bluetooth::audio::V2_1::SampleRate>(
              fromConf.pcmConfig().sampleRate),
          .channelMode = fromConf.pcmConfig().channelMode,
          .bitsPerSample = fromConf.pcmConfig().bitsPerSample,
          .dataIntervalUs = 0};
    }
    return toConf;
  } else if (session_type_2_1_ ==
                 SessionType_2_1::LE_AUDIO_HARDWARE_OFFLOAD_ENCODING_DATAPATH ||
             session_type_2_1_ ==
                 SessionType_2_1::LE_AUDIO_HARDWARE_OFFLOAD_DECODING_DATAPATH) {
    return kInvalidLeOffloadAudioConfiguration;
  } else {
    return kInvalidSoftwareAudioConfiguration;
  }
}

// Those control functions are for the bluetooth_audio module to start, suspend,
// stop stream, to check position, and to update metadata.
bool BluetoothAudioSession_2_2::StartStream() {
  if (HidlToAidlMiddleware_2_0::IsAidlAvailable())
    return HidlToAidlMiddleware_2_2::StartStream(raw_session_type_);
  std::lock_guard<std::recursive_mutex> guard(audio_session->mutex_);
  if (!IsSessionReady()) {
    LOG(DEBUG) << __func__ << " - SessionType=" << toString(session_type_2_1_)
               << " has NO session";
    return false;
  }
  auto hal_retval = audio_session->stack_iface_->startStream();
  if (!hal_retval.isOk()) {
    LOG(WARNING) << __func__ << " - IBluetoothAudioPort SessionType="
                 << toString(session_type_2_1_) << " failed";
    return false;
  }
  return true;
}

bool BluetoothAudioSession_2_2::SuspendStream() {
  if (HidlToAidlMiddleware_2_0::IsAidlAvailable())
    return HidlToAidlMiddleware_2_2::SuspendStream(raw_session_type_);
  std::lock_guard<std::recursive_mutex> guard(audio_session->mutex_);
  if (!IsSessionReady()) {
    LOG(DEBUG) << __func__ << " - SessionType=" << toString(session_type_2_1_)
               << " has NO session";
    return false;
  }
  auto hal_retval = audio_session->stack_iface_->suspendStream();
  if (!hal_retval.isOk()) {
    LOG(WARNING) << __func__ << " - IBluetoothAudioPort SessionType="
                 << toString(session_type_2_1_) << " failed";
    return false;
  }
  return true;
}

void BluetoothAudioSession_2_2::StopStream() {
  if (HidlToAidlMiddleware_2_0::IsAidlAvailable())
    return HidlToAidlMiddleware_2_2::StopStream(raw_session_type_);
  std::lock_guard<std::recursive_mutex> guard(audio_session->mutex_);
  if (!IsSessionReady()) {
    return;
  }
  auto hal_retval = audio_session->stack_iface_->stopStream();
  if (!hal_retval.isOk()) {
    LOG(WARNING) << __func__ << " - IBluetoothAudioPort SessionType="
                 << toString(session_type_2_1_) << " failed";
  }
}

bool BluetoothAudioSession_2_2::UpdateAudioConfig(
    const ::android::hardware::bluetooth::audio::V2_2::AudioConfiguration&
        audio_config) {
  bool is_software_session =
      (session_type_2_1_ == SessionType_2_1::A2DP_SOFTWARE_ENCODING_DATAPATH ||
       session_type_2_1_ ==
           SessionType_2_1::HEARING_AID_SOFTWARE_ENCODING_DATAPATH ||
       session_type_2_1_ ==
           SessionType_2_1::LE_AUDIO_SOFTWARE_ENCODING_DATAPATH ||
       session_type_2_1_ ==
           SessionType_2_1::LE_AUDIO_SOFTWARE_DECODED_DATAPATH);
  bool is_offload_a2dp_session =
      (session_type_2_1_ == SessionType_2_1::A2DP_HARDWARE_OFFLOAD_DATAPATH);
  bool is_offload_le_audio_session =
      (session_type_2_1_ ==
           SessionType_2_1::LE_AUDIO_HARDWARE_OFFLOAD_ENCODING_DATAPATH ||
       session_type_2_1_ ==
           SessionType_2_1::LE_AUDIO_HARDWARE_OFFLOAD_DECODING_DATAPATH);
  auto audio_config_discriminator = audio_config.getDiscriminator();
  bool is_software_audio_config =
      (is_software_session &&
       audio_config_discriminator ==
           ::android::hardware::bluetooth::audio::V2_2::AudioConfiguration::
               hidl_discriminator::pcmConfig);
  bool is_a2dp_offload_audio_config =
      (is_offload_a2dp_session &&
       audio_config_discriminator ==
           ::android::hardware::bluetooth::audio::V2_2::AudioConfiguration::
               hidl_discriminator::codecConfig);
  bool is_le_audio_offload_audio_config =
      (is_offload_le_audio_session &&
       audio_config_discriminator ==
           ::android::hardware::bluetooth::audio::V2_2::AudioConfiguration::
               hidl_discriminator::leAudioConfig);
  if (!is_software_audio_config && !is_a2dp_offload_audio_config &&
      !is_le_audio_offload_audio_config) {
    return false;
  }
  audio_config_2_2_ = audio_config;
  return true;
}

// The report function is used to report that the Bluetooth stack has started
// this session without any failure, and will invoke session_changed_cb_ to
// notify those registered bluetooth_audio outputs
void BluetoothAudioSession_2_2::OnSessionStarted(
    const sp<IBluetoothAudioPort> stack_iface, const DataMQ::Descriptor* dataMQ,
    const ::android::hardware::bluetooth::audio::V2_2::AudioConfiguration&
        audio_config) {
  if (session_type_2_1_ == SessionType_2_1::UNKNOWN) {
    ::android::hardware::bluetooth::audio::V2_0::AudioConfiguration config;
    if (audio_config.getDiscriminator() ==
        ::android::hardware::bluetooth::audio::V2_2::AudioConfiguration::
            hidl_discriminator::codecConfig) {
      config.codecConfig(audio_config.codecConfig());
    } else {
      auto& tmpPcm = audio_config.pcmConfig();
      config.pcmConfig(
          ::android::hardware::bluetooth::audio::V2_0::PcmParameters{
              .sampleRate = static_cast<SampleRate>(tmpPcm.sampleRate),
              .channelMode = tmpPcm.channelMode,
              .bitsPerSample = tmpPcm.bitsPerSample
              /*dataIntervalUs is not passed to 2.0 */
          });
    }

    audio_session->OnSessionStarted(stack_iface, dataMQ, config);
  } else {
    std::lock_guard<std::recursive_mutex> guard(audio_session->mutex_);
    if (stack_iface == nullptr) {
      LOG(ERROR) << __func__ << " - SessionType=" << toString(session_type_2_1_)
                 << ", IBluetoothAudioPort Invalid";
    } else if (!UpdateAudioConfig(audio_config)) {
      LOG(ERROR) << __func__ << " - SessionType=" << toString(session_type_2_1_)
                 << ", AudioConfiguration=" << toString(audio_config)
                 << " Invalid";
    } else if (!audio_session->UpdateDataPath(dataMQ)) {
      LOG(ERROR) << __func__ << " - SessionType=" << toString(session_type_2_1_)
                 << " DataMQ Invalid";
      audio_config_2_2_ =
          ((session_type_2_1_ ==
                SessionType_2_1::LE_AUDIO_HARDWARE_OFFLOAD_ENCODING_DATAPATH ||
            session_type_2_1_ ==
                SessionType_2_1::LE_AUDIO_HARDWARE_OFFLOAD_DECODING_DATAPATH)
               ? kInvalidLeOffloadAudioConfiguration
               : kInvalidSoftwareAudioConfiguration);
    } else {
      audio_session->stack_iface_ = stack_iface;
      LOG(INFO) << __func__ << " - SessionType=" << toString(session_type_2_1_)
                << ", AudioConfiguration=" << toString(audio_config);
      ReportSessionStatus();
    };
  }
}

// The report function is used to report that the Bluetooth stack has ended the
// session, and will invoke session_changed_cb_ to notify registered
// bluetooth_audio outputs
void BluetoothAudioSession_2_2::OnSessionEnded() {
  std::lock_guard<std::recursive_mutex> guard(audio_session->mutex_);
  bool toggled = IsSessionReady();
  LOG(INFO) << __func__ << " - SessionType=" << toString(session_type_2_1_);
  if (session_type_2_1_ == SessionType_2_1::UNKNOWN) {
    audio_session->OnSessionEnded();
    return;
  }

  audio_config_2_2_ =
      ((session_type_2_1_ ==
            SessionType_2_1::LE_AUDIO_HARDWARE_OFFLOAD_ENCODING_DATAPATH ||
        session_type_2_1_ ==
            SessionType_2_1::LE_AUDIO_HARDWARE_OFFLOAD_DECODING_DATAPATH)
           ? kInvalidLeOffloadAudioConfiguration
           : kInvalidSoftwareAudioConfiguration);
  audio_session->stack_iface_ = nullptr;
  audio_session->UpdateDataPath(nullptr);
  if (toggled) {
    ReportSessionStatus();
  }
}

// The control function helps the bluetooth_audio module to register
// PortStatusCallbacks_2_2
// @return: cookie - the assigned number to this bluetooth_audio output
uint16_t BluetoothAudioSession_2_2::RegisterStatusCback(
    const PortStatusCallbacks_2_2& cbacks) {
  if (HidlToAidlMiddleware_2_0::IsAidlAvailable())
    return HidlToAidlMiddleware_2_2::RegisterControlResultCback(
        raw_session_type_, cbacks);
  if (session_type_2_1_ !=
          SessionType_2_1::LE_AUDIO_HARDWARE_OFFLOAD_ENCODING_DATAPATH &&
      session_type_2_1_ !=
          SessionType_2_1::LE_AUDIO_HARDWARE_OFFLOAD_DECODING_DATAPATH) {
    PortStatusCallbacks cb = {
        .control_result_cb_ = cbacks.control_result_cb_,
        .session_changed_cb_ = cbacks.session_changed_cb_};
    return audio_session->RegisterStatusCback(cb);
  }

  std::lock_guard<std::recursive_mutex> guard(audio_session->mutex_);
  uint16_t cookie = ObserversCookieGetInitValue(session_type_2_1_);
  uint16_t cookie_upper_bound = ObserversCookieGetUpperBound(session_type_2_1_);

  while (cookie < cookie_upper_bound) {
    if (observers_.find(cookie) == observers_.end()) {
      break;
    }
    ++cookie;
  }
  if (cookie >= cookie_upper_bound) {
    LOG(ERROR) << __func__ << " - SessionType=" << toString(session_type_2_1_)
               << " has " << observers_.size()
               << " observers already (No Resource)";
    return kObserversCookieUndefined;
  }
  std::shared_ptr<struct PortStatusCallbacks_2_2> cb =
      std::make_shared<struct PortStatusCallbacks_2_2>();
  *cb = cbacks;
  observers_[cookie] = cb;
  return cookie;
}

// The control function helps the bluetooth_audio module to unregister
// PortStatusCallbacks_2_2
// @param: cookie - indicates which bluetooth_audio output is
void BluetoothAudioSession_2_2::UnregisterStatusCback(uint16_t cookie) {
  if (HidlToAidlMiddleware_2_0::IsAidlAvailable())
    return HidlToAidlMiddleware_2_2::UnregisterControlResultCback(
        raw_session_type_, cookie);
  std::lock_guard<std::recursive_mutex> guard(audio_session->mutex_);
  if (session_type_2_1_ !=
          SessionType_2_1::LE_AUDIO_HARDWARE_OFFLOAD_ENCODING_DATAPATH &&
      session_type_2_1_ !=
          SessionType_2_1::LE_AUDIO_HARDWARE_OFFLOAD_DECODING_DATAPATH) {
    audio_session->UnregisterStatusCback(cookie);
    return;
  }
  if (observers_.erase(cookie) != 1) {
    LOG(WARNING) << __func__ << " - SessionType=" << toString(session_type_2_1_)
                 << " no such provider=0x"
                 << android::base::StringPrintf("%04x", cookie);
  }
}

// invoking the registered session_changed_cb_
void BluetoothAudioSession_2_2::ReportSessionStatus() {
  // This is locked already by OnSessionStarted / OnSessionEnded
  if (session_type_2_1_ !=
          SessionType_2_1::LE_AUDIO_HARDWARE_OFFLOAD_ENCODING_DATAPATH &&
      session_type_2_1_ !=
          SessionType_2_1::LE_AUDIO_HARDWARE_OFFLOAD_DECODING_DATAPATH) {
    audio_session->ReportSessionStatus();
    return;
  }
  if (observers_.empty()) {
    LOG(INFO) << __func__ << " - SessionType=" << toString(session_type_2_1_)
              << " has NO port state observer";
    return;
  }
  for (auto& observer : observers_) {
    uint16_t cookie = observer.first;
    std::shared_ptr<struct PortStatusCallbacks_2_2> cb = observer.second;
    LOG(INFO) << __func__ << " - SessionType=" << toString(session_type_2_1_)
              << " notify to bluetooth_audio=0x"
              << android::base::StringPrintf("%04x", cookie);
    cb->session_changed_cb_(cookie);
  }
}

// The report function is used to report that the Bluetooth stack has notified
// the result of startStream or suspendStream, and will invoke
// control_result_cb_ to notify registered bluetooth_audio outputs
void BluetoothAudioSession_2_2::ReportControlStatus(
    bool start_resp, const BluetoothAudioStatus& status) {
  if (session_type_2_1_ !=
          SessionType_2_1::LE_AUDIO_HARDWARE_OFFLOAD_ENCODING_DATAPATH &&
      session_type_2_1_ !=
          SessionType_2_1::LE_AUDIO_HARDWARE_OFFLOAD_DECODING_DATAPATH) {
    audio_session->ReportControlStatus(start_resp, status);
    return;
  }
  std::lock_guard<std::recursive_mutex> guard(audio_session->mutex_);
  if (observers_.empty()) {
    LOG(WARNING) << __func__ << " - SessionType=" << toString(session_type_2_1_)
                 << " has NO port state observer";
    return;
  }
  for (auto& observer : observers_) {
    uint16_t cookie = observer.first;
    std::shared_ptr<struct PortStatusCallbacks_2_2> cb = observer.second;
    LOG(INFO) << __func__ << " - status=" << toString(status)
              << " for SessionType=" << toString(session_type_2_1_)
              << ", bluetooth_audio=0x"
              << android::base::StringPrintf("%04x", cookie)
              << (start_resp ? " started" : " suspended");
    cb->control_result_cb_(cookie, start_resp, status);
  }
}

// The report function is used to report that the Bluetooth stack has notified
// the result of startStream or suspendStream, and will invoke
// control_result_cb_ to notify registered bluetooth_audio outputs
void BluetoothAudioSession_2_2::ReportAudioConfigChanged(
    const ::android::hardware::bluetooth::audio::V2_2::AudioConfiguration&
        audio_config) {
  if (session_type_2_1_ !=
          SessionType_2_1::LE_AUDIO_HARDWARE_OFFLOAD_ENCODING_DATAPATH &&
      session_type_2_1_ !=
          SessionType_2_1::LE_AUDIO_HARDWARE_OFFLOAD_DECODING_DATAPATH) {
    return;
  }
  std::lock_guard<std::recursive_mutex> guard(audio_session->mutex_);
  audio_config_2_2_ = audio_config;
  if (observers_.empty()) {
    LOG(WARNING) << __func__ << " - SessionType=" << toString(session_type_2_1_)
                 << " has NO port state observer";
    return;
  }
  for (auto& observer : observers_) {
    uint16_t cookie = observer.first;
    std::shared_ptr<struct PortStatusCallbacks_2_2> cb = observer.second;
    LOG(INFO) << __func__ << " for SessionType=" << toString(session_type_2_1_)
              << ", bluetooth_audio=0x"
              << android::base::StringPrintf("%04x", cookie);
    if (cb->audio_configuration_changed_cb_ != nullptr) {
      cb->audio_configuration_changed_cb_(cookie);
    }
  }
}

std::unique_ptr<BluetoothAudioSessionInstance_2_2>
    BluetoothAudioSessionInstance_2_2::instance_ptr =
        std::unique_ptr<BluetoothAudioSessionInstance_2_2>(
            new BluetoothAudioSessionInstance_2_2());

// API to fetch the session of A2DP / Hearing Aid
std::shared_ptr<BluetoothAudioSession_2_2>
BluetoothAudioSessionInstance_2_2::GetSessionInstance(
    const SessionType_2_1& session_type) {
  std::lock_guard<std::mutex> guard(instance_ptr->mutex_);
  if (!instance_ptr->sessions_map_.empty()) {
    auto entry = instance_ptr->sessions_map_.find(session_type);
    if (entry != instance_ptr->sessions_map_.end()) {
      return entry->second;
    }
  }
  std::shared_ptr<BluetoothAudioSession_2_2> session_ptr =
      std::make_shared<BluetoothAudioSession_2_2>(session_type);
  instance_ptr->sessions_map_[session_type] = session_ptr;
  return session_ptr;
}

}  // namespace audio
}  // namespace bluetooth
}  // namespace android
