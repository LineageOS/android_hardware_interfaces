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

use android_hardware_security_secretkeeper::aidl::android::hardware::security::secretkeeper::ISecretkeeper::ISecretkeeper;
use android_hardware_security_secretkeeper::aidl::android::hardware::security::secretkeeper::SecretId::SecretId;
use authgraph_vts_test as ag_vts;
use authgraph_boringssl as boring;
use authgraph_core::key;
use coset::{CborOrdering, CborSerializable, CoseEncrypt0, CoseKey};
use dice_policy_builder::{CertIndex, ConstraintSpec, ConstraintType, MissingAction, WILDCARD_FULL_ARRAY, policy_for_dice_chain};
use rdroidtest::{ignore_if, rdroidtest};
use secretkeeper_client::dice::OwnedDiceArtifactsWithExplicitKey;
use secretkeeper_client::{SkSession, Error as SkClientError};
use secretkeeper_core::cipher;
use secretkeeper_comm::data_types::error::SecretkeeperError;
use secretkeeper_comm::data_types::request::Request;
use secretkeeper_comm::data_types::request_response_impl::{
    GetVersionRequest, GetVersionResponse, GetSecretRequest, GetSecretResponse, StoreSecretRequest,
    StoreSecretResponse };
use secretkeeper_comm::data_types::{Id, Secret, SeqNum};
use secretkeeper_comm::data_types::response::Response;
use secretkeeper_comm::data_types::packet::{ResponsePacket, ResponseType};
use secretkeeper_test::{
    AUTHORITY_HASH, MODE, CONFIG_DESC, SECURITY_VERSION, SUBCOMPONENT_AUTHORITY_HASH,
    SUBCOMPONENT_DESCRIPTORS, SUBCOMPONENT_SECURITY_VERSION,
    dice_sample::make_explicit_owned_dice
};
use std::fs;
use std::path::Path;

const SECRETKEEPER_SERVICE: &str = "android.hardware.security.secretkeeper.ISecretkeeper";
const CURRENT_VERSION: u64 = 1;
const SECRETKEEPER_KEY_HOST_DT: &str =
    "/proc/device-tree/avf/reference/avf/secretkeeper_public_key";

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

// Android expects the public key of Secretkeeper instance to be present in the Linux device tree.
// This allows clients to (cryptographically) verify that they are indeed talking to the real
// secretkeeper.
// Note that this is the identity of the `default` instance (and not `nonsecure`)!
fn get_secretkeeper_identity() -> Option<CoseKey> {
    let path = Path::new(SECRETKEEPER_KEY_HOST_DT);
    if path.exists() {
        let key = fs::read(path).unwrap();
        let mut key = CoseKey::from_slice(&key).unwrap();
        key.canonicalize(CborOrdering::Lexicographic);
        Some(key)
    } else {
        None
    }
}

fn get_instances() -> Vec<(String, String)> {
    // Determine which instances are available.
    binder::get_declared_instances(SECRETKEEPER_SERVICE)
        .unwrap_or_default()
        .into_iter()
        .map(|v| (v.clone(), v))
        .collect()
}

fn get_connection(instance: &str) -> binder::Strong<dyn ISecretkeeper> {
    let name = format!("{SECRETKEEPER_SERVICE}/{instance}");
    binder::get_interface(&name).unwrap()
}

/// Secretkeeper client information.
#[derive(Debug)]
struct SkClient {
    sk: binder::Strong<dyn ISecretkeeper>,
    session: SkSession,
    dice_artifacts: OwnedDiceArtifactsWithExplicitKey,
}

impl Drop for SkClient {
    fn drop(&mut self) {
        // Delete any IDs that may be left over.
        self.delete(&[&ID_EXAMPLE, &ID_EXAMPLE_2]);
    }
}

impl SkClient {
    /// Create an `SkClient` using the default `OwnedDiceArtifactsWithExplicitKey` for identity.
    fn new(instance: &str) -> Result<Self, SkClientError> {
        let default_dice = make_explicit_owned_dice(/*Security version in a node */ 5);
        Self::create(instance, default_dice, None)
    }

