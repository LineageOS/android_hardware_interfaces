/*
 * Copyright (C) 2024 The Android Open Source Project
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

package android.hardware.security.see.authmgr;

import android.hardware.security.see.authmgr.DiceLeafArtifacts;
import android.hardware.security.see.authmgr.DicePolicy;
import android.hardware.security.see.authmgr.ExplicitKeyDiceCertChain;
import android.hardware.security.see.authmgr.SignedConnectionRequest;

/**
 * This is the interface to be implemented by an AuthMgr backend component (AuthMgr BE), in order to
 * allow the AuthMgr frontend component (AuthMgr FE) in a pVM instance to authenticate itself and
 * to authorize one or more clients in the pVM instance, in order to let the clients access
 * trusted services in the Trusted Execution Environment (TEE).
 *
 * The following assumptions must be true for the underlying IPC mechanism and the transport layer:
 *     1. Both parties should be able to retrieve a non-spoofable identifier of the other party from
 *        the transport layer (a.k.a transport ID or vM ID), which stays the same throughout a given
 *        boot cycle of a pVM instance. This is important to prevent person-in-the-middle (PITM)
 *        attacks and to authorize a new connection from a pVM instance based on an already
 *        authenicated connection from the same pVM instance.
 *
 *     2. Each of AuthMgr FE and the AuthMgr BE should be able to hand over a connection that is
 *        setup between them to another party so that such connection can be used for communication
 *        between the two new parties subsequently. This is important to be able to handover an
 *        authorized connection established between the AuthMgr FE and the AuthMgr BE to a client in
 *        in a pVM instance and a trusted service in TEE respectively.
 *
 *     3. This API should be exposed over an IPC mechanism that supports statefull connections. This
 *        is important for the AuthMgr FE to setup an authenicated connection once per boot cycle
 *        and reuse it to authorize multiple client connections afterwards, if needed.
 *
 *      4. AuthMgr FE has a mechanism for discovering and establishing a connection to the trusted
 *         AuthMgr BE. Based on this assumptionson, mutual authentication is not covered by this
 *         API.
 *
 * The AuthMgr authorization protocol consists of two phases:
 *     1. Phase 1 authenticates the AuthMgr FE to the AuthMgr BE via the first two methods of this
 *        API: `initAuthentication` and `completeAuthentication`. At the end of the successful
 *        excecution of phase 1, the AuthMgr FE and the AuthMgr BE have an authenticated connection
 *        established between them. Phase 1 also enforces rollback protection on AuthMgr FE in
 *        addition to authentication.
 *
 *        Authentication is performed by verifying the AuthMgr FE's signature on the challenge
 *        issued by the AuthMgr BE. The public signing key of the AuthMgr FE is obtained from the
 *        validated DICE certificate chain for verifying the signature. Rollback protection is
 *        enforced by matching the DICE certificate chain against the stored DICE policy.
 *        AuthMgr FE uses this authenticated connection throughout the boot cycle of the pVM to send
 *        phase 2 requests to the AuthMgr BE. Therefore, phase 1 needs to be executed only once per
 *        boot cycle of the pVM. AuthMgr BE should take measures to prevent any duplicate
 *        authentication attempts from the same instance or from any impersonating instances.
 *
 *     2. Phase 2 authorizes a client in the pVM to access trusted service(s) in the TEE and
 *        establishes a new connection between the client and the trusted service based on the trust
 *        in the authenticated connection established in phase 1. The client and the trusted service
 *        can communicate independently from the AuthMgr(s) after the successful execution of
 *        phase 2 of the authorization protocol.
 *
 *        The AuthMgr FE first opens a new vsock connection to the AuthMgr BE and sends a one-time
 *        token over that connection. The AuthMgr FE then invokes the third method of this API
 *        (`authorizeAndConnectClientToTrustedService`) on the authenticated connection established
 *        with the AuthMgr BE in phase 1. Rollback protection is enforced on the client by matching
 *        the client's DICE certificate against the stored DICE policy. The new connection is
 *        authorized by matching the token sent over the new connection and the token sent over the
 *        authenicated connection.
 *
 * AuthMgr BE should make sure that "use-after-destroy" threats are prevented in the implementation
 * of this authorization protocol. This means that even if a client/pVM instance is created with the
 * same identifier(s) of a deleted client/pVM instance, the new client should not be able to access
 * the deleted client's secrets/resources created in the trusted services. The following
 * requirements should be addressed in order to ensure this:
 * 1) Each client should be identified by a unique identifier at the AuthMgr BE. The uniqueness
 *    should be guaranteed across factory resets.
 * 2) The client's unique identifier should be used when constructing the file path to store the
 *    client's context, including the client's DICE policy, in the AuthMgr BE's secure storage.
 * 3) The client's unique identifier should be conveyed to the trusted service(s) that the client
 *    accesses, when an authorized connection is setup between the client and the trusted service in
 *    phase 2. The trusted service(s) should mix in this unique client identifier when providing the
 *    critical services to the clients (e.g. deriving HW-backed keys by the HWCrypto service,
 *    storing data by the SecureStorage service).
 *
 * An example approach to build a unique identifier for a client is as follows:
 * The AuthMgr BE stores a `global sequence number` in the secure storage that does not get
 * wiped upon factory reset. Everytime the AuthMgr BE sees a new instance or a client, it assigns
 * the current `global sequence number` as the unique sequence number of the instance or the client
 * and increments the `global sequence number`.
 */
