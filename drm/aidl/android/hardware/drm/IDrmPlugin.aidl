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

package android.hardware.drm;

import android.hardware.drm.DrmMetricGroup;
import android.hardware.drm.HdcpLevels;
import android.hardware.drm.IDrmPluginListener;
import android.hardware.drm.KeySetId;
import android.hardware.drm.KeyRequest;
import android.hardware.drm.KeyStatus;
import android.hardware.drm.KeyType;
import android.hardware.drm.KeyValue;
import android.hardware.drm.LogMessage;
import android.hardware.drm.NumberOfSessions;
import android.hardware.drm.OfflineLicenseState;
import android.hardware.drm.OpaqueData;
import android.hardware.drm.ProvideProvisionResponseResult;
import android.hardware.drm.ProvisionRequest;
import android.hardware.drm.SecureStop;
import android.hardware.drm.SecureStopId;
import android.hardware.drm.SecurityLevel;

/**
 * IDrmPlugin is used to interact with a specific drm plugin that was
 * created by IDrmFactory::createPlugin.
 *
 * A drm plugin provides methods for obtaining drm keys to be used by a codec
 * to decrypt protected video content.
 */
@VintfStability
interface IDrmPlugin {
    /**
     * Close a session on the DrmPlugin object
     *
     * @param sessionId the session id the call applies to
     *
     * @return (implicit) the status of the call:
     *     BAD_VALUE if the sessionId is invalid
     *     ERROR_DRM_INVALID_STATE if the HAL is in a state where
     *         the session cannot be closed.
     *     ERROR_DRM_SESSION_NOT_OPENED if the session is not opened
     */
    void closeSession(in byte[] sessionId);

    /**
     * Decrypt the provided input buffer with the cipher algorithm
     * specified by setCipherAlgorithm and the key selected by keyId,
     * and return the decrypted data.
     *
     * @param sessionId the session id the call applies to
     * @param keyId the ID of the key to use for decryption
     * @param input the input data to decrypt
     * @param iv the initialization vector to use for decryption
     *
     * @return decrypted output buffer
     *     Implicit error codes:
     *       + BAD_VALUE if the sessionId is invalid
     *       + ERROR_DRM_INVALID_STATE if the HAL is in a state where
     *             the decrypt operation cannot be performed.
     *       + ERROR_DRM_SESSION_NOT_OPENED if the session is not opened
     */
    byte[] decrypt(in byte[] sessionId, in byte[] keyId, in byte[] input, in byte[] iv);

    /**
     * Encrypt the provided input buffer with the cipher algorithm specified by
     * setCipherAlgorithm and the key selected by keyId, and return the
     * encrypted data.
     *
     * @param sessionId the session id the call applies to
     * @param keyId the ID of the key to use for encryption
     * @param input the input data to encrypt
     * @param iv the initialization vector to use for encryption
     *
     * @return encrypted output buffer
     *     Implicit error codes:
     *       + BAD_VALUE if the sessionId is invalid
     *       + ERROR_DRM_INVALID_STATE if the HAL is in a state where
     *             the encrypt operation cannot be performed.
     *       + ERROR_DRM_SESSION_NOT_OPENED if the session is not opened
     */
    byte[] encrypt(in byte[] sessionId, in byte[] keyId, in byte[] input, in byte[] iv);

    /**
     * Return the currently negotiated and max supported HDCP levels.
     *
     * The current level is based on the display(s) the device is connected to.
     * If multiple HDCP-capable displays are simultaneously connected to
     * separate interfaces, this method returns the lowest negotiated HDCP level
     * of all interfaces.
     *
     * The maximum HDCP level is the highest level that can potentially be
     * negotiated. It is a constant for any device, i.e. it does not depend on
     * downstream receiving devices that could be connected. For example, if
     * the device has HDCP 1.x keys and is capable of negotiating HDCP 1.x, but
     * does not have HDCP 2.x keys, then the maximum HDCP capability would be
     * reported as 1.x. If multiple HDCP-capable interfaces are present, it
     * indicates the highest of the maximum HDCP levels of all interfaces.
     *
     * This method should only be used for informational purposes, not for
     * enforcing compliance with HDCP requirements. Trusted enforcement of HDCP
     * policies must be handled by the DRM system.
     *
     * @return HdcpLevels parcelable
     *     Implicit error codes:
     *       + ERROR_DRM_INVALID_STATE if the HAL is in a state where
     *             the HDCP level cannot be queried
     */
    HdcpLevels getHdcpLevels();

