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

package android.hardware.audio.core;

import android.hardware.audio.common.SourceMetadata;
import android.hardware.audio.core.IStreamCommon;

/**
 * This interface provides means for sending audio data to output devices.
 */
@VintfStability
interface IStreamOut {
    /**
     * Return the interface for common stream operations.
     *
     * This method must always succeed. The implementation must
     * return the same instance object for all subsequent calls to
     * this method.
     *
     * @return The interface for common operations.
     */
    IStreamCommon getStreamCommon();

    /**
     * Update stream metadata.
     *
     * Updates the metadata initially provided at the stream creation.
     *
     * @param sourceMetadata Updated metadata.
     * @throws EX_ILLEGAL_STATE If the stream is closed.
     */
    void updateMetadata(in SourceMetadata sourceMetadata);

    const int HW_VOLUME_MIN = 0;
    const int HW_VOLUME_MAX = 1;
    /**
     * Retrieve current attenuation applied in hardware.
     *
     * Hardware attenuation can be used in cases when the client can not, or is
     * not allowed to modify the audio stream, for example because the stream is
     * encoded.
     *
     * The valid range for attenuation is [0.0f, 1.0f], where 1.0f corresponds
     * to unity gain, 0.0f corresponds to full mute (see HW_VOLUME_*
     * constants). The returned array specifies attenuation for each output
     * channel of the stream.
     *
     * Support of hardware volume control is optional.
     *
     * @return Current attenuation values for each output channel.
     * @throws EX_ILLEGAL_STATE If the stream is closed.
     * @throws EX_UNSUPPORTED_OPERATION If hardware volume control is not supported.
     */
    float[] getHwVolume();
    /**
     * Set attenuation applied in hardware.
     *
     * Hardware attenuation can be used in cases when the client can not, or is
     * not allowed to modify the audio stream, for example because the stream is
     * encoded.
     *
     * The valid range for attenuation is [0.0f, 1.0f], where 1.0f corresponds
     * to unity gain, 0.0f corresponds to full mute (see HW_VOLUME_* constants).
     *
     * Support of hardware volume control is optional.
     *
     * @param channelVolumes Attenuation values for each output channel.
     * @throws EX_ILLEGAL_ARGUMENT If the number of elements in the provided
     *                             array does not match the channel count, or
     *                             attenuation values are out of range.
     * @throws EX_ILLEGAL_STATE If the stream is closed.
     * @throws EX_UNSUPPORTED_OPERATION If hardware volume control is not supported.
     */
    void setHwVolume(in float[] channelVolumes);
}