@VintfStability
interface IAuthMgrAuthorization {
    /**
     * AuthMgr FE initiates the challenge-response protocol with the AuthMgr BE in order to
     * authenticate the AuthMgr FE to the AuthMgr BE. AuthMgr BE creates and returns a challenge
     * (a cryptographic random of 32 bytes) to the AuthMgr FE.
     *
     * The AuthMgr BE extracts the instance identifier from the DICE certificate chain of the
     * AuthMgr FE (given in the input: `diceCertChain`). If the instance identifier is not included
     * in the DICE certificate chain, then it should be sent in the optional
     * input: `instanceIdentifier`. The instance identifier is used by the AuthMgr BE in this step
     * to detect and reject any duplicate authentication attempts.
     * The instance identifier is used in step 2 to build the file path in the secure storage to
     * store the instance's context.
     *
     * If authentication is already started (but not completed) from the same transport ID, return
     * the error code `AUTHENTICATION_ALREADY_STARTED`.
     *
     * @param diceCertChain - DICE certificate chain of the AuthMgr FE.
     *
     * @param instanceIdentifier - optional parameter to send the instance identifier, if it is not
     *                             included in the DICE certificate chain
     *
     * @return challenge to be included in the signed response sent by the AuthMgr FE in
     *         `completeAuthentication`
     *
     * @throws ServiceSpecificException:
     *         Error::INSTANCE_ALREADY_AUTHENTICATED - when a pVM instance with the same
     *         `instanceIdentifier` or the same transport id has already been authenticated.
     *         Error::AUTHENTICATION_ALREADY_STARTED - when a pVM instance with the same
     *         the same transport id has already started authentication
     */
    byte[32] initAuthentication(in ExplicitKeyDiceCertChain diceCertChain,
            in @nullable byte[] instanceIdentifier);

    /**
     * AuthMgr FE invokes this method to complete phase 1 of the authorization protocol. The AuthMgr
     * BE verifies the signature in `signedConnectionRequest` with the public signing key of the
     * AuthMgr FE obtained from the DICE certificate chain.
     *
     * As per the CDDL for `SignedConnectionRequest` in SignedConnectionRequest.cddl, the AuthMgr FE
     * includes the challenge sent by the AuthMgr BE and the unique transport IDs of the AuthMgr FE
     * and AuthMgr BE in the signed response. This is to prevent replay attacks in the presence of
     * more than one AuthMgr BE, where one AuthMgr BE may impersonate a pVM instance/AuthMgr FE to
     * another AuthMgr BE. Both transport IDs are included for completeness, although it is
     * sufficient to include either of them for the purpose of preventing such attacks.
     *
     * AuthMgr BE validates the DICE certificate chain by verifying all the signatures in the chain
     * and by checking wither the root public key is trusted.
     *
     * The AuthMgr BE matches the DICE certificate chain of the AuthMgr FE to the DICE policy given
     * in the input: `dicePolicy`. If this is the first invocation of this method during the
     * lifetime of the AuthMgr FE, the AuthMgr BE stores the DICE policy in the secure storage as
     * part of the pVM instance's context, upon successful matching of DICE chain to the policy.
     * The file path for the storage of the pVM context is constructed using the instance
     * identifier. Note that the creation of a pVM instance's context in the secure storage is
     * allowed only during the factory, for the first version of this API. In the future, we expect
     * to allow the creation of a pVM instance's context in the secure storage even after the device
     * leaves the factory, based on hard-coded DICE policies and/or via a separate
     * `IAuthMgrInstanceContextMaintenance` API.
     *
     * In the subsequent invocations of this method, the AuthMgr BE matches the given DICE chain
     * to the stored DICE policy in order to enforce rollback protection. If that succeeds and if
     * the given DICE poliy is different from the stored DICE policy, the AuthMgr BE replaces the
     * stored DICE policy with the given DICE policy.
     *
     * Upon successful execution of this method, the AuthMgr BE should store some state associated
     * with the connection, in order to distinguish authenicated connections from any
     * non-authenticated connections. The state associated with the connection may cache certain
     * artifacts such as instance identifier, instance sequence number, transport ID, DICE chain
     * and DICE policy of the AuthMgr FE, so that they can be reused when serving phase 2 requests.
     * The requests for phase 2 of the authorization protocol are allowed only on authenticated
     * connections.
     *
     * @param signedConnectionRequest - signature from AuthMgr FE (CBOR encoded according to
     *                                  SignedConnectionRequest.cddl)
     *
     * @param dicePolicy - DICE policy of the AuthMgr FE
     *
     * @throws ServiceSpecificException:
     *         Error::AUTHENTICATION_NOT_STARTED - when the authentication process has not been
     *             started for the pVM instance.
     *         Error::INSTANCE_ALREADY_AUTHENTICATED - when a pVM instance with the same
     *         `instanceIdentifier` or the same transport id has already been authenticated.
     *         Error::SIGNATURE_VERIFICATION_FAILED - when the signature verification fails.
     *         Error::INVALID_DICE_CERT_CHAIN - when the DICE certificate chain validation fails.
     *         Error::DICE_POLICY_MATCHING_FAILED - when the DICE certificate chain to DICE policy
     *             matching fails for the pVM instance.
     *         Error::INSTANCE_CONTEXT_CREATION_DENIED - when the creation of the pVM instances's
     *             context in the AuthMgr BE is not allowed.
     *         Error::INSTANCE_PENDING_DELETION - when a pVM that is being deleted is trying to
     *             authenticate.
     *
     */
    void completeAuthentication(
            in SignedConnectionRequest signedConnectionRequest, in DicePolicy dicePolicy);

