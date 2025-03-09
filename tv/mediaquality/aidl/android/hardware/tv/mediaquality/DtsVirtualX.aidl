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
parcelable DtsVirtualX {
    /*
     * Total Bass Harmonic Distortion (X).
     * Enables/disables TBHDX bass enhancement. Provides a richer low-frequency experience,
     * simulating deeper bass.
     */
    boolean tbHdx;

    /*
     * Activates an audio limiter. Prevents excessive volume peaks that could cause distortion
     * or speaker damage
     */
    boolean limiter;

    /*
     * Enables/disables the core DTS Virtual:X surround sound processing. Creates an immersive,
     * multi-channel audio experience from the speaker configuration.
     */
    boolean truSurroundX;

    /*
     * Activates DTS TruVolume HD. Reduces the dynamic range of audio, minimizing loudness
     * variations between content and channels.
     */
    boolean truVolumeHd;

    /* Enhances the clarity and intelligibility of speech in audio content. */
    boolean dialogClarity;

    /* Applies audio processing to improve overall sound definition and clarity. */
    boolean definition;

    /*
     * Enables/disables the processing of virtual height channels. Creates a more immersive
     * audio experience by simulating sounds from above.
     */
    boolean height;
}
