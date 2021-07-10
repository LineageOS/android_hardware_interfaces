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

package android.hardware.security.keymint;

/**
 * ProtectedData contains the encrypted BCC and the ephemeral MAC key used to
 * authenticate the keysToSign (see keysToSignMac output argument).
 * @hide
 */
@VintfStability
parcelable ProtectedData {
    /**
     * ProtectedData is a COSE_Encrypt structure, specified by the following CDDL
     *
     *     ProtectedData = [               // COSE_Encrypt
     *         protected: bstr .cbor {
     *             1 : 3                   // Algorithm : AES-GCM 256
     *         },
     *         unprotected: {
     *             5 : bstr .size 12       // IV
     *         },
     *         ciphertext: bstr,           // AES-GCM-256(K, .cbor ProtectedDataPayload)
     *         recipients : [
     *             [                       // COSE_Recipient
     *                 protected : bstr .cbor {
     *                     1 : -25         // Algorithm : ECDH-ES + HKDF-256
     *                 },
     *                 unprotected : {
     *                     -1 : PubKeyX25519 / PubKeyEcdhP256  // Of the sender
     *                     4 : bstr,       // KID : EEK ID
     *                 },
     *                 ciphertext : nil
     *             ]
     *         ]
     *     ]
     *
     *     K = HKDF-256(ECDH(EEK_pub, Ephemeral_priv), Context)
     *
     *     Context = [                     // COSE_KDF_Context
     *         AlgorithmID : 3             // AES-GCM 256
     *         PartyUInfo : [
     *             identity : bstr "client"
     *             nonce : bstr .size 0,
     *             other : bstr            // Ephemeral pubkey
     *         ],
     *         PartyVInfo : [
     *             identity : bstr "server",
     *             nonce : bstr .size 0,
     *             other : bstr            // EEK pubkey
     *         ],
     *         SuppPubInfo : [
     *             256,                    // Output key length
     *             protected : bstr .size 0
     *         ]
     *     ]
     *
     *     ProtectedDataPayload [
     *         SignedMac,
     *         Bcc,
     *         ? AdditionalDKSignatures,
     *     ]
     *     AdditionalDKSignatures = {
     *         + SignerName => DKCertChain
     *     }
     *
     *     SignerName = tstr
     *
     *     DKCertChain = [
     *         2* Certificate                      // Root -> Leaf.  Root is the vendor
     *                                             // self-signed cert, leaf contains DK_pub
     *     ]
     *
     *     Certificate = COSE_Sign1 of a public key
     *
     *     SignedMac = [                                  // COSE_Sign1
     *         bstr .cbor {                               // Protected params
     *             1 : AlgorithmEdDSA / AlgorithmES256,   // Algorithm
     *         },
     *         {},                   // Unprotected params
     *         bstr .size 32,                  // MAC key
     *         bstr PureEd25519(KM_priv, .cbor SignedMac_structure) /
     *              ECDSA(KM_priv, bstr .cbor SignedMac_structure)
     *     ]
     *
     *     SignedMac_structure = [
     *         "Signature1",
     *         bstr .cbor {                               // Protected params
     *             1 : AlgorithmEdDSA / AlgorithmES256,   // Algorithm
     *         },
     *         bstr .cbor SignedMacAad
     *         bstr .size 32                              // MAC key
     *     ]
     *
     *     SignedMacAad = [
     *         challenge : bstr,
     *         VerifiedDeviceInfo,
     *         tag: bstr                 // This is the tag from COSE_Mac0 of
     *                                   // KeysToCertify, to tie the key set to
     *                                   // the signature.
     *     ]
     *
     *     Bcc = [
     *         PubKeyEd25519 / PubKeyECDSA256, // DK_pub
     *         + BccEntry,                     // Root -> leaf (KM_pub)
     *     ]
     *
     *     BccPayload = {                     // CWT
     *         1 : tstr,                      // Issuer
     *         2 : tstr,                      // Subject
     *         // See the Open Profile for DICE for details on these fields.
     *         ? -4670545 : bstr,             // Code Hash
     *         ? -4670546 : bstr,             // Code Descriptor
     *         ? -4670547 : bstr,             // Configuration Hash
     *         ? -4670548 : bstr .cbor {      // Configuration Descriptor
     *             ? -70002 : tstr,           // Component name
     *             ? -70003 : int,            // Firmware version
     *             ? -70004 : null,           // Resettable
     *         },
     *         ? -4670549 : bstr,             // Authority Hash
     *         ? -4670550 : bstr,             // Authority Descriptor
     *         ? -4670551 : bstr,             // Mode
     *         -4670552 : bstr .cbor PubKeyEd25519 /
     *                    bstr .cbor PubKeyECDSA256   // Subject Public Key
     *         -4670553 : bstr                // Key Usage
     *     }
     *
     *     BccEntry = [                                  // COSE_Sign1 (untagged)
     *         protected : bstr .cbor {
     *             1 : AlgorithmEdDSA / AlgorithmES256,  // Algorithm
     *         },
     *         unprotected: {},
     *         payload: bstr .cbor BccPayload,
     *         signature: bstr .cbor PureEd25519(SigningKey, bstr .cbor BccEntryInput) /
     *                    bstr .cbor ECDSA(SigningKey, bstr .cbor BccEntryInput)
     *         // See RFC 8032 for details of how to encode the signature value for Ed25519.
     *     ]
     *
     *     BccEntryInput = [
     *         context: "Signature1",
     *         protected: bstr .cbor {
     *             1 : AlgorithmEdDSA / AlgorithmES256,  // Algorithm
     *         },
     *         external_aad: bstr .size 0,
     *         payload: bstr .cbor BccPayload
     *     ]
     *
     *     VerifiedDeviceInfo = DeviceInfo  // See DeviceInfo.aidl
     *
     *     PubKeyX25519 = {                 // COSE_Key
     *          1 : 1,                      // Key type : Octet Key Pair
     *         -1 : 4,                      // Curve : X25519
     *         -2 : bstr                    // Sender X25519 public key
     *     }
     *
     *     PubKeyEd25519 = {                // COSE_Key
     *         1 : 1,                         // Key type : octet key pair
     *         3 : AlgorithmEdDSA,            // Algorithm : EdDSA
     *         4 : 2,                         // Ops: Verify
     *         -1 : 6,                        // Curve : Ed25519
     *         -2 : bstr                      // X coordinate, little-endian
     *     }
     *
     *     PubKeyEcdhP256 = {              // COSE_Key
     *          1 : 2,      // Key type : EC2
     *          -1 : 1,     // Curve : P256
     *          -2 : bstr   // Sender X coordinate
     *          -3 : bstr   // Sender Y coordinate
     *     }
     *
     *     PubKeyECDSA256 = {                 // COSE_Key
     *         1 : 2,                         // Key type : EC2
     *         3 : AlgorithmES256,            // Algorithm : ECDSA w/ SHA-256
     *         4 : 2,                         // Ops: Verify
     *         -1 : 1,                        // Curve: P256
     *         -2 : bstr,                     // X coordinate
     *         -3 : bstr                      // Y coordinate
     *     }
     *
     *     AlgorithmES256 = -7
     *     AlgorithmEdDSA = -8
     */
    byte[] protectedData;
}
