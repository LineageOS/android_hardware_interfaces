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

import android.hardware.audio.core.AudioMode;
import android.media.audio.common.Boolean;
import android.media.audio.common.Float;

/**
 * An instance of ITelephony manages settings which are specific to voice calls
 * and SMS messaging functionality. This interface is optional to implement and
 * provide by the vendor. It needs to be provided only if the device actually
 * supports telephony.
 */
@VintfStability
interface ITelephony {
    /**
     * Return the list of supported audio modes.
     *
     * The first 4 AudioModes: NORMAL, RINGTONE, IN_CALL, IN_COMMUNICATION must
     * be supported by all implementations.
     *
     * This method is only called once, during the audio system initialization,
     * and must return the same result all the time.
     *
     * @return The list of supported audio modes.
     */
    AudioMode[] getSupportedAudioModes();

    /**
     * Switch the HAL into a new audio mode.
     *
     * The current audio mode is always controlled by the client. The HAL must
     * accept all modes returned by 'getSupportedAudioModes' and reject the
     * rest. The HAL must return from this method only after switching itself
     * to the specified mode, or throw an error if there was a problem during
     * switching.
     *
     * @param mode The mode to switch to.
     * @throws EX_UNSUPPORTED_OPERATION If the HAL does not support the specified mode.
     * @throws EX_ILLEGAL_STATE If there was an error during switching.
     */
    void switchAudioMode(AudioMode mode);

    @JavaDerive(equals=true, toString=true)
    @VintfStability
    parcelable TelecomConfig {
        const int VOICE_VOLUME_MIN = 0;
        const int VOICE_VOLUME_MAX = 1;
        /**
         * Volume of a voice call. 1.0f means unity gain, 0.0f is muted,
         * see VOLUME_* constants.
         */
        @nullable Float voiceVolume;
        /**
         * The current mode of teletypewritter (TTY).
         */
        @VintfStability
        @Backing(type="int")
        enum TtyMode {
            /**
             * The default uninitialized value.
             */
            UNSPECIFIED = -1,
            /**
             * TTY mode is off.
             */
            OFF = 0,
            /**
             * TTY mode is on. The speaker is off and the microphone is muted. The
             * user will communicate with the remote party by sending and receiving
             * text messages.
             */
            FULL = 1,
            /**
             * TTY mode is in hearing carryover mode (HCO). The microphone is muted
             * but the speaker is on. The user will communicate with the remote
             * party by sending text messages and hearing an audible reply.
             */
            HCO = 2,
            /**
             * TTY mode is in voice carryover mode (VCO). The speaker is off but the
             * microphone is still on. User will communicate with the remote party
             * by speaking and receiving text message replies.
             */
            VCO = 3,
        }
        TtyMode ttyMode = TtyMode.UNSPECIFIED;
        /**
         * Whether Hearing Aid Compatibility - Telecoil (HAC-T) mode is enabled.
         */
        @nullable Boolean isHacEnabled;
    }

    /**
     * Set the configuration of the telephony audio.
     *
     * In the provided parcelable, the client sets zero, one or more parameters
     * which have to be updated on the HAL side. The parameters that are left
     * unset must retain their current values.
     *
     * In the returned parcelable, all parameter fields known to the HAL module
     * must be populated to their current values.The client can pass an
     * uninitialized parcelable in order to retrieve the current configuration.
     *
     * @return The current configuration (after update). All fields known to
     *         the HAL must be populated.
     * @param config The configuration to set. Any number of fields may be left
     *               uninitialized.
     * @throws EX_UNSUPPORTED_OPERATION If telephony is not supported.
     * @throws EX_ILLEGAL_ARGUMENT If the requested combination of parameter
     *                             values is invalid.
     */
    TelecomConfig setTelecomConfig(in TelecomConfig config);
}
