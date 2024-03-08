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

//! VTS tests for sinks
use super::*;
use authgraph_core::{key, keyexchange as ke};

/// Run AuthGraph tests against the provided sink, using a local test source implementation.
pub fn test(
    local_source: &mut ke::AuthGraphParticipant,
    sink: binder::Strong<dyn IAuthGraphKeyExchange>,
) {
    test_mainline(local_source, sink.clone());
    test_corrupt_sig(local_source, sink.clone());
    test_corrupt_keys(local_source, sink);
}

/// Perform mainline AuthGraph key exchange with the provided sink and local implementation.
/// Return the agreed AES keys in plaintext, together with the session ID.
pub fn test_mainline(
    local_source: &mut ke::AuthGraphParticipant,
    sink: binder::Strong<dyn IAuthGraphKeyExchange>,
) -> ([key::AesKey; 2], Vec<u8>) {
    // Step 1: create an ephemeral ECDH key at the (local) source.
    let source_init_info = local_source
        .create()
        .expect("failed to create() with local impl");

    // Step 2: pass the source's ECDH public key and other session info to the (remote) sink.
    let init_result = sink
        .init(
            &build_plain_pub_key(&source_init_info.ke_key.pub_key),
            &vec_to_identity(&source_init_info.identity),
            &source_init_info.nonce,
            source_init_info.version,
        )
        .expect("failed to init() with remote impl");
    let sink_init_info = init_result.sessionInitiationInfo;
    let sink_pub_key = extract_plain_pub_key(&sink_init_info.key.pubKey);

    let sink_info = init_result.sessionInfo;
    assert!(!sink_info.sessionId.is_empty());

    // The AuthGraph core library will verify the session ID signature, but do it here too.
    let sink_verification_key = local_source
        .peer_verification_key_from_identity(&sink_init_info.identity.identity)
        .expect("failed to get peer verification from identity");
    local_source
        .verify_signature_on_session_id(
            &sink_verification_key,
            &sink_info.sessionId,
            &sink_info.signature.signature,
        )
        .expect("failed verification of signed session ID");

    // Step 3: pass the sink's ECDH public key and other session info to the (local) source, so it
    // can calculate the same pair of symmetric keys.
    let source_info = local_source
        .finish(
            &sink_pub_key.plainPubKey,
            &sink_init_info.identity.identity,
            &sink_info.signature.signature,
            &sink_init_info.nonce,
            sink_init_info.version,
            source_init_info.ke_key,
        )
        .expect("failed to finish() with local impl");
    assert!(!source_info.session_id.is_empty());

    // The AuthGraph core library will verify the session ID signature, but do it here too.
    let source_verification_key = key::Identity::from_slice(&source_init_info.identity)
        .expect("invalid identity CBOR")
        .cert_chain
        .root_key;
    local_source
        .verify_signature_on_session_id(
            &source_verification_key,
            &source_info.session_id,
            &source_info.session_id_signature,
        )
        .expect("failed verification of signed session ID");

    // Both ends should agree on the session ID.
    assert_eq!(source_info.session_id, sink_info.sessionId);

    // Step 4: pass the (local) source's session ID signature back to the sink, so it can check it
    // and update the symmetric keys so they're marked as authentication complete.
    let _sink_arcs = sink
        .authenticationComplete(
            &vec_to_signature(&source_info.session_id_signature),
            &sink_info.sharedKeys,
        )
        .expect("failed to authenticationComplete() with remote sink");
    // Decrypt and return the session keys.
    let decrypted_shared_keys = local_source
        .decipher_shared_keys_from_arcs(&source_info.shared_keys)
        .expect("failed to decrypt shared key arcs")
        .try_into();
    let decrypted_shared_keys_array = match decrypted_shared_keys {
        Ok(array) => array,
        Err(_) => panic!("wrong number of decrypted shared key arcs"),
    };
    (decrypted_shared_keys_array, sink_info.sessionId)
}