    /// Create an `SkClient` using given `OwnedDiceArtifactsWithExplicitKey` as client identity.
    fn with_identity(
        instance: &str,
        dice_artifacts: OwnedDiceArtifactsWithExplicitKey,
    ) -> Result<Self, SkClientError> {
        Self::create(instance, dice_artifacts, None)
    }

    /// Create an `SkClient` with default client identity, requiring Secretkeeper's identity to be
    /// matched against given `expected_sk_key`.
    fn with_expected_sk_identity(
        instance: &str,
        expected_sk_key: coset::CoseKey,
    ) -> Result<Self, SkClientError> {
        let default_dice = make_explicit_owned_dice(/*Security version in a node */ 5);
        Self::create(instance, default_dice, Some(expected_sk_key))
    }

    fn create(
        instance: &str,
        dice_artifacts: OwnedDiceArtifactsWithExplicitKey,
        expected_sk_key: Option<coset::CoseKey>,
    ) -> Result<Self, SkClientError> {
        let sk = get_connection(instance);
        Ok(Self {
            sk: sk.clone(),
            session: SkSession::new(sk, &dice_artifacts, expected_sk_key)?,
            dice_artifacts,
        })
    }

    /// This method is wrapper that use `SkSession::secret_management_request` which handles
    /// encryption and decryption.
    fn secret_management_request(&mut self, req_data: &[u8]) -> Result<Vec<u8>, Error> {
        Ok(self.session.secret_management_request(req_data)?)
    }

    /// Unlike the method [`secret_management_request`], this method directly uses
    /// `cipher::encrypt_message` & `cipher::decrypt_message`, allowing finer control of request
    /// & response aad.
    fn secret_management_request_custom_aad(
        &self,
        req_data: &[u8],
        req_aad: &[u8],
        expected_res_aad: &[u8],
    ) -> Result<Vec<u8>, Error> {
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
        .map_err(|e| secretkeeper_client::Error::CipherError(e))?;

        // Binder call!
        let response_bytes = self.sk.processSecretManagementRequest(&request_bytes)?;

        let response_encrypt0 = CoseEncrypt0::from_slice(&response_bytes)?;
        Ok(cipher::decrypt_message(
            &aes_gcm,
            self.session.decryption_key(),
            &response_encrypt0,
            expected_res_aad,
        )
        .map_err(|e| secretkeeper_client::Error::CipherError(e))?)
    }

    /// Helper method to store a secret. This uses the default compatible sealing_policy on
    /// dice_chain.
    fn store(&mut self, id: &Id, secret: &Secret) -> Result<(), Error> {
        let sealing_policy = sealing_policy(
            self.dice_artifacts.explicit_key_dice_chain().ok_or(Error::UnexpectedError)?,
        );
        let store_request =
            StoreSecretRequest { id: id.clone(), secret: secret.clone(), sealing_policy };
        let store_request = store_request.serialize_to_packet().to_vec()?;

        let store_response = self.secret_management_request(&store_request)?;
        let store_response = ResponsePacket::from_slice(&store_response)?;

        assert_eq!(store_response.response_type()?, ResponseType::Success);
        // Really just checking that the response is indeed StoreSecretResponse
        let _ = StoreSecretResponse::deserialize_from_packet(store_response)?;
        Ok(())
    }

    /// Helper method to get a secret.
    fn get(&mut self, id: &Id) -> Result<Secret, Error> {
        self.get_update_policy(id, None)
    }

    /// Helper method to get a secret, updating the sealing policy along the way.
    fn get_update_policy(
        &mut self,
        id: &Id,
        updated_sealing_policy: Option<Vec<u8>>,
    ) -> Result<Secret, Error> {
        let get_request = GetSecretRequest { id: id.clone(), updated_sealing_policy };
        let get_request = get_request.serialize_to_packet().to_vec()?;

        let get_response = self.secret_management_request(&get_request)?;
        let get_response = ResponsePacket::from_slice(&get_response)?;

        if get_response.response_type()? == ResponseType::Success {
            let get_response = *GetSecretResponse::deserialize_from_packet(get_response)?;
            Ok(Secret(get_response.secret.0))
        } else {
            let err = *SecretkeeperError::deserialize_from_packet(get_response)?;
            Err(Error::SecretkeeperError(err))
        }
    }

