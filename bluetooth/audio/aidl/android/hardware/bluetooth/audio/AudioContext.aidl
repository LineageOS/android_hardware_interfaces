/*
 * Copyright 2023 The Android Open Source Project
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

package android.hardware.bluetooth.audio;

/**
 * Context of the audio configuration.
 * Defined by PACS (Le Audio) and used either by A2DP or LE Audio.
 * The `bitmask` is any combination of BT Sig standardized values
 * [Assigned Numbers - 6.12.3], defined in this scope.
 */
@VintfStability
parcelable AudioContext {
    const int UNSPECIFIED = 0x0001;
    const int CONVERSATIONAL = 0x0002;
    const int MEDIA = 0x0004;
    const int GAME = 0x0008;
    const int INSTRUCTIONAL = 0x0010;
    const int VOICE_ASSISTANTS = 0x0020;
    const int LIVE_AUDIO = 0x0040;
    const int SOUND_EFFECTS = 0x0080;
    const int NOTIFICATIONS = 0x0100;
    const int RINGTONE_ALERTS = 0x0200;
    const int ALERTS = 0x0400;
    const int EMERGENCY_ALARM = 0x0800;

    int bitmask;
}