    /**
     * A key request/response exchange occurs between the app and a License
     * Server to obtain the keys required to decrypt the content.
     * getKeyRequest() is used to obtain an opaque key request blob that is
     * delivered to the license server.
     *
     * @param scope either a sessionId or a keySetId, depending on the
     *     specified keyType. When the keyType is OFFLINE or STREAMING, scope
     *     must be set to the sessionId the keys will be provided to. When the
     *     keyType is RELEASE, scope must be set to the keySetId of the keys
     *     being released.
     * @param initData container-specific data, its meaning is interpreted
     *     based on the mime type provided in the mimeType parameter. It could
     *     contain, for example, the content ID, key ID or other data obtained
     *     from the content metadata that is required to generate the key
     *     request. initData must be empty when keyType is RELEASE.
     * @param mimeType identifies the mime type of the content
     * @param keyType specifies if the keys are to be used for streaming,
     *     offline or a release
     * @param optionalParameters included in the key request message to
     *     allow a client application to provide additional message parameters
     *     to the server.
     *
     * @return KeyRequest parcelable
     *     Implicit error codes:
     *       + BAD_VALUE if any parameters are invalid
     *       + ERROR_DRM_CANNOT_HANDLE if getKeyRequest is not supported at
     *             the time of the call
     *       + ERROR_DRM_INVALID_STATE if the HAL is in a state where
     *             a key request cannot be generated
     *       + ERROR_DRM_NOT_PROVISIONED if the device requires provisioning
     *             before it is able to generate a key request
     *       + ERROR_DRM_RESOURCE_CONTENTION if client applications using the
     *             hal are temporarily exceeding the available crypto resources
     *             such that a retry of the operation is likely to succeed
     *       + ERROR_DRM_SESSION_NOT_OPENED if the session is not opened
     */
    KeyRequest getKeyRequest(in byte[] scope, in byte[] initData, in String mimeType,
            in KeyType keyType, in KeyValue[] optionalParameters);

    /**
     * Get Plugin error messages.
     *
     * @return LogMessages
     *     Implicit error codes:
     *       + GENERAL_OEM_ERROR on OEM-provided, low-level component failures;
     *       + GENERAL_PLUGIN_ERROR on unexpected plugin-level errors.
     */
    List<LogMessage> getLogMessages();

    /**
     * Returns the plugin-specific metrics. Multiple metric groups may be
     * returned in one call to getMetrics(). The scope and definition of the
     * metrics is defined by the plugin.
     *
     * @return collection of metric groups provided by the plugin
     *     Implicit error codes:
     *       + ERROR_DRM_INVALID_STATE if the metrics are not available to be
     *             returned.
     */
    List<DrmMetricGroup> getMetrics();

    /**
     * Return the current number of open sessions and the maximum number of
     * sessions that may be opened simultaneously among all DRM instances
     * for the active DRM scheme.
     *
     * @return NumberOfSessions parcelable
     *     Implicit error codes:
     *       + ERROR_DRM_INVALID_STATE if the HAL is in a state where the
     *             number of sessions cannot be queried
     */
    NumberOfSessions getNumberOfSessions();

