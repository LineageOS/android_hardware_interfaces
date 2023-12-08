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

import android.hardware.bluetooth.audio.AudioContext;
import android.hardware.bluetooth.audio.CodecId;
import android.hardware.bluetooth.audio.CodecParameters;

/**
 * A2DP Configuration Hints
 */
@VintfStability
parcelable A2dpConfigurationHint {
    /**
     * Bluetooth Device Address, intended to be used for interoperabilities.
     */
    byte[6] bdAddr;

    /**
     * Audio configuration hints:
     * - The starting audio context of the session
     * - An optional preference of codec and / or parameters
     */

    AudioContext audioContext;
    @nullable CodecId codecId;
    @nullable CodecParameters codecParameters;
}
