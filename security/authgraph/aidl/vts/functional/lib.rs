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
use authgraph_core::{error::Error as AgError, keyexchange as ke};
use coset::CborSerializable;
use std::{cell::RefCell, rc::Rc};

pub mod sink;
pub mod source;

/// Return an AuthGraphParticipant suitable for testing.
pub fn test_ag_participant() -> Result<ke::AuthGraphParticipant, AgError> {
    Ok(ke::AuthGraphParticipant::new(
        boring::crypto_trait_impls(),
        Rc::new(RefCell::new(boring::test_device::AgDevice::default())),
        ke::MAX_OPENED_SESSIONS,
    )?)
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
