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
pub use diced_open_dice::CDI_SIZE;
use diced_open_dice::{
    derive_cdi_private_key_seed, keypair_from_seed, retry_bcc_format_config_descriptor,
    retry_bcc_main_flow, retry_dice_main_flow, Config, DiceArtifacts, DiceConfigValues, DiceError,
    DiceMode, InputValues, OwnedDiceArtifacts, HASH_SIZE, HIDDEN_SIZE,
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
    make_explicit_owned_dice_for_uds(security_version, UDS)
}

/// Makes a DICE chain (BCC) from the sample input for the given UDS.
pub fn make_explicit_owned_dice_for_uds(
    security_version: u64,
    uds: &[u8; CDI_SIZE],
) -> OwnedDiceArtifactsWithExplicitKey {
    let dice = make_sample_bcc_and_cdis(security_version, uds, Subcomponents::Minimal);
    OwnedDiceArtifactsWithExplicitKey::from_owned_artifacts(dice).unwrap()
}

/// Makes an XXL DICE chain.
pub fn make_large_explicit_owned_dice(security_version: u64) -> OwnedDiceArtifactsWithExplicitKey {
    let dice = make_sample_bcc_and_cdis(security_version, UDS, Subcomponents::CompOs);
    OwnedDiceArtifactsWithExplicitKey::from_owned_artifacts(dice).unwrap()
}

/// Indicate which subcomponent DICE information to include.
enum Subcomponents {
    Minimal,
    CompOs,
}

