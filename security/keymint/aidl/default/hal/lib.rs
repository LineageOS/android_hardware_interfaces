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

//! KeyMint helper functions that are only suitable for non-secure environments
//! such as Cuttlefish.

use kmr_hal::{env::get_property, HalServiceError, SerializedChannel};
use log::{error, info};
use std::{sync::{Arc, Mutex}, ops::DerefMut};

/// Send boot info and attestation info to TA via the given communication channel.
pub fn send_boot_info_and_attestation_id_info<T: SerializedChannel>(
    channel: &Arc<Mutex<T>>
) -> Result<(), HalServiceError> {
    // Retrieve root-of-trust information (with the exception of the verified boot key
    // hash) from Android properties, and populate the TA with this information. On a
    // real device, the bootloader should provide this data to the TA directly.
    let boot_req = get_boot_info();
    info!("boot/HAL->TA: boot info is {:?}", boot_req);
    kmr_hal::send_boot_info(channel.lock().unwrap().deref_mut(), boot_req)
        .map_err(|e| format!("Failed to send boot info: {e:?}"))?;
    info!("Successfully sent non-secure boot info to TA.");

    // Retrieve device ID information (except for IMEI/MEID values) from Android properties
    // and populate the TA with this information. On a real device, a factory provisioning
    // process would populate this information.
    let attest_ids = attestation_id_info();
    if let Err(e) = kmr_hal::send_attest_ids(channel.lock().unwrap().deref_mut(), attest_ids) {
        error!("Failed to send attestation ID info: {e:?}. \
               Core functionality will be available, but attestation functionality will likely \
               to be affected");
    } else {
        info!("Successfully sent non-secure attestation ID info to TA.");
    }
    Ok(())
}

/// Retrieve the most significant attestation property for `name`.
fn attestation_property(name: &str) -> Vec<u8> {
    let prop_val = get_property(&format!("ro.product.vendor.{}", name)).unwrap_or_default();
    if !prop_val.is_empty() {
        prop_val
    } else {
        get_property(&format!("ro.product.{}", name))
            .unwrap_or_else(|prop_name| format!("{} unavailable", prop_name))
    }
    .as_bytes()
    .to_vec()
}

/// Populate attestation ID information based on properties (where available).
/// Retrieving the serial number requires SELinux permission.
pub fn attestation_id_info() -> kmr_wire::AttestationIdInfo {
    let prop = |name| {
        get_property(name)
            .unwrap_or_else(|_| format!("{} unavailable", name))
            .as_bytes()
            .to_vec()
    };
    kmr_wire::AttestationIdInfo {
        brand: attestation_property("brand"),
        device: attestation_property("device"),
        product: attestation_property("name"),
        serial: prop("ro.serialno"),
        manufacturer: attestation_property("manufacturer"),
        model: attestation_property("model"),
        // Currently modem_simulator always returns one fixed value. See `handleGetIMEI` in
        // device/google/cuttlefish/host/commands/modem_simulator/misc_service.cpp for more details.
        // TODO(b/263188546): Use device-specific IMEI values when available.
        imei: b"867400022047199".to_vec(),
        imei2: b"867400022047199".to_vec(),
        meid: vec![],
    }
}

/// Get boot information based on system properties.
pub fn get_boot_info() -> kmr_wire::SetBootInfoRequest {
    let vbmeta_digest = get_property("ro.boot.vbmeta.digest").unwrap_or_else(|_| "00".repeat(32));
    let verified_boot_hash = hex::decode(&vbmeta_digest).unwrap_or_else(|_e| {
        error!("failed to parse VBMeta digest hex data in '{vbmeta_digest}': {_e:?}");
        vec![0; 32]
    });
    let device_boot_locked = match get_property("ro.boot.vbmeta.device_state")
        .unwrap_or_else(|_| "no-prop".to_string())
        .as_str()
    {
        "locked" => true,
        "unlocked" => false,
        v => {
            error!("Unknown device_state '{}', treating as unlocked", v);
            false
        }
    };
    let verified_boot_key_digest =
        get_property("ro.boot.vbmeta.public_key_digest").unwrap_or_else(|_| "00".repeat(32));
    let verified_boot_key = match device_boot_locked {
        true => hex::decode(&verified_boot_key_digest).unwrap_or_else(|_e| {
            error!("Failed to parse Verified Boot key hex data in '{verified_boot_key_digest}': {_e:?}");
            vec![0; 32]
        }),
        // VTS-16+ requires the attested Verified Boot key to be 32 bytes of zeroes when the
        // bootloader is unlocked, so we ignore the property's value in that case. Behaviour
        // prior to VTS-16 is unspecified, so it's fine to return the same.
        false => vec![0; 32],
    };
    let verified_boot_state = match get_property("ro.boot.verifiedbootstate")
        .unwrap_or_else(|_| "no-prop".to_string())
        .as_str()
    {
        "green" => 0,  // Verified
        "yellow" => 1, // SelfSigned
        "orange" => 2, // Unverified,
        "red" => 3,    // Failed,
        v => {
            error!("Unknown boot state '{}', treating as Unverified", v);
            2
        }
    };

    // Attempt to get the boot patchlevel from a system property.  This requires an SELinux
    // permission, so fall back to re-using the OS patchlevel if this can't be done.
    let boot_patchlevel_prop = get_property("ro.vendor.boot_security_patch").unwrap_or_else(|e| {
        error!("Failed to retrieve boot patchlevel: {:?}", e);
        get_property(kmr_hal::env::OS_PATCHLEVEL_PROPERTY)
            .unwrap_or_else(|_| "1970-09-19".to_string())
    });
    let boot_patchlevel =
        kmr_hal::env::extract_patchlevel(&boot_patchlevel_prop).unwrap_or(19700919);

    kmr_wire::SetBootInfoRequest {
        verified_boot_key,
        device_boot_locked,
        verified_boot_state,
        verified_boot_hash,
        boot_patchlevel,
    }
}
