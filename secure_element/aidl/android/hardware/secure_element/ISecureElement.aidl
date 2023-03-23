/*
 * Copyright (C) 2022 The Android Open Source Project
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

package android.hardware.secure_element;

import android.hardware.secure_element.ISecureElementCallback;
import android.hardware.secure_element.LogicalChannelResponse;

@VintfStability
interface ISecureElement {
    const int FAILED = 1;
    const int CHANNEL_NOT_AVAILABLE = 2;
    const int NO_SUCH_ELEMENT_ERROR = 3;
    const int UNSUPPORTED_OPERATION = 4;
    const int IOERROR = 5;

    /**
     * Closes the channel indicated by the channelNumber.
     *
     * @throws ServiceSpecificException Closing a channel must return
     *     FAILED on an error or if a basic channel (i.e. channel 0)
     *     is used.
     *
     * @param channelNumber to be closed
     */
    void closeChannel(in byte channelNumber);

    /**
     * Returns Answer to Reset as per ISO/IEC 7816
     *
     * @return containing the response. Empty vector if Secure Element
     *                  doesn't support ATR.
     */
    byte[] getAtr();

    /**
     * Initializes the Secure Element. This may include updating the applet
     * and/or vendor-specific initialization.
     *
     * HAL service must send onStateChange() with connected equal to true
     * after all the initialization has been successfully completed.
     * Clients must wait for a onStateChange(true) before opening channels.
     *
     * @param clientCallback callback used to sent status of the SE back to the
     *                       client
     */
    void init(in ISecureElementCallback clientCallback);

    /**
     * Returns the current state of the card.
     *
     * This is useful for removable Secure Elements like UICC,
     * Secure Elements on SD cards etc.
     *
     * @return true if present, false otherwise
     */
    boolean isCardPresent();

    /**
     * Opens a basic channel with the Secure Element, selecting the applet
     * represented by the Application ID (AID). A basic channel has channel
     * number 0.
     *
     * @throws ServiceSpecificException with codes
     *  - CHANNEL_NOT_AVAILABLE if secure element has reached the maximum
     *    limit on the number of channels it can support.
     *  - NO_SUCH_ELEMENT_ERROR if AID provided doesn't match any applet
     *    on the secure element.
     *  - UNSUPPORTED_OPERATION if operation provided by the P2 parameter
     *    is not permitted by the applet.
     *  - IOERROR if there was an error communicating with the Secure Element.
     *
     * @param aid AID to uniquely identify the applet on the Secure Element
     * @param p2 P2 parameter of SELECT APDU as per ISO 7816-4
     *
     * @return On success, response to SELECT command.
     */
    byte[] openBasicChannel(in byte[] aid, in byte p2);

    /**
     * Opens a logical channel with the Secure Element, selecting the applet
     * represented by the Application ID (AID).
     *
     * @param aid AID to uniquely identify the applet on the Secure Element
     * @param p2 P2 parameter of SELECT APDU as per ISO 7816-4
     * @throws ServiceSpecificException on error with the following code:
     *  - CHANNEL_NOT_AVAILABLE if secure element has reached the maximum
     *    limit on the number of channels it can support.
     *  - NO_SUCH_ELEMENT_ERROR if AID provided doesn't match any applet
     *    on the secure element.
     *  - UNSUPPORTED_OPERATION if operation provided by the P2 parameter
     *    is not permitted by the applet.
     *  - IOERROR if there was an error communicating with the Secure Element.
     *
     * @return On success, response to SELECT command
     */
    LogicalChannelResponse openLogicalChannel(in byte[] aid, in byte p2);

    /**
     * Reset the Secure Element.
     *
     * HAL should trigger reset to the secure element. It could hardware power cycle or
     * a soft reset depends on the hardware design. All channels opened are
     * closed by this operation.
     * HAL service must send onStateChange() with connected equal to true
     * after resetting and all the re-initialization has been successfully completed.
     *
     * @throws ServiceSpecificException on error with the following code:
     *  - FAILED if the service was unable to reset the secure element.
     */
    void reset();

    /**
     * Transmits an APDU command (as per ISO/IEC 7816) to the SE.
     *
     * @throws ServiceSpecificException with code CHANNEL_NOT_AVAILABLE
     *  if there was an error in communicating with the secure element.
     *
     * @param data APDU command to be sent
     * @return response to the command
     */
    byte[] transmit(in byte[] data);
}