fn make_sample_bcc_and_cdis(
    security_version: u64,
    uds: &[u8; CDI_SIZE],
    subcomponents: Subcomponents,
) -> OwnedDiceArtifacts {
    let private_key_seed = derive_cdi_private_key_seed(uds).unwrap();

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
    let (cdi_values, cert) = retry_dice_main_flow(uds, uds, &input_values).unwrap();
    let bcc_value =
        Value::Array(vec![ed25519_public_key_value, de::from_reader(&cert[..]).unwrap()]);
    let mut bcc: Vec<u8> = vec![];
    ser::into_writer(&bcc_value, &mut bcc).unwrap();

    // Appends AVB certificate to DICE chain.
    let subcomponents = match subcomponents {
        Subcomponents::CompOs => compos_subcomponent_descriptors(),
        Subcomponents::Minimal => cbor!([
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
        ])
        .unwrap(),
    };
    let config_desc = cbor!({
        COMPONENT_NAME => "AVB",
        COMPONENT_VERSION => 1,
        COMPONENT_RESETTABLE => null,
        SUBCOMPONENT_DESCRIPTORS => subcomponents,
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

fn compos_subcomponent_descriptors() -> Value {
    // Subcomponent descriptors taken from a CompOS chain.
    cbor!([
        {
            SUBCOMPONENT_NAME => "apk:com.android.compos.payload",
            SUBCOMPONENT_SECURITY_VERSION => 34,
            SUBCOMPONENT_CODE_HASH => hex::decode("9f9a7ca6367a602ee8aeb83c783ae37d1028e4e7fe7492c53ca6b0ac3a5a4918").unwrap(),
            SUBCOMPONENT_AUTHORITY_HASH => hex::decode("5216ccb62004c4534f35c780ad7c582f4ee528371e27d4151f0553325de9ccbe6b34ec4233f5f640703581053abfea303977272d17958704d89b7711292a4569").unwrap(),
        },
        {
            SUBCOMPONENT_NAME => "apk:com.android.security.fsverity_metadata.system",
            SUBCOMPONENT_SECURITY_VERSION => 34,
            SUBCOMPONENT_CODE_HASH => hex::decode("c5f0a71179daa76d5897e391ea882a2f22911b5c2c81794ed6bcd2366dfc6474").unwrap(),
            SUBCOMPONENT_AUTHORITY_HASH => hex::decode("5216ccb62004c4534f35c780ad7c582f4ee528371e27d4151f0553325de9ccbe6b34ec4233f5f640703581053abfea303977272d17958704d89b7711292a4569").unwrap(),
        },
        {
            SUBCOMPONENT_NAME => "apk:com.android.security.fsverity_metadata.system_ext",
            SUBCOMPONENT_SECURITY_VERSION => 34,
            SUBCOMPONENT_CODE_HASH => hex::decode("d043740bc1b45ef8eecb093714321f458f1df17e4d76652a02f3b6c038da8305").unwrap(),
            SUBCOMPONENT_AUTHORITY_HASH => hex::decode("5216ccb62004c4534f35c780ad7c582f4ee528371e27d4151f0553325de9ccbe6b34ec4233f5f640703581053abfea303977272d17958704d89b7711292a4569").unwrap(),
        },
        {
            SUBCOMPONENT_NAME => "apex:com.android.adbd",
            SUBCOMPONENT_SECURITY_VERSION => 990090000,
            SUBCOMPONENT_CODE_HASH => hex::decode("fa4d9b37ff0b534c02865357cce0e2fb5e39a00da00880b438de9a8dd13e79f7").unwrap(),
            SUBCOMPONENT_AUTHORITY_HASH => hex::decode("4df2b359bfe246a1301166dfa9d9a74ae7c11eebe2b6edc360fcc7630974533c4ac28b216af7af3c8c88de2869d0f7b043872eaf75964e311c10bc1beb1cb89c").unwrap(),
        },
        {
            SUBCOMPONENT_NAME => "apex:com.android.adservices",
            SUBCOMPONENT_SECURITY_VERSION => 990090000,
            SUBCOMPONENT_CODE_HASH => hex::decode("debbb6f029c92dcb23b8589b751e945855bdff2cb903fed1e8f9f3ee4740bc00").unwrap(),
            SUBCOMPONENT_AUTHORITY_HASH => hex::decode("2f85397ad0e9c7e7afa3add6c18d18a1a2b9501d2dc51f481ae57fa789f381228ca905459e871b5bfcb300e5a101260ffb6bf58a920e6b7dfc17941ab7a565c4").unwrap(),
        },
        {
            SUBCOMPONENT_NAME => "apex:com.android.appsearch",
            SUBCOMPONENT_SECURITY_VERSION => 990090000,
            SUBCOMPONENT_CODE_HASH => hex::decode("44dd279b861339cfcf92d55b7d4aa1cc21a856a8b0a0bf1bfe66574fdd681194").unwrap(),
            SUBCOMPONENT_AUTHORITY_HASH => hex::decode("e326a8d1cf4a3b18b45c530e153bd310b9bff04949e37a8886b526dc546e2baf403c3384fef01c18341b3e5de0566c294c8373aa8f7b92e07dd9c938a96f7e35").unwrap(),
        },
        {
            SUBCOMPONENT_NAME => "apex:com.android.art",
            SUBCOMPONENT_SECURITY_VERSION => 990090000,
            SUBCOMPONENT_CODE_HASH => hex::decode("e4a8eae20cee7fd98dd202b32321a5feaae73cf125b880763d810edbf6b394dd").unwrap(),
            SUBCOMPONENT_AUTHORITY_HASH => hex::decode("ed255ae9ea98826f3f3a966849f0aaaf356d140c766a869048016e0ba10141af039fec5c53658ddebdad9c2339587c5ef5487bde89237ca79802238d91aebba8").unwrap(),
        },
        {
            SUBCOMPONENT_NAME => "apex:com.android.btservices",
            SUBCOMPONENT_SECURITY_VERSION => 990090000,
            SUBCOMPONENT_CODE_HASH => hex::decode("d7aa86dfdf92e662d2210cd2b3ad4e4522c917e9e287268363aa90e20f9ae16c").unwrap(),
            SUBCOMPONENT_AUTHORITY_HASH => hex::decode("a0d577d4a56cfad09aaa7abcd2355cd78872df85672f2faf9ac2fdf15c06147394e704c7473f28bed737803581a3d097275cc26d8095a4a896ee76167f9ee40e").unwrap(),
        },
        {
            SUBCOMPONENT_NAME => "apex:com.android.compos",
            SUBCOMPONENT_SECURITY_VERSION => 2,
            SUBCOMPONENT_CODE_HASH => hex::decode("64c4b31c7de83ecb31632aff1fb6433741b5f870b1d9f258673787715b83e785").unwrap(),
            SUBCOMPONENT_AUTHORITY_HASH => hex::decode("3e2691174d210a6479c586ef655ee5af1ee53ff960f6291d7b695237d56f73027c5cb30a6d6df07848a0c0b65b6d697e31ed98ba0711a0cb39002c4186b4ad95").unwrap(),
        },
        {
            SUBCOMPONENT_NAME => "apex:com.android.configinfrastructure",
            SUBCOMPONENT_SECURITY_VERSION => 990090000,
            SUBCOMPONENT_CODE_HASH => hex::decode("69d589bcd38decdee64f5bdd359af461e95a5f9c9cf7e6c767db25f0ab81b5e7").unwrap(),
            SUBCOMPONENT_AUTHORITY_HASH => hex::decode("13cde315955806bb1f9ec00714166255148e6baa43f3473bcbe5082bc35d3525605470a6c7ac208337dd79d2250e4adcd4f89f09036f2cbbb553f387c622be07").unwrap(),
        },
        {
            SUBCOMPONENT_NAME => "apex:com.android.conscrypt",
            SUBCOMPONENT_SECURITY_VERSION => 990090000,
            SUBCOMPONENT_CODE_HASH => hex::decode("a91183cc1c12e2d0c9f2b0cd9c97c4592246035c2b07f080d9921fa57ed42900").unwrap(),
            SUBCOMPONENT_AUTHORITY_HASH => hex::decode("4d76d909dd77ee0f8299880b27215a327dda261fb4504125d730fc4f78b105b0947c4103b4bcf49ea9a44d6d04e3c4d385d9ca02a2ef43b8850fca0d91b11e57").unwrap(),
        },
        {
            SUBCOMPONENT_NAME => "apex:com.android.devicelock",
            SUBCOMPONENT_SECURITY_VERSION => 1,
            SUBCOMPONENT_CODE_HASH => hex::decode("85f7bdd116f9c2069f5bfb0039ec1ea165ccaaa517f340440b8eb8f58d044fa8").unwrap(),
            SUBCOMPONENT_AUTHORITY_HASH => hex::decode("2949543df13e57c5dfa49aa3ade0c450514432a7e2710bbb1fd4b768e158bbadf17be6f1446be7d321960e13f2f10f648d2ee551ec41475169e629ed71f2cc5f").unwrap(),
        },
        {
            SUBCOMPONENT_NAME => "apex:com.android.healthfitness",
            SUBCOMPONENT_SECURITY_VERSION => 990090000,
            SUBCOMPONENT_CODE_HASH => hex::decode("ad38a1e8186cb62ac75e47592496582ef7ab26b3f0dd405340cee2fe8d73dc47").unwrap(),
            SUBCOMPONENT_AUTHORITY_HASH => hex::decode("2273e8a7113a103d2b7461c9ae8149ba4cfabe5edeb48b1703c4b4f2fab1a4e9c5a66bf75a9f2063f27df6390d310f1091e9511ad2e41baae822fde1fb022f4f").unwrap(),
        },
        {
            SUBCOMPONENT_NAME => "apex:com.android.i18n",
            SUBCOMPONENT_SECURITY_VERSION => 1,
            SUBCOMPONENT_CODE_HASH => hex::decode("2107e7972afeb70f6653643aebf5e0198c5bf13d71b4c792960f78344bf7e439").unwrap(),
            SUBCOMPONENT_AUTHORITY_HASH => hex::decode("668ac67e4b8a00bf5863ee175db92aefa64138eccbc86a7f528d6fdabec3443e781f4f4c5c3db123994d45696e13e07aa207da25bc70709dcaba3a740b679f7b").unwrap(),
        },
        {
            SUBCOMPONENT_NAME => "apex:com.android.ipsec",
            SUBCOMPONENT_SECURITY_VERSION => 990090000,
            SUBCOMPONENT_CODE_HASH => hex::decode("dfc3ff28eaf429535b651bdb675fbc6d6a7e2834919c02ce56bc20f736562c2c").unwrap(),
            SUBCOMPONENT_AUTHORITY_HASH => hex::decode("8ae0b721d55d3d3f80a1bce694dd85fbbbbba990b0479864c694a47912d4f42a60ca328f76b462a6624b89d1d8b1212fe06fc7749e2c2b0cccd9d86f1058dee2").unwrap(),
        },
        {
            SUBCOMPONENT_NAME => "apex:com.android.media",
            SUBCOMPONENT_SECURITY_VERSION => 990090000,
            SUBCOMPONENT_CODE_HASH => hex::decode("cb1288d098962dbe0d069e78512138e6031d4f1bb4052fea30866f0d8226c541").unwrap(),
            SUBCOMPONENT_AUTHORITY_HASH => hex::decode("7a374d48802077d032daa41fb60123e470808d4bae5d7fd87e6f6e6039fee67cf9cb37b960edb5014247ffc57f4673a0d6a52a07e477c9c7d570594ce0a867af").unwrap(),
        },
        {
            SUBCOMPONENT_NAME => "apex:com.android.mediaprovider",
            SUBCOMPONENT_SECURITY_VERSION => 990090000,
            SUBCOMPONENT_CODE_HASH => hex::decode("bf343bfbe145a81974f05244b523b47d5ecc606c534a65723bb5b7a5f40ab4e1").unwrap(),
            SUBCOMPONENT_AUTHORITY_HASH => hex::decode("272c2cbde4cdd914978246bf6150e30db54f603ee5602a1a48e0b31aa9569a533ff9eedab10bcb852c988d1d46f09de28fc2f0596b070adc38fed9edc12270d8").unwrap(),
        },
        {
            SUBCOMPONENT_NAME => "apex:com.android.nfcservices",
            SUBCOMPONENT_SECURITY_VERSION => 990090000,
            SUBCOMPONENT_CODE_HASH => hex::decode("ff0caa7f86efd179fda394b387e2ef875272d7035138ca1309229fe80dc3dd3a").unwrap(),
            SUBCOMPONENT_AUTHORITY_HASH => hex::decode("26af902c55fa92240fa15f060849e29803775249b5d53a02f7c4a57b039c0be6570809c2d81d63d6d6a923bc58ace14b05d64dcf0f9fdce0f99e6ef18ea292b4").unwrap(),
        },
        {
            SUBCOMPONENT_NAME => "apex:com.android.ondevicepersonalization",
            SUBCOMPONENT_SECURITY_VERSION => 990090000,
            SUBCOMPONENT_CODE_HASH => hex::decode("9a339ac3a29bf2852dde9318575799c23c144515cca129eed5055a589d8da33d").unwrap(),
            SUBCOMPONENT_AUTHORITY_HASH => hex::decode("96c20dbd0a57864a6aa055ebc5611da744c969d37838e7c75712c3f6f37bdbf9eda0dfc240d8f545f9b6fb500edd6d8ca5f48a70acde9a7121e545187df8705d").unwrap(),
        },
        {
            SUBCOMPONENT_NAME => "apex:com.android.os.statsd",
            SUBCOMPONENT_SECURITY_VERSION => 990090000,
            SUBCOMPONENT_CODE_HASH => hex::decode("523e068e66be46eb3789d82aecfba7f58287a9cbc4bcf3c45fd32291db3da048").unwrap(),
            SUBCOMPONENT_AUTHORITY_HASH => hex::decode("431db3773485c432e706fc8040373a373f0dac5a96ba0150ac813d80c00f351496dfe789c6c88dd673aaf642a64c0e09754fb0bb2c12bb12b62968427f9d3f87").unwrap(),
        },
        {
            SUBCOMPONENT_NAME => "apex:com.android.permission",
            SUBCOMPONENT_SECURITY_VERSION => 990090000,
            SUBCOMPONENT_CODE_HASH => hex::decode("2f3de015ab80900579db7935d425ee228ea597ac07ef47b8e7e6366a91d93be9").unwrap(),
            SUBCOMPONENT_AUTHORITY_HASH => hex::decode("0bcf05c42dcff0d2f5dc151369681179b0489749b27c5d2dff9ce69c990e7f299fd9782be1d46698101758f39bf18804e1043e3bd8e3da6c3554a6cccc34a891").unwrap(),
        },
        {
            SUBCOMPONENT_NAME => "apex:com.android.rkpd",
            SUBCOMPONENT_SECURITY_VERSION => 990090000,
            SUBCOMPONENT_CODE_HASH => hex::decode("acb4c10a0f01065b787cf8349e7f371d91dda352d51a25e08fca229375de2ef1").unwrap(),
            SUBCOMPONENT_AUTHORITY_HASH => hex::decode("56854df8c24c9670413396120c89bf18d5f6e2d5ade48b14a102be3bb29751fad1da3b754588da27f33ec5187258a8ec806a323ecf3e99cf8f051499e8cc8b5b").unwrap(),
        },
        {
            SUBCOMPONENT_NAME => "apex:com.android.scheduling",
            SUBCOMPONENT_SECURITY_VERSION => 990090000,
            SUBCOMPONENT_CODE_HASH => hex::decode("b802e45078bbff1648ef97c38743005983d25ba47261b9e9fb7c758defec920e").unwrap(),
            SUBCOMPONENT_AUTHORITY_HASH => hex::decode("d8e3c0f8501b639074c003fd1a45424756a91a79f326e2b50a66f39c9ced5bc0cd0811f6055b5f2c8330a845f95bd26d6f6d3962e3436f65fdfda3343f26cb69").unwrap(),
        },
        {
            SUBCOMPONENT_NAME => "apex:com.android.sdkext",
            SUBCOMPONENT_SECURITY_VERSION => 990090000,
            SUBCOMPONENT_CODE_HASH => hex::decode("fb2a3d5437766135838d7ce078870a403ae6929937d58ec8b40182057587af21").unwrap(),
            SUBCOMPONENT_AUTHORITY_HASH => hex::decode("6bdc69078b58bc524648ce606c8050ffd6a88a8e169c23cbce7a6cfc444cde58a2a9a77968e3f1454a0eaeb0ad00bb846e5447473b0730bbd28e0b71189af808").unwrap(),
        },
        {
            SUBCOMPONENT_NAME => "apex:com.android.tethering",
            SUBCOMPONENT_SECURITY_VERSION => 990090000,
            SUBCOMPONENT_CODE_HASH => hex::decode("d352cfd92179ec854ae30c9ce54562b1a31f01738524ba11ceae10db6207c995").unwrap(),
            SUBCOMPONENT_AUTHORITY_HASH => hex::decode("1af6fe33f7035ea7a4353a02ed40fd7b72f2668b58794d2bbccce8b61aa9878eb817cdcc813e1eab1a2f287c2f15e8b2bb620cf024e55210a659f27c3064bd7f").unwrap(),
        },
        {
            SUBCOMPONENT_NAME => "apex:com.android.uwb",
            SUBCOMPONENT_SECURITY_VERSION => 990090000,
            SUBCOMPONENT_CODE_HASH => hex::decode("bfc970bc51670ade054b5fcafa6ed9fb90f0aa14168ea5a97d20d5b236ffac00").unwrap(),
            SUBCOMPONENT_AUTHORITY_HASH => hex::decode("993f35bbdbad1db72f17382058b10020bb1c932cef8f540c240cb26e867ea03bab4ade22f41823a8be3ea5e82306f47368e294e153328ba38ad35b3aafabdf84").unwrap(),
        },
        {
            SUBCOMPONENT_NAME => "apex:com.android.virt",
            SUBCOMPONENT_SECURITY_VERSION => 2,
            SUBCOMPONENT_CODE_HASH => hex::decode("efff05a5354236dc3efca323bf25d0488f7c8212a393349f9a7d329f7db88e73").unwrap(),
            SUBCOMPONENT_AUTHORITY_HASH => hex::decode("a279d6d530ae0cea2b2f2c1d3dee1e5b59dbdac861e137995eaf44b1a57c5cfb02b7892f00e7fe647756fd2cfef66e74350b517c4d79463c6e6a6f96eb01693b").unwrap(),
        },
        {
            SUBCOMPONENT_NAME => "apex:com.android.wifi",
            SUBCOMPONENT_SECURITY_VERSION => 990090000,
            SUBCOMPONENT_CODE_HASH => hex::decode("2d6db7bcfb436ff9a8f22788e4666071d18e03063422d5b58e378530d304e0af").unwrap(),
            SUBCOMPONENT_AUTHORITY_HASH => hex::decode("29b3ef73b51aff982b3136c944add0ee40a12eba762ca69ae9646c4f08fd8145e593c8b1fe4208e52f87e0735134c573612ec0566ebbf5ab08b2054a5954b599").unwrap(),
        },
    ]).unwrap()
}
