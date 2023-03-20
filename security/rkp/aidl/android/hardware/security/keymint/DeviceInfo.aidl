/*
 * Copyright (C) 2021 The Android Open Source Project
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

package android.hardware.security.keymint;

/**
 * DeviceInfo contains information about the device that's signed by the
 * IRemotelyProvisionedComponent HAL. These values are intended to be checked by the server to
 * verify that the certificate signing request crafted by an IRemotelyProvisionedComponent HAL
 * instance is coming from the expected device based on values initially uploaded during device
 * manufacture in the factory.
 * @hide
 */
@VintfStability
parcelable DeviceInfo {
    /**
     * DeviceInfo is a CBOR Map structure described by the following CDDL. DeviceInfo must be
     * canonicalized according to the specification in RFC 7049. The ordering presented here is
     * non-canonical to group similar entries semantically.
     *
     * The DeviceInfo has changed across versions 1, 2, and 3 of the HAL. All versions of the
     * DeviceInfo CDDL are described as follows. Please refer to the CDDL structure version
     * that corresponds to the HAL version you are working with:
     *
     * Version 3, introduced in Android 14:
     *     DeviceInfo = {
     *         "brand" : tstr,
     *         "manufacturer" : tstr,
     *         "product" : tstr,
     *         "model" : tstr,
     *         "device" : tstr,
     *         "vb_state" : "green" / "yellow" / "orange",    ; Taken from the AVB values
     *         "bootloader_state" : "locked" / "unlocked",    ; Taken from the AVB values
     *         "vbmeta_digest": bstr,                         ; Taken from the AVB values
     *         ? "os_version" : tstr,                         ; Same as
     *                                                        ; android.os.Build.VERSION.release
     *                                                        ; Not optional for TEE.
     *         "system_patch_level" : uint,     ; YYYYMM, must match KeyMint OS_PATCHLEVEL
     *         "boot_patch_level" : uint,       ; YYYYMMDD, must match KeyMint BOOT_PATCHLEVEL
     *         "vendor_patch_level" : uint,     ; YYYYMMDD, must match KeyMint VENDOR_PATCHLEVEL
     *         "security_level" : "tee" / "strongbox",
     *         "fused": 1 / 0,  ; 1 if secure boot is enforced for the processor that the IRPC
     *                          ; implementation is contained in. 0 otherwise.
     *     }
     *
     * ---------------------------------------------------------------------------------------------
     *
     * Version 2, introduced in Android 13:
     *     DeviceInfo = {
     *         "brand" : tstr,
     *         "manufacturer" : tstr,
     *         "product" : tstr,
     *         "model" : tstr,
     *         "device" : tstr,
     *         "vb_state" : "green" / "yellow" / "orange",    ; Taken from the AVB values
     *         "bootloader_state" : "locked" / "unlocked",    ; Taken from the AVB values
     *         "vbmeta_digest": bstr,                         ; Taken from the AVB values
     *         ? "os_version" : tstr,                         ; Same as
     *                                                        ; android.os.Build.VERSION.release
     *                                                        ; Not optional for TEE.
     *         "system_patch_level" : uint,     ; YYYYMM, must match KeyMint OS_PATCHLEVEL
     *         "boot_patch_level" : uint,       ; YYYYMMDD, must match KeyMint BOOT_PATCHLEVEL
     *         "vendor_patch_level" : uint,     ; YYYYMMDD, must match KeyMint VENDOR_PATCHLEVEL
     *         "version" : 2,                                 ; The CDDL schema version.
     *         "security_level" : "tee" / "strongbox",
     *         "fused": 1 / 0,  ; 1 if secure boot is enforced for the processor that the IRPC
     *                          ; implementation is contained in. 0 otherwise.
     *
     * ---------------------------------------------------------------------------------------------
     *
     * Version 1, introduced in Android 12:
     *     DeviceInfo = {
     *         ? "brand" : tstr,
     *         ? "manufacturer" : tstr,
     *         ? "product" : tstr,
     *         ? "model" : tstr,
     *         ? "board" : tstr,
     *         ? "vb_state" : "green" / "yellow" / "orange",  ; Taken from the AVB values
     *         ? "bootloader_state" : "locked" / "unlocked",  ; Taken from the AVB values
     *         ? "vbmeta_digest": bstr,                       ; Taken from the AVB values
     *         ? "os_version" : tstr,                         ; Same as
     *                                                        ; android.os.Build.VERSION.release
     *         ? "system_patch_level" : uint,     ; YYYYMM, must match KeyMint OS_PATCHLEVEL
     *         ? "boot_patch_level" : uint,       ; YYYYMMDD, must match KeyMint BOOT_PATCHLEVEL
     *         ? "vendor_patch_level" : uint,     ; YYYYMMDD, must match KeyMint VENDOR_PATCHLEVEL
     *         "version" : 1,                                 ; The CDDL schema version.
     *         "security_level" : "tee" / "strongbox"
     *         "att_id_state": "locked" / "open",  ; Attestation IDs State. If "locked", this
     *                                             ; indicates a device's attestable IDs are
     *                                             ; factory-locked and immutable. If "open",
     *                                             ; this indicates the device is still in a
     *                                             ; provisionable state and the attestable IDs
     *                                             ; are not yet frozen.
     *     }
     */
    byte[] deviceInfo;
}
