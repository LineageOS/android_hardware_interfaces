/*
 * Copyright (C) 2023 The Android Open Source Project
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

package android.hardware.audio.effect;

import android.hardware.audio.effect.VendorExtension;
import android.media.audio.common.AudioChannelLayout;
import android.media.audio.common.HeadTracking;
import android.media.audio.common.Spatialization;

/**
 * Union representing parameters for audio spatialization effects.
 *
 * Sound spatialization simulates sounds around the listener as if they were emanating from virtual
 * positions based on the original recording.
 * For more details, refer to the documentation:
 * https://developer.android.com/reference/android/media/Spatializer.
 *
 * android.hardware.audio.effect.Spatializer specifies parameters for the implementation of audio
 * spatialization effects.
 *
 * A Spatializer implementation must report its supported parameter ranges using Capability.Range.
 * spatializer.
 */
@VintfStability
union Spatializer {
    /**
     * Parameter tag to identify the parameters for getParameter().
     */
    @VintfStability
    union Id {
        VendorExtension vendorExtensionTag;
        Spatializer.Tag commonTag;
    }

    /**
     * Vendor extension implementation for additional parameters.
     */
    VendorExtension vendor;

    /**
     * List of supported input channel layouts.
     */
    AudioChannelLayout[] supportedChannelLayout;

    /**
     * Level of spatialization.
     */
    Spatialization.Level spatializationLevel;

    /**
     * Spatialization mode, Binaural or Transaural for example.
     */
    Spatialization.Mode spatializationMode;

    /**
     * Identifies the head tracking sensor using its unique sensor ID.
     * The value corresponds to android.hardware.sensors.SensorInfo.sensorHandle.
     */
    int headTrackingSensorId;

    /**
     * Head tracking mode for spatialization.
     */
    HeadTracking.Mode headTrackingMode;

    /**
     * Head tracking sensor connection mode for spatialization.
     */
    HeadTracking.ConnectionMode headTrackingConnectionMode;

    /**
     * Headtracking sensor data.
     */
    HeadTracking.SensorData headTrackingSensorData;

    /**
     * Spatialized channel layouts.
     * A spatialized channel layout is one where each virtual speaker position is rendered
     * at its corresponding virtual position, and is not downmixed with any other.
     * For instance if a spatializer is only capable of distinct positions for 5.1, it would only
     * return 5.1:
     *  - the list wouldn't include 4.0, because that mask is "contained" within 5.1
     *  - the list wouldn't include 7.1 (and so on) because the side and rear channels would be
     *     downmixed together.
     * Another example is a spatializer that can only spatialize up to 9 channels (not counting .1)
     * and that supports 5.1.4, and 7.1.2, the list should include both.
     * The values must also be part of the values reported by supportedChannelLayout.
     * The array containing the values cannot be empty.
     */
    AudioChannelLayout[] spatializedChannelLayout;
}
