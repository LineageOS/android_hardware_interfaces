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
     * Return a list of effect identities supported by this device, with the optional
     * filter by type and/or by instance UUID.
     *
     * @param type UUID identifying the effect type.
     *        This is an optional parameter, pass in null if this parameter is not necessary; if non
     *        null, used as a filter for effect type UUIDs.
     * @param implementation Indicates the particular implementation of the effect in that type.
     *        This is an optional parameter, pass in null if this parameter is not necessary; if
     *        non null, used as a filter for effect type UUIDs.
     * @return List of effect identities supported and filtered by type/implementation UUID.
     */
    Descriptor.Identity[] queryEffects(
            in @nullable AudioUuid type, in @nullable AudioUuid implementation);
}
