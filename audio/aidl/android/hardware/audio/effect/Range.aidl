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

import android.hardware.audio.effect.AcousticEchoCanceler;
import android.hardware.audio.effect.AutomaticGainControlV1;
import android.hardware.audio.effect.AutomaticGainControlV2;
import android.hardware.audio.effect.BassBoost;
import android.hardware.audio.effect.Downmix;
import android.hardware.audio.effect.DynamicsProcessing;
import android.hardware.audio.effect.EnvironmentalReverb;
import android.hardware.audio.effect.Equalizer;
import android.hardware.audio.effect.HapticGenerator;
import android.hardware.audio.effect.LoudnessEnhancer;
import android.hardware.audio.effect.NoiseSuppression;
import android.hardware.audio.effect.PresetReverb;
import android.hardware.audio.effect.Spatializer;
import android.hardware.audio.effect.VendorExtension;
import android.hardware.audio.effect.Virtualizer;
import android.hardware.audio.effect.Visualizer;
import android.hardware.audio.effect.Volume;

/**
 * Define the supported range of effect parameters.
 * Effect implementation must report the supported range of parameter/capability if there is any
 * limitation.
 * Range of each effect type is defined with a vector, because each $EffectType$Range can only
 * describe the range of one parameter. For example, Range::AcousticEchoCancelerRange can only
 * describe one of vendor, echoDelayUs, and mobileMode.
 *
 * If an effect implementation needs to define the valid range for a certain parameter, it can
 * write the minimum/maximum supported value to the corresponding effect range, and add the range
 * to vector of this effect type.
 * Say if an AcousticEchoCanceler implementation wants to define the supported range of echoDelayUs
 * to [0, 500], then the Capability range should include an item in acousticEchoCanceler list:
 * std::vector<Range::AcousticEchoCancelerRange> kRanges = {
 *       MAKE_RANGE(AcousticEchoCanceler, echoDelayUs, 0, 500)};
 *
 * For a more complex example, if a DynamicsProcessing implementation wants to define the
 * supported range of preEqBand channel to [0, 1], band index to [0, 5], and cutoffFrequencyHz to
 * [220, 20000]:
 * Range::DynamicsProcessingRange kRange = {
 *   .min = DynamicsProcessing::make<DynamicsProcessing::preEqBand>(
 *           {EqBandConfig({.channel = 0,
 *                          .band = 0,
 *                          .enable = false,
 *                          .cutoffFrequencyHz = 220,
 *                          .gainDb = std::numeric_limits<float>::min()})}),
 *   .max = DynamicsProcessing::make<DynamicsProcessing::preEqBand>(
 *           {EqBandConfig({.channel = 1,
 *                          .band = 5,
 *                          .enable = true,
 *                          .cutoffFrequencyHz = 20000,
 *                          .gainDb = std::numeric_limits<float>::max()})})};
 *
 * For get only parameters, the effect implementation must define an invalid range (min > max), to
 * indicate no parameter from the effect client can be accepted for setting. The effect
 * implementation must return EX_ILLEGAL_ARGUMENT if it receives any setParameter call for a get
 * only parameter.
 * As an example, the get-only parameter Virtualizer.speakerAngles can be defined with:
 *   Range::VirtualizerRange kRanges = {
 *       MAKE_RANGE(Virtualizer, speakerAngles,
 *                  {Virtualizer::ChannelAngle({.channel = 1},
 *                  {Virtualizer::ChannelAngle({.channel = 0})};
 *
 * For a capability definition (which is also get only), the effect implementation must define a
 * range with min == max, to indicate this is a fixed capability which shouldn't set by client.
 * As an example, the Equalizer presets capability can be defined with:
 * std::vector<Equalizer::Preset> kPresets = {
 *       {0, "Normal"},      {1, "Classical"}, {2, "Dance"}, {3, "Flat"}, {4, "Folk"},
 *       {5, "Heavy Metal"}, {6, "Hip Hop"},   {7, "Jazz"},  {8, "Pop"},  {9, "Rock"}};
 * Range::EqualizerRange kRanges =
 *      MAKE_RANGE(Equalizer, presets, EqualizerSw::kPresets, EqualizerSw::kPresets);
 *
 * For enum capability, it's necessary to either list a range, or explicitly list out all enums in
 * the Range definition, see PresetReverb.supportedPresets as example.
 *
 * The effect implementation must return EX_ILLEGAL_ARGUMENT if:
 * 1. receive any setParameter call for get only parameter (min > max).
 * 2. receive any setParameter call for capability parameter definition (min == max).
 * 3. receive any setParameter call for parameters not in the range of [min, max].
 *
 */
