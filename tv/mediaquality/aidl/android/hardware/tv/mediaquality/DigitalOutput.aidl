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
enum DigitalOutput {
    /**
     * Automatically selects the best audio format to send to the connected audio device
     * based on the incoming audio stream. This mode prioritizes high-quality formats
     * like Dolby Digital or DTS if supported by the device, otherwise falls back to PCM.
     */
    AUTO,

    /**
     * Sends the raw, unprocessed audio stream directly to the connected audio device.
     * This mode requires the audio device to handle decoding and processing of various
     * audio formats like Dolby Digital or DTS.
     */
    BYPASS,

    /**
     * Converts all incoming audio to 2-channel PCM (Pulse Code Modulation) stereo
     * before sending it to the audio device. This ensures compatibility with a wide
     * range of devices but sacrifices surround sound capabilities.
     */
    PCM,
}
