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

import android.hardware.audio.common.SinkMetadata;
import android.hardware.audio.common.SourceMetadata;
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
import android.media.audio.common.AudioConfig;
import android.media.audio.common.AudioDeviceDescription;
import android.media.audio.common.AudioMode;
import android.media.audio.common.AudioSource;

/**
 * Defines all parameters supported by the effect instance.
 *
 * There are three groups of parameters:
 * 1. Common parameters are essential parameters, MUST pass to effects at open() interface.
 * 2. Parameters defined for a specific effect type.
 * 3. Extension parameters ParcelableHolder can be used for vendor effect definition.
 *
 * All parameter settings must be inside the range of Capability.Range.$EffectType$ definition. If
 * an effect implementation doesn't have limitation for a parameter, then don't define any effect
 * range.
 *
 * All parameters are get-able, if any parameter doesn't support set, effect implementation should
 * report the supported range for this parameter as range.min > range.max. If no support range is
 * defined for a parameter, it means this parameter doesn't have any limitation.
 *
 */
@VintfStability
union Parameter {
    /**
     * Client can pass in Parameter.Id with the corresponding tag value in IEffect.getParameter()
     * call to get android.hardware.audio.effect.Parameter.
     *
     * As an example, if a client want to get audio.hardware.audio.effect.Specific.Equalizer, the
     * value of Id should be audio.hardware.audio.effect.Parameter.Specific.equalizer.
     */
    @VintfStability
    union Id {
        /**
         * Parameter tag defined for vendor effects. Use VendorExtension here so it's possible to
         * pass customized information.
         */
        VendorExtension vendorEffectTag;
        /**
         * Parameter tag defined for nested parameters. Can be used to get any parameter defined in
         * nested Union structure.
         *
         * Example:
         * To get BassBoost strength in param from effectInstance:
         *  IEffect effectInstance;
         *  Parameter param;
         *  BassBoost::Id bassId = BassBoost::Id::make<BassBoost::Id::tag>(BassBoost::strengthPm);
         *  Parameter::Id id = Parameter::Id::make<Parameter::Id::bassBoostTag>(bassId);
         *  effectInstance.getParameter(id, &param);
         *
         */
        AcousticEchoCanceler.Id acousticEchoCancelerTag;
        AutomaticGainControlV1.Id automaticGainControlV1Tag;
        AutomaticGainControlV2.Id automaticGainControlV2Tag;
        BassBoost.Id bassBoostTag;
        Downmix.Id downmixTag;
        DynamicsProcessing.Id dynamicsProcessingTag;
        EnvironmentalReverb.Id environmentalReverbTag;
        Equalizer.Id equalizerTag;
        HapticGenerator.Id hapticGeneratorTag;
        LoudnessEnhancer.Id loudnessEnhancerTag;
        NoiseSuppression.Id noiseSuppressionTag;
        PresetReverb.Id presetReverbTag;
        Virtualizer.Id virtualizerTag;
        Visualizer.Id visualizerTag;
        Volume.Id volumeTag;
        /**
         * Non-nested parameter tag. Can be used to get any parameter defined in Union Parameter
         * directly.
         */
        Parameter.Tag commonTag;

        /**
         * Parameter tag defined for Spatializer parameters.
         */
        Spatializer.Id spatializerTag;
    }

    /**
     * Common parameters MUST be supported by all effect implementations.
     */
    @VintfStability
    parcelable Common {
        /**
         * Type of Audio device.
         */
        int session;
        /**
         * I/O Handle.
         */
        int ioHandle;
        /**
         * Input config.
         */
        AudioConfig input;
        /**
         * Output config.
         */
        AudioConfig output;
    }
    Common common;

    /**
     * Used by audio framework to set the device type(s) to effect engine.
     * Effect engine must apply all AudioDeviceDescription in the list.
     * Effect must implement setParameter(deviceDescription) if Flags.deviceIndication set to true.
     */
    AudioDeviceDescription[] deviceDescription;

    /**
     * Used by audio framework to set the audio mode to effect engine.
     * Effect must implement setParameter(mode) if Flags.audioModeIndication set to true.
     */
    AudioMode mode;

    /**
     * Used by audio framework to set the audio source to effect engine.
     * Effect must implement setParameter(source) if Flags.audioSourceIndication set to true.
     */
    AudioSource source;

    /**
     * Used by audio framework to indicate whether the playback thread the effect is attached to is
     * offloaded or not.
     */
    boolean offload;

    /**
     * The volume gain for left and right channel, left and right equals to same value if it's mono.
     */
    @VintfStability
    parcelable VolumeStereo {
        float left;
        float right;
    }
    /**
     * Used by audio framework to delegate volume control to effect engine.
     * Effect must implement setParameter(volume) if Flags.volume set to Volume.IND.
     */
    VolumeStereo volumeStereo;

    /**
     * Parameters MUST be supported by a Specific type of effect.
     */
    @VintfStability
    union Specific {
        VendorExtension vendorEffect;
        AcousticEchoCanceler acousticEchoCanceler;
        AutomaticGainControlV1 automaticGainControlV1;
        AutomaticGainControlV2 automaticGainControlV2;
        BassBoost bassBoost;
        Downmix downmix;
        DynamicsProcessing dynamicsProcessing;
        EnvironmentalReverb environmentalReverb;
        Equalizer equalizer;
        HapticGenerator hapticGenerator;
        LoudnessEnhancer loudnessEnhancer;
        NoiseSuppression noiseSuppression;
        PresetReverb presetReverb;
        Virtualizer virtualizer;
        Visualizer visualizer;
        Volume volume;
        Spatializer spatializer;
    }
    Specific specific;

    /**
     * SinkMetadata defines the metadata of record AudioTracks which the effect instance associate
     * with.
     * The effect engine is required to set Flags.sinkMetadataIndication to true if it wants to
     * receive sinkMetadata update from the audio framework.
     */
    SinkMetadata sinkMetadata;

    /**
     * SourceMetadata defines the metadata of playback AudioTracks which the effect instance
     * associate with.
     * The effect engine is required to set Flags.sourceMetadataIndication to true if it wants to
     * receive sourceMetadata update from the audio framework.
     */
    SourceMetadata sourceMetadata;
}
