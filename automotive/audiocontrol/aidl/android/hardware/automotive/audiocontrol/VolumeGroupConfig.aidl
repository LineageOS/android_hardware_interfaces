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

import android.hardware.automotive.audiocontrol.DeviceToContextEntry;
import android.hardware.automotive.audiocontrol.VolumeActivationConfiguration;

/**
 * Encapsulates the audio volume grouping for audio zone config.
 */
@JavaDerive(equals=true, toString=true)
@VintfStability
parcelable VolumeGroupConfig {
    /**
     * Value indicating the volume group is not assigned an ID.
     */
    const int UNASSIGNED_ID = -1;

    /**
     * Audio zone group name.
     *
     * <p>Must be non-empty if using configurable audio policy engine volume management,
     * {@see AudioDeviceConfiguration#useCoreAudioVolume} for details. For non-core volume group
     * management this can be left empty or use for debugging purposes.
     */
    String name;

    /**
     * Audio zone group id.
     *
     * <p>Must be set if using configurable audio policy engine volume management, can be
     * {@code #UNASSIGNED_ID} otherwise. See {@code AudioDeviceConfiguration#useCoreAudioVolume}
     * for details.
     */
    int id = UNASSIGNED_ID;

    /**
     * Entries of audio device to audio context that are managed similarly for this volume group.
     */
    List<DeviceToContextEntry> carAudioRoutes;

    /**
     * Optional volume activation configuration that should be used for this volume group.
     */
    @nullable VolumeActivationConfiguration activationConfiguration;
}
