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

import android.hardware.audio.common.SinkMetadata;
import android.hardware.audio.core.IStreamCommon;
import android.media.audio.common.MicrophoneDynamicInfo;

/**
 * This interface provides means for receiving audio data from input devices.
 */
@VintfStability
interface IStreamIn {
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
     * Provides information on the microphones that are active for this stream.
     *
     * The returned array contains dynamic information on the microphones which
     * are active for this stream. Each entry in the returned array must have a
     * corresponding entry (matched by the 'MicrophoneInfo.id' field value) in
     * the list of all available microphones which is provided by the
     * 'IModule.getMicrophones' method.
     *
     * This method must be supported by the HAL module if
     * 'IModule.getMicrophones' is supported.
     *
     * @return The vector with dynamic information on the microphones.
     * @throws EX_ILLEGAL_STATE If the stream has already been closed.
     * @throws EX_UNSUPPORTED_OPERATION If the information is unavailable.
     */
    MicrophoneDynamicInfo[] getActiveMicrophones();

    @VintfStability
    @Backing(type="int")
    enum MicrophoneDirection {
        /**
         * Don't do any directionality processing of the activated microphone(s).
         */
        UNSPECIFIED = 0,
        /**
         * Optimize capture for audio coming from the screen-side of the device.
         */
        FRONT = 1,
        /**
         * Optimize capture for audio coming from the side of the device opposite the screen.
         */
        BACK = 2,
        /**
         * Optimize capture for audio coming from an off-device microphone.
         */
        EXTERNAL = 3,
    }
    /**
     * Get the current logical microphone direction.
     *
     * @return The current logical microphone direction.
     * @throws EX_ILLEGAL_STATE If the stream has already been closed.
     * @throws EX_UNSUPPORTED_OPERATION If the information is unavailable.
     */
    MicrophoneDirection getMicrophoneDirection();
    /**
     * Set the current logical microphone direction.
     *
     * The client sets this parameter in order to specify its preference for
     * optimizing the direction of capture when multiple microphones are in use.
     *
     * @param direction The preferred capture direction.
     * @throws EX_ILLEGAL_STATE If the stream has already been closed.
     * @throws EX_UNSUPPORTED_OPERATION If the operation is not supported.
     */
    void setMicrophoneDirection(MicrophoneDirection direction);

    const int MIC_FIELD_DIMENSION_WIDE_ANGLE = -1;
    const int MIC_FIELD_DIMENSION_NO_ZOOM = 0;
    const int MIC_FIELD_DIMENSION_MAX_ZOOM = 1;
    /**
     * Get the "zoom factor" for the logical microphone.
     *
     * The returned value must be within the range of [-1.0, 1.0] (see
     * MIC_FIELD_DIMENSION_* constants).
     *
     * @throws EX_ILLEGAL_STATE If the stream has already been closed.
     * @throws EX_UNSUPPORTED_OPERATION If the information is unavailable.
     */
    float getMicrophoneFieldDimension();
    /**
     * Set the "zoom factor" for the logical microphone.
     *
     * If multiple microphones are in use, the provided zoom factor must be
     * treated as a preference for their combined field dimension. The zoom
     * factor must be within the range of [-1.0, 1.0] (see MIC_FIELD_DIMENSION_*
     * constants).
     *
     * @param zoom The preferred field dimension of the microphone capture.
     * @throws EX_ILLEGAL_ARGUMENT If the dimension value is outside of the range.
     * @throws EX_ILLEGAL_STATE If the stream has already been closed.
     * @throws EX_UNSUPPORTED_OPERATION If the operation is not supported.
     */
    void setMicrophoneFieldDimension(float zoom);

    /**
     * Update stream metadata.
     *
     * Updates the metadata initially provided at the stream creation.
     *
     * @param sinkMetadata Updated metadata.
     * @throws EX_ILLEGAL_STATE If the stream is closed.
     */
    void updateMetadata(in SinkMetadata sinkMetadata);

    const int HW_GAIN_MIN = 0;
    const int HW_GAIN_MAX = 1;
    /**
     * Retrieve current gain applied in hardware.
     *
     * In case when the HAL module has a gain controller, this method returns
     * the current value of its gain for each input channel.
     *
     * The valid range for gain is [0.0f, 1.0f], where 1.0f corresponds to unity
     * gain, 0.0f corresponds to full mute (see HW_GAIN_* constants).
     *
     * @return Current gain values for each input channel.
     * @throws EX_ILLEGAL_STATE If the stream is closed.
     * @throws EX_UNSUPPORTED_OPERATION If hardware gain control is not supported.
     */
    float[] getHwGain();
    /**
     * Set gain applied in hardware.
     *
     * In case when the HAL module has a gain controller, this method sets the
     * current value of its gain for each input channel.
     *
     * The valid range for gain is [0.0f, 1.0f], where 1.0f corresponds to unity
     * gain, 0.0f corresponds to full mute (see HW_GAIN_* constants).
     *
     * @param gain Gain values for each input channel.
     * @throws EX_ILLEGAL_ARGUMENT If the number of elements in the provided
     *                             array does not match the channel count, or
     *                             gain values are out of range.
     * @throws EX_ILLEGAL_STATE If the stream is closed.
     * @throws EX_UNSUPPORTED_OPERATION If hardware gain control is not supported.
     */
    void setHwGain(in float[] channelGains);
}
