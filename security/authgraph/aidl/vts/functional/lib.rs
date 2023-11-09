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

//! VTS test library for AuthGraph functionality.
//!
//! This test code is bundled as a library, not as `[cfg(test)]`, to allow it to be
//! re-used inside the (Rust) VTS tests of components that use AuthGraph.

use android_hardware_security_authgraph::aidl::android::hardware::security::authgraph::{
    Error::Error, IAuthGraphKeyExchange::IAuthGraphKeyExchange, Identity::Identity,
    PlainPubKey::PlainPubKey, PubKey::PubKey, SessionIdSignature::SessionIdSignature,
};
use authgraph_boringssl as boring;
use authgraph_core::keyexchange as ke;
use authgraph_core::{arc, key, traits};
use authgraph_nonsecure::StdClock;
use coset::CborSerializable;

pub mod sink;
pub mod source;

/// Return a collection of AuthGraph trait implementations suitable for testing.
pub fn test_impls() -> traits::TraitImpl {
    // Note that the local implementation is using a clock with a potentially different epoch than
    // the implementation under test.
    boring::trait_impls(
        Box::<boring::test_device::AgDevice>::default(),
        Some(Box::new(StdClock::default())),
    )
}

fn build_plain_pub_key(pub_key: &Option<Vec<u8>>) -> PubKey {
    PubKey::PlainKey(PlainPubKey {
        plainPubKey: pub_key.clone().unwrap(),
    })
}

fn extract_plain_pub_key(pub_key: &Option<PubKey>) -> &PlainPubKey {
    match pub_key {
        Some(PubKey::PlainKey(pub_key)) => pub_key,
        Some(PubKey::SignedKey(_)) => panic!("expect unsigned public key"),
        None => panic!("expect pubKey to be populated"),
    }
}

fn verification_key_from_identity(impls: &traits::TraitImpl, identity: &[u8]) -> key::EcVerifyKey {
    let identity = key::Identity::from_slice(identity).expect("invalid identity CBOR");
    impls
        .device
        .process_peer_cert_chain(&identity.cert_chain, &*impls.ecdsa)
        .expect("failed to extract signing key")
}

fn vec_to_identity(data: &[u8]) -> Identity {
    Identity {
        identity: data.to_vec(),
    }
}

fn vec_to_signature(data: &[u8]) -> SessionIdSignature {
    SessionIdSignature {
        signature: data.to_vec(),
    }
}

/// Decrypt a pair of AES-256 keys encrypted with the AuthGraph PBK.
pub fn decipher_aes_keys(imp: &traits::TraitImpl, arc: &[Vec<u8>; 2]) -> [key::AesKey; 2] {
    [
        decipher_aes_key(imp, &arc[0]),
        decipher_aes_key(imp, &arc[1]),
    ]
}

/// Decrypt an AES-256 key encrypted with the AuthGraph PBK.
pub fn decipher_aes_key(imp: &traits::TraitImpl, arc: &[u8]) -> key::AesKey {
    let pbk = imp.device.get_per_boot_key().expect("no PBK available");
    let arc::ArcContent {
        payload,
        protected_headers: _,
        unprotected_headers: _,
    } = arc::decipher_arc(&pbk, arc, &*imp.aes_gcm).expect("failed to decrypt arc");
    assert_eq!(payload.0.len(), 32);
    let mut key = key::AesKey([0; 32]);
    key.0.copy_from_slice(&payload.0);
    assert_ne!(key.0, [0; 32], "agreed AES-256 key should be non-zero");
    key
}
