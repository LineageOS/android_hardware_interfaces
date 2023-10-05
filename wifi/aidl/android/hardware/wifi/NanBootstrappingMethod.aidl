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

package android.hardware.wifi;

/**
 * Pairing bootstrapping method flag.
 * See Wi-Fi Aware R4.0 section 9.5.21.7 table 128
 */
@VintfStability
@Backing(type="int")
enum NanBootstrappingMethod {
    BOOTSTRAPPING_OPPORTUNISTIC_MASK = 1 << 0,
    BOOTSTRAPPING_PIN_CODE_DISPLAY_MASK = 1 << 1,
    BOOTSTRAPPING_PASSPHRASE_DISPLAY_MASK = 1 << 2,
    BOOTSTRAPPING_QR_DISPLAY_MASK = 1 << 3,
    BOOTSTRAPPING_NFC_TAG_MASK = 1 << 4,
    BOOTSTRAPPING_PIN_CODE_KEYPAD_MASK = 1 << 5,
    BOOTSTRAPPING_PASSPHRASE_KEYPAD_MASK = 1 << 6,
    BOOTSTRAPPING_QR_SCAN_MASK = 1 << 7,
    BOOTSTRAPPING_NFC_READER_MASK = 1 << 8,
    BOOTSTRAPPING_SERVICE_MANAGED_MASK = 1 << 14,
    BOOTSTRAPPING_HANDSHAKE_SHIP_MASK = 1 << 15
}
