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

#include <sys/types.h>
#define LOG_TAG "BTAudioSessionAidl"

#include <android-base/logging.h>
#include <android-base/stringprintf.h>
#include <android/binder_manager.h>

#include "BluetoothAudioSession.h"

namespace aidl {
namespace android {
namespace hardware {
namespace bluetooth {
namespace audio {

static constexpr int kFmqSendTimeoutMs = 1000;  // 1000 ms timeout for sending
static constexpr int kFmqReceiveTimeoutMs =
    1000;                               // 1000 ms timeout for receiving
static constexpr int kWritePollMs = 1;  // polled non-blocking interval
static constexpr int kReadPollMs = 1;   // polled non-blocking interval

BluetoothAudioSession::BluetoothAudioSession(const SessionType& session_type)
    : session_type_(session_type), stack_iface_(nullptr), data_mq_(nullptr) {}

/***
 *
 * Callback methods
 *
 ***/

void BluetoothAudioSession::OnSessionStarted(
    const std::shared_ptr<IBluetoothAudioPort> stack_iface,
    const DataMQDesc* mq_desc, const AudioConfiguration& audio_config,
    const std::vector<LatencyMode>& latency_modes) {
  std::lock_guard<std::recursive_mutex> guard(mutex_);
  if (stack_iface == nullptr) {
    LOG(ERROR) << __func__ << " - SessionType=" << toString(session_type_)
               << ", IBluetoothAudioPort Invalid";
  } else if (!UpdateAudioConfig(audio_config)) {
    LOG(ERROR) << __func__ << " - SessionType=" << toString(session_type_)
               << ", AudioConfiguration=" << audio_config.toString()
               << " Invalid";
  } else if (!UpdateDataPath(mq_desc)) {
    LOG(ERROR) << __func__ << " - SessionType=" << toString(session_type_)
               << " MqDescriptor Invalid";
    audio_config_ = nullptr;
    leaudio_connection_map_ = nullptr;
  } else {
    stack_iface_ = stack_iface;
    latency_modes_ = latency_modes;
    LOG(INFO) << __func__ << " - SessionType=" << toString(session_type_)
              << ", AudioConfiguration=" << audio_config.toString();
    ReportSessionStatus();
    is_streaming_ = false;
  }
}

void BluetoothAudioSession::OnSessionEnded() {
  std::lock_guard<std::recursive_mutex> guard(mutex_);
  bool toggled = IsSessionReady();
  LOG(INFO) << __func__ << " - SessionType=" << toString(session_type_);
  audio_config_ = nullptr;
  leaudio_connection_map_ = nullptr;
  stack_iface_ = nullptr;
  UpdateDataPath(nullptr);
  if (toggled) {
    ReportSessionStatus();
  }
  is_streaming_ = false;
}

/***
 *
 * Util methods
 *
 ***/

const AudioConfiguration BluetoothAudioSession::GetAudioConfig() {
  std::lock_guard<std::recursive_mutex> guard(mutex_);
  if (!IsSessionReady()) {
    switch (session_type_) {
      case SessionType::A2DP_HARDWARE_OFFLOAD_ENCODING_DATAPATH:
      case SessionType::A2DP_HARDWARE_OFFLOAD_DECODING_DATAPATH:
        return AudioConfiguration(CodecConfiguration{});
      case SessionType::LE_AUDIO_HARDWARE_OFFLOAD_ENCODING_DATAPATH:
      case SessionType::LE_AUDIO_HARDWARE_OFFLOAD_DECODING_DATAPATH:
        return AudioConfiguration(LeAudioConfiguration{});
      case SessionType::LE_AUDIO_BROADCAST_HARDWARE_OFFLOAD_ENCODING_DATAPATH:
        return AudioConfiguration(LeAudioBroadcastConfiguration{});
      default:
        return AudioConfiguration(PcmConfiguration{});
    }
  }
  return *audio_config_;
}

const AudioConfiguration BluetoothAudioSession::GetLeAudioConnectionMap() {
  std::lock_guard<std::recursive_mutex> guard(mutex_);
  if (!IsSessionReady()) {
    return AudioConfiguration(LeAudioConfiguration{});
  }
  return *leaudio_connection_map_;
}

void BluetoothAudioSession::ReportAudioConfigChanged(
    const AudioConfiguration& audio_config) {
  if (session_type_ !=
          SessionType::LE_AUDIO_HARDWARE_OFFLOAD_ENCODING_DATAPATH &&
      session_type_ !=
          SessionType::LE_AUDIO_HARDWARE_OFFLOAD_DECODING_DATAPATH) {
    return;
  }

  std::lock_guard<std::recursive_mutex> guard(mutex_);
  if (audio_config.getTag() != AudioConfiguration::leAudioConfig) {
    LOG(ERROR) << __func__ << " invalid audio config type for SessionType ="
               << toString(session_type_);
    return;
  }

  if (is_streaming_) {
    if (audio_config_ == nullptr) {
      LOG(ERROR) << __func__ << " for SessionType=" << toString(session_type_)
                 << " audio_config_ is nullptr during streaming. It shouldn't "
                    "be happened";
      return;
    }

    auto new_leaudio_config =
        audio_config.get<AudioConfiguration::leAudioConfig>();
    auto current_leaudio_config =
        (*audio_config_).get<AudioConfiguration::leAudioConfig>();
    if (new_leaudio_config.codecType != current_leaudio_config.codecType) {
      LOG(ERROR)
          << __func__ << " for SessionType=" << toString(session_type_)
          << " codec type changed during streaming. It shouldn't be happened ";
    }
    auto new_lc3_config = new_leaudio_config.leAudioCodecConfig
                              .get<LeAudioCodecConfiguration::lc3Config>();
    auto current_lc3_config = current_leaudio_config.leAudioCodecConfig
                                  .get<LeAudioCodecConfiguration::lc3Config>();
    if ((new_lc3_config.pcmBitDepth != current_lc3_config.pcmBitDepth) ||
        (new_lc3_config.samplingFrequencyHz !=
         current_lc3_config.samplingFrequencyHz) ||
        (new_lc3_config.frameDurationUs !=
         current_lc3_config.frameDurationUs) ||
        (new_lc3_config.octetsPerFrame != current_lc3_config.octetsPerFrame) ||
        (new_lc3_config.blocksPerSdu != current_lc3_config.blocksPerSdu)) {
      LOG(ERROR)
          << __func__ << " for SessionType=" << toString(session_type_)
          << " lc3 config changed during streaming. It shouldn't be happened";
      return;
    }

    leaudio_connection_map_ =
        std::make_unique<AudioConfiguration>(audio_config);
  } else {
    audio_config_ = std::make_unique<AudioConfiguration>(audio_config);
    leaudio_connection_map_ =
        std::make_unique<AudioConfiguration>(audio_config);
  }

  if (observers_.empty()) {
    LOG(WARNING) << __func__ << " - SessionType=" << toString(session_type_)
                 << " has NO port state observer";
    return;
  }
  for (auto& observer : observers_) {
    uint16_t cookie = observer.first;
    std::shared_ptr<struct PortStatusCallbacks> cb = observer.second;
    LOG(INFO) << __func__ << " for SessionType=" << toString(session_type_)
              << ", bluetooth_audio=0x"
              << ::android::base::StringPrintf("%04x", cookie);
    if (is_streaming_) {
      if (cb->soft_audio_configuration_changed_cb_ != nullptr) {
        cb->soft_audio_configuration_changed_cb_(cookie);
      }
    } else if (cb->audio_configuration_changed_cb_ != nullptr) {
      cb->audio_configuration_changed_cb_(cookie);
    }
  }
}

bool BluetoothAudioSession::IsSessionReady() {
  std::lock_guard<std::recursive_mutex> guard(mutex_);

  bool is_mq_valid =
      (session_type_ == SessionType::A2DP_HARDWARE_OFFLOAD_ENCODING_DATAPATH ||
       session_type_ ==
           SessionType::LE_AUDIO_HARDWARE_OFFLOAD_ENCODING_DATAPATH ||
       session_type_ ==
           SessionType::LE_AUDIO_HARDWARE_OFFLOAD_DECODING_DATAPATH ||
       session_type_ ==
           SessionType::LE_AUDIO_BROADCAST_HARDWARE_OFFLOAD_ENCODING_DATAPATH ||
       session_type_ == SessionType::A2DP_HARDWARE_OFFLOAD_DECODING_DATAPATH ||
       (data_mq_ != nullptr && data_mq_->isValid()));
  return stack_iface_ != nullptr && is_mq_valid && audio_config_ != nullptr;
}

/***
 *
 * Status callback methods
 *
 ***/

uint16_t BluetoothAudioSession::RegisterStatusCback(
    const PortStatusCallbacks& callbacks) {
  std::lock_guard<std::recursive_mutex> guard(mutex_);
  uint16_t cookie = ObserversCookieGetInitValue(session_type_);
  uint16_t cookie_upper_bound = ObserversCookieGetUpperBound(session_type_);

  while (cookie < cookie_upper_bound) {
    if (observers_.find(cookie) == observers_.end()) {
      break;
    }
    ++cookie;
  }
  if (cookie >= cookie_upper_bound) {
    LOG(ERROR) << __func__ << " - SessionType=" << toString(session_type_)
               << " has " << observers_.size()
               << " observers already (No Resource)";
    return kObserversCookieUndefined;
  }
  std::shared_ptr<PortStatusCallbacks> cb =
      std::make_shared<PortStatusCallbacks>();
  *cb = callbacks;
  observers_[cookie] = cb;
  return cookie;
}

void BluetoothAudioSession::UnregisterStatusCback(uint16_t cookie) {
  std::lock_guard<std::recursive_mutex> guard(mutex_);
  if (observers_.erase(cookie) != 1) {
    LOG(WARNING) << __func__ << " - SessionType=" << toString(session_type_)
                 << " no such provider=0x"
                 << ::android::base::StringPrintf("%04x", cookie);
  }
}

/***
 *
 * Stream methods
 *
 ***/

bool BluetoothAudioSession::StartStream(bool is_low_latency) {
  std::lock_guard<std::recursive_mutex> guard(mutex_);
  if (!IsSessionReady()) {
    LOG(DEBUG) << __func__ << " - SessionType=" << toString(session_type_)
               << " has NO session";
    return false;
  }
  auto hal_retval = stack_iface_->startStream(is_low_latency);
  if (!hal_retval.isOk()) {
    LOG(WARNING) << __func__ << " - IBluetoothAudioPort SessionType="
                 << toString(session_type_) << " failed";
    return false;
  }
  return true;
}

bool BluetoothAudioSession::SuspendStream() {
  std::lock_guard<std::recursive_mutex> guard(mutex_);
  if (!IsSessionReady()) {
    LOG(DEBUG) << __func__ << " - SessionType=" << toString(session_type_)
               << " has NO session";
    return false;
  }
  auto hal_retval = stack_iface_->suspendStream();
  if (!hal_retval.isOk()) {
    LOG(WARNING) << __func__ << " - IBluetoothAudioPort SessionType="
                 << toString(session_type_) << " failed";
    return false;
  }
  return true;
}

void BluetoothAudioSession::StopStream() {
  std::lock_guard<std::recursive_mutex> guard(mutex_);
  if (!IsSessionReady()) {
    return;
  }
  auto hal_retval = stack_iface_->stopStream();
  if (!hal_retval.isOk()) {
    LOG(WARNING) << __func__ << " - IBluetoothAudioPort SessionType="
                 << toString(session_type_) << " failed";
  }
}

/***
 *
 * Private methods
 *
 ***/

bool BluetoothAudioSession::UpdateDataPath(const DataMQDesc* mq_desc) {
  if (mq_desc == nullptr) {
    // usecase of reset by nullptr
    data_mq_ = nullptr;
    return true;
  }
  std::unique_ptr<DataMQ> temp_mq;
  temp_mq.reset(new DataMQ(*mq_desc));
  if (!temp_mq || !temp_mq->isValid()) {
    data_mq_ = nullptr;
    return false;
  }
  data_mq_ = std::move(temp_mq);
  return true;
}

bool BluetoothAudioSession::UpdateAudioConfig(
    const AudioConfiguration& audio_config) {
  bool is_software_session =
      (session_type_ == SessionType::A2DP_SOFTWARE_ENCODING_DATAPATH ||
       session_type_ == SessionType::HEARING_AID_SOFTWARE_ENCODING_DATAPATH ||
       session_type_ == SessionType::LE_AUDIO_SOFTWARE_DECODING_DATAPATH ||
       session_type_ == SessionType::LE_AUDIO_SOFTWARE_ENCODING_DATAPATH ||
       session_type_ ==
           SessionType::LE_AUDIO_BROADCAST_SOFTWARE_ENCODING_DATAPATH ||
       session_type_ == SessionType::A2DP_SOFTWARE_DECODING_DATAPATH);
  bool is_offload_a2dp_session =
      (session_type_ == SessionType::A2DP_HARDWARE_OFFLOAD_ENCODING_DATAPATH ||
       session_type_ == SessionType::A2DP_HARDWARE_OFFLOAD_DECODING_DATAPATH);
  bool is_offload_le_audio_unicast_session =
      (session_type_ ==
           SessionType::LE_AUDIO_HARDWARE_OFFLOAD_ENCODING_DATAPATH ||
       session_type_ ==
           SessionType::LE_AUDIO_HARDWARE_OFFLOAD_DECODING_DATAPATH);
  bool is_offload_le_audio_broadcast_session =
      (session_type_ ==
       SessionType::LE_AUDIO_BROADCAST_HARDWARE_OFFLOAD_ENCODING_DATAPATH);
  auto audio_config_tag = audio_config.getTag();
  bool is_software_audio_config =
      (is_software_session &&
       audio_config_tag == AudioConfiguration::pcmConfig);
  bool is_a2dp_offload_audio_config =
      (is_offload_a2dp_session &&
       audio_config_tag == AudioConfiguration::a2dpConfig);
  bool is_le_audio_offload_unicast_audio_config =
      (is_offload_le_audio_unicast_session &&
       audio_config_tag == AudioConfiguration::leAudioConfig);
  bool is_le_audio_offload_broadcast_audio_config =
      (is_offload_le_audio_broadcast_session &&
       audio_config_tag == AudioConfiguration::leAudioBroadcastConfig);
  if (!is_software_audio_config && !is_a2dp_offload_audio_config &&
      !is_le_audio_offload_unicast_audio_config &&
      !is_le_audio_offload_broadcast_audio_config) {
    return false;
  }
  audio_config_ = std::make_unique<AudioConfiguration>(audio_config);
  return true;
}

void BluetoothAudioSession::ReportSessionStatus() {
  // This is locked already by OnSessionStarted / OnSessionEnded
  if (observers_.empty()) {
    LOG(INFO) << __func__ << " - SessionType=" << toString(session_type_)
              << " has NO port state observer";
    return;
  }
  for (auto& observer : observers_) {
    uint16_t cookie = observer.first;
    std::shared_ptr<PortStatusCallbacks> callback = observer.second;
    LOG(INFO) << __func__ << " - SessionType=" << toString(session_type_)
              << " notify to bluetooth_audio=0x"
              << ::android::base::StringPrintf("%04x", cookie);
    callback->session_changed_cb_(cookie);
  }
}

/***
 *
 * PCM methods
 *
 ***/

size_t BluetoothAudioSession::OutWritePcmData(const void* buffer,
                                              size_t bytes) {
  if (buffer == nullptr || bytes <= 0) {
    return 0;
  }
  size_t total_written = 0;
  int timeout_ms = kFmqSendTimeoutMs;
  do {
    std::unique_lock<std::recursive_mutex> lock(mutex_);
    if (!IsSessionReady()) {
      break;
    }
    size_t num_bytes_to_write = data_mq_->availableToWrite();
    if (num_bytes_to_write) {
      if (num_bytes_to_write > (bytes - total_written)) {
        num_bytes_to_write = bytes - total_written;
      }

      if (!data_mq_->write(
              static_cast<const MQDataType*>(buffer) + total_written,
              num_bytes_to_write)) {
        LOG(ERROR) << "FMQ datapath writing " << total_written << "/" << bytes
                   << " failed";
        return total_written;
      }
      total_written += num_bytes_to_write;
    } else if (timeout_ms >= kWritePollMs) {
      lock.unlock();
      usleep(kWritePollMs * 1000);
      timeout_ms -= kWritePollMs;
    } else {
      LOG(DEBUG) << "Data " << total_written << "/" << bytes << " overflow "
                 << (kFmqSendTimeoutMs - timeout_ms) << " ms";
      return total_written;
    }
  } while (total_written < bytes);
  return total_written;
}

size_t BluetoothAudioSession::InReadPcmData(void* buffer, size_t bytes) {
  if (buffer == nullptr || bytes <= 0) {
    return 0;
  }
  size_t total_read = 0;
  int timeout_ms = kFmqReceiveTimeoutMs;
  do {
    std::unique_lock<std::recursive_mutex> lock(mutex_);
    if (!IsSessionReady()) {
      break;
    }
    size_t num_bytes_to_read = data_mq_->availableToRead();
    if (num_bytes_to_read) {
      if (num_bytes_to_read > (bytes - total_read)) {
        num_bytes_to_read = bytes - total_read;
      }
      if (!data_mq_->read(static_cast<MQDataType*>(buffer) + total_read,
                          num_bytes_to_read)) {
        LOG(ERROR) << "FMQ datapath reading " << total_read << "/" << bytes
                   << " failed";
        return total_read;
      }
      total_read += num_bytes_to_read;
    } else if (timeout_ms >= kReadPollMs) {
      lock.unlock();
      usleep(kReadPollMs * 1000);
      timeout_ms -= kReadPollMs;
      continue;
    } else {
      LOG(DEBUG) << "Data " << total_read << "/" << bytes << " overflow "
                 << (kFmqReceiveTimeoutMs - timeout_ms) << " ms";
      return total_read;
    }
  } while (total_read < bytes);
  return total_read;
}

/***
 *
 * Other methods
 *
 ***/

void BluetoothAudioSession::ReportControlStatus(bool start_resp,
                                                BluetoothAudioStatus status) {
  std::lock_guard<std::recursive_mutex> guard(mutex_);
  if (observers_.empty()) {
    LOG(WARNING) << __func__ << " - SessionType=" << toString(session_type_)
                 << " has NO port state observer";
    return;
  }
  if (start_resp && status == BluetoothAudioStatus::SUCCESS) {
    is_streaming_ = true;
  } else if (!start_resp && (status == BluetoothAudioStatus::SUCCESS ||
                             status == BluetoothAudioStatus::RECONFIGURATION)) {
    is_streaming_ = false;
  }
  for (auto& observer : observers_) {
    uint16_t cookie = observer.first;
    std::shared_ptr<PortStatusCallbacks> callback = observer.second;
    LOG(INFO) << __func__ << " - status=" << toString(status)
              << " for SessionType=" << toString(session_type_)
              << ", bluetooth_audio=0x"
              << ::android::base::StringPrintf("%04x", cookie)
              << (start_resp ? " started" : " suspended");
    callback->control_result_cb_(cookie, start_resp, status);
  }
}

void BluetoothAudioSession::ReportLowLatencyModeAllowedChanged(bool allowed) {
  std::lock_guard<std::recursive_mutex> guard(mutex_);
  low_latency_allowed_ = allowed;
  if (observers_.empty()) {
    LOG(WARNING) << __func__ << " - SessionType=" << toString(session_type_)
                 << " has NO port state observer";
    return;
  }
  for (auto& observer : observers_) {
    uint16_t cookie = observer.first;
    std::shared_ptr<PortStatusCallbacks> callback = observer.second;
    LOG(INFO) << __func__
              << " - allowed=" << (allowed ? " allowed" : " disallowed");
    if (callback->low_latency_mode_allowed_cb_ != nullptr) {
      callback->low_latency_mode_allowed_cb_(cookie, allowed);
    }
  }
}

bool BluetoothAudioSession::GetPresentationPosition(
    PresentationPosition& presentation_position) {
  std::lock_guard<std::recursive_mutex> guard(mutex_);
  if (!IsSessionReady()) {
    LOG(DEBUG) << __func__ << " - SessionType=" << toString(session_type_)
               << " has NO session";
    return false;
  }
  bool retval = false;

  if (!stack_iface_->getPresentationPosition(&presentation_position).isOk()) {
    LOG(WARNING) << __func__ << " - IBluetoothAudioPort SessionType="
                 << toString(session_type_) << " failed";
    return false;
  }
  return retval;
}

void BluetoothAudioSession::UpdateSourceMetadata(
    const struct source_metadata& source_metadata) {
  std::lock_guard<std::recursive_mutex> guard(mutex_);
  if (!IsSessionReady()) {
    LOG(DEBUG) << __func__ << " - SessionType=" << toString(session_type_)
               << " has NO session";
    return;
  }

  ssize_t track_count = source_metadata.track_count;
  LOG(INFO) << __func__ << " - SessionType=" << toString(session_type_) << ","
            << track_count << " track(s)";
  if (session_type_ == SessionType::A2DP_SOFTWARE_ENCODING_DATAPATH ||
      session_type_ == SessionType::A2DP_HARDWARE_OFFLOAD_ENCODING_DATAPATH ||
      session_type_ == SessionType::A2DP_SOFTWARE_DECODING_DATAPATH ||
      session_type_ == SessionType::A2DP_HARDWARE_OFFLOAD_DECODING_DATAPATH) {
    return;
  }

  SourceMetadata hal_source_metadata;
  hal_source_metadata.tracks.resize(track_count);
  for (int i = 0; i < track_count; i++) {
    hal_source_metadata.tracks[i].usage =
        static_cast<media::audio::common::AudioUsage>(
            source_metadata.tracks[i].usage);
    hal_source_metadata.tracks[i].contentType =
        static_cast<media::audio::common::AudioContentType>(
            source_metadata.tracks[i].content_type);
    hal_source_metadata.tracks[i].gain = source_metadata.tracks[i].gain;
    LOG(VERBOSE) << __func__ << " - SessionType=" << toString(session_type_)
                 << ", usage=" << toString(hal_source_metadata.tracks[i].usage)
                 << ", content="
                 << toString(hal_source_metadata.tracks[i].contentType)
                 << ", gain=" << hal_source_metadata.tracks[i].gain;
  }

  auto hal_retval = stack_iface_->updateSourceMetadata(hal_source_metadata);
  if (!hal_retval.isOk()) {
    LOG(WARNING) << __func__ << " - IBluetoothAudioPort SessionType="
                 << toString(session_type_) << " failed";
  }
}

void BluetoothAudioSession::UpdateSinkMetadata(
    const struct sink_metadata& sink_metadata) {
  std::lock_guard<std::recursive_mutex> guard(mutex_);
  if (!IsSessionReady()) {
    LOG(DEBUG) << __func__ << " - SessionType=" << toString(session_type_)
               << " has NO session";
    return;
  }

  ssize_t track_count = sink_metadata.track_count;
  LOG(INFO) << __func__ << " - SessionType=" << toString(session_type_) << ","
            << track_count << " track(s)";
  if (session_type_ == SessionType::A2DP_SOFTWARE_ENCODING_DATAPATH ||
      session_type_ == SessionType::A2DP_HARDWARE_OFFLOAD_ENCODING_DATAPATH ||
      session_type_ == SessionType::A2DP_SOFTWARE_DECODING_DATAPATH ||
      session_type_ == SessionType::A2DP_HARDWARE_OFFLOAD_DECODING_DATAPATH) {
    return;
  }

  SinkMetadata hal_sink_metadata;
  hal_sink_metadata.tracks.resize(track_count);
  for (int i = 0; i < track_count; i++) {
    hal_sink_metadata.tracks[i].source =
        static_cast<media::audio::common::AudioSource>(
            sink_metadata.tracks[i].source);
    hal_sink_metadata.tracks[i].gain = sink_metadata.tracks[i].gain;
    LOG(INFO) << __func__ << " - SessionType=" << toString(session_type_)
              << ", source=" << sink_metadata.tracks[i].source
              << ", dest_device=" << sink_metadata.tracks[i].dest_device
              << ", gain=" << sink_metadata.tracks[i].gain
              << ", dest_device_address="
              << sink_metadata.tracks[i].dest_device_address;
  }

  auto hal_retval = stack_iface_->updateSinkMetadata(hal_sink_metadata);
  if (!hal_retval.isOk()) {
    LOG(WARNING) << __func__ << " - IBluetoothAudioPort SessionType="
                 << toString(session_type_) << " failed";
  }
}

std::vector<LatencyMode> BluetoothAudioSession::GetSupportedLatencyModes() {
  std::lock_guard<std::recursive_mutex> guard(mutex_);
  if (!IsSessionReady()) {
    LOG(DEBUG) << __func__ << " - SessionType=" << toString(session_type_)
               << " has NO session";
    return std::vector<LatencyMode>();
  }
  if (low_latency_allowed_) return latency_modes_;
  std::vector<LatencyMode> modes;
  for (LatencyMode mode : latency_modes_) {
    if (mode == LatencyMode::LOW_LATENCY)
      // ignore those low latency mode if Bluetooth stack doesn't allow
      continue;
    modes.push_back(mode);
  }
  return modes;
}

void BluetoothAudioSession::SetLatencyMode(const LatencyMode& latency_mode) {
  std::lock_guard<std::recursive_mutex> guard(mutex_);
  if (!IsSessionReady()) {
    LOG(DEBUG) << __func__ << " - SessionType=" << toString(session_type_)
               << " has NO session";
    return;
  }

  auto hal_retval = stack_iface_->setLatencyMode(latency_mode);
  if (!hal_retval.isOk()) {
    LOG(WARNING) << __func__ << " - IBluetoothAudioPort SessionType="
                 << toString(session_type_) << " failed";
  }
}

bool BluetoothAudioSession::IsAidlAvailable() {
  if (is_aidl_checked) return is_aidl_available;
  is_aidl_available =
      (AServiceManager_checkService(
           kDefaultAudioProviderFactoryInterface.c_str()) != nullptr);
  is_aidl_checked = true;
  return is_aidl_available;
}

/***
 *
 * BluetoothAudioSessionInstance
 *
 ***/
std::mutex BluetoothAudioSessionInstance::mutex_;
std::unordered_map<SessionType, std::shared_ptr<BluetoothAudioSession>>
    BluetoothAudioSessionInstance::sessions_map_;

std::shared_ptr<BluetoothAudioSession>
BluetoothAudioSessionInstance::GetSessionInstance(
    const SessionType& session_type) {
  std::lock_guard<std::mutex> guard(mutex_);

  if (!sessions_map_.empty()) {
    auto entry = sessions_map_.find(session_type);
    if (entry != sessions_map_.end()) {
      return entry->second;
    }
  }
  std::shared_ptr<BluetoothAudioSession> session_ptr =
      std::make_shared<BluetoothAudioSession>(session_type);
  sessions_map_[session_type] = session_ptr;
  return session_ptr;
}

}  // namespace audio
}  // namespace bluetooth
}  // namespace hardware
}  // namespace android
}  // namespace aidl
