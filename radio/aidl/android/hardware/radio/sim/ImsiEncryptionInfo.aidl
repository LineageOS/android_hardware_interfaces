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

package android.hardware.radio.sim;

/**
 * Carrier specific Information sent by the carrier, which will be used to encrypt IMSI and IMPI.
 * @hide
 */
@VintfStability
@JavaDerive(toString=true)
parcelable ImsiEncryptionInfo {
    /**
     * Key type to be used for ePDG
     */
    const byte PUBLIC_KEY_TYPE_EPDG = 1;
    /**
     * Key type to be used for WLAN
     */
    const byte PUBLIC_KEY_TYPE_WLAN = 2;

    /**
     * MCC of the Carrier.
     */
    String mcc;
    /**
     * MNC of the Carrier.
     */
    String mnc;
    /**
     * Carrier specific key to be used for encryption. It must be opaque to the framework.
     * This is the byte-stream representation of the key. This is an external encoded form for the
     * key used when a standard representation of the key is needed outside the Java Virtual
     * Machine, as when transmitting the key to some other party. The key is encoded according to a
     * standard format (such as X.509 SubjectPublicKeyInfo or PKCS#8), and is returned using the
     * getEncoded method as defined on the java.security.Key interface.
     */
    byte[] carrierKey;
    /**
     * This is an opaque value we're given by the carrier and is returned to the carrier.
     * This is used by the server to help it locate the private key to decrypt the
     * permanent identity.
     */
    String keyIdentifier;
    /**
     * Date-time in UTC when the key will expire.
     */
    long expirationTime;
    /**
     * Public key type from carrier certificate.
     * Values are PUBLIC_KEY_TYPE_
     */
    byte keyType;
}
