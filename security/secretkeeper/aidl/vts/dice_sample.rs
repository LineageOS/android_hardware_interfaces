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

//! This module provides a set of sample DICE chains for testing purpose only. Note that this
//! module duplicates a large chunk of code in libdiced_sample_inputs. We avoid modifying the
//! latter for testing purposes because it is installed on device.

use crate::{
    COMPONENT_NAME, COMPONENT_RESETTABLE, COMPONENT_VERSION, SUBCOMPONENT_AUTHORITY_HASH,
    SUBCOMPONENT_CODE_HASH, SUBCOMPONENT_DESCRIPTORS, SUBCOMPONENT_NAME,
    SUBCOMPONENT_SECURITY_VERSION,
};
use ciborium::{cbor, de, ser, value::Value};
use core::ffi::CStr;
use coset::{
    iana, Algorithm, AsCborValue, CborSerializable, CoseKey, KeyOperation, KeyType, Label,
};
use diced_open_dice::{
    derive_cdi_private_key_seed, keypair_from_seed, retry_bcc_format_config_descriptor,
    retry_bcc_main_flow, retry_dice_main_flow, Config, DiceArtifacts, DiceConfigValues, DiceError,
    DiceMode, InputValues, OwnedDiceArtifacts, CDI_SIZE, HASH_SIZE, HIDDEN_SIZE,
};
use log::error;
use secretkeeper_client::dice::OwnedDiceArtifactsWithExplicitKey;

/// Sample UDS used to perform the root DICE flow by `make_sample_bcc_and_cdis`.
const UDS: &[u8; CDI_SIZE] = &[
    0x65, 0x4f, 0xab, 0xa9, 0xa5, 0xad, 0x0f, 0x5e, 0x15, 0xc3, 0x12, 0xf7, 0x77, 0x45, 0xfa, 0x55,
    0x18, 0x6a, 0xa6, 0x34, 0xb6, 0x7c, 0x82, 0x7b, 0x89, 0x4c, 0xc5, 0x52, 0xd3, 0x27, 0x35, 0x8e,
];