    /**
     * The keys in an offline license allow protected content to be
     * played even if the device is not connected to a network.
     * Offline licenses are stored on the device after a key
     * request/response exchange when the key request KeyType is
     * OFFLINE. Normally each app is responsible for keeping track of
     * the KeySetIds it has created. In some situations however, it
     * will be necessary to request the list of stored offline license
     * KeySetIds. If an app loses the KeySetId for any stored licenses
     * that it created, for example, it must be able to recover the
     * stored KeySetIds so those licenses will be removed when they
     * expire or when the app is uninstalled.
     *
     * This method returns a list of the KeySetIds for all offline
     * licenses. The offline license KeySetId allows an app to query
     * the status of an offline license or remove it.
     *
     * @return list of keySetIds
     *     Implicit error codes:
     *       + ERROR_DRM_INVALID_STATE if the HAL is in a state where the
     *             KeySetIds can't be returned
     */
    List<KeySetId> getOfflineLicenseKeySetIds();

    /**
     * Request the state of an offline license. An offline license must
     * be usable or inactive. The keys in a usable offline license are
     * available for decryption. When the offline license state is
     * inactive, the keys have been marked for release using
     * getKeyRequest with KeyType RELEASE but the key response has not
     * been received. The keys in an inactive offline license are not
     * usable for decryption.
     *
     * @param keySetId the id of the offline license
     *
     * @return The offline license state, UNKNOWN, USABLE or INACTIVE.
     *     Implicit error codes:
     *       + BAD_VALUE if the license is not found
     *       + ERROR_DRM_INVALID_STATE if the HAL is in a state where the
     *             offline license state can't be queried
     */
    OfflineLicenseState getOfflineLicenseState(in KeySetId keySetId);

    /**
     * Read a byte array property value given the property name.
     * See getPropertyString.
     *
     * @param propertyName the name of the property
     *
     * @return property value bye array
     *     Implicit error codes:
     *       + BAD_VALUE if the property name is invalid
     *       + ERROR_DRM_CANNOT_HANDLE if the property is not supported
     *       + ERROR_DRM_INVALID_STATE if the HAL is in a state where the
     *             property cannot be obtained
     */
    byte[] getPropertyByteArray(in String propertyName);

    /**
     * A drm scheme can have properties that are settable and readable
     * by an app. There are a few forms of property access methods,
     * depending on the data type of the property.
     *
     * Property values defined by the public API are:
     *   "vendor" [string] identifies the maker of the drm scheme
     *   "version" [string] identifies the version of the drm scheme
     *   "description" [string] describes the drm scheme
     *   'deviceUniqueId' [byte array] The device unique identifier is
     *   established during device provisioning and provides a means of
     *   uniquely identifying each device.
     *
     * Since drm scheme properties may vary, additional field names may be
     * defined by each DRM vendor. Refer to your DRM provider documentation
     * for definitions of its additional field names.
     *
     * Read a string property value given the property name.
     *
     * @param propertyName the name of the property
     *
     * @return the property value string.
     *     Implicit error codes:
     *       + BAD_VALUE if the property name is invalid
     *       + ERROR_DRM_CANNOT_HANDLE if the property is not supported
     *       + ERROR_DRM_INVALID_STATE if the HAL is in a state where the
     *             property cannot be obtained
     */
    String getPropertyString(in String propertyName);

    /**
     * A provision request/response exchange occurs between the app
     * and a provisioning server to retrieve a device certificate.
     * getProvisionRequest is used to obtain an opaque provisioning
     * request blob that is delivered to the provisioning server.
     *
     * @param certificateType the type of certificate requested, e.g. "X.509"
     * @param certificateAuthority identifies the certificate authority.
     *     A certificate authority (CA) is an entity which issues digital
     *     certificates for use by other parties. It is an example of a
     *     trusted third party.
     *
     * @return ProvisionRequest parcelable
     *     Implicit error codes:
     *       + ERROR_DRM_CANNOT_HANDLE if the drm scheme does not require
     *             provisioning
     *       + ERROR_DRM_INVALID_STATE if the HAL is in a state where
     *             the provision request cannot be generated
     *       + ERROR_DRM_RESOURCE_CONTENTION if client applications using
     *             the hal are temporarily exceeding the available crypto
     *             resources such that a retry of the operation is likely
     *             to succeed
     */
    ProvisionRequest getProvisionRequest(
            in String certificateType, in String certificateAuthority);