    /// Helper method to delete secrets.
    fn delete(&self, ids: &[&Id]) {
        let ids: Vec<SecretId> = ids.iter().map(|id| SecretId { id: id.0 }).collect();
        self.sk.deleteIds(&ids).unwrap();
    }

    /// Helper method to delete everything.
    fn delete_all(&self) {
        self.sk.deleteAll().unwrap();
    }
}

#[derive(Debug)]
enum Error {
    // Errors from Secretkeeper API errors. These are thrown by core SecretManagement and
    // not visible without decryption.
    SecretkeeperError(SecretkeeperError),
    InfraError(secretkeeper_client::Error),
    UnexpectedError,
}

impl From<secretkeeper_client::Error> for Error {
    fn from(e: secretkeeper_client::Error) -> Self {
        Self::InfraError(e)
    }
}

impl From<SecretkeeperError> for Error {
    fn from(e: SecretkeeperError) -> Self {
        Self::SecretkeeperError(e)
    }
}

impl From<coset::CoseError> for Error {
    fn from(e: coset::CoseError) -> Self {
        Self::InfraError(secretkeeper_client::Error::from(e))
    }
}

impl From<binder::Status> for Error {
    fn from(s: binder::Status) -> Self {
        Self::InfraError(secretkeeper_client::Error::from(s))
    }
}

impl From<secretkeeper_comm::data_types::error::Error> for Error {
    fn from(e: secretkeeper_comm::data_types::error::Error) -> Self {
        Self::InfraError(secretkeeper_client::Error::from(e))
    }
}

// Assert that the error is EntryNotFound
fn assert_entry_not_found(res: Result<Secret, Error>) {
    assert!(matches!(res.unwrap_err(), Error::SecretkeeperError(SecretkeeperError::EntryNotFound)))
}

/// Construct a sealing policy on the dice chain. This method uses the following set of
/// constraints which are compatible with sample DICE chains used in VTS.
/// 1. ExactMatch on AUTHORITY_HASH (non-optional).
/// 2. ExactMatch on MODE (non-optional).
/// 3. GreaterOrEqual on SECURITY_VERSION (optional).
/// 4. The second last DiceChainEntry contain SubcomponentDescriptor, for each of those:
///     a) GreaterOrEqual on SECURITY_VERSION (Required)
//      b) ExactMatch on AUTHORITY_HASH (Required).
fn sealing_policy(dice: &[u8]) -> Vec<u8> {
    let constraint_spec = [
        ConstraintSpec::new(
            ConstraintType::ExactMatch,
            vec![AUTHORITY_HASH],
            MissingAction::Fail,
            CertIndex::All,
        ),
        ConstraintSpec::new(
            ConstraintType::ExactMatch,
            vec![MODE],
            MissingAction::Fail,
            CertIndex::All,
        ),
        ConstraintSpec::new(
            ConstraintType::GreaterOrEqual,
            vec![CONFIG_DESC, SECURITY_VERSION],
            MissingAction::Ignore,
            CertIndex::All,
        ),
        // Constraints on sub components in the second last DiceChainEntry
        ConstraintSpec::new(
            ConstraintType::GreaterOrEqual,
            vec![
                CONFIG_DESC,
                SUBCOMPONENT_DESCRIPTORS,
                WILDCARD_FULL_ARRAY,
                SUBCOMPONENT_SECURITY_VERSION,
            ],
            MissingAction::Fail,
            CertIndex::FromEnd(1),
        ),
        ConstraintSpec::new(
            ConstraintType::ExactMatch,
            vec![
                CONFIG_DESC,
                SUBCOMPONENT_DESCRIPTORS,
                WILDCARD_FULL_ARRAY,
                SUBCOMPONENT_AUTHORITY_HASH,
            ],
            MissingAction::Fail,
            CertIndex::FromEnd(1),
        ),
    ];

    policy_for_dice_chain(dice, &constraint_spec).unwrap().to_vec().unwrap()
}

/// Perform AuthGraph key exchange, returning the session keys and session ID.
fn authgraph_key_exchange(sk: binder::Strong<dyn ISecretkeeper>) -> ([key::AesKey; 2], Vec<u8>) {
    let sink = sk.getAuthGraphKe().expect("failed to get AuthGraph");
    let mut source = ag_vts::test_ag_participant().expect("failed to create a local source");
    ag_vts::sink::test_mainline(&mut source, sink)
}