/// Perform mainline AuthGraph key exchange with the provided sink, but provide an invalid
/// session ID signature.
pub fn test_corrupt_sig(
    local_source: &mut ke::AuthGraphParticipant,
    sink: binder::Strong<dyn IAuthGraphKeyExchange>,
) {
    // Step 1: create an ephemeral ECDH key at the (local) source.
    let source_init_info = local_source
        .create()
        .expect("failed to create() with local impl");

    // Step 2: pass the source's ECDH public key and other session info to the (remote) sink.
    let init_result = sink
        .init(
            &build_plain_pub_key(&source_init_info.ke_key.pub_key),
            &vec_to_identity(&source_init_info.identity),
            &source_init_info.nonce,
            source_init_info.version,
        )
        .expect("failed to init() with remote impl");
    let sink_init_info = init_result.sessionInitiationInfo;
    let sink_pub_key = extract_plain_pub_key(&sink_init_info.key.pubKey);

    let sink_info = init_result.sessionInfo;
    assert!(!sink_info.sessionId.is_empty());

    // Step 3: pass the sink's ECDH public key and other session info to the (local) source, so it
    // can calculate the same pair of symmetric keys.
    let source_info = local_source
        .finish(
            &sink_pub_key.plainPubKey,
            &sink_init_info.identity.identity,
            &sink_info.signature.signature,
            &sink_init_info.nonce,
            sink_init_info.version,
            source_init_info.ke_key,
        )
        .expect("failed to finish() with local impl");
    assert!(!source_info.session_id.is_empty());

    // Build a corrupted version of the (local) source's session ID signature.
    let mut corrupt_signature = source_info.session_id_signature.clone();
    let sig_len = corrupt_signature.len();
    corrupt_signature[sig_len - 1] ^= 0x01;

    // Step 4: pass the (local) source's **invalid** session ID signature back to the sink,
    // which should reject it.
    let result =
        sink.authenticationComplete(&vec_to_signature(&corrupt_signature), &sink_info.sharedKeys);
    let err = result.expect_err("expect failure with corrupt signature");
    assert_eq!(
        err,
        binder::Status::new_service_specific_error(Error::INVALID_SIGNATURE.0, None)
    );
}

/// Perform mainline AuthGraph key exchange with the provided sink, but provide an invalid
/// Arc for the sink's key.
pub fn test_corrupt_keys(
    local_source: &mut ke::AuthGraphParticipant,
    sink: binder::Strong<dyn IAuthGraphKeyExchange>,
) {
    // Step 1: create an ephemeral ECDH key at the (local) source.
    let source_init_info = local_source
        .create()
        .expect("failed to create() with local impl");

    // Step 2: pass the source's ECDH public key and other session info to the (remote) sink.
    let init_result = sink
        .init(
            &build_plain_pub_key(&source_init_info.ke_key.pub_key),
            &vec_to_identity(&source_init_info.identity),
            &source_init_info.nonce,
            source_init_info.version,
        )
        .expect("failed to init() with remote impl");
    let sink_init_info = init_result.sessionInitiationInfo;
    let sink_pub_key = extract_plain_pub_key(&sink_init_info.key.pubKey);

    let sink_info = init_result.sessionInfo;
    assert!(!sink_info.sessionId.is_empty());

    // Step 3: pass the sink's ECDH public key and other session info to the (local) source, so it
    // can calculate the same pair of symmetric keys.
    let source_info = local_source
        .finish(
            &sink_pub_key.plainPubKey,
            &sink_init_info.identity.identity,
            &sink_info.signature.signature,
            &sink_init_info.nonce,
            sink_init_info.version,
            source_init_info.ke_key,
        )
        .expect("failed to finish() with local impl");
    assert!(!source_info.session_id.is_empty());

    // Deliberately corrupt the sink's shared key Arcs before returning them
    let mut corrupt_keys = sink_info.sharedKeys.clone();
    let len0 = corrupt_keys[0].arc.len();
    let len1 = corrupt_keys[1].arc.len();
    corrupt_keys[0].arc[len0 - 1] ^= 0x01;
    corrupt_keys[1].arc[len1 - 1] ^= 0x01;

    // Step 4: pass the (local) source's session ID signature back to the sink, but with corrupted
    // keys, which should be rejected.
    let result = sink.authenticationComplete(
        &vec_to_signature(&source_info.session_id_signature),
        &corrupt_keys,
    );
    let err = result.expect_err("expect failure with corrupt keys");
    assert_eq!(
        err,
        binder::Status::new_service_specific_error(Error::INVALID_SHARED_KEY_ARCS.0, None)
    );
}
