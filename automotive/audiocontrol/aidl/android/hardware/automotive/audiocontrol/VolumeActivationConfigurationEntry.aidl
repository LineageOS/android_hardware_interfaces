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

import android.hardware.automotive.audiocontrol.VolumeInvocationType;

/**
 * Audio activiation volume configuration entry.
 *
 * <p> The entry can defined both the minimum and maximum activation values or only one. The latter
 * allows activations to occur only on the minimum value or maximum value as configured.
 */
@JavaDerive(equals=true, toString=true)
@VintfStability
parcelable VolumeActivationConfigurationEntry {
    /**
     * Default maximum activation value.
     */
    const int DEFAULT_MAX_ACTIVATION_VALUE = 100;

    /**
     * Default minimum activation value.
     */
    const int DEFAULT_MIN_ACTIVATION_VALUE = 0;

    /**
     * Activation type, should be one of:
     *  ON_PLAYBACK_CHANGED, ON_SOURCE_CHANGED, ON_BOOT
     */
    VolumeInvocationType type = VolumeInvocationType.ON_PLAYBACK_CHANGED;

    /**
     * Max activation percentage between {@code DEFAULT_MIN_ACTIVATION_VALUE} to
     * {@code DEFAULT_MAX_ACTIVATION_VALUE} percen.
     *
     * <p>The value should be {@code DEFAULT_MAX_ACTIVATION_VALUE} if max activation should not
     * apply.
     */
    int maxActivationVolumePercentage = DEFAULT_MAX_ACTIVATION_VALUE;

    /**
     * Min activation percentage between {@code DEFAULT_MIN_ACTIVATION_VALUE} to
     * {@code DEFAULT_MAX_ACTIVATION_VALUE} percent.
     *
     * <p>The value should be {@code DEFAULT_MIN_ACTIVATION_VALUE} if min activation should not
     * apply.
     */
    int minActivationVolumePercentage = DEFAULT_MIN_ACTIVATION_VALUE;
}
