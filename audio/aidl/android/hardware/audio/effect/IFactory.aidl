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

import android.hardware.audio.effect.Descriptor;
import android.hardware.audio.effect.IEffect;
import android.hardware.audio.effect.Processing;
import android.media.audio.common.AudioUuid;

/**
 * Provides system-wide effect factory interfaces.
 *
 * An android.hardware.audio.effect.IFactory platform service is registered with ServiceManager, and
 * is always available on the device.
 *
 */
@VintfStability
interface IFactory {
    /**
     * Return a list of effect descriptors supported by this device, with the optional filter by
     * type and/or by instance UUID.
     *
     * @param type UUID identifying the effect type.
     *        This is an optional parameter, pass in null if this parameter is not necessary; if non
     *        null, used as a filter for effect type UUIDs.
     * @param implementation Indicates the particular implementation of the effect in that type.
     *        This is an optional parameter, pass in null if this parameter is not necessary; if
     *        non null, used as a filter for effect implementation UUIDs.
     * @param proxy Indicates the proxy UUID filter to query.
     *        This is an optional parameter, pass in null if this parameter is not necessary; if
     *        non null, used as a filter for effect proxy UUIDs.
     * @return List of effect Descriptors supported and filtered by type/implementation/proxy UUID.
     */
    Descriptor[] queryEffects(in @nullable AudioUuid type,
            in @nullable AudioUuid implementation, in @nullable AudioUuid proxy);

    /**
     * Return a list of defined processings, with the optional filter by Processing type.
     * An effect can exist more than once in the returned list, which means this effect must be used
     * in more than one processing type.
     *
     * @param type Type of processing to query, can be AudioStreamType, AudioSource, or null.
     * @return list of processing defined with the optional filter by Processing.Type.
     */
    Processing[] queryProcessing(in @nullable Processing.Type type);

    /**
     * Called by the audio framework to create the effect (identified by the implementation UUID
     * parameter).
     *
     * The effect instance should be able to maintain its own context and parameters after creation.
     *
     * @param implUuid UUID for the effect implementation which instance will be created based on.
     * @return The effect instance handle created.
     * @throws EX_ILLEGAL_ARGUMENT if the implUuid is not valid.
     * @throws EX_TRANSACTION_FAILED if device capability/resource is not enough to create the
     *         effect instance.
     */
    IEffect createEffect(in AudioUuid implUuid);

    /**
     * Called by the framework to destroy the effect and free up all currently allocated resources.
     * It is recommended to destroy the effect from the client side as soon as it is becomes unused.
     *
     * The client must ensure effect instance is closed before destroy.
     *
     * @param handle The handle of effect instance to be destroyed.
     * @throws EX_ILLEGAL_ARGUMENT if the effect handle is not valid.
     * @throws EX_ILLEGAL_STATE if the effect instance is not in a proper state to be destroyed.
     */
    void destroyEffect(in IEffect handle);
}