    /**
     * Get all secure stops by secure stop ID
     *
     * @param secureStopId the ID of the secure stop to return.
     *     The secure stop ID is delivered by the key server
     *     as part of the key response and must also be known by the app.
     *
     * @return secure stop opaque object.
     *     Implicit error codes:
     *       + BAD_VALUE if the secureStopId is invalid
     *       + ERROR_DRM_INVALID_STATE if the HAL is in a state where
     *             the secure stop cannot be returned
     */
    SecureStop getSecureStop(in SecureStopId secureStopId);

    /**
     * Get the IDs of all secure stops on the device
     *
     * @return list of secure stops IDs.
     *     Implicit error codes:
     *       + ERROR_DRM_INVALID_STATE if the HAL is in a state where
     *             the secure stop IDs list cannot be returned
     */
    List<SecureStopId> getSecureStopIds();

    /**
     * SecureStop is a way of enforcing the concurrent stream limit per
     * subscriber.
     *
     * It can securely monitor the lifetime of sessions across device reboots
     * by periodically persisting the session lifetime status in secure
     * storage.
     *
     * A signed version of the sessionID is written to persistent storage on the
     * device when each MediaCrypto object is created and periodically during
     * playback. The sessionID is signed by the device private key to prevent
     * tampering.
     *
     * When playback is completed the session is destroyed, and the secure
     * stops are queried by the app. The app then delivers the secure stop
     * message to a server which verifies the signature to confirm that the
     * session and its keys have been removed from the device. The persisted
     * record on the device is removed after receiving and verifying the
     * signed response from the server.
     *
     * Get all secure stops on the device
     *
     * @return list of the opaque secure stop objects.
     *     Implicit error codes:
     *       + ERROR_DRM_INVALID_STATE if the HAL is in a state where
     *             the secure stops cannot be returned
     */
    List<SecureStop> getSecureStops();

    /**
     * Return the current security level of a session. A session has an initial
     * security level determined by the robustness of the DRM system's
     * implementation on the device.
     *
     * @param sessionId the session id the call applies to
     *
     * @return the current security level for the session.
     *     Implicit error codes:
     *       + BAD_VALUE if the sessionId is invalid
     *       + ERROR_DRM_INVALID_STATE if the HAL is in a state where
     *             the security level cannot be queried
     *       + ERROR_DRM_SESSION_NOT_OPENED if the session is not opened
     */
    SecurityLevel getSecurityLevel(in byte[] sessionId);

    /**
     * Open a new session at a requested security level. The security level
     * represents the robustness of the device's DRM implementation. By default,
     * sessions are opened at the native security level of the device which is
     * the maximum level that can be supported. Overriding the security level is
     * necessary when the decrypted frames need to be manipulated, such as for
     * image compositing. The security level parameter must be equal to or lower
     * than the native level. If the requested level is not supported, the next
     * lower supported security level must be set. The level can be queried
     * using {@link #getSecurityLevel}. A session ID is returned.
     *
     * @param level the requested security level
     *
     * @return sessionId
     */
    byte[] openSession(in SecurityLevel securityLevel);

    /**
     * After a key response is received by the app, it is provided to the
     * Drm plugin using provideKeyResponse.
     *
     * @param scope may be a sessionId or a keySetId depending on the
     *     type of the response. Scope should be set to the sessionId
     *     when the response is for either streaming or offline key requests.
     *     Scope should be set to the keySetId when the response is for
     *     a release request.
     * @param response the response from the key server that is being
     *     provided to the drm HAL.
     *
     * @return a keySetId that can be used to later restore the keys to a new
     *     session with the method restoreKeys when the response is for an
     *     offline key request.
     *     Implicit error codes:
     *       + BAD_VALUE if any parameters are invalid
     *       + ERROR_DRM_CANNOT_HANDLE if provideKeyResponse is not supported
     *             at the time of the call
     *       + ERROR_DRM_DEVICE_REVOKED if the device has been disabled by
     *             the license policy
     *       + ERROR_DRM_INVALID_STATE if the HAL is in a state where
     *             a key response cannot be handled.
     *       + ERROR_DRM_NOT_PROVISIONED if the device requires provisioning
     *             before it can handle the key response
     *       + ERROR_DRM_SESSION_NOT_OPENED if the session is not opened
     */
    KeySetId provideKeyResponse(in byte[] scope, in byte[] response);

