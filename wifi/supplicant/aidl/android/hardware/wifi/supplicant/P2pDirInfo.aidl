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
 * The Device Identity Resolution (DIR) Info is used to identify a previously
 * paired P2P device.
 * The device advertises this information in Bluetooth LE advertising packets
 * and Unsynchronized Service Discovery (USD) frames. The device receiving DIR
 * Info uses this information to identify that the peer device is a previously paired device.
 * For Details, refer Wi-Fi Alliance Wi-Fi Direct R2 specification section 3.8.2 Pairing Identity
 * and section 3.9.2.3.2 Optional Advertising Data Elements.
 */
@VintfStability
parcelable P2pDirInfo {
    /**
     * Enums for the |cipherVersion| field.
     */
    @VintfStability
    @Backing(type="int")
    enum CipherVersion {
        NONE,
        /**
         * DIRA cipher version 128 bit.
         * 128-bit Device Identity Key, 64-bit Nonce, 64-bit Tag.
         * 64-bit Tag = Truncate-64(HMAC-SHA-256(DevIk, "DIR" ||
         * P2P Device Address || Nonce))
         */
        DIRA_CIPHER_VERSION_128_BIT,
    }

    /**
     * DIRA cipher version. The value of cipher version indicates the
     * cryptographic parameters and method used to derive the dirTag field.
     * Set to one of the |DIRA_CIPHER_VERSION_*|.
     */
    CipherVersion cipherVersion;

    /**
     * The MAC address of the P2P device interface.
     */
    byte[6] deviceInterfaceMacAddress;

    /**
     * Random number. The size limit is defined in the cipher version comment.
     */
    byte[] nonce;

    /**
     * A resolvable identity. The size limit is defined in the cipher version comment.
     */
    byte[] dirTag;
}
