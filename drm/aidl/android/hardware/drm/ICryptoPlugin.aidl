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

import android.hardware.common.Ashmem;
import android.hardware.drm.DecryptArgs;
import android.hardware.drm.DestinationBuffer;
import android.hardware.drm.LogMessage;
import android.hardware.drm.Mode;
import android.hardware.drm.Pattern;
import android.hardware.drm.SharedBuffer;
import android.hardware.drm.Status;
import android.hardware.drm.SubSample;

/**
 * ICryptoPlugin is the HAL for vendor-provided crypto plugins.
 *
 * It allows crypto sessions to be opened and operated on, to
 * load crypto keys for a codec to decrypt protected video content.
 */
@VintfStability
interface ICryptoPlugin {
    /**
     * Decrypt an array of subsamples from the source memory buffer to the
     * destination memory buffer.
     *
     * @return number of decrypted bytes
     *     Implicit error codes:
     *       + ERROR_DRM_CANNOT_HANDLE in other failure cases
     *       + ERROR_DRM_DECRYPT if the decrypt operation fails
     *       + ERROR_DRM_FRAME_TOO_LARGE if the frame being decrypted into
     *             the secure output buffer exceeds the size of the buffer
     *       + ERROR_DRM_INSUFFICIENT_OUTPUT_PROTECTION if required output
     *             protections are not active
     *       + ERROR_DRM_INSUFFICIENT_SECURITY if the security level of the
     *             device is not sufficient to meet the requirements in
     *             the license policy
     *       + ERROR_DRM_INVALID_STATE if the device is in a state where it
     *             is not able to perform decryption
     *       + ERROR_DRM_LICENSE_EXPIRED if the license keys have expired
     *       + ERROR_DRM_NO_LICENSE if no license keys have been loaded
     *       + ERROR_DRM_RESOURCE_BUSY if the resources required to perform
     *             the decryption are not available
     *       + ERROR_DRM_SESSION_NOT_OPENED if the decrypt session is not
     *             opened
     */
    int decrypt(in DecryptArgs args);

    /**
     * Get OEMCrypto or plugin error messages.
     *
     * @return LogMessages
     *     Implicit error codes:
     *       + GENERAL_OEM_ERROR on OEM-provided, low-level component failures;
     *       + GENERAL_PLUGIN_ERROR on unexpected plugin-level errors.
     */
    List<LogMessage> getLogMessages();

    /**
     * Notify a plugin of the currently configured resolution.
     *
     * @param width - the display resolutions's width
     * @param height - the display resolution's height
     */
    void notifyResolution(in int width, in int height);

    /**
     * Check if the specified mime-type requires a secure decoder
     * component.
     *
     * @param mime The content mime-type
     * @return must be true only if a secure decoder is required
     * for the specified mime-type
     */
    boolean requiresSecureDecoderComponent(in String mime);

    /**
     * Associate a mediadrm session with this crypto session.
     *
     * @param sessionId the MediaDrm session ID to associate with
     *     this crypto session
     * @return (implicit) the status of the call, status can be:
     *     ERROR_DRM_SESSION_NOT_OPENED if the session is not opened, or
     *     ERROR_DRM_CANNOT_HANDLE if the operation is not supported by
     *         the drm scheme
     */
    void setMediaDrmSession(in byte[] sessionId);

    /**
     * Set a shared memory base for subsequent decrypt operations.
     * The buffer base is mmaped from a ParcelFileDesciptor in Ashmem
     * which maps shared memory in the HAL module.
     * After the shared buffer base is established, the decrypt() method
     * receives SharedBuffer instances which specify the buffer address range
     * for decrypt source and destination addresses.
     *
     * There can be multiple shared buffers per crypto plugin. The buffers
     * are distinguished by the bufferId.
     *
     * @param base the base of the memory buffer abstracted by
     *     SharedBuffer parcelable (bufferId, size, handle)
     */
    void setSharedBufferBase(in SharedBuffer base);
}