    /**
     * After a provision response is received by the app from a provisioning
     * server, it is provided to the Drm HAL using provideProvisionResponse.
     * The HAL implementation must receive the provision request and
     * store the provisioned credentials.
     *
     * @param response the opaque provisioning response received by the
     * app from a provisioning server.
     *
     * @return ProvideProvisionResponseResult parcelable, which contains
     *     the public certificate and encrypted private key that can be
     *     used by signRSA to compute an RSA signature on a message.
     *     Implicit error codes:
     *       + BAD_VALUE if any parameters are invalid
     *       + ERROR_DRM_DEVICE_REVOKED if the device has been disabled by
     *             the license policy
     *       + ERROR_DRM_INVALID_STATE if the HAL is in a state where the
     *             provision response cannot be handled
     */
    ProvideProvisionResponseResult provideProvisionResponse(in byte[] response);

    /**
     * Request an informative description of the license for the session.
     * The status is in the form of {name, value} pairs. Since DRM license
     * policies vary by vendor, the specific status field names are
     * determined by each DRM vendor. Refer to your DRM provider
     * documentation for definitions of the field names for a particular
     * drm scheme.
     *
     * @param sessionId the session id the call applies to
     *
     * @return a list of name value pairs describing the license.
     *     Implicit error codes:
     *       + ERROR_DRM_SESSION_NOT_OPENED if the session is not opened
     *       + BAD_VALUE if any parameters are invalid
     *       + ERROR_DRM_INVALID_STATE if the HAL is in a state where the
     *             key status cannot be queried.
     */
    List<KeyValue> queryKeyStatus(in byte[] sessionId);

    /**
     * Release all secure stops on the device
     *
     * @return (implicit) the status of the call:
     *     ERROR_DRM_INVALID_STATE if the HAL is in a state where
     *         the secure stops cannot be released.
     */
    void releaseAllSecureStops();

    /**
     * Release a secure stop by secure stop ID
     *
     * @param secureStopId the ID of the secure stop to release.
     *     The secure stop ID is delivered by the key server as
     *     part of the key response and must also be known by the app.
     *
     * @return (implicit) the status of the call:
     *     BAD_VALUE if the secureStopId is invalid
     *     ERROR_DRM_INVALID_STATE if the HAL is in a state where
     *         the secure stop cannot be released.
     */
    void releaseSecureStop(in SecureStopId secureStopId);

    /**
     * Release secure stops given a release message from the key server
     *
     * @param ssRelease the secure stop release message identifying
     *     one or more secure stops to release. ssRelease is opaque,
     *     it is passed directly from a DRM license server through
     *     the app and media framework to the vendor HAL module.
     *     The format and content of ssRelease must be defined by the
     *     DRM scheme being implemented according to this HAL.
     *     The DRM scheme can be identified by its UUID which
     *     can be queried using IDrmFactory::isCryptoSchemeSupported.
     *
     * @return (implicit) the status of the call:
     *     BAD_VALUE if ssRelease is invalid
     *     ERROR_DRM_INVALID_STATE if the HAL is in a state wherei
     *         the secure stop cannot be released.
     */
    void releaseSecureStops(in OpaqueData ssRelease);

    /**
     * Remove all secure stops on the device without requiring a secure
     * stop release response message from the key server.
     *
     * @return (implicit) the status of the call:
     *     ERROR_DRM_INVALID_STATE if the HAL is in a state where
     *         the secure stops cannot be removed.
     */
    void removeAllSecureStops();

