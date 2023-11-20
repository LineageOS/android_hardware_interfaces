/*
 * Copyright (C) 2023 The Android Open Source Project
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

package android.hardware.security.authgraph;

import android.hardware.security.authgraph.Arc;
import android.hardware.security.authgraph.Identity;
import android.hardware.security.authgraph.KeInitResult;
import android.hardware.security.authgraph.Key;
import android.hardware.security.authgraph.PubKey;
import android.hardware.security.authgraph.SessionIdSignature;
import android.hardware.security.authgraph.SessionInfo;
import android.hardware.security.authgraph.SessionInitiationInfo;

/**
 * AuthGraph interface definition for authenticated key exchange between two parties: P1 (source)
 * and P2 (sink).
 * Pre-requisites: each participant should have a:
 *     1. Persistent identity - e.g. a signing key pair with a self signed certificate or a DICE
 *        certificate chain.
 *     2. A symmetric encryption key kept in memory with per-boot life time of the participant
 *        (a.k.a per-boot key)
 *
 * ErrorCodes are defined in android.hardware.security.authgraph.ErrorCode.aidl.
 * @hide
 */
@VintfStability
interface IAuthGraphKeyExchange {
    /**
     * This method is invoked on P1 (source).
     * Create an ephermeral EC key pair on NIST curve P-256 and a nonce (a cryptographic random
     * number of 16 bytes) for key exchange.
     *
     * @return SessionInitiationInfo including the `Key` containing the public key of the created
     * key pair and an arc from the per-boot key to the private key, the nonce, the persistent
     * identity and the latest protocol version (i.e. AIDL version) supported.
     *
     * Note: The arc from the per-boot key to the private key in `Key` of the return type:
     * `SessionInitiationInfo` serves two purposes:
     * i. A mapping to correlate `create` and `finish` calls to P1 in a particular instance of the
     *    key exchange protocol.
     * ii.A way to minimize the in-memory storage of P1 allocated for key exchange (P1 can include
     *    the nonce in the protected headers of the arc).
     * However, P1 should maintain some form of in-memory record to be able to verify that the input
     * `Key` sent to `finish` is from an unfinished instance of a key exchange protocol, to prevent
     * any replay attacks in `finish`.
     */
    SessionInitiationInfo create();

