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

package android.hardware.bluetooth.lmp_event;

import android.hardware.bluetooth.lmp_event.Direction;
import android.hardware.bluetooth.lmp_event.AddressType;
import android.hardware.bluetooth.lmp_event.LmpEventId;
import android.hardware.bluetooth.lmp_event.Timestamp;

@VintfStability
interface IBluetoothLmpEventCallback {
    /**
     * Callback when monitored LMP event invoked.
     *
     * @param timestamp Timestamp when the LMP event invoked
     * @param addressType  Type of Bluetooth address.
     * @param address Remote bluetooth address that invoke LMP event
     * @param direction Direction of the invoked LMP event
     * @param lmpEventId LMP event id that bluetooth chip invoked
     * @param connEventCounter counter incremented by one for each new connection event
     */
    void onEventGenerated(in Timestamp timestamp,
                          in AddressType addressType,
                          in byte[6] address,
                          in Direction direction,
                          in LmpEventId lmpEventId,
                          in char connEventCounter);

    /**
     * Callback when registration done.
     */
    void onRegistered(in boolean status);
}
