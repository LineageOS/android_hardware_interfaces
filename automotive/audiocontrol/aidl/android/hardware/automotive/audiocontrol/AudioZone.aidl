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

import android.hardware.automotive.audiocontrol.AudioZoneConfig;
import android.hardware.automotive.audiocontrol.AudioZoneContext;
import android.media.audio.common.AudioHalProductStrategy;
import android.media.audio.common.AudioPort;

/**
 * Encapsulates the audio configurations for each audio zone
 */
@JavaDerive(equals=true, toString=true)
@VintfStability
parcelable AudioZone {
    /**
     * Value indicating the occupant zone is not assigned.
     */
    const int UNASSIGNED_OCCUPANT = -1;

    /**
     * Audio zone name, only use for debug purposes.
     *
     * <p>If present it must be non-empty otherwise car audio service will construct a name
     * based on audio zone id.
     */
    String name;

    /**
     * Audio zone id use to distiguish between the different audio zones for
     * volume management, fade, and min/max activation management.
     *
     * <p>Value must start at {@link AudioHalProductStrategy#ZoneId#DEFAULT} for the primary zone
     * and increase for each different zone. Zone id must also not repeat for different zones.
     */
    int id = AudioHalProductStrategy.ZoneId.DEFAULT;

    /**
     * Occupant zone id that should be mapped to this audio zone.
     *
     * <p>For audio zones not mapped to an occupant zone use UNASSIGNED_OCCUPANT
     */
    int occupantZoneId = UNASSIGNED_OCCUPANT;

    /**
     * Car audio context which can be used in the audio zone
     */
    AudioZoneContext audioZoneContext;

    /**
     * List of car audio configurations
     */
    List<AudioZoneConfig> audioZoneConfigs;

    /**
     * List of input audio devices used for this zone
     */
    List<AudioPort> inputAudioDevices;
}