    /**
     * When the AuthMgr FE receives a request from a client to access a trusted service, the
     * AuthMgr FE first creates a new (out-of-band) connection with the AuthMgr BE and sends a
     * one-time cryptographic token of 32 bytes over that new connection.
     *
     * The AuthMgr FE then invokes this method on the authenticated connection established with the
     * AuthMgr BE in phase 1. When this method is invoked, the AuthMgr BE checks whether the
     * underlying connection of this method call is already authenticated.
     *
     * The AuthMgr FE acts as the DICE manager for all the clients in the pVM and generates the DICE
     * leaf certificate and the DICE leaf policy for the client, which are sent in the input:
     * `clientDiceArtifacts`.
     *
     * The AuthMgr BE matches the client's DICE leaf certificate to the client's DICE policy.
     * If this is the first invocation of this method in the lifetime of the client, the AuthMgr BE
     * stores the client's DICE policy in the secure storage as part of the client's context, upon
     * successful matching of the DICE certificate to the policy. The file path for the storage of
     * the client's context should be constructed using the unique id assigned to the pVM instance
     * by the AuthMgr BE (e.g. instance sequence number)  and the client ID. There is no use
     * case for deleting a client context or a pVM context created in the secure storage, for the
     * first version of this API, outside of the factory reset. In the future, we expect to
     * expose APIs for those tasks.
     *
     * In the subsequent invocations of this method, the AuthMgr BE matches the given DICE leaf
     * certificate to the stored DICE policy in order to enforce rollback protection. If that
     * succeeds and if the given DICE policy is different from the stored DICE policy, the AuthMgr
     * BE replaces the stored DICE policy with the given DICE policy.
     *
     * If the same client requests multiple trusted services or connects to the same trusted service
     * multiple times during the same boot cycle of the pVM instance, it is recommended to validate
     * the client's DICE artifacts only once for a given client as an optimization.
     *
     * The AuthMgr BE keeps track of the aforementioned new connections that are pending
     * authorization along with the tokens sent over them and the transport ID of the pVM instance
     * which created those connections.
     *
     * The AuthMgr FE sends the same token that was sent over an aforementioned new connection
     * in the input: `token` of this method call, in order to authorize the new connection, based on
     * the trust in the authenticated connection established in phase 1.
     *
     * Once the validation of the client's DICE artifacts is completed, the AuthMgr BE retrieves the
     * pending new connection to be authorized, which is associated with a token that matches the
     * token sent in this method call and a transport ID that matches the transport ID associated
     * with the connection underlying this method call.
     *
     * Next the AuthMgr BE connects to the trusted service requested by the client in order to
     * handover the new authorized connection to the trusted service. Once the connection
     * handover is successful, the AuthMgr BE returns OK to the AuthMgr FE. Then the AuthMgr FE
     * returns to the client a handle to the new connection (created at the beginning of phase 2).
     * At this point, an authorized connection is setup between the client and the trusted service,
     * which they can use to communicate independently of the AuthMgr FE and the AuthMgr BE.
     *
     * @param clientID - the identifier of the client in the pVM instance, which is unique in the
     *                   context of the pVM instance
     *
     * @param service name - the name of the trusted service requested by the client
     *
     * @param token - the one-time token used to authorize the new connection created between the
     *                AuthMgr FE and the AuthMgr BE
     *
     * @param clientDiceArtifacts - DICE leaf certificate and the DICE leaf policy of the client
     *
     * @throws ServiceSpecificException:
     *         Error::CONNECTION_NOT_AUTHENTICATED - when the underlying connection of this method
     *             call is not authenticated.
     *         Error::DICE_POLICY_MATCHING_FAILED - when the DICE certificate chain to DICE policy
     *             matching fails for the client.
     *         Error::NO_CONNECTION_TO_AUTHORIZE - when there is no pending new connection that
     *             is associated with a token and a transport ID that matches those of this
     *             method call.
     *         Error::CONNECTION_HANDOVER_FAILED - when the hanover of the authorized connection to
     *             the trusted service fails.
     *         Error::CLIENT_PENDING_DELETION - when a client that is being deleted is trying to be
     *             authorized.
     */
    void authorizeAndConnectClientToTrustedService(in byte[] clientID, String serviceName,
            in byte[32] token, in DiceLeafArtifacts clientDiceArtifacts);
}
