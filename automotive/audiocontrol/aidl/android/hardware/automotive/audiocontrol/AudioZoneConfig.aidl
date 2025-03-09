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

import android.hardware.automotive.audiocontrol.AudioZoneFadeConfiguration;
import android.hardware.automotive.audiocontrol.VolumeGroupConfig;

/**
 * Encapsulates the audio zone config information
 */
@JavaDerive(equals=true, toString=true)
@VintfStability
parcelable AudioZoneConfig {
    /**
     * Audio zone config name
     *
     * <p>Must be non-empty and unique among the configurations within a zone.
     */
    String name;

    /**
     * Determines if the audio configuration is the default configuration.
     *
     * <p>There can only be a single default configuration per zone.
     */
    boolean isDefault;

    /**
     * List car volume group that should be managed within this configuration
     */
    List<VolumeGroupConfig> volumeGroups;

    /**
     * Car audio zone fade configuration
     */
    @nullable AudioZoneFadeConfiguration fadeConfiguration;
}
