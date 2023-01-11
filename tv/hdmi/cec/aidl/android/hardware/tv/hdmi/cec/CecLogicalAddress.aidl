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

package android.hardware.tv.hdmi.cec;

@VintfStability
@Backing(type="byte")
enum CecLogicalAddress {
    TV = 0,
    RECORDER_1 = 1,
    RECORDER_2 = 2,
    TUNER_1 = 3,
    PLAYBACK_1 = 4,
    AUDIO_SYSTEM = 5,
    TUNER_2 = 6,
    TUNER_3 = 7,
    PLAYBACK_2 = 8,
    RECORDER_3 = 9,
    TUNER_4 = 10,
    PLAYBACK_3 = 11,
    BACKUP_1 = 12,
    BACKUP_2 = 13,
    FREE_USE = 14,
    BROADCAST = 15,
    UNREGISTERED = 15,
}
