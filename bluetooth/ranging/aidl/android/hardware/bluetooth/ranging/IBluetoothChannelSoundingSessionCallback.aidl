/*
 * Copyright 2023 The Android Open Source Project
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

package android.hardware.bluetooth.ranging;

import android.hardware.bluetooth.ranging.RangingResult;
import android.hardware.bluetooth.ranging.Reason;

/**
 * The callback from the HAL to the stack.
 * Register by IBluetoothChannelSoundingSession.openSession().
 */
@VintfStability
interface IBluetoothChannelSoundingSessionCallback {
    /**
     * Invoked when IBluetoothChannelSounding.openSession() is successful.
     */
    void onOpened(Reason reason);
    /**
     * Invoked when IBluetoothChannelSounding.openSession() fails.
     */
    void onOpenFailed(Reason reason);
    /**
     * Invoked when HAL get raning result.
     */
    void onResult(in RangingResult result);
    /**
     * Invoked when IBluetoothChannelSoundingSession.close() is successful.
     */
    void onClose(Reason reason);
    /**
     * Invoked when IBluetoothChannelSoundingSession.close() fails.
     */
    void onCloseFailed(Reason reason);
}
