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

#![cfg(test)]

use android_hardware_security_secretkeeper::aidl::android::hardware::security::secretkeeper::ISecretkeeper::ISecretkeeper;
use android_hardware_security_secretkeeper::aidl::android::hardware::security::secretkeeper::SecretId::SecretId;
use authgraph_vts_test as ag_vts;
use authgraph_boringssl as boring;
use authgraph_core::key;
use binder::StatusCode;
use coset::{CborSerializable, CoseEncrypt0};
use log::{info, warn};
use secretkeeper_client::SkSession;
use secretkeeper_core::cipher;
use secretkeeper_comm::data_types::error::SecretkeeperError;
use secretkeeper_comm::data_types::request::Request;
use secretkeeper_comm::data_types::request_response_impl::{
    GetVersionRequest, GetVersionResponse, GetSecretRequest, GetSecretResponse, StoreSecretRequest,
    StoreSecretResponse };
use secretkeeper_comm::data_types::{Id, Secret, SeqNum};
use secretkeeper_comm::data_types::response::Response;
use secretkeeper_comm::data_types::packet::{ResponsePacket, ResponseType};

const SECRETKEEPER_SERVICE: &str = "android.hardware.security.secretkeeper.ISecretkeeper";
const SECRETKEEPER_INSTANCES: [&'static str; 2] = ["default", "nonsecure"];
const CURRENT_VERSION: u64 = 1;

// TODO(b/291238565): This will change once libdice_policy switches to Explicit-key DiceCertChain
// This is generated by patching libdice_policy such that it dumps an example dice chain &
// a policy, such that the former matches the latter.
const HYPOTHETICAL_DICE_POLICY: [u8; 43] = [
    0x83, 0x01, 0x81, 0x83, 0x01, 0x80, 0xA1, 0x01, 0x00, 0x82, 0x83, 0x01, 0x81, 0x01, 0x73, 0x74,
    0x65, 0x73, 0x74, 0x69, 0x6E, 0x67, 0x5F, 0x64, 0x69, 0x63, 0x65, 0x5F, 0x70, 0x6F, 0x6C, 0x69,
    0x63, 0x79, 0x83, 0x02, 0x82, 0x03, 0x18, 0x64, 0x19, 0xE9, 0x75,
];

// Random bytes (of ID_SIZE/SECRET_SIZE) generated for tests.
const ID_EXAMPLE: Id = Id([
    0xF1, 0xB2, 0xED, 0x3B, 0xD1, 0xBD, 0xF0, 0x7D, 0xE1, 0xF0, 0x01, 0xFC, 0x61, 0x71, 0xD3, 0x42,
    0xE5, 0x8A, 0xAF, 0x33, 0x6C, 0x11, 0xDC, 0xC8, 0x6F, 0xAE, 0x12, 0x5C, 0x26, 0x44, 0x6B, 0x86,
    0xCC, 0x24, 0xFD, 0xBF, 0x91, 0x4A, 0x54, 0x84, 0xF9, 0x01, 0x59, 0x25, 0x70, 0x89, 0x38, 0x8D,
    0x5E, 0xE6, 0x91, 0xDF, 0x68, 0x60, 0x69, 0x26, 0xBE, 0xFE, 0x79, 0x58, 0xF7, 0xEA, 0x81, 0x7D,
]);
const ID_EXAMPLE_2: Id = Id([
    0x6A, 0xCC, 0xB1, 0xEB, 0xBB, 0xAB, 0xE3, 0xEA, 0x44, 0xBD, 0xDC, 0x75, 0x75, 0x7D, 0xC0, 0xE5,
    0xC7, 0x86, 0x41, 0x56, 0x39, 0x66, 0x96, 0x10, 0xCB, 0x43, 0x10, 0x79, 0x03, 0xDC, 0xE6, 0x9F,
    0x12, 0x2B, 0xEF, 0x28, 0x9C, 0x1E, 0x32, 0x46, 0x5F, 0xA3, 0xE7, 0x8D, 0x53, 0x63, 0xE8, 0x30,
    0x5A, 0x17, 0x6F, 0xEF, 0x42, 0xD6, 0x58, 0x7A, 0xF0, 0xCB, 0xD4, 0x40, 0x58, 0x96, 0x32, 0xF4,
]);
const ID_NOT_STORED: Id = Id([
    0x56, 0xD0, 0x4E, 0xAA, 0xC1, 0x7B, 0x55, 0x6B, 0xA0, 0x2C, 0x65, 0x43, 0x39, 0x0A, 0x6C, 0xE9,
    0x1F, 0xD0, 0x0E, 0x20, 0x3E, 0xFB, 0xF5, 0xF9, 0x3F, 0x5B, 0x11, 0x1B, 0x18, 0x73, 0xF6, 0xBB,
    0xAB, 0x9F, 0xF2, 0xD6, 0xBD, 0xBA, 0x25, 0x68, 0x22, 0x30, 0xF2, 0x1F, 0x90, 0x05, 0xF3, 0x64,
    0xE7, 0xEF, 0xC6, 0xB6, 0xA0, 0x85, 0xC9, 0x40, 0x40, 0xF0, 0xB4, 0xB9, 0xD8, 0x28, 0xEE, 0x9C,
]);
const SECRET_EXAMPLE: Secret = Secret([
    0xA9, 0x89, 0x97, 0xFE, 0xAE, 0x97, 0x55, 0x4B, 0x32, 0x35, 0xF0, 0xE8, 0x93, 0xDA, 0xEA, 0x24,
    0x06, 0xAC, 0x36, 0x8B, 0x3C, 0x95, 0x50, 0x16, 0x67, 0x71, 0x65, 0x26, 0xEB, 0xD0, 0xC3, 0x98,
]);

fn get_connection() -> Option<(binder::Strong<dyn ISecretkeeper>, String)> {
    // Initialize logging (which is OK to call multiple times).
    logger::init(logger::Config::default().with_min_level(log::Level::Debug));

    // Determine which instances are available.
    let available = binder::get_declared_instances(SECRETKEEPER_SERVICE).unwrap_or_default();

    // TODO: replace this with a parameterized set of tests that run for each available instance of
    // ISecretkeeper (rather than having a fixed set of instance names to look for).
    for instance in &SECRETKEEPER_INSTANCES {
        if available.iter().find(|s| s == instance).is_none() {
            // Skip undeclared instances.
            continue;
        }
        let name = format!("{SECRETKEEPER_SERVICE}/{instance}");
        match binder::get_interface(&name) {
            Ok(sk) => {
                info!("Running test against /{instance}");
                return Some((sk, name));
            }
            Err(StatusCode::NAME_NOT_FOUND) => {
                info!("No /{instance} instance of ISecretkeeper present");
            }
            Err(e) => {
                panic!(
                    "unexpected error while fetching connection to Secretkeeper {:?}",
                    e
                );
            }
        }
    }
    info!("no Secretkeeper instances in {SECRETKEEPER_INSTANCES:?} are declared and present");
    None
}

/// Macro to perform test setup. Invokes `return` if no Secretkeeper instance available.
macro_rules! setup_client {
    {} => {
        match SkClient::new() {
            Some(sk) => sk,
            None => {
                warn!("Secretkeeper HAL is unavailable, skipping test");
                return;
            }
        }
    }
}

/// Secretkeeper client information.
struct SkClient {
    sk: binder::Strong<dyn ISecretkeeper>,
    name: String,
    session: SkSession,
}

impl Drop for SkClient {
    fn drop(&mut self) {
        // Delete any IDs that may be left over.
        self.delete(&[&ID_EXAMPLE, &ID_EXAMPLE_2]);
    }
}

impl SkClient {
    fn new() -> Option<Self> {
        let (sk, name) = get_connection()?;
        Some(Self {
            sk: sk.clone(),
            name,
            session: SkSession::new(sk).unwrap(),
        })
    }

    /// This method is wrapper that use `SkSession::secret_management_request` which handles
    /// encryption and decryption.
    fn secret_management_request(&mut self, req_data: &[u8]) -> Vec<u8> {
        self.session.secret_management_request(req_data).unwrap()
    }

    /// Unlike the method [`secret_management_request`], this method directly uses
    /// `cipher::encrypt_message` & `cipher::decrypt_message`, allowing finer control of request
    /// & response aad.
    fn secret_management_request_custom_aad(
        &self,
        req_data: &[u8],
        req_aad: &[u8],
        expected_res_aad: &[u8],
    ) -> Vec<u8> {
        let aes_gcm = boring::BoringAes;
        let rng = boring::BoringRng;
        let request_bytes = cipher::encrypt_message(
            &aes_gcm,
            &rng,
            self.session.encryption_key(),
            self.session.session_id(),
            &req_data,
            req_aad,
        )
        .unwrap();

        // Binder call!
        let response_bytes = self
            .sk
            .processSecretManagementRequest(&request_bytes)
            .unwrap();

        let response_encrypt0 = CoseEncrypt0::from_slice(&response_bytes).unwrap();
        cipher::decrypt_message(
            &aes_gcm,
            self.session.decryption_key(),
            &response_encrypt0,
            expected_res_aad,
        )
        .unwrap()
    }

    /// Helper method to store a secret.
    fn store(&mut self, id: &Id, secret: &Secret) {
        let store_request = StoreSecretRequest {
            id: id.clone(),
            secret: secret.clone(),
            sealing_policy: HYPOTHETICAL_DICE_POLICY.to_vec(),
        };
        let store_request = store_request.serialize_to_packet().to_vec().unwrap();

        let store_response = self.secret_management_request(&store_request);
        let store_response = ResponsePacket::from_slice(&store_response).unwrap();

        assert_eq!(
            store_response.response_type().unwrap(),
            ResponseType::Success
        );
        // Really just checking that the response is indeed StoreSecretResponse
        let _ = StoreSecretResponse::deserialize_from_packet(store_response).unwrap();
    }

    /// Helper method to get a secret.
    fn get(&mut self, id: &Id) -> Option<Secret> {
        let get_request = GetSecretRequest {
            id: id.clone(),
            updated_sealing_policy: None,
        };
        let get_request = get_request.serialize_to_packet().to_vec().unwrap();

        let get_response = self.secret_management_request(&get_request);
        let get_response = ResponsePacket::from_slice(&get_response).unwrap();

        if get_response.response_type().unwrap() == ResponseType::Success {
            let get_response = *GetSecretResponse::deserialize_from_packet(get_response).unwrap();
            Some(Secret(get_response.secret.0))
        } else {
            // Only expect a not-found failure.
            let err = *SecretkeeperError::deserialize_from_packet(get_response).unwrap();
            assert_eq!(err, SecretkeeperError::EntryNotFound);
            None
        }
    }

    /// Helper method to delete secrets.
    fn delete(&self, ids: &[&Id]) {
        let ids: Vec<SecretId> = ids
            .iter()
            .map(|id| SecretId { id: id.0 })
            .collect();
        self.sk.deleteIds(&ids).unwrap();
    }

    /// Helper method to delete everything.
    fn delete_all(&self) {
        self.sk.deleteAll().unwrap();
    }
}

/// Perform AuthGraph key exchange, returning the session keys and session ID.
fn authgraph_key_exchange(sk: binder::Strong<dyn ISecretkeeper>) -> ([key::AesKey; 2], Vec<u8>) {
    let sink = sk.getAuthGraphKe().expect("failed to get AuthGraph");
    let mut source = ag_vts::test_ag_participant().expect("failed to create a local source");
    ag_vts::sink::test_mainline(&mut source, sink)
}

/// Test that the AuthGraph instance returned by SecretKeeper correctly performs
/// mainline key exchange against a local source implementation.
#[test]
fn authgraph_mainline() {
    let (sk, _) = match get_connection() {
        Some(sk) => sk,
        None => {
            warn!("Secretkeeper HAL is unavailable, skipping test");
            return;
        }
    };
    let (_aes_keys, _session_id) = authgraph_key_exchange(sk);
}

/// Test that the AuthGraph instance returned by SecretKeeper correctly rejects
/// a corrupted session ID signature.
#[test]
fn authgraph_corrupt_sig() {
    let (sk, _) = match get_connection() {
        Some(sk) => sk,
        None => {
            warn!("Secretkeeper HAL is unavailable, skipping test");
            return;
        }
    };
    let sink = sk.getAuthGraphKe().expect("failed to get AuthGraph");
    let mut source = ag_vts::test_ag_participant().expect("failed to create a local source");
    ag_vts::sink::test_corrupt_sig(&mut source, sink);
}

/// Test that the AuthGraph instance returned by SecretKeeper correctly detects
/// when corrupted keys are returned to it.
#[test]
fn authgraph_corrupt_keys() {
    let (sk, _) = match get_connection() {
        Some(sk) => sk,
        None => {
            warn!("Secretkeeper HAL is unavailable, skipping test");
            return;
        }
    };
    let sink = sk.getAuthGraphKe().expect("failed to get AuthGraph");
    let mut source = ag_vts::test_ag_participant().expect("failed to create a local source");
    ag_vts::sink::test_corrupt_keys(&mut source, sink);
}

// TODO(b/2797757): Add tests that match different HAL defined objects (like request/response)
// with expected bytes.

#[test]
fn secret_management_get_version() {
    let mut sk_client = setup_client!();

    let request = GetVersionRequest {};
    let request_packet = request.serialize_to_packet();
    let request_bytes = request_packet.to_vec().unwrap();

    let response_bytes = sk_client.secret_management_request(&request_bytes);

    let response_packet = ResponsePacket::from_slice(&response_bytes).unwrap();
    assert_eq!(
        response_packet.response_type().unwrap(),
        ResponseType::Success
    );
    let get_version_response =
        *GetVersionResponse::deserialize_from_packet(response_packet).unwrap();
    assert_eq!(get_version_response.version, CURRENT_VERSION);
}

#[test]
fn secret_management_malformed_request() {
    let mut sk_client = setup_client!();

    let request = GetVersionRequest {};
    let request_packet = request.serialize_to_packet();
    let mut request_bytes = request_packet.to_vec().unwrap();

    // Deform the request
    request_bytes[0] = !request_bytes[0];

    let response_bytes = sk_client.secret_management_request(&request_bytes);

    let response_packet = ResponsePacket::from_slice(&response_bytes).unwrap();
    assert_eq!(
        response_packet.response_type().unwrap(),
        ResponseType::Error
    );
    let err = *SecretkeeperError::deserialize_from_packet(response_packet).unwrap();
    assert_eq!(err, SecretkeeperError::RequestMalformed);
}

#[test]
fn secret_management_store_get_secret_found() {
    let mut sk_client = setup_client!();

    sk_client.store(&ID_EXAMPLE, &SECRET_EXAMPLE);

    // Get the secret that was just stored
    assert_eq!(sk_client.get(&ID_EXAMPLE), Some(SECRET_EXAMPLE));
}

#[test]
fn secret_management_store_get_secret_not_found() {
    let mut sk_client = setup_client!();

    // Store a secret (corresponding to an id).
    sk_client.store(&ID_EXAMPLE, &SECRET_EXAMPLE);

    // Get the secret that was never stored
    assert_eq!(sk_client.get(&ID_NOT_STORED), None);
}

#[test]
fn secretkeeper_store_delete_ids() {
    let mut sk_client = setup_client!();

    sk_client.store(&ID_EXAMPLE, &SECRET_EXAMPLE);
    sk_client.store(&ID_EXAMPLE_2, &SECRET_EXAMPLE);
    sk_client.delete(&[&ID_EXAMPLE]);

    assert_eq!(sk_client.get(&ID_EXAMPLE), None);
    assert_eq!(sk_client.get(&ID_EXAMPLE_2), Some(SECRET_EXAMPLE));

    sk_client.delete(&[&ID_EXAMPLE_2]);

    assert_eq!(sk_client.get(&ID_EXAMPLE), None);
    assert_eq!(sk_client.get(&ID_EXAMPLE_2), None);
}

#[test]
fn secretkeeper_store_delete_multiple_ids() {
    let mut sk_client = setup_client!();

    sk_client.store(&ID_EXAMPLE, &SECRET_EXAMPLE);
    sk_client.store(&ID_EXAMPLE_2, &SECRET_EXAMPLE);
    sk_client.delete(&[&ID_EXAMPLE, &ID_EXAMPLE_2]);

    assert_eq!(sk_client.get(&ID_EXAMPLE), None);
    assert_eq!(sk_client.get(&ID_EXAMPLE_2), None);
}

#[test]
fn secretkeeper_store_delete_duplicate_ids() {
    let mut sk_client = setup_client!();

    sk_client.store(&ID_EXAMPLE, &SECRET_EXAMPLE);
    sk_client.store(&ID_EXAMPLE_2, &SECRET_EXAMPLE);
    // Delete the same secret twice.
    sk_client.delete(&[&ID_EXAMPLE, &ID_EXAMPLE]);

    assert_eq!(sk_client.get(&ID_EXAMPLE), None);
    assert_eq!(sk_client.get(&ID_EXAMPLE_2), Some(SECRET_EXAMPLE));
}

#[test]
fn secretkeeper_store_delete_nonexistent() {
    let mut sk_client = setup_client!();

    sk_client.store(&ID_EXAMPLE, &SECRET_EXAMPLE);
    sk_client.store(&ID_EXAMPLE_2, &SECRET_EXAMPLE);
    sk_client.delete(&[&ID_NOT_STORED]);

    assert_eq!(sk_client.get(&ID_EXAMPLE), Some(SECRET_EXAMPLE));
    assert_eq!(sk_client.get(&ID_EXAMPLE_2), Some(SECRET_EXAMPLE));
    assert_eq!(sk_client.get(&ID_NOT_STORED), None);
}

#[test]
fn secretkeeper_store_delete_all() {
    let mut sk_client = setup_client!();

    if sk_client.name != "nonsecure" {
        // Don't run deleteAll() on a secure device, as it might affect
        // real secrets.
        warn!("skipping deleteAll test due to real impl");
        return;
    }

    sk_client.store(&ID_EXAMPLE, &SECRET_EXAMPLE);
    sk_client.store(&ID_EXAMPLE_2, &SECRET_EXAMPLE);

    sk_client.delete_all();

    assert_eq!(sk_client.get(&ID_EXAMPLE), None);
    assert_eq!(sk_client.get(&ID_EXAMPLE_2), None);

    // Store a new secret (corresponding to an id).
    sk_client.store(&ID_EXAMPLE, &SECRET_EXAMPLE);

    // Get the restored secret.
    assert_eq!(sk_client.get(&ID_EXAMPLE), Some(SECRET_EXAMPLE));

    // (Try to) Get the secret that was never stored
    assert_eq!(sk_client.get(&ID_NOT_STORED), None);
}

// This test checks that Secretkeeper uses the expected [`RequestSeqNum`] as aad while
// decrypting requests and the responses are encrypted with correct [`ResponseSeqNum`] for the
// first few messages.
#[test]
fn secret_management_replay_protection_seq_num() {
    let sk_client = setup_client!();
    // Construct encoded request packets for the test
    let (req_1, req_2, req_3) = construct_secret_management_requests();

    // Lets now construct the seq_numbers(in request & expected in response)
    let mut seq_a = SeqNum::new();
    let [seq_0, seq_1, seq_2] = std::array::from_fn(|_| seq_a.get_then_increment().unwrap());

    // Check first request/response is successful
    let res = ResponsePacket::from_slice(
        &sk_client.secret_management_request_custom_aad(&req_1, &seq_0, &seq_0),
    )
    .unwrap();
    assert_eq!(res.response_type().unwrap(), ResponseType::Success);

    // Check 2nd request/response is successful
    let res = ResponsePacket::from_slice(
        &sk_client.secret_management_request_custom_aad(&req_2, &seq_1, &seq_1),
    )
    .unwrap();
    assert_eq!(res.response_type().unwrap(), ResponseType::Success);

    // Check 3rd request/response is successful
    let res = ResponsePacket::from_slice(
        &sk_client.secret_management_request_custom_aad(&req_3, &seq_2, &seq_2),
    )
    .unwrap();
    assert_eq!(res.response_type().unwrap(), ResponseType::Success);
}

// This test checks that Secretkeeper uses fresh [`RequestSeqNum`] & [`ResponseSeqNum`]
// for new sessions.
#[test]
fn secret_management_replay_protection_seq_num_per_session() {
    let sk_client = setup_client!();

    // Construct encoded request packets for the test
    let (req_1, _, _) = construct_secret_management_requests();

    // Lets now construct the seq_number (in request & expected in response)
    let mut seq_a = SeqNum::new();
    let seq_0 = seq_a.get_then_increment().unwrap();
    // Check first request/response is successful
    let res = ResponsePacket::from_slice(
        &sk_client.secret_management_request_custom_aad(&req_1, &seq_0, &seq_0),
    )
    .unwrap();
    assert_eq!(res.response_type().unwrap(), ResponseType::Success);

    // Start another session
    let sk_client_diff = setup_client!();
    // Check first request/response is with seq_0 is successful
    let res = ResponsePacket::from_slice(
        &sk_client_diff.secret_management_request_custom_aad(&req_1, &seq_0, &seq_0),
    )
    .unwrap();
    assert_eq!(res.response_type().unwrap(), ResponseType::Success);
}

// This test checks that Secretkeeper rejects requests with out of order [`RequestSeqNum`]
#[test]
// TODO(b/317416663): This test fails, when HAL is not present in the device. Refactor to fix this.
#[ignore]
#[should_panic]
fn secret_management_replay_protection_out_of_seq_req_not_accepted() {
    let sk_client = setup_client!();

    // Construct encoded request packets for the test
    let (req_1, req_2, _) = construct_secret_management_requests();

    // Lets now construct the seq_numbers(in request & expected in response)
    let mut seq_a = SeqNum::new();
    let [seq_0, seq_1, seq_2] = std::array::from_fn(|_| seq_a.get_then_increment().unwrap());

    // Assume First request/response is successful
    sk_client.secret_management_request_custom_aad(&req_1, &seq_0, &seq_0);

    // Check 2nd request/response with skipped seq_num in request is a binder error
    // This should panic!
    sk_client.secret_management_request_custom_aad(&req_2, /*Skipping seq_1*/ &seq_2, &seq_1);
}

fn construct_secret_management_requests() -> (Vec<u8>, Vec<u8>, Vec<u8>) {
    let version_request = GetVersionRequest {};
    let version_request = version_request.serialize_to_packet().to_vec().unwrap();
    let store_request = StoreSecretRequest {
        id: ID_EXAMPLE,
        secret: SECRET_EXAMPLE,
        sealing_policy: HYPOTHETICAL_DICE_POLICY.to_vec(),
    };
    let store_request = store_request.serialize_to_packet().to_vec().unwrap();
    let get_request = GetSecretRequest {
        id: ID_EXAMPLE,
        updated_sealing_policy: None,
    };
    let get_request = get_request.serialize_to_packet().to_vec().unwrap();
    (version_request, store_request, get_request)
}