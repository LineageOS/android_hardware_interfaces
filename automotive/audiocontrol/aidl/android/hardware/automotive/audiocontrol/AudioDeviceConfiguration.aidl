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

import android.hardware.automotive.audiocontrol.RoutingDeviceConfiguration;

/**
 * Use to configure audio configurations at boot up time.
 */
@JavaDerive(equals=true, toString=true)
@VintfStability
parcelable AudioDeviceConfiguration {
    /**
     * Use to configure audio device routing mechanism
     */
    RoutingDeviceConfiguration routingConfig;

    /**
     * Use to configure core audio volume usage in car audio service
     */
    boolean useCoreAudioVolume;

    /**
     * Use to determine if HAL ducking signal should be sent to audio control HAL from car audio
     * service
     */
    boolean useHalDuckingSignals;

    /**
     * Use to determine if HAL volume signal should be sent to audio control HAL from car audio
     * service
     */
    boolean useCarVolumeGroupMuting;
}
