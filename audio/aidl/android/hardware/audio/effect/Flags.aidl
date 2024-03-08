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

/**
 * Some common capability for an effect instance.
 */
@VintfStability
parcelable Flags {
    /**
     * Type of connection.
     */
    @VintfStability
    @Backing(type="byte")
    enum Type {
        /**
         * After track process.
         */
        INSERT = 0,
        /**
         * Connect to track auxiliary output and use send level.
         */
        AUXILIARY = 1,
        /**
         * Rreplaces track process function; must implement SRC, volume and mono to stereo.
         */
        REPLACE = 2,
        /**
         * Applied below audio HAL on in.
         */
        PRE_PROC = 3,
        /**
         * Applied below audio HAL on out.
         */
        POST_PROC = 4,
    }
    Type type = Type.INSERT;

    /**
     * Insertion preference.
     */
    @VintfStability
    @Backing(type="byte")
    enum Insert {
        ANY = 0,
        /**
         * First of the chain.
         */
        FIRST = 1,
        /**
         * Last of the chain.
         */
        LAST = 2,
        /**
         * Exclusive (only effect in the insert chain.
         */
        EXCLUSIVE = 3,
    }
    Insert insert = Insert.ANY;

    @VintfStability
    @Backing(type="byte")
    enum Volume {
        NONE = 0,
        /**
         * Implements volume control.
         */
        CTRL = 1,
        /**
         * Requires volume indication.
         */
        IND = 2,
        /**
         * Monitors requested volume.
         */
        MONITOR = 3,
    }
    Volume volume = Volume.NONE;

    @VintfStability
    @Backing(type="byte")
    enum HardwareAccelerator {
        /**
         * No hardware acceleration
         */
        NONE = 0,
        /**
         * Non tunneled hw acceleration: effect reads the samples, send them to HW accelerated
         * effect processor, reads back the processed samples and returns them to the output buffer.
         */
        SIMPLE = 1,
        /**
         * The effect interface is only used to control the effect engine. This mode is relevant for
         * global effects actually applied by the audio hardware on the output stream.
         */
        TUNNEL = 2,
    }
    HardwareAccelerator hwAcceleratorMode = HardwareAccelerator.NONE;

    /**
     * Effect instance sets this flag to true if it requires updates on whether the playback thread
     * the effect is attached to is offloaded or not.  In this case the framework must call
     * IEffect.setParameter(Parameter.offload) to notify effect instance when playback thread
     * offload changes.
     */
    boolean offloadIndication;

    /**
     * Effect instance sets this flag to true if it requires device change update. In this case the
     * framework must call IEffect.setParameter(Parameter.device) to notify effect instance when the
     * device changes.
     */
    boolean deviceIndication;

    /**
     * Effect instance sets this flag to true if it requires audio mode change update. In this case
     * the framework must call IEffect.setParameter(Parameter.mode) to notify effect instance when
     * the audio mode changes.
     */
    boolean audioModeIndication;

    /**
     * Effect instance sets this flag to true if it requires audio source change update. In this
     * case the framework must call IEffect.setParameter(Parameter.source) to notify effect instance
     * when the audio source changes.
     */
    boolean audioSourceIndication;

    /**
     * Set to true if the effect instance bypass audio data (no processing).
     */
    boolean bypass;

    /**
     * Effect instance sets this flag to true if it requires record AudioTrack metadata update. In
     * this case the framework must call IEffect.setParameter to notify effect instance when there
     * is a change in sinkMetadata.
     */
    boolean sinkMetadataIndication;

    /**
     * Effect instance sets this flag to true if it requires playback AudioTrack metadata update. In
     * this case the framework must call IEffect.setParameter to notify effect instance when there
     * is a change in sourceMetadata.
     */
    boolean sourceMetadataIndication;
}