const CODE_HASH_ABL: [u8; HASH_SIZE] = [
    0x16, 0x48, 0xf2, 0x55, 0x53, 0x23, 0xdd, 0x15, 0x2e, 0x83, 0x38, 0xc3, 0x64, 0x38, 0x63, 0x26,
    0x0f, 0xcf, 0x5b, 0xd1, 0x3a, 0xd3, 0x40, 0x3e, 0x23, 0xf8, 0x34, 0x4c, 0x6d, 0xa2, 0xbe, 0x25,
    0x1c, 0xb0, 0x29, 0xe8, 0xc3, 0xfb, 0xb8, 0x80, 0xdc, 0xb1, 0xd2, 0xb3, 0x91, 0x4d, 0xd3, 0xfb,
    0x01, 0x0f, 0xe4, 0xe9, 0x46, 0xa2, 0xc0, 0x26, 0x57, 0x5a, 0xba, 0x30, 0xf7, 0x15, 0x98, 0x14,
];
const AUTHORITY_HASH_ABL: [u8; HASH_SIZE] = [
    0xf9, 0x00, 0x9d, 0xc2, 0x59, 0x09, 0xe0, 0xb6, 0x98, 0xbd, 0xe3, 0x97, 0x4a, 0xcb, 0x3c, 0xe7,
    0x6b, 0x24, 0xc3, 0xe4, 0x98, 0xdd, 0xa9, 0x6a, 0x41, 0x59, 0x15, 0xb1, 0x23, 0xe6, 0xc8, 0xdf,
    0xfb, 0x52, 0xb4, 0x52, 0xc1, 0xb9, 0x61, 0xdd, 0xbc, 0x5b, 0x37, 0x0e, 0x12, 0x12, 0xb2, 0xfd,
    0xc1, 0x09, 0xb0, 0xcf, 0x33, 0x81, 0x4c, 0xc6, 0x29, 0x1b, 0x99, 0xea, 0xae, 0xfd, 0xaa, 0x0d,
];
const HIDDEN_ABL: [u8; HIDDEN_SIZE] = [
    0xa2, 0x01, 0xd0, 0xc0, 0xaa, 0x75, 0x3c, 0x06, 0x43, 0x98, 0x6c, 0xc3, 0x5a, 0xb5, 0x5f, 0x1f,
    0x0f, 0x92, 0x44, 0x3b, 0x0e, 0xd4, 0x29, 0x75, 0xe3, 0xdb, 0x36, 0xda, 0xc8, 0x07, 0x97, 0x4d,
    0xff, 0xbc, 0x6a, 0xa4, 0x8a, 0xef, 0xc4, 0x7f, 0xf8, 0x61, 0x7d, 0x51, 0x4d, 0x2f, 0xdf, 0x7e,
    0x8c, 0x3d, 0xa3, 0xfc, 0x63, 0xd4, 0xd4, 0x74, 0x8a, 0xc4, 0x14, 0x45, 0x83, 0x6b, 0x12, 0x7e,
];
const CODE_HASH_AVB: [u8; HASH_SIZE] = [
    0xa4, 0x0c, 0xcb, 0xc1, 0xbf, 0xfa, 0xcc, 0xfd, 0xeb, 0xf4, 0xfc, 0x43, 0x83, 0x7f, 0x46, 0x8d,
    0xd8, 0xd8, 0x14, 0xc1, 0x96, 0x14, 0x1f, 0x6e, 0xb3, 0xa0, 0xd9, 0x56, 0xb3, 0xbf, 0x2f, 0xfa,
    0x88, 0x70, 0x11, 0x07, 0x39, 0xa4, 0xd2, 0xa9, 0x6b, 0x18, 0x28, 0xe8, 0x29, 0x20, 0x49, 0x0f,
    0xbb, 0x8d, 0x08, 0x8c, 0xc6, 0x54, 0xe9, 0x71, 0xd2, 0x7e, 0xa4, 0xfe, 0x58, 0x7f, 0xd3, 0xc7,
];
const AUTHORITY_HASH_AVB: [u8; HASH_SIZE] = [
    0xb2, 0x69, 0x05, 0x48, 0x56, 0xb5, 0xfa, 0x55, 0x6f, 0xac, 0x56, 0xd9, 0x02, 0x35, 0x2b, 0xaa,
    0x4c, 0xba, 0x28, 0xdd, 0x82, 0x3a, 0x86, 0xf5, 0xd4, 0xc2, 0xf1, 0xf9, 0x35, 0x7d, 0xe4, 0x43,
    0x13, 0xbf, 0xfe, 0xd3, 0x36, 0xd8, 0x1c, 0x12, 0x78, 0x5c, 0x9c, 0x3e, 0xf6, 0x66, 0xef, 0xab,
    0x3d, 0x0f, 0x89, 0xa4, 0x6f, 0xc9, 0x72, 0xee, 0x73, 0x43, 0x02, 0x8a, 0xef, 0xbc, 0x05, 0x98,
];
const HIDDEN_AVB: [u8; HIDDEN_SIZE] = [
    0x5b, 0x3f, 0xc9, 0x6b, 0xe3, 0x95, 0x59, 0x40, 0x5e, 0x64, 0xe5, 0x64, 0x3f, 0xfd, 0x21, 0x09,
    0x9d, 0xf3, 0xcd, 0xc7, 0xa4, 0x2a, 0xe2, 0x97, 0xdd, 0xe2, 0x4f, 0xb0, 0x7d, 0x7e, 0xf5, 0x8e,
    0xd6, 0x4d, 0x84, 0x25, 0x54, 0x41, 0x3f, 0x8f, 0x78, 0x64, 0x1a, 0x51, 0x27, 0x9d, 0x55, 0x8a,
    0xe9, 0x90, 0x35, 0xab, 0x39, 0x80, 0x4b, 0x94, 0x40, 0x84, 0xa2, 0xfd, 0x73, 0xeb, 0x35, 0x7a,
];
const AUTHORITY_HASH_ANDROID: [u8; HASH_SIZE] = [
    0x04, 0x25, 0x5d, 0x60, 0x5f, 0x5c, 0x45, 0x0d, 0xf2, 0x9a, 0x6e, 0x99, 0x30, 0x03, 0xb8, 0xd6,
    0xe1, 0x99, 0x71, 0x1b, 0xf8, 0x44, 0xfa, 0xb5, 0x31, 0x79, 0x1c, 0x37, 0x68, 0x4e, 0x1d, 0xc0,
    0x24, 0x74, 0x68, 0xf8, 0x80, 0x20, 0x3e, 0x44, 0xb1, 0x43, 0xd2, 0x9c, 0xfc, 0x12, 0x9e, 0x77,
    0x0a, 0xde, 0x29, 0x24, 0xff, 0x2e, 0xfa, 0xc7, 0x10, 0xd5, 0x73, 0xd4, 0xc6, 0xdf, 0x62, 0x9f,
];

/// Encode the public key to CBOR Value. The input (raw 32 bytes) is wrapped into CoseKey.
fn ed25519_public_key_to_cbor_value(public_key: &[u8]) -> Value {
    let key = CoseKey {
        kty: KeyType::Assigned(iana::KeyType::OKP),
        alg: Some(Algorithm::Assigned(iana::Algorithm::EdDSA)),
        key_ops: vec![KeyOperation::Assigned(iana::KeyOperation::Verify)].into_iter().collect(),
        params: vec![
            (
                Label::Int(iana::Ec2KeyParameter::Crv as i64),
                Value::from(iana::EllipticCurve::Ed25519 as u64),
            ),
            (Label::Int(iana::Ec2KeyParameter::X as i64), Value::Bytes(public_key.to_vec())),
        ],
        ..Default::default()
    };
    key.to_cbor_value().unwrap()
}

