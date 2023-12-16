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

//! VTS tests for sources
use super::*;
use authgraph_core::{key, keyexchange as ke};

/// Run AuthGraph tests against the provided source, using a local test sink implementation.
pub fn test(
    local_sink: &mut ke::AuthGraphParticipant,
    source: binder::Strong<dyn IAuthGraphKeyExchange>,
) {
    test_mainline(local_sink, source.clone());
    test_corrupt_sig(local_sink, source.clone());
    test_corrupt_key(local_sink, source);
}

/// Perform mainline AuthGraph key exchange with the provided source.
/// Return the agreed AES keys in plaintext, together with the session ID.
pub fn test_mainline(
    local_sink: &mut ke::AuthGraphParticipant,
    source: binder::Strong<dyn IAuthGraphKeyExchange>,
) -> ([key::AesKey; 2], Vec<u8>) {
    // Step 1: create an ephemeral ECDH key at the (remote) source.
    let source_init_info = source
        .create()
        .expect("failed to create() with remote impl");
    assert!(source_init_info.key.pubKey.is_some());
    assert!(source_init_info.key.arcFromPBK.is_some());
    let source_pub_key = extract_plain_pub_key(&source_init_info.key.pubKey);

    // Step 2: pass the source's ECDH public key and other session info to the (local) sink.
    let init_result = local_sink
        .init(
            &source_pub_key.plainPubKey,
            &source_init_info.identity.identity,
            &source_init_info.nonce,
            source_init_info.version,
        )
        .expect("failed to init() with local impl");
    let sink_init_info = init_result.session_init_info;
    let sink_pub_key = sink_init_info
        .ke_key
        .pub_key
        .expect("expect pub_key to be populated");

    let sink_info = init_result.session_info;
    assert!(!sink_info.session_id.is_empty());

    // The AuthGraph core library will verify the session ID signature, but do it here too.
    let sink_verification_key = key::Identity::from_slice(&sink_init_info.identity)
        .expect("invalid identity CBOR")
        .cert_chain
        .root_key;
    local_sink
        .verify_signature_on_session_id(
            &sink_verification_key,
            &sink_info.session_id,
            &sink_info.session_id_signature,
        )
        .expect("failed verification of signed session ID");

    // Step 3: pass the sink's ECDH public key and other session info to the (remote) source, so it
    // can calculate the same pair of symmetric keys.
    let source_info = source
        .finish(
            &PubKey::PlainKey(PlainPubKey {
                plainPubKey: sink_pub_key,
            }),
            &Identity {
                identity: sink_init_info.identity,
            },
            &vec_to_signature(&sink_info.session_id_signature),
            &sink_init_info.nonce,
            sink_init_info.version,
            &source_init_info.key,
        )
        .expect("failed to finish() with remote impl");
    assert!(!source_info.sessionId.is_empty());

    // The AuthGraph core library will verify the session ID signature, but do it here too.
    let source_verification_key = local_sink
        .peer_verification_key_from_identity(&source_init_info.identity.identity)
        .expect("failed to get peer verification from identity");
    local_sink
        .verify_signature_on_session_id(
            &source_verification_key,
            &source_info.sessionId,
            &source_info.signature.signature,
        )
        .expect("failed verification of signed session ID");

    // Both ends should agree on the session ID.
    assert_eq!(source_info.sessionId, sink_info.session_id);

    // Step 4: pass the (remote) source's session ID signature back to the sink, so it can check it
    // and update the symmetric keys so they're marked as authentication complete.
    let sink_arcs = local_sink
        .authentication_complete(&source_info.signature.signature, sink_info.shared_keys)
        .expect("failed to authenticationComplete() with local sink");
    // Decrypt and return the session keys.
    let decrypted_shared_keys = local_sink
        .decipher_shared_keys_from_arcs(&sink_arcs)
        .expect("failed to decrypt shared key arcs")
        .try_into();
    let decrypted_shared_keys_array = match decrypted_shared_keys {
        Ok(array) => array,
        Err(_) => panic!("wrong number of decrypted shared key arcs"),
    };
    (decrypted_shared_keys_array, source_info.sessionId)
}

