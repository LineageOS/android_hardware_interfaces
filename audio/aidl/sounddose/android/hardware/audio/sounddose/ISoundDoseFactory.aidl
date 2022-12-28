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

package android.hardware.audio.sounddose;

import android.hardware.audio.core.sounddose.ISoundDose;

/**
 * This interface is used to provide an easy way to implement the ISoundDose interface
 * without switching the audio HAL to AIDL. The implementation is intended as a workaround
 * for the certification with IEC62368-1 3rd edition and EN50332-3.
 * Note that this interface will be deprecated in favor of the audio AIDL HAL.
 */
@VintfStability
interface ISoundDoseFactory {
    /**
     * Retrieve the sound dose interface for a given audio HAL module name.
     *
     * If a device must comply to IEC62368-1 3rd edition audio safety requirements and is
     * implementing audio offload decoding or other direct playback paths where volume control
     * happens below the audio HAL, it must return an instance of the ISoundDose interface.
     * The same instance must be returned during the lifetime of the HAL module.
     * If the HAL module does not support sound dose, null must be returned, without throwing
     * any errors.
     *
     * @param module for which we trigger sound dose updates.
     * @return An instance of the ISoundDose interface implementation.
     * @throws EX_ILLEGAL_STATE If there was an error creating an instance.
     */
    @nullable ISoundDose getSoundDose(in @utf8InCpp String module);
}
