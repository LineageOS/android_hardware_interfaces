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

@VintfStability
parcelable DolbyAudioProcessing {
    enum SoundMode {
        GAME,
        MOVIE,
        MUSIC,
        NEWS,
    }

    /**
     * sound mode for dolby audio processing.
     */
    SoundMode soundMode;

    /**
     * Indicates whether Volume Leveler is enabled.
     *
     * <p>Volume Leveler helps to maintain a consistent volume level across different
     * types of content and even within the same program. It minimizes the jarring jumps
     * between loud commercials and quiet dialogue or action sequences.
     */
    boolean volumeLeveler;

    /**
     * Indicates whether Surround Virtualizer is enabled.
     *
     * <p>Surround Virtualizer creates a virtual surround sound experience from stereo
     * content, making it seem like the sound is coming from multiple speakers, even if
     * you only have your TV's built-in speakers. It expands the soundstage and adds
     * depth to the audio.
     */
    boolean surroundVirtualizer;

    /**
     * Indicates whether Dolby Atmos is enabled.
     *
     * <p>Dolby Atmos creates a more immersive and realistic sound experience by adding
     * a height dimension to surround sound. It allows sound to be placed and moved
     * precisely around you, including overhead.
     *
     * <p>Note: To experience Dolby Atmos, you need content that has been specifically
     * mixed in Dolby Atmos and a compatible sound system with upward-firing speakers
     * or a Dolby Atmos soundbar.
     */
    boolean doblyAtmos;
}