/// Makes a DICE chain (BCC) from the sample input.
///
/// The DICE chain is of the following format:
/// public key derived from UDS -> ABL certificate -> AVB certificate -> Android certificate
/// The `security_version` is included in the Android certificate as well as each subcomponent
/// of AVB certificate.
pub fn make_explicit_owned_dice(security_version: u64) -> OwnedDiceArtifactsWithExplicitKey {
    let dice = make_sample_bcc_and_cdis(security_version);
    OwnedDiceArtifactsWithExplicitKey::from_owned_artifacts(dice).unwrap()
}

fn make_sample_bcc_and_cdis(security_version: u64) -> OwnedDiceArtifacts {
    let private_key_seed = derive_cdi_private_key_seed(UDS).unwrap();

    // Gets the root public key in DICE chain (BCC).
    let (public_key, _) = keypair_from_seed(private_key_seed.as_array()).unwrap();
    let ed25519_public_key_value = ed25519_public_key_to_cbor_value(&public_key);

    // Gets the ABL certificate to as the root certificate of DICE chain.
    let config_values = DiceConfigValues {
        component_name: Some(CStr::from_bytes_with_nul(b"ABL\0").unwrap()),
        component_version: Some(1),
        resettable: true,
        ..Default::default()
    };
    let config_descriptor = retry_bcc_format_config_descriptor(&config_values).unwrap();
    let input_values = InputValues::new(
        CODE_HASH_ABL,
        Config::Descriptor(config_descriptor.as_slice()),
        AUTHORITY_HASH_ABL,
        DiceMode::kDiceModeNormal,
        HIDDEN_ABL,
    );
    let (cdi_values, cert) = retry_dice_main_flow(UDS, UDS, &input_values).unwrap();
    let bcc_value =
        Value::Array(vec![ed25519_public_key_value, de::from_reader(&cert[..]).unwrap()]);
    let mut bcc: Vec<u8> = vec![];
    ser::into_writer(&bcc_value, &mut bcc).unwrap();

    // Appends AVB certificate to DICE chain.
    let config_desc = cbor!({
        COMPONENT_NAME => "AVB",
        COMPONENT_VERSION => 1,
        COMPONENT_RESETTABLE => null,
        SUBCOMPONENT_DESCRIPTORS => [
            {
                SUBCOMPONENT_NAME => "sub_1",
                SUBCOMPONENT_SECURITY_VERSION => security_version,
                SUBCOMPONENT_CODE_HASH=> b"xoxo",
                SUBCOMPONENT_AUTHORITY_HASH => b"oxox"
            },
            {
                SUBCOMPONENT_NAME => "sub_2",
                SUBCOMPONENT_SECURITY_VERSION => security_version,
                SUBCOMPONENT_CODE_HASH => b"xoxo",
                SUBCOMPONENT_AUTHORITY_HASH => b"oxox",
            }
        ]
    })
    .unwrap()
    .to_vec()
    .unwrap();
    let input_values = InputValues::new(
        CODE_HASH_AVB,
        Config::Descriptor(&config_desc),
        AUTHORITY_HASH_AVB,
        DiceMode::kDiceModeNormal,
        HIDDEN_AVB,
    );
    let dice_artifacts =
        retry_bcc_main_flow(&cdi_values.cdi_attest, &cdi_values.cdi_seal, &bcc, &input_values)
            .unwrap();

    // Appends Android certificate to DICE chain.
    let config_values = DiceConfigValues {
        component_name: Some(CStr::from_bytes_with_nul(b"Android\0").unwrap()),
        component_version: Some(12),
        security_version: Some(security_version),
        resettable: true,
        ..Default::default()
    };
    let config_descriptor = retry_bcc_format_config_descriptor(&config_values).unwrap();
    let input_values = InputValues::new(
        [0u8; HASH_SIZE], // code_hash
        Config::Descriptor(config_descriptor.as_slice()),
        AUTHORITY_HASH_ANDROID,
        DiceMode::kDiceModeNormal,
        [0u8; HIDDEN_SIZE], // hidden
    );
    retry_bcc_main_flow(
        dice_artifacts.cdi_attest(),
        dice_artifacts.cdi_seal(),
        dice_artifacts
            .bcc()
            .ok_or_else(|| {
                error!("bcc is none");
                DiceError::InvalidInput
            })
            .unwrap(),
        &input_values,
    )
    .unwrap()
}