    /**
     * This method is invoked on P2 (sink).
     * Perform the following steps for key exchange:
     *     0. If either `peerPubKey`, `peerId`, `peerNonce` is not in the expected format, return
     *        errors: INVALID_PEER_KE_KEY, INVALID_IDENTITY, INVALID_PEER_NONCE respectively.
     *     1. Create an ephemeral EC key pair on NIST curve P-256.
     *     2. Create a nonce (a cryptographic random number of 16 bytes).
     *     3. Compute the Diffie-Hellman shared secret: Z.
     *     4. Compute a salt_input = bstr .cbor [
     *            source_version:    int,                    ; from input `peerVersion`
     *            sink_pub_key:      bstr .cbor PlainPubKey, ; from step #1
     *            source_pub_key:    bstr .cbor PlainPubKey, ; from input `peerPubKey`
     *            sink_nonce:        bstr .size 16,          ; from step #2
     *            source_nonce:      bstr .size 16,          ; from input `peerNonce`
     *            sink_cert_chain:   bstr .cbor ExplicitKeyDiceCertChain, ; from own identity
     *            source_cert_chain: bstr .cbor ExplicitKeyDiceCertChain, ; from input `peerId`
     *        ]
     *     5. Extract a cryptographic secret S from Z, using the SHA256 digest of the salt_input
     *        as the salt.
     *     6. Derive two symmetric encryption keys of 256 bits with:
     *        i. b"KE_ENCRYPTION_KEY_SOURCE_TO_SINK" as context for the key used to encrypt incoming
     *           messages
     *       ii. b"KE_ENCRYPTION_KEY_SINK_TO_SOURCE" as context for the key used to encrypt outgoing
     *           messages
     *     7. Create arcs from the per-boot key to each of the two shared keys from step #6 and
     *        mark authentication_complete = false in arcs' protected headers.
     *     8. Derive a MAC key with b"KE_HMAC_KEY" as the context.
     *     9. Compute session_id_input = bstr .cbor [
     *            sink_nonce:     bstr .size 16,
     *            source_nonce:   bstr .size 16,
     *        ],
     *     10.Compute a session_id as a 256 bits HMAC over the session_id_input from step#9 with
     *        the key from step #8.
     *     11.Create a signature over the session_id from step #10, using the signing key which is
     *        part of the party's identity.
     *
     * @param peerPubKey - the public key of the key pair created by the peer (P1) for key exchange
     *                     in `create`
     *
     * @param peerId - the persistent identity of the peer
     *
     * @param peerNonce - nonce created by the peer in `create`
     *
     * @param peerVersion - an integer representing the latest protocol version (i.e. AIDL version)
     *                      supported by the peer
     *
     * @return KeInitResult including the `Key` containing the public key of the key pair created in
     * step #1, the nonce from step #2, the persistent identity of P2, two shared key arcs
     * from step #7, session id from step #10, signature over the session id from step #11 and the
     * negotiated protocol version. The negotiated protocol version should be less than or equal to
     * the `peerVersion`.
     *
     * Note: The two shared key arcs in the return type: `KeInitResult` serve two purposes:
     * i. A mapping to correlate `init` and `authenticationComplete` calls to P2 in a particular
     *    instance of the key exchange protocol.
     * ii.A way to minimize the in-memory storage of P2 allocated for key exchange.
     * However, P2 should maintain some in-memory record to be able to verify that the input
     * `sharedkeys` sent to `authenticationComplete` are from an unfinished instance of a key
     * exchange protocol carried out with the party identified by `peerId`, to prevent any replay
     * attacks in `authenticationComplete`.
     */
    KeInitResult init(
            in PubKey peerPubKey, in Identity peerId, in byte[] peerNonce, in int peerVersion);

