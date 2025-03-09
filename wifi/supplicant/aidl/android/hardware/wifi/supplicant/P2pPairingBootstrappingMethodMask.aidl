/*
 * Copyright (C) 2024 The Android Open Source Project
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

package android.hardware.wifi.supplicant;

/**
 * P2P Pairing Bootstrapping Method.
 */
@VintfStability
parcelable P2pPairingBootstrappingMethodMask {
    /** Opportunistic bootstrapping */
    const int BOOTSTRAPPING_OPPORTUNISTIC = 1 << 0;
    /** Display pin-code only */
    const int BOOTSTRAPPING_DISPLAY_PINCODE = 1 << 1;
    /** Display passphrase */
    const int BOOTSTRAPPING_DISPLAY_PASSPHRASE = 1 << 2;
    /** Keypad pin-code only */
    const int BOOTSTRAPPING_KEYPAD_PINCODE = 1 << 3;
    /** Keypad passphrase */
    const int BOOTSTRAPPING_KEYPAD_PASSPHRASE = 1 << 4;
    /**
     * Pairing bootstrapping done Out of band (For example: Over Bluetooth LE.
     * Refer Wi-Fi Alliance Wi-Fi Direct R2 specification Section 3.9 for the details).
     */
    const int BOOTSTRAPPING_OUT_OF_BAND = 1 << 5;
}
