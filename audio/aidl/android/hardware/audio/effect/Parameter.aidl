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

import android.hardware.audio.effect.BassBoost;
import android.hardware.audio.effect.Downmix;
import android.hardware.audio.effect.DynamicsProcessing;
import android.hardware.audio.effect.Equalizer;
import android.hardware.audio.effect.HapticGenerator;
import android.hardware.audio.effect.LoudnessEnhancer;
import android.hardware.audio.effect.Reverb;
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
         * Parameter tag defined for vendor effects. Use int here so there is flexibility for vendor
         * to define different tag.
         */
        int vendorEffectTag;
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
        BassBoost.Id bassBoostTag;
        Downmix.Id downmixTag;
        DynamicsProcessing.Id dynamicsProcessingTag;
        Equalizer.Id equalizerTag;
        HapticGenerator.Id hapticGeneratorTag;
        LoudnessEnhancer.Id loudnessEnhancerTag;
        Reverb.Id reverbTag;
        Virtualizer.Id virtualizerTag;
        Visualizer.Id visualizerTag;
        Volume.Id volumeTag;
        /**
         * Non-nested parameter tag. Can be used to get any parameter defined in Union Parameter
         * directly.
         */
        Parameter.Tag commonTag;
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
     * Used by audio framework to set the device type to effect engine.
     * Effect must implement setParameter(device) if Flags.deviceIndication set to true.
     */
    AudioDeviceDescription deviceDescription;
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
        BassBoost bassBoost;
        Downmix downmix;
        DynamicsProcessing dynamicsProcessing;
        Equalizer equalizer;
        LoudnessEnhancer loudnessEnhancer;
        HapticGenerator hapticGenerator;
        Reverb reverb;
        Virtualizer virtualizer;
        Visualizer visualizer;
        Volume volume;
    }
    Specific specific;
}
