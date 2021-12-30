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

#pragma once

#include <android/hardware/bluetooth/audio/2.2/types.h>

#include <mutex>
#include <unordered_map>

#include "BluetoothAudioSession.h"
#include "BluetoothAudioSession_2_1.h"

namespace android {
namespace bluetooth {
namespace audio {

inline uint16_t ObserversCookieGetInitValue(
    const ::android::hardware::bluetooth::audio::V2_1::SessionType&
        session_type) {
  return (static_cast<uint16_t>(session_type) << 8 & 0xff00);
}
inline uint16_t ObserversCookieGetUpperBound(
    const ::android::hardware::bluetooth::audio::V2_1::SessionType&
        session_type) {
  return (static_cast<uint16_t>(session_type) << 8 & 0xff00) +
         kObserversCookieSize;
}

struct PortStatusCallbacks_2_2 {
  // control_result_cb_ - when the Bluetooth stack reports results of
  // streamStarted or streamSuspended, the BluetoothAudioProvider will invoke
  // this callback to report to the bluetooth_audio module.
  // @param: cookie - indicates which bluetooth_audio output should handle
  // @param: start_resp - this report is for startStream or not
  // @param: status - the result of startStream
  std::function<void(uint16_t cookie, bool start_resp,
                     const BluetoothAudioStatus& status)>
      control_result_cb_;
  // session_changed_cb_ - when the Bluetooth stack start / end session, the
  // BluetoothAudioProvider will invoke this callback to notify to the
  // bluetooth_audio module.
  // @param: cookie - indicates which bluetooth_audio output should handle
  std::function<void(uint16_t cookie)> session_changed_cb_;
  // audio_configuration_changed_cb_ - when the Bluetooth stack change the audio
  // configuration, the BluetoothAudioProvider will invoke this callback to
  // notify to the bluetooth_audio module.
  // @param: cookie - indicates which bluetooth_audio output should handle
  std::function<void(uint16_t cookie)> audio_configuration_changed_cb_;
};

class BluetoothAudioSession_2_2 {
 private:
  std::shared_ptr<BluetoothAudioSession> audio_session;
  std::shared_ptr<BluetoothAudioSession_2_1> audio_session_2_1;

  ::android::hardware::bluetooth::audio::V2_1::SessionType session_type_2_1_;
  ::android::hardware::bluetooth::audio::V2_1::SessionType raw_session_type_;

  // audio data configuration for both software and offloading
  ::android::hardware::bluetooth::audio::V2_2::AudioConfiguration
      audio_config_2_2_;

  bool UpdateAudioConfig(
      const ::android::hardware::bluetooth::audio::V2_2::AudioConfiguration&
          audio_config);

  static ::android::hardware::bluetooth::audio::V2_2::AudioConfiguration
      invalidSoftwareAudioConfiguration;
  static ::android::hardware::bluetooth::audio::V2_2::AudioConfiguration
      invalidOffloadAudioConfiguration;
  static ::android::hardware::bluetooth::audio::V2_2::AudioConfiguration
      invalidLeOffloadAudioConfiguration;

  // saving those registered bluetooth_audio's callbacks
  std::unordered_map<uint16_t, std::shared_ptr<struct PortStatusCallbacks_2_2>>
      observers_;

  // invoking the registered session_changed_cb_
  void ReportSessionStatus();

 public:
  BluetoothAudioSession_2_2(
      const ::android::hardware::bluetooth::audio::V2_1::SessionType&
          session_type);

  // The function helps to check if this session is ready or not
  // @return: true if the Bluetooth stack has started the specified session
  bool IsSessionReady();

  std::shared_ptr<BluetoothAudioSession> GetAudioSession();
  std::shared_ptr<BluetoothAudioSession_2_1> GetAudioSession_2_1();

  // The report function is used to report that the Bluetooth stack has started
  // this session without any failure, and will invoke session_changed_cb_ to
  // notify those registered bluetooth_audio outputs
  void OnSessionStarted(
      const sp<IBluetoothAudioPort> stack_iface,
      const DataMQ::Descriptor* dataMQ,
      const ::android::hardware::bluetooth::audio::V2_2::AudioConfiguration&
          audio_config);

  // The report function is used to report that the Bluetooth stack has ended
  // the session, and will invoke session_changed_cb_ to notify registered
  // bluetooth_audio outputs
  void OnSessionEnded();

  // Those control functions are for the bluetooth_audio module to start,
  // suspend, stop stream, to check position, and to update metadata.
  bool StartStream();
  bool SuspendStream();
  void StopStream();

  // The control function helps the bluetooth_audio module to register
  // PortStatusCallbacks_2_2
  // @return: cookie - the assigned number to this bluetooth_audio output
  uint16_t RegisterStatusCback(const PortStatusCallbacks_2_2& cbacks);

  // The control function helps the bluetooth_audio module to unregister
  // PortStatusCallbacks_2_2
  // @param: cookie - indicates which bluetooth_audio output is
  void UnregisterStatusCback(uint16_t cookie);

  // The report function is used to report that the Bluetooth stack has notified
  // the result of startStream or suspendStream, and will invoke
  // control_result_cb_ to notify registered bluetooth_audio outputs
  void ReportControlStatus(bool start_resp, const BluetoothAudioStatus& status);

  // The report function is used to report that the Bluetooth stack has notified
  // the audio configuration changed, and will invoke
  // audio_configuration_changed_cb_ to notify registered bluetooth_audio
  // outputs
  void ReportAudioConfigChanged(
      const ::android::hardware::bluetooth::audio::V2_2::AudioConfiguration&
          audio_config);

  // The control function is for the bluetooth_audio module to get the current
  // AudioConfiguration
  const ::android::hardware::bluetooth::audio::V2_2::AudioConfiguration
  GetAudioConfig();

  void UpdateSinkMetadata(const struct sink_metadata* sink_metadata);

  static constexpr ::android::hardware::bluetooth::audio::V2_2::
      AudioConfiguration& kInvalidSoftwareAudioConfiguration =
          invalidSoftwareAudioConfiguration;
  static constexpr ::android::hardware::bluetooth::audio::V2_2::
      AudioConfiguration& kInvalidOffloadAudioConfiguration =
          invalidOffloadAudioConfiguration;
  static constexpr ::android::hardware::bluetooth::audio::V2_2::
      AudioConfiguration& kInvalidLeOffloadAudioConfiguration =
          invalidLeOffloadAudioConfiguration;
};

class BluetoothAudioSessionInstance_2_2 {
 public:
  // The API is to fetch the specified session of A2DP / Hearing Aid
  static std::shared_ptr<BluetoothAudioSession_2_2> GetSessionInstance(
      const ::android::hardware::bluetooth::audio::V2_1::SessionType&
          session_type);

 private:
  static std::unique_ptr<BluetoothAudioSessionInstance_2_2> instance_ptr;
  std::mutex mutex_;
  std::unordered_map<::android::hardware::bluetooth::audio::V2_1::SessionType,
                     std::shared_ptr<BluetoothAudioSession_2_2>>
      sessions_map_;
};

}  // namespace audio
}  // namespace bluetooth
}  // namespace android
