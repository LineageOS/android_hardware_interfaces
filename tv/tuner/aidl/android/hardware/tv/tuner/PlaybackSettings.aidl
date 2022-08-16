/*
 * Copyright 2021 The Android Open Source Project
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

package android.hardware.tv.tuner;

import android.hardware.tv.tuner.DataFormat;

/**
 * The Setting for the playback in DVR.
 * @hide
 */
@VintfStability
parcelable PlaybackSettings {
    /**
     * Register for interested PlaybackStatus events so that the HAL can send these
     * PlaybackStatus events back to client.
     */
    int statusMask;

    /**
     * Unused space size in bytes in the playback. The HAL uses it to trigger
     * InputStatus::SPACE_ALMOST_EMPTY.
     */
    long lowThreshold;

    /**
     * Unused space size in bytes in the playback. The HAL uses it to trigger
     * InputStatus::SPACE_ALMOST_FULL.
     */
    long highThreshold;

    /**
     * The data format in the playback.
     */
    DataFormat dataFormat = DataFormat.UNDEFINED;

    /**
     * The packet size in bytes in the playback.
     */
    long packetSize;
}
