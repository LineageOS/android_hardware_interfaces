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

import android.hardware.audio.effect.Capability;
import android.hardware.audio.effect.Flags;
import android.media.audio.common.AudioUuid;

/**
 * Descriptor contains all information (capabilities, attributes, etc) for an effect implementation.
 * The client uses this information to decide when and how to apply an effect implementation.
 *
 * Each type of effect can have more than one implementation (differentiated by implementation
 * UUID), the effect proxy act as a combination of two implementations (usually one software and
 * one offload implementation), so effect processing can be seamlessly switched between
 * implementations in same proxy depending on the configuration and/or use case. If the optional
 * proxy UUID is specified in Descriptor.Identity, then client must consider the effect instance as
 * part of the effect proxy.
 */
@VintfStability
parcelable Descriptor {
    /**
     * UUID for effect types, these definitions are in sync with SDK, see @c AudioEffect.java.
     */
    /**
     * UUID for Acoustic Echo Canceler (AEC) type.
     */
    const String EFFECT_TYPE_UUID_AEC = "7b491460-8d4d-11e0-bd61-0002a5d5c51b";

    /**
     * UUID for Automatic Gain Control V1 (AGC1) type.
     */
    const String EFFECT_TYPE_UUID_AGC1 = "0a8abfe0-654c-11e0-ba26-0002a5d5c51b";
    /**
     * UUID for Automatic Gain Control V2 (AGC2) type.
     */
    const String EFFECT_TYPE_UUID_AGC2 = "ae3c653b-be18-4ab8-8938-418f0a7f06ac";
    /**
     * UUID for bass boost effect type.
     */
    const String EFFECT_TYPE_UUID_BASS_BOOST = "0634f220-ddd4-11db-a0fc-0002a5d5c51b";
    /**
     * UUID for downmix effect type.
     */
    const String EFFECT_TYPE_UUID_DOWNMIX = "381e49cc-a858-4aa2-87f6-e8388e7601b2";
    /**
     * UUID for Dynamics Processing type.
     */
    const String EFFECT_TYPE_UUID_DYNAMICS_PROCESSING = "7261676f-6d75-7369-6364-28e2fd3ac39e";
    /**
     * UUID for environmental reverberation effect type.
     */
    const String EFFECT_TYPE_UUID_ENV_REVERB = "c2e5d5f0-94bd-4763-9cac-4e234d06839e";
    /**
     * UUID for equalizer effect type.
     */
    const String EFFECT_TYPE_UUID_EQUALIZER = "0bed4300-ddd6-11db-8f34-0002a5d5c51b";
    /**
     * UUID for Haptic Generator type.
     */
    const String EFFECT_TYPE_UUID_HAPTIC_GENERATOR = "1411e6d6-aecd-4021-a1cf-a6aceb0d71e5";
    /**
     * UUID for Loudness Enhancer type.
     */
    const String EFFECT_TYPE_UUID_LOUDNESS_ENHANCER = "fe3199be-aed0-413f-87bb-11260eb63cf1";
    /**
     * UUID for Noise Suppressor (NS) type.
     */
    const String EFFECT_TYPE_UUID_NS = "58b4b260-8e06-11e0-aa8e-0002a5d5c51b";
    /**
     * UUID for preset reverberation effect type.
     */
    const String EFFECT_TYPE_UUID_PRESET_REVERB = "47382d60-ddd8-11db-bf3a-0002a5d5c51b";
    /**
     * UUID for Spatializer type.
     */
    const String EFFECT_TYPE_UUID_SPATIALIZER = "ccd4cf09-a79d-46c2-9aae-06a1698d6c8f";
    /**
     * UUID for virtualizer effect type.
     */
    const String EFFECT_TYPE_UUID_VIRTUALIZER = "37cc2c00-dddd-11db-8577-0002a5d5c51b";
    /**
     * UUID for visualizer effect type.
     */
    const String EFFECT_TYPE_UUID_VISUALIZER = "e46b26a0-dddd-11db-8afd-0002a5d5c51b";
    /**
     * UUID for Volume effect type.
     */
    const String EFFECT_TYPE_UUID_VOLUME = "09e8ede0-ddde-11db-b4f6-0002a5d5c51b";

    /**
     * This structure completely identifies an effect implementation.
     */
    @VintfStability
    parcelable Identity {
        /**
         * UUID for the type of effect.
         */
        AudioUuid type;
        /**
         * UUID for this particular implementation.
         */
        AudioUuid uuid;
        /**
         * Optional proxy UUID. This field must be set to the proxy effect type UUID if the effect
         * implementation is part of a proxy effect.
         */
        @nullable AudioUuid proxy;
    }

    /**
     * Common attributes of all effect implementation.
     */
    @VintfStability
    parcelable Common {
        /**
         * Identity of effect implementation.
         */
        Identity id;
        /**
         * Capability flags defined for the effect implementation.
         */
        Flags flags;
        /**
         * CPU load indication expressed in 0.1 MIPS units as estimated on an ARM9E core (ARMv5TE)
         * with 0 WS.
         */
        int cpuLoad;
        /**
         * Data memory usage expressed in KB and includes only dynamically allocated memory.
         */
        int memoryUsage;
        /**
         * Human readable effect name, no intended to display on UI directly.
         */
        @utf8InCpp String name;
        /**
         * Human readable effect implementor name, no intended to display on UI directly.
         */
        @utf8InCpp String implementor;
    }
    Common common;

    /**
     * Effect implementation capability.
     */
    Capability capability;
}