// Test that the AuthGraph instance returned by SecretKeeper correctly performs
// mainline key exchange against a local source implementation.
#[rdroidtest(get_instances())]
fn authgraph_mainline(instance: String) {
    let sk = get_connection(&instance);
    let (_aes_keys, _session_id) = authgraph_key_exchange(sk);
}

// Test that the AuthGraph instance returned by SecretKeeper correctly rejects
// a corrupted session ID signature.
#[rdroidtest(get_instances())]
fn authgraph_corrupt_sig(instance: String) {
    let sk = get_connection(&instance);
    let sink = sk.getAuthGraphKe().expect("failed to get AuthGraph");
    let mut source = ag_vts::test_ag_participant().expect("failed to create a local source");
    ag_vts::sink::test_corrupt_sig(&mut source, sink);
}

// Test that the AuthGraph instance returned by SecretKeeper correctly detects
// when corrupted keys are returned to it.
#[rdroidtest(get_instances())]
fn authgraph_corrupt_keys(instance: String) {
    let sk = get_connection(&instance);
    let sink = sk.getAuthGraphKe().expect("failed to get AuthGraph");
    let mut source = ag_vts::test_ag_participant().expect("failed to create a local source");
    ag_vts::sink::test_corrupt_keys(&mut source, sink);
}

// TODO(b/2797757): Add tests that match different HAL defined objects (like request/response)
// with expected bytes.

#[rdroidtest(get_instances())]
fn secret_management_get_version(instance: String) {
    let mut sk_client = SkClient::new(&instance).unwrap();

    let request = GetVersionRequest {};
    let request_packet = request.serialize_to_packet();
    let request_bytes = request_packet.to_vec().unwrap();

    let response_bytes = sk_client.secret_management_request(&request_bytes).unwrap();

    let response_packet = ResponsePacket::from_slice(&response_bytes).unwrap();
    assert_eq!(response_packet.response_type().unwrap(), ResponseType::Success);
    let get_version_response =
        *GetVersionResponse::deserialize_from_packet(response_packet).unwrap();
    assert_eq!(get_version_response.version, CURRENT_VERSION);
}

#[rdroidtest(get_instances())]
fn secret_management_malformed_request(instance: String) {
    let mut sk_client = SkClient::new(&instance).unwrap();

    let request = GetVersionRequest {};
    let request_packet = request.serialize_to_packet();
    let mut request_bytes = request_packet.to_vec().unwrap();

    // Deform the request
    request_bytes[0] = !request_bytes[0];

    let response_bytes = sk_client.secret_management_request(&request_bytes).unwrap();

    let response_packet = ResponsePacket::from_slice(&response_bytes).unwrap();
    assert_eq!(response_packet.response_type().unwrap(), ResponseType::Error);
    let err = *SecretkeeperError::deserialize_from_packet(response_packet).unwrap();
    assert_eq!(err, SecretkeeperError::RequestMalformed);
}

#[rdroidtest(get_instances())]
fn secret_management_store_get_secret_found(instance: String) {
    let mut sk_client = SkClient::new(&instance).unwrap();

    sk_client.store(&ID_EXAMPLE, &SECRET_EXAMPLE).unwrap();

    // Get the secret that was just stored
    assert_eq!(sk_client.get(&ID_EXAMPLE).unwrap(), SECRET_EXAMPLE);
}

#[rdroidtest(get_instances())]
fn secret_management_store_get_secret_not_found(instance: String) {
    let mut sk_client = SkClient::new(&instance).unwrap();

    // Store a secret (corresponding to an id).
    sk_client.store(&ID_EXAMPLE, &SECRET_EXAMPLE).unwrap();

    // Get the secret that was never stored
    assert_entry_not_found(sk_client.get(&ID_NOT_STORED));
}

