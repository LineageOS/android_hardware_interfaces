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

#[cfg(test)]
use binder::StatusCode;
use log::warn;
use secretkeeper_comm::data_types::error::SecretkeeperError;
use secretkeeper_comm::data_types::request::Request;
use secretkeeper_comm::data_types::request_response_impl::{
    GetVersionRequest, GetVersionResponse,
};
use secretkeeper_comm::data_types::response::Response;
use secretkeeper_comm::data_types::packet::{ResponsePacket, ResponseType};
use android_hardware_security_secretkeeper::aidl::android::hardware::security::secretkeeper::ISecretkeeper::ISecretkeeper;
use authgraph_vts_test as ag_vts;
use authgraph_core::key;

const SECRETKEEPER_IDENTIFIER: &str =
    "android.hardware.security.secretkeeper.ISecretkeeper/nonsecure";
const CURRENT_VERSION: u64 = 1;

fn get_connection() -> Option<binder::Strong<dyn ISecretkeeper>> {
    match binder::get_interface(SECRETKEEPER_IDENTIFIER) {
        Ok(sk) => Some(sk),
        Err(StatusCode::NAME_NOT_FOUND) => None,
        Err(e) => {
            panic!(
                "unexpected error while fetching connection to Secretkeeper {:?}",
                e
            );
        }
    }
}
fn authgraph_key_exchange(sk: binder::Strong<dyn ISecretkeeper>) -> [key::AesKey; 2] {
    let sink = sk.getAuthGraphKe().expect("failed to get AuthGraph");
    let mut source = ag_vts::test_ag_participant().expect("failed to create a local source");
    ag_vts::sink::test_mainline(&mut source, sink)
}

/// Test that the AuthGraph instance returned by SecretKeeper correctly performs
/// mainline key exchange against a local source implementation.
#[test]
fn authgraph_mainline() {
    let sk = match get_connection() {
        Some(sk) => sk,
        None => {
            warn!("Secretkeeper HAL is unavailable, skipping test");
            return;
        }
    };
    let _aes_keys = authgraph_key_exchange(sk);
}

/// Test that the AuthGraph instance returned by SecretKeeper correctly rejects
/// a corrupted session ID signature.
#[test]
fn authgraph_corrupt_sig() {
    let sk = match get_connection() {
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
    let sk = match get_connection() {
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
    let secretkeeper = match get_connection() {
        Some(sk) => sk,
        None => {
            warn!("Secretkeeper HAL is unavailable, skipping test");
            return;
        }
    };
    let request = GetVersionRequest {};
    let request_packet = request.serialize_to_packet();
    let request_bytes = request_packet.into_bytes().unwrap();

    // TODO(b/291224769) The request will need to be encrypted & response need to be decrypted
    // with key & related artifacts pre-shared via Authgraph Key Exchange HAL.

    let response_bytes = secretkeeper
        .processSecretManagementRequest(&request_bytes)
        .unwrap();

    let response_packet = ResponsePacket::from_bytes(&response_bytes).unwrap();
    assert_eq!(
        response_packet.response_type().unwrap(),
        ResponseType::Success
    );
    let get_version_response =
        *GetVersionResponse::deserialize_from_packet(response_packet).unwrap();
    assert_eq!(get_version_response.version(), CURRENT_VERSION);
}

#[test]
fn secret_management_malformed_request() {
    let secretkeeper = match get_connection() {
        Some(sk) => sk,
        None => {
            warn!("Secretkeeper HAL is unavailable, skipping test");
            return;
        }
    };
    let request = GetVersionRequest {};
    let request_packet = request.serialize_to_packet();
    let mut request_bytes = request_packet.into_bytes().unwrap();

    // Deform the request
    request_bytes[0] = !request_bytes[0];

    // TODO(b/291224769) The request will need to be encrypted & response need to be decrypted
    // with key & related artifacts pre-shared via Authgraph Key Exchange HAL.

    let response_bytes = secretkeeper
        .processSecretManagementRequest(&request_bytes)
        .unwrap();

    let response_packet = ResponsePacket::from_bytes(&response_bytes).unwrap();
    assert_eq!(
        response_packet.response_type().unwrap(),
        ResponseType::Error
    );
    let err = *SecretkeeperError::deserialize_from_packet(response_packet).unwrap();
    assert_eq!(err, SecretkeeperError::RequestMalformed);
}
