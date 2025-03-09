/*
 * Copyright (C) 2024 The Android Open Source Project
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

package android.hardware.tv.mediaquality;

import android.hardware.tv.mediaquality.DigitalOutput;
import android.hardware.tv.mediaquality.DolbyAudioProcessing;
import android.hardware.tv.mediaquality.DownmixMode;
import android.hardware.tv.mediaquality.DtsVirtualX;
import android.hardware.tv.mediaquality.EqualizerDetail;
import android.hardware.tv.mediaquality.QualityLevel;

/**
 * The parameters for Sound Profile.
 */
@VintfStability
union SoundParameter {
    /*
     * This parameter controls the balance between the left and tight speakers.
     * The valid range is -50 to 50, where:
     *   - Negative values shift the balance towards the left speaker.
     *   - Positive values shift the balance towards the right speaker.
     *   - 0 represents a balanced output.
     */
    int balance;

    /*
     * Bass controls the intensity of low-frequency sounds.
     * The valid range is 0 - 100.
     */
    int bass;

    /*
     * Treble controls the intensity of high-frequency sounds.
     * The valid range is 0 - 100.
     */
    int treble;

    /* Enable surround sound. */
    boolean surroundSoundEnabled;

    /*
     * Equalizer can fine-tune the audio output by adjusting the loudness of different
     * frequency bands;
     * The frequency bands are 120Hz, 500Hz, 1.5kHz, 5kHz, 10kHz.
     * Each band have a value of -50 to 50.
     */
    EqualizerDetail equalizerDetail;

    /* Enable speaker output. */
    boolean speakersEnabled;

    /* Speaker delay in ms. */
    int speakersDelayMs;

    /* eARC allows for higher bandwidth audio transmission over HDMI */
    boolean enhancedAudioReturnChannelEnabled;

    /* Enable auto volume control sound effect. */
    boolean autoVolumeControl;

    /* Enable downmix mode. */
    DownmixMode downmixMode;

    /* Enable dynamic range compression */
    boolean dtsDrc;

    /* Sound effects from Dobly */
    @nullable DolbyAudioProcessing dolbyAudioProcessing;

    /* Sound effect from Dolby. */
    QualityLevel dolbyDialogueEnhancer;

    /* Sound effect from DTS. */
    @nullable DtsVirtualX dtsVirtualX;

    /* Digital output mode. */
    DigitalOutput digitalOutput;

    /* Digital output delay in ms. */
    int digitalOutputDelayMs;
}