#[rdroidtest(get_instances())]
fn secretkeeper_store_delete_ids(instance: String) {
    let mut sk_client = SkClient::new(&instance).unwrap();

    sk_client.store(&ID_EXAMPLE, &SECRET_EXAMPLE).unwrap();
    sk_client.store(&ID_EXAMPLE_2, &SECRET_EXAMPLE).unwrap();
    sk_client.delete(&[&ID_EXAMPLE]);
    assert_entry_not_found(sk_client.get(&ID_EXAMPLE));
    assert_eq!(sk_client.get(&ID_EXAMPLE_2).unwrap(), SECRET_EXAMPLE);

    sk_client.delete(&[&ID_EXAMPLE_2]);

    assert_entry_not_found(sk_client.get(&ID_EXAMPLE));
    assert_entry_not_found(sk_client.get(&ID_EXAMPLE_2));
}

#[rdroidtest(get_instances())]
fn secretkeeper_store_delete_multiple_ids(instance: String) {
    let mut sk_client = SkClient::new(&instance).unwrap();

    sk_client.store(&ID_EXAMPLE, &SECRET_EXAMPLE).unwrap();
    sk_client.store(&ID_EXAMPLE_2, &SECRET_EXAMPLE).unwrap();
    sk_client.delete(&[&ID_EXAMPLE, &ID_EXAMPLE_2]);

    assert_entry_not_found(sk_client.get(&ID_EXAMPLE));
    assert_entry_not_found(sk_client.get(&ID_EXAMPLE_2));
}
#[rdroidtest(get_instances())]
fn secretkeeper_store_delete_duplicate_ids(instance: String) {
    let mut sk_client = SkClient::new(&instance).unwrap();

    sk_client.store(&ID_EXAMPLE, &SECRET_EXAMPLE).unwrap();
    sk_client.store(&ID_EXAMPLE_2, &SECRET_EXAMPLE).unwrap();
    // Delete the same secret twice.
    sk_client.delete(&[&ID_EXAMPLE, &ID_EXAMPLE]);

    assert_entry_not_found(sk_client.get(&ID_EXAMPLE));
    assert_eq!(sk_client.get(&ID_EXAMPLE_2).unwrap(), SECRET_EXAMPLE);
}

#[rdroidtest(get_instances())]
fn secretkeeper_store_delete_nonexistent(instance: String) {
    let mut sk_client = SkClient::new(&instance).unwrap();

    sk_client.store(&ID_EXAMPLE, &SECRET_EXAMPLE).unwrap();
    sk_client.store(&ID_EXAMPLE_2, &SECRET_EXAMPLE).unwrap();
    sk_client.delete(&[&ID_NOT_STORED]);

    assert_eq!(sk_client.get(&ID_EXAMPLE).unwrap(), SECRET_EXAMPLE);
    assert_eq!(sk_client.get(&ID_EXAMPLE_2).unwrap(), SECRET_EXAMPLE);
    assert_entry_not_found(sk_client.get(&ID_NOT_STORED));
}

// Don't run deleteAll() on a secure device, as it might affect real secrets.
#[rdroidtest(get_instances())]
#[ignore_if(|p| p != "nonsecure")]
fn secretkeeper_store_delete_all(instance: String) {
    let mut sk_client = SkClient::new(&instance).unwrap();

    sk_client.store(&ID_EXAMPLE, &SECRET_EXAMPLE).unwrap();
    sk_client.store(&ID_EXAMPLE_2, &SECRET_EXAMPLE).unwrap();

    sk_client.delete_all();

    assert_entry_not_found(sk_client.get(&ID_EXAMPLE));
    assert_entry_not_found(sk_client.get(&ID_EXAMPLE_2));

    // Store a new secret (corresponding to an id).
    sk_client.store(&ID_EXAMPLE, &SECRET_EXAMPLE).unwrap();

    // Get the restored secret.
    assert_eq!(sk_client.get(&ID_EXAMPLE).unwrap(), SECRET_EXAMPLE);

    // (Try to) Get the secret that was never stored
    assert_entry_not_found(sk_client.get(&ID_NOT_STORED));
}

