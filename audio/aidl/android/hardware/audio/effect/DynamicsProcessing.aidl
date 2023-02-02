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
 * All parameter settings must be inside the range of Capability.Range.dynamicsProcessing definition
 * if the definition for the corresponding parameter tag exist. See more detals about Range in
 * Range.aidl.
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
     * Stage enablement configuration.
     */
    @VintfStability
    parcelable StageEnablement {
        /**
         * True if stage is in use.
         */
        boolean inUse;
        /**
         * Number of bands configured for this stage. Must be positive when inUse is true.
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
         * Preferred processing duration in milliseconds (ms). Must not be negative, 0 means no
         * preference.
         */
        float preferredProcessingDurationMs;
        /**
         * PreEq stage (Multi-band Equalizer) configuration.
         */
        StageEnablement preEqStage;
        /**
         * PostEq stage (Multi-band Equalizer) configuration.
         */
        StageEnablement postEqStage;
        /**
         * MBC stage (Multi-band Compressor) configuration.
         */
        StageEnablement mbcStage;
        /**
         * True if Limiter stage is in use.
         */
        boolean limiterInUse;
    }

    /**
     * Enablement configuration for a specific channel.
     */
    @VintfStability
    parcelable ChannelConfig {
        /**
         * Channel index. Must not be negative, and not exceed the channel count calculated from
         * Parameter.common.input.base.channelMask.
         */
        int channel;
        /**
         * Channel enablement configuration. Can not be true if corresponding stage is not in use.
         */
        boolean enable;
    }

    /**
     * Equalizer band configuration for a specific channel and band.
     */
    @VintfStability
    parcelable EqBandConfig {
        /**
         * Channel index. Must not be negative, and not exceed the channel count calculated from
         * Parameter.common.input.base.channelMask.
         */
        int channel;
        /**
         * Band index, must in the range of [0, bandCount-1].
         */
        int band;
        /**
         * True if EQ band is enabled.
         * If EngineArchitecture EQ stage inUse was set to false, then enable can not be set to
         * true.
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
    }

    /**
     * MBC configuration for a specific channel and band.
     */
    @VintfStability
    parcelable MbcBandConfig {
        /**
         * Channel index. Must not be negative, and not exceed the channel count calculated from
         * Parameter.common.input.base.channelMask.
         */
        int channel;
        /**
         * Band index. Must be in the range of [0, bandCount-1].
         */
        int band;
        /**
         * True if MBC band is enabled.
         * If EngineArchitecture MBC inUse was set to false, then enable here can not be set to
         * true.
         */
        boolean enable;
        /**
         * Topmost frequency number (in Hz) this band will process.
         */
        float cutoffFrequencyHz;
        /**
         * Attack Time for compressor in milliseconds (ms). Must not be negative.
         */
        float attackTimeMs;
        /**
         * Release Time for compressor in milliseconds (ms). Must not be negative.
         */
        float releaseTimeMs;
        /**
         * Compressor ratio (N:1) (input:output). Must not be negative.
         */
        float ratio;
        /**
         * Compressor threshold measured in decibels (dB) from 0 dB Full Scale (dBFS).  Must not be
         * positive.
         */
        float thresholdDb;
        /**
         * Width in decibels (dB) around compressor threshold point. Must not be negative.
         */
        float kneeWidthDb;
        /**
         * Noise gate threshold in decibels (dB) from 0 dB Full Scale (dBFS). Must not be positive.
         */
        float noiseGateThresholdDb;
        /**
         * Expander ratio (1:N) (input:output) for signals below the Noise Gate Threshold. Must not
         * be negative.
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
         * Channel index. Must not be negative, and not exceed the channel count calculated from
         * Parameter.common.input.base.channelMask.
         */
        int channel;
        /**
         * True if Limiter band is enabled.
         * If EngineArchitecture limiterInUse was set to false, then enable can not be set to true.
         */
        boolean enable;
        /**
         * Index of group assigned to this Limiter. Only limiters that share the same linkGroup
         * index will react together.
         */
        int linkGroup;
        /**
         * Attack Time for compressor in milliseconds (ms). Must not be negative.
         */
        float attackTimeMs;
        /**
         * Release Time for compressor in milliseconds (ms). Must not be negative.
         */
        float releaseTimeMs;
        /**
         * Compressor ratio (N:1) (input:output). Must not be negative.
         */
        float ratio;
        /**
         * Compressor threshold measured in decibels (dB) from 0 dB Full Scale (dBFS). Must not be
         * positive.
         */
        float thresholdDb;
        /**
         * Gain applied to the signal AFTER compression in dB.
         */
        float postGainDb;
    }

    /**
     * Input gain for a channel (specified by the channel index).
     */
    @VintfStability
    parcelable InputGain {
        /**
         * Channel index. Must not be negative, and not exceed the channel count calculated from
         * Parameter.common.input.base.channelMask.
         */
        int channel;
        /**
         * Gain applied to the input signal in decibels (dB). 0 dB means no change in level.
         */
        float gainDb;
    }

    /**
     * Effect engine architecture.
     */
    EngineArchitecture engineArchitecture;
    /**
     * PreEq stage per channel configuration. Only valid when pre EQ stage inUse is true.
     */
    ChannelConfig[] preEq;
    /**
     * PostEq stage per channel configuration. Only valid when post EQ stage inUse is true.
     */
    ChannelConfig[] postEq;
    /**
     * PreEq stage per band configuration. Only valid when pre EQ stage inUse is true.
     */
    EqBandConfig[] preEqBand;
    /**
     * PostEq stage per band configuration. Only valid when post EQ stage inUse is true.
     */
    EqBandConfig[] postEqBand;
    /**
     * MBC stage per channel configuration. Only valid when MBC stage inUse is true.
     */
    ChannelConfig[] mbc;
    /**
     * PostEq stage per band configuration. Only valid when MBC stage inUse is true.
     */
    MbcBandConfig[] mbcBand;
    /**
     * Limiter stage configuration. Only valid when limiter stage inUse is true.
     */
    LimiterConfig[] limiter;
    /**
     * Input gain factor.
     */
    InputGain[] inputGain;
}
