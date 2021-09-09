/*
 * Copyright (C) 2021 The Android Open Source Project
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

package android.hardware.radio;

/**
 * The source to tell where the corresponding EmergencyNumber comes from.
 * Reference: 3gpp 22.101, Section 10 - Emergency Calls
 */
@VintfStability
@Backing(type="int")
enum EmergencyNumberSource {
    /**
     * Indicates the number is from the network signal.
     */
    NETWORK_SIGNALING = 1 << 0,
    /**
     * Indicates the number is from the sim card.
     */
    SIM = 1 << 1,
    /**
     * Indicates the number is from the modem config.
     */
    MODEM_CONFIG = 1 << 2,
    /**
     * Indicates the number is available as default. Per the reference, 112, 911 must always be
     * available; additionally, 000, 08, 110, 999, 118 and 119 must be available when sim is not
     * present.
     */
    DEFAULT = 1 << 3,
}
