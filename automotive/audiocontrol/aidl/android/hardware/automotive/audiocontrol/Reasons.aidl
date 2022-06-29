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

package android.hardware.automotive.audiocontrol;

/**
 * Enum to identify the reason(s) of
 * {@link android.hardware.automotive.audiocontrol.AudioGainConfigInfo} changed event
 */
@Backing(type="int")
@VintfStability
enum Reasons {
    /**
     * Magic Key Code (may be SWRC button combination) to force muting all audio sources.
     * This may be used for example in case of cyber attach to ensure driver can safely drive back
     * to garage to restore sw.
     */
    FORCED_MASTER_MUTE = 0x1,
    /**
     * Reports a mute request outside the IVI (Android) system.
     * It may target to mute the list of
     * {@link android.hardware.automotive.audiocontrol.AudioGainConfigInfo}.
     * A focus request may also be reported in addition if the use case that initiates the mute
     * has matching {@link android.hardware.automotive.audiocontrol.PlaybackTrackMetadata}
     * For regulation issue, the action of mute could be managed by HAL itself.
     */
    REMOTE_MUTE = 0x2,
    /**
     * Reports a mute initiated by the TCU. It may be applied to all audio source (no
     * associated {@link android.hardware.automotive.audiocontrol.AudioGainConfigInfo} reported, or
     * it may target to mute only the given list of ports.
     * A focus request may also be reported in addition.
     * For regulation issue, the action of mute could be managed by HAL itself.
     */
    TCU_MUTE = 0x4,
    /**
     * Reports a duck due to ADAS use case. A focus request may also be reported in addition.
     * For regulation issue, the action of duck could be managed by HAL itself.
     * It gives a chance to CarAudioService to decide whether contextual volume change may be
     * applied from the ducked index base or not.
     */
    ADAS_DUCKING = 0x8,
    /**
     * Reports a duck due to navigation use case. It gives a chance to CarAudioService to decide
     * whether contextual volume change may be applied from the ducked index base or not.
     */
    NAV_DUCKING = 0x10,
    /**
     * Some device projection stack may send signal to IVI to duck / unduck main audio stream.
     * In this case, Contextual Volume Policy may be adapted to control the alternate / secondary
     * audio stream.
     */
    PROJECTION_DUCKING = 0x20,
    /**
     * When the amplifier is overheating, it may be recovered by limiting the volume.
     */
    THERMAL_LIMITATION = 0x40,
    /**
     * Before the system enters suspend, it may ensure while exiting suspend or during cold boot
     * that the volume is limited to prevent from sound explosion.
     */
    SUSPEND_EXIT_VOL_LIMITATION = 0x80,
    /**
     * When using an external amplifier, it may be required to keep volume in sync and have
     * asynchronous notification of effective volume change.
     */
    EXTERNAL_AMP_VOL_FEEDBACK = 0x100,
    /**
     * For other OEM use.
     */
    OTHER = 0x80000000,
}