    /**
     * Remove the current keys from a session
     *
     * @param sessionId the session id the call applies to
     *
     * @return (implicit) the status of the call:
     *     BAD_VALUE if the sessionId is invalid
     *     ERROR_DRM_INVALID_STATE if the HAL is in a state where
     *         the keys cannot be removed.
     *     ERROR_DRM_SESSION_NOT_OPENED if the session is not opened
     */
    void removeKeys(in byte[] sessionId);

    /**
     * Normally offline licenses are released using a key
     * request/response exchange using getKeyRequest where the KeyType
     * is RELEASE, followed by provideKeyResponse. This allows the
     * server to cryptographically confirm that the license has been
     * removed and then adjust the count of offline licenses allocated
     * to the device.
     * <p>
     * In some exceptional situations it will be necessary to directly
     * remove offline licenses without notifying the server, which is
     * performed by this method.
     *
     * @param keySetId the id of the offline license to remove
     *
     * @return (implicit) the status of the call:
     *     BAD_VALUE if the license is not found
     *     ERROR_DRM_INVALID_STATE if the HAL is in a state where
     *         the KeySetIds can't be removed.
     */
    void removeOfflineLicense(in KeySetId keySetId);

    /**
     * Remove a secure stop given its secure stop ID, without requiring
     * a secure stop release response message from the key server.
     *
     * @param secureStopId the ID of the secure stop to release.
     *
     * @return the status of the call:
     *     BAD_VALUE if the secureStopId is invalid
     *     ERROR_DRM_INVALID_STATE if the HAL is in a state where
     *         the secure stop cannot be removed.
     */
    void removeSecureStop(in SecureStopId secureStopId);

    /**
     * Check if the specified mime-type & security level require a secure decoder
     * component.
     *
     * @param mime The content mime-type
     * @param level the requested security level
     *
     * @return must be true if and only if a secure decoder is
     *     required for the specified mime-type & security level
     */
    boolean requiresSecureDecoder(in String mime, in SecurityLevel level);

    /**
     * Restore persisted offline keys into a new session
     *
     * @param sessionId the session id the call applies to
     * @param keySetId identifies the keys to load, obtained from
     *     a prior call to provideKeyResponse().
     *
     * @return (implicit) the status of the call:
     *     ERROR_DRM_SESSION_NOT_OPENED if the session is not opened
     *     BAD_VALUE if any parameters are invalid
     *     ERROR_DRM_INVALID_STATE if the HAL is in a state where
     *         keys cannot be restored.
     */
    void restoreKeys(in byte[] sessionId, in KeySetId keySetId);

    /**
     * The following methods implement operations on a CryptoSession to support
     * encrypt, decrypt, sign verify operations on operator-provided
     * session keys.
     *
     *
     * Set the cipher algorithm to be used for the specified session.
     *
     * @param sessionId the session id the call applies to
     * @param algorithm the algorithm to use. The string conforms to JCA
     *     Standard Names for Cipher Transforms and is case insensitive. An
     *     example algorithm is "AES/CBC/PKCS5Padding".
     *
     * @return (implicit) the status of the call:
     *     BAD_VALUE if any parameters are invalid
     *     ERROR_DRM_INVALID_STATE  if the HAL is in a state where
     *         the algorithm cannot be set.
     *     ERROR_DRM_SESSION_NOT_OPENED if the session is not opened`
     */
    void setCipherAlgorithm(in byte[] sessionId, in String algorithm);

    /**
     * Plugins call the following methods to deliver events to the
     * java app.
     *
     *
     * Set a listener for a drm session. This allows the drm HAL to
     * make asynchronous calls back to the client of IDrm.
     *
     * @param listener instance of IDrmPluginListener to receive the events
     */
    void setListener(in IDrmPluginListener listener);

    /**
     * Set the MAC algorithm to be used for computing hashes in a session.
     *
     * @param sessionId the session id the call applies to
     * @param algorithm the algorithm to use. The string conforms to JCA
     *     Standard Names for Mac Algorithms and is case insensitive. An example MAC
     *     algorithm string is "HmacSHA256".
     *
     * @return (implicit) the status of the call:
     *     BAD_VALUE if any parameters are invalid
     *     ERROR_DRM_INVALID_STATE  if the HAL is in a state where
     *         the algorithm cannot be set.
     *     ERROR_DRM_SESSION_NOT_OPENED if the session is not opened`
     */
    void setMacAlgorithm(in byte[] sessionId, in String algorithm);