// This test checks that Secretkeeper uses the expected [`RequestSeqNum`] as aad while
// decrypting requests and the responses are encrypted with correct [`ResponseSeqNum`] for the
// first few messages.
#[rdroidtest(get_instances())]
fn secret_management_replay_protection_seq_num(instance: String) {
    let dice_chain = make_explicit_owned_dice(/*Security version in a node */ 5);
    let sealing_policy = sealing_policy(dice_chain.explicit_key_dice_chain().unwrap());
    let sk_client = SkClient::with_identity(&instance, dice_chain).unwrap();
    // Construct encoded request packets for the test
    let (req_1, req_2, req_3) = construct_secret_management_requests(sealing_policy);

    // Lets now construct the seq_numbers(in request & expected in response)
    let mut seq_a = SeqNum::new();
    let [seq_0, seq_1, seq_2] = std::array::from_fn(|_| seq_a.get_then_increment().unwrap());

    // Check first request/response is successful
    let res = ResponsePacket::from_slice(
        &sk_client.secret_management_request_custom_aad(&req_1, &seq_0, &seq_0).unwrap(),
    )
    .unwrap();
    assert_eq!(res.response_type().unwrap(), ResponseType::Success);

    // Check 2nd request/response is successful
    let res = ResponsePacket::from_slice(
        &sk_client.secret_management_request_custom_aad(&req_2, &seq_1, &seq_1).unwrap(),
    )
    .unwrap();
    assert_eq!(res.response_type().unwrap(), ResponseType::Success);

    // Check 3rd request/response is successful
    let res = ResponsePacket::from_slice(
        &sk_client.secret_management_request_custom_aad(&req_3, &seq_2, &seq_2).unwrap(),
    )
    .unwrap();
    assert_eq!(res.response_type().unwrap(), ResponseType::Success);
}

// This test checks that Secretkeeper uses fresh [`RequestSeqNum`] & [`ResponseSeqNum`]
// for new sessions.
#[rdroidtest(get_instances())]
fn secret_management_replay_protection_seq_num_per_session(instance: String) {
    let dice_chain = make_explicit_owned_dice(/*Security version in a node */ 5);
    let sealing_policy = sealing_policy(dice_chain.explicit_key_dice_chain().unwrap());
    let sk_client = SkClient::with_identity(&instance, dice_chain).unwrap();

    // Construct encoded request packets for the test
    let (req_1, _, _) = construct_secret_management_requests(sealing_policy);

    // Lets now construct the seq_number (in request & expected in response)
    let mut seq_a = SeqNum::new();
    let seq_0 = seq_a.get_then_increment().unwrap();
    // Check first request/response is successful
    let res = ResponsePacket::from_slice(
        &sk_client.secret_management_request_custom_aad(&req_1, &seq_0, &seq_0).unwrap(),
    )
    .unwrap();
    assert_eq!(res.response_type().unwrap(), ResponseType::Success);

    // Start another session
    let sk_client_diff = SkClient::new(&instance).unwrap();
    // Check first request/response is with seq_0 is successful
    let res = ResponsePacket::from_slice(
        &sk_client_diff.secret_management_request_custom_aad(&req_1, &seq_0, &seq_0).unwrap(),
    )
    .unwrap();
    assert_eq!(res.response_type().unwrap(), ResponseType::Success);
}

// This test checks that Secretkeeper rejects requests with out of order [`RequestSeqNum`]
// TODO(b/317416663): This test fails, when HAL is not present in the device. Refactor to fix this.
#[rdroidtest(get_instances())]
fn secret_management_replay_protection_out_of_seq_req_not_accepted(instance: String) {
    let dice_chain = make_explicit_owned_dice(/*Security version in a node */ 5);
    let sealing_policy = sealing_policy(dice_chain.explicit_key_dice_chain().unwrap());
    let sk_client = SkClient::with_identity(&instance, dice_chain).unwrap();

    // Construct encoded request packets for the test
    let (req_1, req_2, _) = construct_secret_management_requests(sealing_policy);

    // Lets now construct the seq_numbers(in request & expected in response)
    let mut seq_a = SeqNum::new();
    let [seq_0, seq_1, seq_2] = std::array::from_fn(|_| seq_a.get_then_increment().unwrap());

    // Assume First request/response is successful
    sk_client.secret_management_request_custom_aad(&req_1, &seq_0, &seq_0).unwrap();

    // Check 2nd request/response with skipped seq_num in request is a binder error
    let res = sk_client
        .secret_management_request_custom_aad(&req_2, /*Skipping seq_1*/ &seq_2, &seq_1);
    let err = res.expect_err("Out of Seq messages accepted!");
    // Incorrect sequence numbers lead to failed decryption. The resultant error should be
    // thrown in clear text & wrapped in Binder errors.
    assert!(matches!(err, Error::InfraError(secretkeeper_client::Error::BinderStatus(_e))));
}

