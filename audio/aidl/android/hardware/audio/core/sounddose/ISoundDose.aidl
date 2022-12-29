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

package android.hardware.audio.core.sounddose;

import android.media.audio.common.AudioDevice;

/**
 * This interface provides functions related to sound exposure control required for compliance to
 * EN/IEC 62368-1 3rd edition. Implementing this interface is mandatory for devices for which
 * compliance to this standard is mandated and implementing audio offload decoding or other direct
 * playback paths where volume control happens below the audio HAL.
 */
@VintfStability
interface ISoundDose {
    /**
     * Max value in dBA used for momentary exposure warnings as defined by IEC62368-1
     * 3rd edition. This value represents the default RS2 value.
     */
    const int DEFAULT_MAX_RS2 = 100;
    /** Min value of the RS2 threshold in dBA as defined by IEC62368-1 3rd edition. */
    const int MIN_RS2 = 80;

    /**
     * Sets the RS2 value used for momentary exposure warnings. Default value is
     * DEFAULT_MAX_RS2 as specified in IEC62368-1 3rd edition.
     *
     * @param rs2ValueDbA custom RS2 value to use. Must not be higher than DEFAULT_MAX_RS2
     * @throws EX_ILLEGAL_ARGUMENT if rs2ValueDbA is greater than DEFAULT_MAX_RS2 or lower
     *                             than 80dBA
     */
    void setOutputRs2(float rs2ValueDbA);

    /**
     * Gets the RS2 value used for momentary exposure warnings.
     *
     * @return the RS2 value in dBA
     */
    float getOutputRs2();

    /**
     * Registers the HAL callback for sound dose computation. If sound dose is supported
     * the MEL values and exposure notifications will be received through this callback
     * only. The internal framework MEL computation will be disabled.
     * It is not possible to unregister the callback. The HAL is responsible to provide
     * the MEL values throughout its lifecycle.
     * This method should only be called once (no updates allowed) with a valid callback.
     *
     * @param callback to use when new updates are available for sound dose
     * @throws EX_ILLEGAL_STATE if the method is called more than once
     * @throws EX_ILLEGAL_ARGUMENT if the passed callback is null
     */
    void registerSoundDoseCallback(in IHalSoundDoseCallback callback);

    @VintfStability
    oneway interface IHalSoundDoseCallback {
        /**
         * Called whenever the current MEL value exceeds the set RS2 value.
         *
         * @param currentDbA the current MEL value which exceeds the RS2 value
         * @param audioDevice the audio device where the MEL exposure warning was recorded
         */
        void onMomentaryExposureWarning(float currentDbA, in AudioDevice audioDevice);

        @VintfStability
        parcelable MelRecord {
            /**
             * Array of continuously recorded MEL values >= RS1 (1 per second).
             * First value in the array was recorded at 'timestamp'.
             */
            float[] melValues;
            /**
             * Corresponds to the time in seconds when the first MEL entry in melValues
             * was recorded. The timestamp values have to be consistent throughout all
             * audio ports, equal timestamp values will be aggregated.
             */
            long timestamp;
        }

        /**
         * Provides a MelRecord containing continuous MEL values sorted by timestamp.
         * Note that all the MEL values originate from the audio device specified by audioDevice.
         * In case values from multiple devices need to be reported, the caller should execute
         * this callback once for every device.
         *
         * @param melRecord contains the MEL values used for CSD
         * @param audioDevice the audio device where the MEL values were recorded
         */
        void onNewMelValues(in MelRecord melRecord, in AudioDevice audioDevice);
    }
}