    /**
     * Set playback id of a drm session. The playback id can be used to join drm session metrics
     * with metrics from other low level media components, e.g. codecs, or metrics from the high
     * level player.
     *
     * @param sessionId drm session id
     * @param playbackId high level playback id
     *
     * @return (implicit) the status of the call:
     *    ERROR_DRM_SESSION_NOT_OPENED if the drm session cannot be found
     */
    void setPlaybackId(in byte[] sessionId, in String playbackId);

    /**
     * Write a property byte array value given the property name
     *
     * @param propertyName the name of the property
     * @param value the value to write
     *
     * @return (implicit) the status of the call:
     *     BAD_VALUE if the property name is invalid
     *     ERROR_DRM_CANNOT_HANDLE if the property is not supported
     *     ERROR_DRM_INVALID_STATE if the HAL is in a state where the
     *         property cannot be set
     */
    void setPropertyByteArray(in String propertyName, in byte[] value);

    /**
     * Write a property string value given the property name
     *
     * @param propertyName the name of the property
     * @param value the value to write
     *
     * @return (implicit) status of the call:
     *     BAD_VALUE if the property name is invalid
     *     ERROR_DRM_CANNOT_HANDLE if the property is not supported
     *     ERROR_DRM_INVALID_STATE if the HAL is in a state where the
     *         property cannot be set
     */
    void setPropertyString(in String propertyName, in String value);

    /**
     * Compute a signature over the provided message using the mac algorithm
     * specified by setMacAlgorithm and the key selected by keyId and return
     * the signature.
     *
     * @param sessionId the session id the call applies to
     * @param keyId the ID of the key to use for decryption
     * @param message the message to compute a signature over
     *
     * @return signature computed over the message
     *     Implicit error codes:
     *       + ERROR_DRM_SESSION_NOT_OPENED if the session is not opened
     *       + BAD_VALUE if any parameters are invalid
     *       + ERROR_DRM_INVALID_STATE if the HAL is in a state where the
     *             sign operation cannot be performed.
     */
    byte[] sign(in byte[] sessionId, in byte[] keyId, in byte[] message);

    /**
     * Compute an RSA signature on the provided message using the specified
     * algorithm.
     *
     * @param sessionId the session id the call applies to
     * @param algorithm the signing algorithm, such as "RSASSA-PSS-SHA1"
     *     or "PKCS1-BlockType1"
     * @param message the message to compute the signature on
     * @param wrappedKey the private key returned during provisioning as
     *     returned by provideProvisionResponse.
     *
     * @return signature computed over the message
     *     Implicit error codes:
     *       + BAD_VALUE if any parameters are invalid
     *       + ERROR_DRM_INVALID_STATE if the HAL is in a state where the
     *             signRSA operation operation cannot be performed
     *       + ERROR_DRM_SESSION_NOT_OPENED if the session is not opened
     */
    byte[] signRSA(
            in byte[] sessionId, in String algorithm, in byte[] message,
            in byte[] wrappedkey);

    /**
     * Compute a hash of the provided message using the mac algorithm specified
     * by setMacAlgorithm and the key selected by keyId, and compare with the
     * expected result.
     *
     * @param sessionId the session id the call applies to
     * @param keyId the ID of the key to use for decryption
     * @param message the message to compute a hash of
     * @param signature the signature to verify
     *
     * @return true if the signature is verified positively, false otherwise.
     *     Implicit error codes:
     *       + ERROR_DRM_SESSION_NOT_OPENED if the session is not opened
     *       + BAD_VALUE if any parameters are invalid
     *       + ERROR_DRM_INVALID_STATE if the HAL is in a state where the
     *             verify operation cannot be performed.
     */
    boolean verify(
            in byte[] sessionId, in byte[] keyId, in byte[] message,
            in byte[] signature);
}
