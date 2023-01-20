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
import android.media.audio.common.AudioDualMonoMode;
import android.media.audio.common.AudioLatencyMode;
import android.media.audio.common.AudioPlaybackRate;

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

    // aidl: Constant of type float is not supported (b/251286924).
    // const float AUDIO_DESCRIPTION_MIX_LEVEL_MIN = -Inf;
    const int AUDIO_DESCRIPTION_MIX_LEVEL_MAX = 48;
    /**
     * Returns the Audio Description Mix level in dB.
     *
     * The level is applied to streams incorporating a secondary Audio
     * Description stream. It specifies the relative level of mixing for
     * the Audio Description with a reference to the Main Audio.
     *
     * The value of the relative level is in the range from negative infinity
     * to +48, see AUDIO_DESCRIPTION_MIX_LEVEL_* constants.
     *
     * @return The current Audio Description Mix Level in dB.
     * @throws EX_ILLEGAL_STATE If the stream is closed.
     * @throws EX_UNSUPPORTED_OPERATION If the information is unavailable.
     */
    float getAudioDescriptionMixLevel();
    /**
     * Sets the Audio Description Mix level in dB.
     *
     * For streams incorporating a secondary Audio Description stream the
     * relative level of mixing of the Audio Description to the Main Audio is
     * controlled by this method.
     *
     * The value of the relative level must be in the range from negative
     * infinity to +48, see AUDIO_DESCRIPTION_MIX_LEVEL_* constants.
     *
     * @param leveldB Audio Description Mix Level in dB.
     * @throws EX_ILLEGAL_ARGUMENT If the provided value is out of range.
     * @throws EX_ILLEGAL_STATE If the stream is closed.
     * @throws EX_UNSUPPORTED_OPERATION If setting of this parameter is not supported.
     */
    void setAudioDescriptionMixLevel(float leveldB);

    /**
     * Returns the Dual Mono mode presentation setting.
     *
     * @return The current setting of Dual Mono mode.
     * @throws EX_ILLEGAL_STATE If the stream is closed.
     * @throws EX_UNSUPPORTED_OPERATION If the information is unavailable.
     */
    AudioDualMonoMode getDualMonoMode();
    /**
     * Sets the Dual Mono mode presentation on the output device.
     *
     * The Dual Mono mode is generally applied to stereo audio streams
     * where the left and right channels come from separate sources.
     *
     * @param mode Selected Dual Mono mode.
     * @throws EX_ILLEGAL_STATE If the stream is closed.
     * @throws EX_UNSUPPORTED_OPERATION If setting of this parameter is not supported.
     */
    void setDualMonoMode(AudioDualMonoMode mode);

    /**
     * Retrieve supported latency modes.
     *
     * Indicates which latency modes are currently supported on this output
     * stream. If the transport protocol (for example, Bluetooth A2DP) used by
     * this output stream to reach the output device supports variable latency
     * modes, the HAL indicates which modes are currently supported. The client
     * can then call setLatencyMode() with one of the supported modes to select
     * the desired operation mode.
     *
     * Implementation for this method is mandatory only on specific spatial
     * audio streams indicated by AUDIO_OUTPUT_FLAG_SPATIALIZER flag if they can
     * be routed to a BT sinks or if the implementation indicates support
     * on all streams via IModule.supportsVariableLatency().
     *
     * @return Currently supported latency modes.
     * @throws EX_ILLEGAL_STATE If the stream is closed.
     * @throws EX_UNSUPPORTED_OPERATION If the information is unavailable.
     */
    AudioLatencyMode[] getRecommendedLatencyModes();
    /**
     * Sets the latency mode.
     *
     * The requested mode must be one of the modes returned by the
     * 'getRecommendedLatencyModes()' method.
     *
     * Implementation for this method is mandatory only on specific spatial
     * audio streams indicated by AUDIO_OUTPUT_FLAG_SPATIALIZER flag if they can
     * be routed to a BT sinks or if the implementation indicates support
     * on all streams via IModule.supportsVariableLatency().
     *
     * @throws EX_ILLEGAL_ARGUMENT If the specified mode is not supported.
     * @throws EX_ILLEGAL_STATE If the stream is closed.
     * @throws EX_UNSUPPORTED_OPERATION If setting of this parameter is not supported.
     */
    void setLatencyMode(AudioLatencyMode mode);

    /**
     * Retrieve current playback rate parameters.
     *
     * @return Current playback parameters.
     * @throws EX_ILLEGAL_STATE If the stream is closed.
     * @throws EX_UNSUPPORTED_OPERATION If the information is unavailable.
     */
    AudioPlaybackRate getPlaybackRateParameters();
    /**
     * Set playback rate parameters.
     *
     * Sets the playback rate parameters that control playback behavior. This
     * is normally used when playing encoded content and decoding is performed
     * in hardware. Otherwise, the client can apply necessary transformations
     * itself.
     *
     * The range of supported values for speed and pitch factors is provided by
     * the 'IModule.getSupportedPlaybackRateFactors' method. Out of range speed
     * and pitch values must not be rejected if the fallback mode is 'MUTE'.
     *
     * @param playbackRate Playback parameters to set.
     * @throws EX_ILLEGAL_ARGUMENT If provided parameters are out of acceptable range.
     * @throws EX_ILLEGAL_STATE If the stream is closed.
     * @throws EX_UNSUPPORTED_OPERATION If setting playback rate parameters
     *                                  is not supported.
     */
    void setPlaybackRateParameters(in AudioPlaybackRate playbackRate);

    /**
     * Select presentation and program from for decoding.
     *
     * Selects a presentation for decoding from a next generation media stream
     * (as defined per ETSI TS 103 190-2) and a program within the presentation.
     * The client must obtain valid presentation and program IDs from the media
     * stream on its own.
     *
     * @param presentationId Selected audio presentation.
     * @param programId Refinement for the presentation.
     * @throws EX_ILLEGAL_ARGUMENT If the HAL module is unable to locate
     *                             the specified presentation or program in
     *                             the media stream.
     * @throws EX_ILLEGAL_STATE If the stream is closed.
     * @throws EX_UNSUPPORTED_OPERATION If presentation selection is not supported.
     */
    void selectPresentation(int presentationId, int programId);
}
