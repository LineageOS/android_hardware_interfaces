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

package android.hardware.security.dice;

import android.hardware.security.dice.Bcc;
import android.hardware.security.dice.BccHandover;
import android.hardware.security.dice.InputValues;
import android.hardware.security.dice.Signature;

/**
 * IDiceDevice specifies an interface that allows access to the Android instance's DICE artifacts.
 *
 * <h2>Features</h2>
 *
 * The dice device provides access to the component's CDI_SEAL and CDI_ATTEST secrets as well
 * as to its attestation certificate chain. The "component" is the Android instance running this
 * HAL service and the secrets and attestation chain must include all boot stage components,
 * the kernel, and the verified boot information (VBA).
 *
 * Implementations provide the following operations:
 * <li> sign - Signing a payload with a key derived from CDI_ATTEST.
 * <li> getAttestationChain - Retrieve the component's attestation certificate chain.
 * <li> derive - Retrieve the component's DICE artifacts.
 *
 * @see <a
 *         href="https://pigweed.googlesource.com/open-dice/+/refs/heads/main/docs/specification.md">
 *     Open-dice Specification
 * </a>
 * @see <a
 *         href="https://cs.android.com/android/platform/superproject/+/master:hardware/interfaces/security/keymint/aidl/android/hardware/security/keymint/IRemotelyProvisionedComponent.aidl">
 *     Boot Certificate Chain (BCC) CDDL specification
 * </a>
 * @hide
 */
@SensitiveData
@VintfStability
interface IDiceDevice {
    /**
     * Uses the a key derived from the component's, or a child's given by <code>inputValues</code>,
     * attestation secret to sign the payload using RFC 8032 Pure Ed25519 and returns the
     * signature. The payload is limited to 1024 bytes.
     *
     * @see <a href="https://datatracker.ietf.org/doc/html/rfc8032">RFC 8032</a>
     */
    Signature sign(in InputValues[] id, in byte[] payload);

    /**
     * Returns the attestation chain of the component if <code>inputValues</code> is empty or the
     * chain to the given child of the component identified by the <code>inputValues</code> vector.
     *
     * ## Error as service specific exception:
     *     ResponseCode::PERMISSION_DENIED if the caller is not sufficiently privileged.
     */
    Bcc getAttestationChain(in InputValues[] inputValues);

    /**
     * This function allows a client to become a resident node. A resident node is a node that
     * manages its own dice secrets as opposed to using them by proxy, i.e., by calling sign
     * and getAttestationChain. Called with empty <code>inputValues</code> vectors, an
     * implementation returns the component's DICE secrets. If the <code>inputValues</code> vector
     * is given the appropriate derivations are performed starting from the component's level.
     *
     * ## Error as service specific exception:
     *     ResponseCode::PERMISSION_DENIED if the implementation does not allow resident nodes
     *     at the client's level.
     */
    BccHandover derive(in InputValues[] inputValues);

    /**
     * This demotes the implementation of this interface.
     * When called, the implementation performs appropriate derivation steps using
     * <code>inputValues</code>, traversing the vector in ascending order. Then it replaces its
     * stored DICE artifacts with the newly derived ones.
     *
     * IMPORTANT: When the function returns, all remnants of the previous DICE artifacts must
     * have been purged from memory.
     *
     * This operation is not reversible until the next reboot. Further demotion is always
     * possible.
     *
     * ## Error as service specific exception:
     *     ResponseCode::DEMOTION_FAILED if the implementation failed to demote itself
     *     or was unable to purge previous DICE artifacts from memory.
     */
    void demote(in InputValues[] inputValues);
}
