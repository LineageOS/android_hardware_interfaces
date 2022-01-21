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

#include <android/hardware/bluetooth/audio/2.2/types.h>

#include "../session/BluetoothAudioSession.h"
#include "../session/BluetoothAudioSession_2_2.h"

namespace aidl {
namespace android {
namespace hardware {
namespace bluetooth {
namespace audio {

using SessionType_2_1 =
    ::android::hardware::bluetooth::audio::V2_1::SessionType;
using PortStatusCallbacks_2_0 =
    ::android::bluetooth::audio::PortStatusCallbacks;
using PortStatusCallbacks_2_2 =
    ::android::bluetooth::audio::PortStatusCallbacks_2_2;
using AudioConfig_2_2 =
    ::android::hardware::bluetooth::audio::V2_2::AudioConfiguration;

class HidlToAidlMiddleware_2_2 {
 public:
  static bool IsSessionReady(const SessionType_2_1& session_type);

  static uint16_t RegisterControlResultCback(
      const SessionType_2_1& session_type,
      const PortStatusCallbacks_2_2& cbacks);

  static void UnregisterControlResultCback(const SessionType_2_1& session_type,
                                           uint16_t cookie);

  static const AudioConfig_2_2 GetAudioConfig(
      const SessionType_2_1& session_type);

  static bool StartStream(const SessionType_2_1& session_type);

  static bool SuspendStream(const SessionType_2_1& session_type);

  static void StopStream(const SessionType_2_1& session_type);

  static void UpdateTracksMetadata(
      const SessionType_2_1& session_type,
      const struct source_metadata* source_metadata);

  static void UpdateSinkMetadata(const SessionType_2_1& session_type,
                                 const struct sink_metadata* sink_metadata);
};

}  // namespace audio
}  // namespace bluetooth
}  // namespace hardware
}  // namespace android
}  // namespace aidl
