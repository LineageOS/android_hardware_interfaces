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

package android.hardware.security.secretkeeper;

import android.hardware.security.authgraph.IAuthGraphKeyExchange;
import android.hardware.security.secretkeeper.SecretId;

@VintfStability
/**
 * Secretkeeper service definition.
 *
 * An ISecretkeeper instance provides secure storage of secrets on behalf of other components in
 * Android, in particular for protected virtual machine instances. From the perspective of security
 * privilege, Secretkeeper must be implemented in an environment with privilege higher than any of
 * its clients. Since AVF based protected Virtual Machines are one set of its clients, the
 * implementation of ISecretkeeper should live in a secure environment, such as:
 * - A trusted execution environment such as ARM TrustZone.
 * - A completely separate, purpose-built and certified secure CPU.
 *
 */
interface ISecretkeeper {
    const int ERROR_UNKNOWN_KEY_ID = 1;
    const int ERROR_INTERNAL_ERROR = 2;
    const int ERROR_REQUEST_MALFORMED = 3;

    /**
     * Retrieve the instance of the `IAuthGraphKeyExchange` HAL that should be used for shared
     * session key establishment. These keys are used to perform encryption of messages as
     * described in SecretManagement.cddl, allowing the client and Secretkeeper to have a
     * cryptographically secure channel. In the key exchange protocol the client acts as P1
     * (source) and Secretkeeper as P2 (sink). The interface returned here can be used to invoke
     * methods on the sink.
     *
     * The client's identity is its DICE chain; Secretkeeper's identity is a
     * per-boot key pair.
     */
    IAuthGraphKeyExchange getAuthGraphKe();

    /**
     * processSecretManagementRequest method is used for interacting with the Secret Management API
     *
     * Secret Management API: The clients can use this API to store (& get) 32 bytes of data.
     * The API is a CBOR based protocol, which follows request/response model.
     * See SecretManagement.cddl for the API spec.
     *
     * Further, the requests (from client) & responses (from service) must be encrypted into
     * ProtectedRequestPacket & ProtectedResponsePacket using symmetric keys agreed between
     * the client & service. This cryptographic protection is required because the messages are
     * ferried via Android, which is allowed to be outside the TCB of clients (for example protected
     * Virtual Machines). For this, service (& client) must implement the AuthGraph key exchange
     * protocol to establish a secure channel between them.
     *
     * If an encrypted response cannot be generated, then a service-specific Binder error using one
     * of the ERROR_ codes above will be returned.
     *
     * Secretkeeper database should guarantee the following properties:
     *
     * 1. Confidentiality: No entity (of security privilege lower than Secretkeeper) should
     *    be able to get a client's data in clear.
     *
     * 2. Integrity: The data is protected against malicious Android OS tampering with database.
     *    ie, if Android (userspace & kernel) tampers with the client's secret, the Secretkeeper
     *    service must be able to detect it & return error when clients requests for their secrets.
     *    Note: the integrity requirements also include Antirollback protection ie, reverting the
     *    database into an old state should be detected.
     *
     * 3. The data is persistent across device boot.
     *    Note: Denial of service is not in scope. A malicious Android may be able to delete data,
     *    but for ideal Android, the data should be persistent.
     *
     * @param CBOR-encoded ProtectedRequestPacket. See SecretManagement.cddl for its definition.
     * @return CBOR-encoded ProtectedResponsePacket. See SecretManagement.cddl for its definition
     */
    byte[] processSecretManagementRequest(in byte[] request);

    /**
     * Delete the data corresponding to a collection of IDs.
     *
     * Note that unlike `processSecretManagementRequest`, the contents of this method are in
     * plaintext, and no client authentication is required.
     *
     * @param Secret identifiers to delete.
     */
    void deleteIds(in SecretId[] ids);

    /**
     * Delete data of all clients.
     */
    void deleteAll();
}
