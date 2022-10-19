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

import android.hardware.audio.effect.Equalizer;
import android.media.audio.common.AudioConfig;
import android.media.audio.common.AudioDeviceType;
import android.media.audio.common.AudioMode;
import android.media.audio.common.AudioSource;

/**
 * Defines all parameters supported by the effect instance.
 *
 * There are three groups of parameters:
 * 1. Common parameters are essential parameters, MUST pass to effects at open() interface.
 * 2. Parameters defined for a specific effect type.
 * 3. Extension parameters for vendor.
 *
 * For all supported parameter, implementation MUST support both set and get.
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
         *  Common parameter tag.
         */
        int commonTag;
        /**
         * Vendor defined parameter tag.
         */
        int vendorTag;
        /**
         * Specific effect parameter tag.
         */
        Specific.Id specificId;
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
    AudioDeviceType device;
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
    parcelable Volume {
        float left;
        float right;
    }
    /**
     * Used by audio framework to delegate volume control to effect engine.
     * Effect must implement setParameter(volume) if Flags.volume set to Volume.IND.
     */
    Volume volume;

    /**
     * Used by audio framework to delegate offload information to effect engine.
     * Effect must implement setParameter(offload) if Flags.offloadSupported set to true.
     */
    boolean offload;

    /**
     * Parameters for vendor extension effect implementation usage.
     */
    @VintfStability
    parcelable VendorEffectParameter {
        ParcelableHolder extension;
    }
    VendorEffectParameter vendorEffect;

    /**
     * Parameters MUST be supported by a Specific type of effect.
     */
    @VintfStability
    union Specific {
        @VintfStability
        union Id {
            /**
             * Equalizer.Tag to identify the parameters in Equalizer.
             */
            Equalizer.Tag equalizerTag = Equalizer.Tag.vendor;
        }
        Id id;

        Equalizer equalizer;
    }
    Specific specific;
}
