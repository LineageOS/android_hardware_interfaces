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

//! Local in-process implementation of the KeyMint TA. This is insecure and should
//! only be used for testing purposes.

// This crate is `std` using, but some of the code uses macros from a `no_std` world.
extern crate alloc;

use kmr_common::crypto;
use kmr_crypto_boring::{
    aes::BoringAes, aes_cmac::BoringAesCmac, des::BoringDes, ec::BoringEc, eq::BoringEq,
    hmac::BoringHmac, rng::BoringRng, rsa::BoringRsa, sha256::BoringSha256,
};
use kmr_ta::device::{
    BootloaderDone, CsrSigningAlgorithm, Implementation, TrustedPresenceUnsupported,
};
use kmr_ta::{HardwareInfo, KeyMintTa, RpcInfo, RpcInfoV3};
use kmr_wire::keymint::SecurityLevel;
use kmr_wire::rpc::MINIMUM_SUPPORTED_KEYS_IN_CSR;
use log::info;

pub mod attest;
pub mod clock;
pub mod rpc;
pub mod soft;

/// Build a set of crypto trait implementations based around BoringSSL and the standard library
/// clock.
pub fn boringssl_crypto_impls() -> crypto::Implementation {
    let rng = BoringRng;
    let clock = clock::StdClock::new();
    let rsa = BoringRsa::default();
    let ec = BoringEc::default();
    crypto::Implementation {
        rng: Box::new(rng),
        clock: Some(Box::new(clock)),
        compare: Box::new(BoringEq),
        aes: Box::new(BoringAes),
        des: Box::new(BoringDes),
        hmac: Box::new(BoringHmac),
        rsa: Box::new(rsa),
        ec: Box::new(ec),
        ckdf: Box::new(BoringAesCmac),
        hkdf: Box::new(BoringHmac),
        sha256: Box::new(BoringSha256),
    }
}

/// Build a [`kmr_ta::KeyMintTa`] instance for nonsecure use.
pub fn build_ta() -> kmr_ta::KeyMintTa {
    info!("Building NON-SECURE KeyMint Rust TA");
    let hw_info = HardwareInfo {
        version_number: 1,
        security_level: SecurityLevel::TrustedEnvironment,
        impl_name: "Rust reference implementation",
        author_name: "Google",
        unique_id: "NON-SECURE KeyMint TA",
    };
    let rpc_sign_algo = CsrSigningAlgorithm::EdDSA;
    let rpc_info_v3 = RpcInfoV3 {
        author_name: "Google",
        unique_id: "NON-SECURE KeyMint TA",
        fused: false,
        supported_num_of_keys_in_csr: MINIMUM_SUPPORTED_KEYS_IN_CSR,
    };

    let sign_info = attest::CertSignInfo::new();
    let keys: Box<dyn kmr_ta::device::RetrieveKeyMaterial> = Box::new(soft::Keys);
    let rpc: Box<dyn kmr_ta::device::RetrieveRpcArtifacts> = Box::new(soft::RpcArtifacts::new(
        soft::Derive::default(),
        rpc_sign_algo,
    ));
    let dev = Implementation {
        keys,
        sign_info: Box::new(sign_info),
        // HAL populates attestation IDs from properties.
        attest_ids: None,
        sdd_mgr: None,
        // `BOOTLOADER_ONLY` keys not supported.
        bootloader: Box::new(BootloaderDone),
        // `STORAGE_KEY` keys not supported.
        sk_wrapper: None,
        // `TRUSTED_USER_PRESENCE_REQUIRED` keys not supported
        tup: Box::new(TrustedPresenceUnsupported),
        // No support for converting previous implementation's keyblobs.
        legacy_key: None,
        rpc,
    };
    KeyMintTa::new(
        hw_info,
        RpcInfo::V3(rpc_info_v3),
        boringssl_crypto_impls(),
        dev,
    )
}
