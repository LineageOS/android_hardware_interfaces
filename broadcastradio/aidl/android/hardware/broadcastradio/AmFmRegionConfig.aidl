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

package android.hardware.broadcastradio;

import android.hardware.broadcastradio.AmFmBandRange;

/**
 * Regional configuration for AM/FM.
 *
 * <p>For hardware capabilities check (with full=true when calling
 * {@link IBroadcastRadio#getAmFmRegionConfig}), HAL implementation fills
 * entire supported range of frequencies and features.
 *
 * When checking current configuration, at most one bit in each bitset
 * can be set.
 */
@VintfStability
@JavaDerive(equals=true, toString=true)
parcelable AmFmRegionConfig {
    /**
     * Noth D50 and D75 are FM de-emphasis filter supported or configured.
     *
     * Both might be set for hardware capabilities check (with full={@code true}
     * when calling getAmFmRegionConfig), but exactly one for specific region
     * settings.
     */
    const int DEEMPHASIS_D50 = 1 << 0;

    const int DEEMPHASIS_D75 = 1 << 1;

    /**
     * Both RDS and RBDS are supported or configured RDS variants.
     *
     * Both might be set for hardware capabilities check (with full={@code true}
     * when calling getAmFmRegionConfig), but only one (or none) for specific
     * region settings.
     *
     * RDS is Standard variant, used everywhere except North America.
     */
    const int RDS = 1 << 0;

    /**
     * Variant used in North America (see RDS).
     */
    const int RBDS = 1 << 1;

    /**
     * All supported or configured AM/FM bands.
     *
     * AM/FM bands are identified by frequency value
     * (see {@link IdentifierType#AMFM_FREQUENCY_KHZ}).
     *
     * With typical configuration, it's expected to have two frequency ranges
     * for capabilities check (AM and FM) and four ranges for specific region
     * configuration (AM LW, AM MW, AM SW, FM).
     */
    AmFmBandRange[] ranges;

    /**
     * De-emphasis filter supported/configured.
     *
     * It can be a combination of de-emphasis values ({@link #DEEMPHASIS_D50} and
     * {@link #DEEMPHASIS_D75}).
     */
    int fmDeemphasis;

    /**
     * RDS/RBDS variant supported/configured.
     *
     * It can be a combination of RDS values ({@link #RDS} and {@link #RBDS}).
     */
    int fmRds;
}