/// Perform mainline AuthGraph key exchange with the provided source, but provide an invalid session
/// ID signature.
pub fn test_corrupt_sig(
    local_sink: &mut ke::AuthGraphParticipant,
    source: binder::Strong<dyn IAuthGraphKeyExchange>,
) {
    // Step 1: create an ephemeral ECDH key at the (remote) source.
    let source_init_info = source
        .create()
        .expect("failed to create() with remote impl");
    assert!(source_init_info.key.pubKey.is_some());
    assert!(source_init_info.key.arcFromPBK.is_some());
    let source_pub_key = extract_plain_pub_key(&source_init_info.key.pubKey);

    // Step 2: pass the source's ECDH public key and other session info to the (local) sink.
    let init_result = local_sink
        .init(
            &source_pub_key.plainPubKey,
            &source_init_info.identity.identity,
            &source_init_info.nonce,
            source_init_info.version,
        )
        .expect("failed to init() with local impl");
    let sink_init_info = init_result.session_init_info;
    let sink_pub_key = sink_init_info
        .ke_key
        .pub_key
        .expect("expect pub_key to be populated");
    let sink_info = init_result.session_info;
    assert!(!sink_info.session_id.is_empty());

    // Deliberately corrupt the sink's session ID signature.
    let mut corrupt_signature = sink_info.session_id_signature.clone();
    let sig_len = corrupt_signature.len();
    corrupt_signature[sig_len - 1] ^= 0x01;

    // Step 3: pass the sink's ECDH public key and other session info to the (remote) source, so it
    // can calculate the same pair of symmetric keys.
    let result = source.finish(
        &PubKey::PlainKey(PlainPubKey {
            plainPubKey: sink_pub_key,
        }),
        &Identity {
            identity: sink_init_info.identity,
        },
        &vec_to_signature(&corrupt_signature),
        &sink_init_info.nonce,
        sink_init_info.version,
        &source_init_info.key,
    );
    let err = result.expect_err("expect failure with corrupt signature");
    assert_eq!(
        err,
        binder::Status::new_service_specific_error(Error::INVALID_SIGNATURE.0, None)
    );
}

/// Perform mainline AuthGraph key exchange with the provided source, but give it back
/// a corrupted key.
pub fn test_corrupt_key(
    local_sink: &mut ke::AuthGraphParticipant,
    source: binder::Strong<dyn IAuthGraphKeyExchange>,
) {
    // Step 1: create an ephemeral ECDH key at the (remote) source.
    let source_init_info = source
        .create()
        .expect("failed to create() with remote impl");
    assert!(source_init_info.key.pubKey.is_some());
    assert!(source_init_info.key.arcFromPBK.is_some());
    let source_pub_key = extract_plain_pub_key(&source_init_info.key.pubKey);

    // Step 2: pass the source's ECDH public key and other session info to the (local) sink.
    let init_result = local_sink
        .init(
            &source_pub_key.plainPubKey,
            &source_init_info.identity.identity,
            &source_init_info.nonce,
            source_init_info.version,
        )
        .expect("failed to init() with local impl");
    let sink_init_info = init_result.session_init_info;
    let sink_pub_key = sink_init_info
        .ke_key
        .pub_key
        .expect("expect pub_key to be populated");

    let sink_info = init_result.session_info;
    assert!(!sink_info.session_id.is_empty());

    // The AuthGraph core library will verify the session ID signature, but do it here too.
    let sink_verification_key = key::Identity::from_slice(&sink_init_info.identity)
        .expect("invalid identity CBOR")
        .cert_chain
        .root_key;
    local_sink
        .verify_signature_on_session_id(
            &sink_verification_key,
            &sink_info.session_id,
            &sink_info.session_id_signature,
        )
        .expect("failed verification of signed session ID");

    // Deliberately corrupt the source's encrypted key.
    let mut corrupt_key = source_init_info.key.clone();
    match &mut corrupt_key.arcFromPBK {
        Some(a) => {
            let len = a.arc.len();
            a.arc[len - 1] ^= 0x01;
        }
        None => panic!("no arc data"),
    }

    // Step 3: pass the sink's ECDH public key and other session info to the (remote) source, but
    // give it back a corrupted version of its own key.
    let result = source.finish(
        &PubKey::PlainKey(PlainPubKey {
            plainPubKey: sink_pub_key,
        }),
        &Identity {
            identity: sink_init_info.identity,
        },
        &vec_to_signature(&sink_info.session_id_signature),
        &sink_init_info.nonce,
        sink_init_info.version,
        &corrupt_key,
    );

    let err = result.expect_err("expect failure with corrupt key");
    assert!(
        err == binder::Status::new_service_specific_error(Error::INVALID_KE_KEY.0, None)
            || err
                == binder::Status::new_service_specific_error(
                    Error::INVALID_PRIV_KEY_ARC_IN_KEY.0,
                    None
                )
    );
}
