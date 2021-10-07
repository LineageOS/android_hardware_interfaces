/*
 * Copyright (C) 2020 The Android Open Source Project
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

package android.hardware.vibrator;

import android.hardware.vibrator.IVibrator;
import android.hardware.vibrator.IVibratorCallback;

@VintfStability
interface IVibratorManager {
    /**
     * Whether prepare/trigger synced are supported.
     */
    const int CAP_SYNC = 1 << 0;
    /**
     * Whether IVibrator 'on' can be used with 'prepareSynced' function.
     */
    const int CAP_PREPARE_ON = 1 << 1;
    /**
     * Whether IVibrator 'perform' can be used with 'prepareSynced' function.
     */
    const int CAP_PREPARE_PERFORM = 1 << 2;
    /**
     * Whether IVibrator 'compose' can be used with 'prepareSynced' function.
     */
    const int CAP_PREPARE_COMPOSE = 1 << 3;
    /**
     * Whether IVibrator 'on' can be triggered with other functions in sync with 'triggerSynced'.
     */
    const int CAP_MIXED_TRIGGER_ON = 1 << 4;
    /**
     * Whether IVibrator 'perform' can be triggered with other functions in sync with 'triggerSynced'.
     */
    const int CAP_MIXED_TRIGGER_PERFORM = 1 << 5;
    /**
     * Whether IVibrator 'compose' can be triggered with other functions in sync with 'triggerSynced'.
     */
    const int CAP_MIXED_TRIGGER_COMPOSE = 1 << 6;
    /**
     * Whether on w/ IVibratorCallback can be used w/ 'trigerSynced' function.
     */
    const int CAP_TRIGGER_CALLBACK = 1 << 7;

    /**
     * Determine capabilities of the vibrator manager HAL (CAP_* mask)
     */
    int getCapabilities();

    /**
     * List the id of available vibrators. This result should be static and not change.
     */
    int[] getVibratorIds();

    /**
     * Return an available vibrator identified with given id.
     */
    IVibrator getVibrator(in int vibratorId);

    /**
     * Start preparation for a synced vibration
     *
     * This function must only be called after the previous synced vibration was triggered or
     * canceled (through cancelSynced()).
     *
     * Doing this operation while any of the specified vibrators is already on is undefined behavior.
     * Clients should explicitly call off in each vibrator.
     *
     * @param vibratorIds ids of the vibrators to play vibrations in sync.
     */
    void prepareSynced(in int[] vibratorIds);

    /**
     * Trigger a prepared synced vibration
     *
     * Trigger a previously-started preparation for synced vibration, if any.
     * A callback is only expected to be supported when getCapabilities CAP_TRIGGER_CALLBACK
     * is specified.
     *
     * @param callback A callback used to inform Frameworks of state change, if supported.
     */
    void triggerSynced(in IVibratorCallback callback);

    /**
     * Cancel preparation of synced vibration
     *
     * Cancel a previously-started preparation for synced vibration, if any.
     */
    void cancelSynced();
}
