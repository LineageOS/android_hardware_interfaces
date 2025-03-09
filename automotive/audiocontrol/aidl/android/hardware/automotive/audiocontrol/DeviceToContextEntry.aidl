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

package android.hardware.automotive.audiocontrol;

import android.media.audio.common.AudioPort;

/**
 * Encapsulates the audio context that should be route to particular device
 */
@JavaDerive(equals=true, toString=true)
@VintfStability
parcelable DeviceToContextEntry {
    /**
     * List of audio context names that should be routed to the audio device.
     *
     * <p>The names must match a {@link AudioZoneContextInfo#name} in the corresponding
     * {@link AudioZone#audioZoneContext).
     *
     * <p>Within a {@link AudioZoneConfig} a context name must not repeat among the different
     * {@link VolumeGroupConfig}. The value can repeat among different {@link AudioZoneConfig}
     * within a {@link AudioZone}.
     */
    List<String> contextNames;

    /**
     * Audio port where contexts should be routed.
     *
     * <p>For dynamic devices (OUT_HEADSET, OUT_HEADPHONE, etc.) , the audio device address can be
     * omitted since the information will be obtained at run time when the device is
     * connected/enabled.
     */
    AudioPort device;
}
