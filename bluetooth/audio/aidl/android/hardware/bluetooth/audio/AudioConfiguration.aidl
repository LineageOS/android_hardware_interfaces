/*
 * Copyright 2021 The Android Open Source Project
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

import android.hardware.bluetooth.audio.A2dpStreamConfiguration;
import android.hardware.bluetooth.audio.CodecConfiguration;
import android.hardware.bluetooth.audio.HfpConfiguration;
import android.hardware.bluetooth.audio.LeAudioBroadcastConfiguration;
import android.hardware.bluetooth.audio.LeAudioConfiguration;
import android.hardware.bluetooth.audio.PcmConfiguration;

/**
 * Used to configure either a Hardware or Software Encoding session based on session type
 */
@VintfStability
union AudioConfiguration {
    PcmConfiguration pcmConfig;
    CodecConfiguration a2dpConfig;
    LeAudioConfiguration leAudioConfig;
    LeAudioBroadcastConfiguration leAudioBroadcastConfig;
    HfpConfiguration hfpConfig;
    A2dpStreamConfiguration a2dp;
}
