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

package android.hardware.audio.effect;

import android.hardware.audio.effect.VendorExtension;

/**
 * Visualizer specific definitions. Visualizer enables application to retrieve part of the currently
 * playing audio for visualization purpose
 *
 * All parameter settings must be inside the range of Capability.Range.visualizer definition if the
 * definition for the corresponding parameter tag exist. See more detals about Range in Range.aidl.
 *
 */
@VintfStability
union Visualizer {
    /**
     * Effect parameter tag to identify the parameters for getParameter().
     */
    @VintfStability
    union Id {
        VendorExtension vendorExtensionTag;
        Visualizer.Tag commonTag;
    }
    Id id;

    /**
     * Vendor Visualizer implementation definition for additional parameters.
     */
    VendorExtension vendor;

    /**
     * Type of scaling applied on the captured visualization data.
     */
    @VintfStability
    enum ScalingMode {
        /**
         * Defines a capture mode where amplification is applied based on the content of the
         * captured data. This is the default Visualizer mode, and is suitable for music
         * visualization.
         */
        NORMALIZED = 0,
        /**
         * Defines a capture mode where the playback volume will affect (scale) the range of the
         * captured data. A low playback volume will lead to low sample and fft values, and
         * vice-versa.
         */
        AS_PLAYED,
    }

    /**
     * Measurement modes to be performed.
     */
    @VintfStability
    enum MeasurementMode {
        /**
         * No measurements are performed.
         */
        NONE = 0,
        /**
         * Defines a measurement mode which computes the peak and RMS value in mB below the "full
         * scale", where 0mB is normally the maximum sample value (but see the note below). Minimum
         * value depends on the resolution of audio samples used by the audio framework. The value
         * of -9600mB is the minimum value for 16-bit audio systems and -14400mB or below for "high
         * resolution" systems. Values for peak and RMS can be retrieved with {@link
         * #getMeasurementPeakRms(MeasurementPeakRms)}.
         */
        PEAK_RMS,
    }

    /**
     * Get only parameter to get the current measurements.
     */
    @VintfStability
    parcelable Measurement {
        int rms;
        int peak;
    }
    Measurement measurement;

    /**
     * Get only parameter to get the latest captured samples of PCM samples (8 bits per sample).
     */
    byte[] captureSampleBuffer;

    /**
     * Used by framework to inform the visualizer about the downstream latency (audio hardware
     * driver estimated latency in milliseconds).
     *
     * Visualizer implementation must use Range.VisualizerRange to define the range of supported
     * latency.
     */
    int latencyMs;

    /**
     * Current capture size in number of samples.
     *
     * Visualizer implementation must use Range.VisualizerRange to define the range of supported
     * capture size.
     */
    int captureSamples;

    /**
     * Visualizer capture mode
     */
    ScalingMode scalingMode;

    /**
     * Visualizer measurement mode.
     */
    MeasurementMode measurementMode;
}