// This test checks DICE policy based access control of Secretkeeper.
#[rdroidtest(get_instances())]
fn secret_management_policy_gate(instance: String) {
    let dice_chain = make_explicit_owned_dice(/*Security version in a node */ 100);
    let mut sk_client_original = SkClient::with_identity(&instance, dice_chain).unwrap();
    sk_client_original.store(&ID_EXAMPLE, &SECRET_EXAMPLE).unwrap();
    assert_eq!(sk_client_original.get(&ID_EXAMPLE).unwrap(), SECRET_EXAMPLE);

    // Start a session with higher security_version & get the stored secret.
    let dice_chain_upgraded = make_explicit_owned_dice(/*Security version in a node */ 101);
    let mut sk_client_upgraded = SkClient::with_identity(&instance, dice_chain_upgraded).unwrap();
    assert_eq!(sk_client_upgraded.get(&ID_EXAMPLE).unwrap(), SECRET_EXAMPLE);

    // Start a session with lower security_version (This should be denied access to the secret).
    let dice_chain_downgraded = make_explicit_owned_dice(/*Security version in a node */ 99);
    let mut sk_client_downgraded =
        SkClient::with_identity(&instance, dice_chain_downgraded).unwrap();
    assert!(matches!(
        sk_client_downgraded.get(&ID_EXAMPLE).unwrap_err(),
        Error::SecretkeeperError(SecretkeeperError::DicePolicyError)
    ));

    // Now get the secret with the later version, and upgrade the sealing policy along the way.
    let sealing_policy =
        sealing_policy(sk_client_upgraded.dice_artifacts.explicit_key_dice_chain().unwrap());
    assert_eq!(
        sk_client_upgraded.get_update_policy(&ID_EXAMPLE, Some(sealing_policy)).unwrap(),
        SECRET_EXAMPLE
    );

    // The original version of the client should no longer be able to retrieve the secret.
    assert!(matches!(
        sk_client_original.get(&ID_EXAMPLE).unwrap_err(),
        Error::SecretkeeperError(SecretkeeperError::DicePolicyError)
    ));
}

// This test checks that the identity of Secretkeeper (in context of AuthGraph key exchange) is
// same as the one advertized in Linux device tree. This is only expected from `default` instance.
#[rdroidtest(get_instances())]
#[ignore_if(|p| p != "default")]
fn secretkeeper_check_identity(instance: String) {
    let sk_key = get_secretkeeper_identity()
        .expect("Failed to extract identity of default instance from device tree");
    // Create a session with this expected identity. This succeeds only if the identity used by
    // Secretkeeper is sk_key.
    let _ = SkClient::with_expected_sk_identity(&instance, sk_key).unwrap();
    // Create a session using any other expected sk identity, this should fail. Note that the
    // failure arises from validation which happens at the local participant.
    let mut any_other_key = CoseKey::default();
    any_other_key.canonicalize(CborOrdering::Lexicographic);
    let err = SkClient::with_expected_sk_identity(&instance, any_other_key).unwrap_err();
    assert!(matches!(
        err,
        SkClientError::AuthgraphError(authgraph_core::error::Error(
            authgraph_wire::ErrorCode::InvalidPeerKeKey,
            _
        ))
    ));
}

// Helper method that constructs 3 SecretManagement requests. Callers would usually not care about
// what each of the request concretely is.
fn construct_secret_management_requests(sealing_policy: Vec<u8>) -> (Vec<u8>, Vec<u8>, Vec<u8>) {
    let version_request = GetVersionRequest {};
    let version_request = version_request.serialize_to_packet().to_vec().unwrap();
    let store_request =
        StoreSecretRequest { id: ID_EXAMPLE, secret: SECRET_EXAMPLE, sealing_policy };
    let store_request = store_request.serialize_to_packet().to_vec().unwrap();
    let get_request = GetSecretRequest { id: ID_EXAMPLE, updated_sealing_policy: None };
    let get_request = get_request.serialize_to_packet().to_vec().unwrap();
    (version_request, store_request, get_request)
}

rdroidtest::test_main!();