    /**
     * This method is invoked on P1 (source).
     * Perform the following steps:
     *     0. If either `peerPubKey`, `peerId`, `peerNonce` is not in the expected format, return
     *        errors: INVALID_PEER_KE_KEY, INVALID_IDENTITY, INVALID_PEER_NONCE respectively. If
     *        `peerVersion` is greater than the version advertised in `create`, return error:
     *        INCOMPATIBLE_PROTOCOL_VERSION.
     *        If `ownKey` is not in the in-memory records for unfinished instances of a key
     *        exchange protocol, return error: INVALID_KE_KEY. Similarly, if the public key or the
     *        arc containing the private key in `ownKey` is invalid, return INVALID_PUB_KEY_IN_KEY
     *        and INVALID_PRIV_KEY_ARC_IN_KEY respectively.
     *     1. Compute the Diffie-Hellman shared secret: Z.
     *     2. Compute a salt_input = bstr .cbor [
     *            source_version:    int,                    ; the protocol version used in `create`
     *            sink_pub_key:      bstr .cbor PlainPubKey, ; from input `peerPubKey`
     *            source_pub_key:    bstr .cbor PlainPubKey, ; from the output of `create`
     *            sink_nonce:        bstr .size 16,          ; from input `peerNonce`
     *            source_nonce:      bstr .size 16,          ; from the output of `create`
     *            sink_cert_chain:   bstr .cbor ExplicitKeyDiceCertChain, ; from input `peerId`
     *            source_cert_chain: bstr .cbor ExplicitKeyDiceCertChain, ; from own identity
     *        ]
     *     3. Extract a cryptographic secret S from Z, using the SHA256 digest of the salt_input
     *        as the salt.
     *     4. Derive two symmetric encryption keys of 256 bits with:
     *        i. b"KE_ENCRYPTION_KEY_SOURCE_TO_SINK" as context for the key used to encrypt outgoing
     *           messages
     *       ii. b"KE_ENCRYPTION_KEY_SINK_TO_SOURCE" as context for the key used to encrypt incoming
     *           messages
     *     5. Derive a MAC key with b"KE_HMAC_KEY" as the context.
     *     6. Compute session_id_input = bstr .cbor [
     *            sink_nonce:     bstr .size 16,
     *            source_nonce:   bstr .size 16,
     *        ],
     *     7. Compute a session_id as a 256 bits HMAC over the session_id_input from step #6 with
     *        the key from step #5.
     *     8. Verify the peer's signature over the session_id from step #7. If successful, proceed,
     *        otherwise, return error: INVALID_SIGNATURE.
     *     9. Create arcs from the per-boot key to each of the two shared keys from step #4 and
     *        mark authentication_complete = true in arcs' protected headers.
     *     10.Create a signature over the session_id from step #7, using the signing key which is
     *        part of the party's identity.
     *
     * @param peerPubKey - the public key of the key pair created by the peer (P2) for key exchange
     *                     in `init`
     *
     * @param peerId - the persistent identity of the peer
     *
     * @param peerSignature - the signature created by the peer over the session id computed by the
     *                        peer in `init`
     *
     * @param peerNonce - nonce created by the peer in `init`
     *
     * @param peerVersion - an integer representing the protocol version (i.e. AIDL version)
     *                      negotiated with the peer
     *
     * @param ownKey - the key created by P1 (source) in `create` for key exchange
     *
     * @return SessionInfo including the two shared key arcs from step #9, session id from step #7
     * and the signature over the session id from step #10.
     *
     * Note: The two shared key arcs in the return type: `SessionInfo` serve two purposes:
     * i. A mapping to correlate the key exchange protocol taken place with a particular peer and
     *    subsequent AuthGraph protocols executed with the same peer.
     * ii.A way to minimize the in-memory storage for shared keys.
     * However, P1 should maintain some in-memory record to be able to verify that the shared key
     * arcs sent to any subsequent AuthGraph protocol methods are valid shared keys agreed with the
     * party identified by `peerId`, to prevent any replay attacks.
     */
    SessionInfo finish(in PubKey peerPubKey, in Identity peerId,
            in SessionIdSignature peerSignature, in byte[] peerNonce, in int peerVersion,
            in Key ownKey);

    /**
     * This method is invoked on P2 (sink).
     * Perform the following steps:
     *   0. If input `sharedKeys` is invalid (i.e. they cannot be decrypted with P2's per-boot key
     *      or they are not in P2's in-memory records for unfinished instances of a key exchange
     *      protocol carried out with the party identified by the identity included in the
     *      `source_id` protected header of the shared key arcs),
     *      return error: INVALID_SHARED_KEY_ARCS.
     *   1. Verify that both shared key arcs have the same session id and peer identity.
     *   2. Verify the `peerSignature` over the session id included in the `session_id` protected
     *      header of the shared key arcs.
     *      If successful, proceed, otherwise, return error: INVALID_SIGNATURE.
     *   3. Mark authentication_complete = true in the shared key arcs' headers.
     *
     * @param peerSignature - the signature created by the peer over the session id computed by the
     *                        peer in `finish`
     *
     * @param sharedKeys - two shared key arcs created by P2 in `init`. P2 obtains from the arcs'
     *                     protected headers, the session id and the peer's identity to verify the
     *                     peer's signature over the session id.
     *
     * @return Arc[] - an array of two updated shared key arcs
     *
     * Note: The two returned shared key arcs serve two purposes:
     * i. A mapping to correlate the key exchange protocol taken place with a particular peer and
     *    subsequent AuthGraph protocols executed with the same peer.
     * ii.A way to minimize the in-memory storage for shared keys.
     * However, P2 should maintain some in-memory record to be able to verify that the shared key
     * arcs sent to any subsequent AuthGraph protocol methods are valid shared keys agreed with the
     * party identified by the identity included in the `source_id` protected header of the shared
     * key arcs, to prevent any replay attacks.
     */
    Arc[2] authenticationComplete(in SessionIdSignature peerSignature, in Arc[2] sharedKeys);
}