@VintfStability
union Range {
    @VintfStability
    parcelable AcousticEchoCancelerRange {
        AcousticEchoCanceler min;
        AcousticEchoCanceler max;
    }

    @VintfStability
    parcelable AutomaticGainControlV1Range {
        AutomaticGainControlV1 min;
        AutomaticGainControlV1 max;
    }

    @VintfStability
    parcelable AutomaticGainControlV2Range {
        AutomaticGainControlV2 min;
        AutomaticGainControlV2 max;
    }

    @VintfStability
    parcelable BassBoostRange {
        BassBoost min;
        BassBoost max;
    }

    @VintfStability
    parcelable DownmixRange {
        Downmix min;
        Downmix max;
    }

    @VintfStability
    parcelable DynamicsProcessingRange {
        DynamicsProcessing min;
        DynamicsProcessing max;
    }

    @VintfStability
    parcelable EnvironmentalReverbRange {
        EnvironmentalReverb min;
        EnvironmentalReverb max;
    }

    @VintfStability
    parcelable EqualizerRange {
        Equalizer min;
        Equalizer max;
    }

    @VintfStability
    parcelable HapticGeneratorRange {
        HapticGenerator min;
        HapticGenerator max;
    }

    @VintfStability
    parcelable LoudnessEnhancerRange {
        LoudnessEnhancer min;
        LoudnessEnhancer max;
    }

    @VintfStability
    parcelable NoiseSuppressionRange {
        NoiseSuppression min;
        NoiseSuppression max;
    }

    @VintfStability
    parcelable PresetReverbRange {
        PresetReverb min;
        PresetReverb max;
    }

    @VintfStability
    parcelable SpatializerRange {
        Spatializer min;
        Spatializer max;
    }

    @VintfStability
    parcelable VendorExtensionRange {
        VendorExtension min;
        VendorExtension max;
    }

    @VintfStability
    parcelable VirtualizerRange {
        Virtualizer min;
        Virtualizer max;
    }

    @VintfStability
    parcelable VisualizerRange {
        Visualizer min;
        Visualizer max;
    }

    @VintfStability
    parcelable VolumeRange {
        Volume min;
        Volume max;
    }

    /**
     * The vector of range defined for parameters of each effect implementation.
     * Each element of the vector represents the min and max range of a parameter, so the size of
     * vector is the number of parameter ranges defined for this effect implementation.
     *
     * The effect implementation must not have duplicated parameter range definition in the vector.
     * Client side must go though the vector and make sure the parameter setting to effect
     * implementation is in valid range.
     */
    VendorExtensionRange[] vendorExtension = {};
    AcousticEchoCancelerRange[] acousticEchoCanceler;
    AutomaticGainControlV1Range[] automaticGainControlV1;
    AutomaticGainControlV2Range[] automaticGainControlV2;
    BassBoostRange[] bassBoost;
    DownmixRange[] downmix;
    DynamicsProcessingRange[] dynamicsProcessing;
    EnvironmentalReverbRange[] environmentalReverb;
    EqualizerRange[] equalizer;
    HapticGeneratorRange[] hapticGenerator;
    LoudnessEnhancerRange[] loudnessEnhancer;
    NoiseSuppressionRange[] noiseSuppression;
    PresetReverbRange[] presetReverb;
    VirtualizerRange[] virtualizer;
    VisualizerRange[] visualizer;
    VolumeRange[] volume;
    SpatializerRange[] spatializer;
}
