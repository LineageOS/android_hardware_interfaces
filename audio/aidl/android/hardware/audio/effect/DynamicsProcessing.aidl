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
 * DynamicsProcessing specific definitions.
 *
 * All parameters defined in union DynamicsProcessing must be gettable and settable. The
 * capabilities defined in DynamicsProcessing.Capability can only acquired with
 * IEffect.getDescriptor() and not settable.
 */
@VintfStability
union DynamicsProcessing {
    /**
     * Effect parameter tag to identify the parameters for getParameter().
     */
    @VintfStability
    union Id {
        int vendorExtensionTag;
        DynamicsProcessing.Tag commonTag;
    }

    /**
     * Vendor DynamicsProcessing implementation definition for additional parameters.
     */
    VendorExtension vendorExtension;

    /**
     * Capability supported by DynamicsProcessing implementation.
     */
    @VintfStability
    parcelable Capability {
        /**
         * DynamicsProcessing capability extension, vendor can use this extension in case existing
         * capability definition not enough.
         */
        ParcelableHolder extension;
    }

    /**
     * Resolution preference definition.
     */
    enum ResolutionPreference {
        /**
         * Favors frequency domain based implementation.
         */
        FAVOR_FREQUENCY_RESOLUTION,
        /**
         * Favors tme domain based implementation.
         */
        FAVOR_TIME_RESOLUTION,
    }

    /**
     * Band enablement configuration.
     */
    @VintfStability
    parcelable BandEnablement {
        /**
         * True if multi-band stage is in use.
         */
        boolean inUse;
        /**
         * Number of bands configured for this stage.
         */
        int bandCount;
    }

    /**
     * Effect engine configuration. Set the enablement of all stages.
     */
    @VintfStability
    parcelable EngineArchitecture {
        /**
         * Resolution preference.
         */
        ResolutionPreference resolutionPreference = ResolutionPreference.FAVOR_FREQUENCY_RESOLUTION;
        /**
         * Preferred frame duration in milliseconds (ms).
         */
        float preferredFrameDurationMs;
        /**
         * PreEq stage (Multi-band Equalizer) configuration.
         */
        BandEnablement preEqBand;
        /**
         * PostEq stage (Multi-band Equalizer) configuration.
         */
        BandEnablement postEqBand;
        /**
         * MBC stage (Multi-band Compressor) configuration.
         */
        BandEnablement mbcBand;
        /**
         * True if Limiter stage is in use.
         */
        boolean limiterInUse;
    }

    /**
     * Band enablement configuration for a specific channel.
     */
    @VintfStability
    parcelable BandChannelConfig {
        /**
         * Channel index.
         */
        int channel;
        /**
         * Channel index.
         */
        BandEnablement enablement;
    }

    /**
     * Equalizer band configuration for a specific channel and band.
     */
    @VintfStability
    parcelable EqBandConfig {
        /**
         * Channel index.
         */
        int channel;
        /**
         * Band index, must in the range of [0, bandCount-1].
         */
        int band;
        /**
         * True if EQ stage is enabled.
         */
        boolean enable;
        /**
         * Topmost frequency number (in Hz) this band will process.
         */
        float cutoffFrequency;
        /**
         * Gain factor in decibels (dB).
         */
        float gain;
    }

    /**
     * MBC configuration for a specific channel and band.
     */
    @VintfStability
    parcelable MbcBandConfig {
        /**
         * Channel index.
         */
        int channel;
        /**
         * Band index, must in the range of [0, bandCount-1].
         */
        int band;
        /**
         * True if MBC stage is enabled.
         */
        boolean enable;
        /**
         * Topmost frequency number (in Hz) this band will process.
         */
        float cutoffFrequencyHz;
        /**
         * Gain factor in decibels (dB).
         */
        float gainDb;
        /**
         * Attack Time for compressor in milliseconds (ms).
         */
        float attackTimeMs;
        /**
         * Release Time for compressor in milliseconds (ms).
         */
        float releaseTimeMs;
        /**
         * Compressor ratio (N:1) (input:output).
         */
        float ratio;
        /**
         * Compressor threshold measured in decibels (dB) from 0 dB Full Scale (dBFS).
         */
        float thresholdDb;
        /**
         * Width in decibels (dB) around compressor threshold point.
         */
        float kneeWidthDb;
        /**
         * Noise gate threshold in decibels (dB) from 0 dB Full Scale (dBFS).
         */
        float noiseGateThresholdDb;
        /**
         * Expander ratio (1:N) (input:output) for signals below the Noise Gate Threshold.
         */
        float expanderRatio;
        /**
         * Gain applied to the signal BEFORE the compression in dB.
         */
        float preGainDb;
        /**
         * Gain applied to the signal AFTER compression in dB.
         */
        float postGainDb;
    }

    /**
     * Limiter configuration for a specific channel.
     */
    @VintfStability
    parcelable LimiterConfig {
        /**
         * Channel index.
         */
        int channel;
        /**
         * True if Limiter stage is enabled.
         */
        boolean enable;
        /**
         * True if Limiter stage is in use.
         */
        boolean inUse;
        /**
         * Index of group assigned to this Limiter. Only limiters that share the same linkGroup
         * index will react together.
         */
        int linkGroup;
        /**
         * Attack Time for compressor in milliseconds (ms).
         */
        float attackTimeMs;
        /**
         * Release Time for compressor in milliseconds (ms).
         */
        float releaseTimeMs;
        /**
         * Compressor ratio (N:1) (input:output).
         */
        float ratio;
        /**
         * Compressor threshold measured in decibels (dB) from 0 dB Full Scale (dBFS).
         */
        float thresholdDb;
        /**
         * Gain applied to the signal AFTER compression in dB.
         */
        float postGainDb;
    }

    /**
     * Effect engine architecture.
     */
    EngineArchitecture engineArchitecture;
    /**
     * PreEq stage per channel configuration.
     */
    BandChannelConfig preEq;
    /**
     * PostEq stage per channel configuration.
     */
    BandChannelConfig postEq;
    /**
     * PreEq stage per band configuration.
     */
    EqBandConfig preEqBand;
    /**
     * PostEq stage per band configuration.
     */
    EqBandConfig postEqBand;
    /**
     * MBC stage per channel configuration.
     */
    BandChannelConfig mbc;
    /**
     * PostEq stage per band configuration.
     */
    MbcBandConfig mbcBand;
    /**
     * Limiter stage configuration.
     */
    LimiterConfig limiter;
    /**
     * Input gain factor in decibels (dB). 0 dB means no change in level.
     */
    float inputGainDb;
}
