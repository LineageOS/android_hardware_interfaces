/*
 * Copyright (C) 2025 The Android Open Source Project
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

package android.hardware.radio.data;

/**
 * Represents the state of a data network connection.
 * This is designed to have a direct one-to-one mapping with the states
 * defined in {@link android.telephony.TelephonyManager.DataState}.
 *
 * @hide
 */
@VintfStability
@Backing(type="int")
@JavaDerive(toString=true)
enum DataNetworkState {
    /**
     * Data network state is unknown.
     */
    UNKNOWN = -1,
    /**
     * Data network state is disconnected.
     */
    DISCONNECTED = 0,
    /**
     * Data network state is connecting.
     */
    CONNECTING = 1,
    /**
     * Data network state is connected.
     */
    CONNECTED = 2,
    /**
     * Data network state is disconnecting.
     */
    DISCONNECTING = 4,
}
