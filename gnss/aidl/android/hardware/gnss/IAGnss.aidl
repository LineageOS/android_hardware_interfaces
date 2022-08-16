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

package android.hardware.gnss;

import android.hardware.gnss.IAGnssCallback;
import android.hardware.gnss.IAGnssCallback.AGnssType;

/**
 * Extended interface for Assisted GNSS support.
 *
 * @hide
 */
@VintfStability
interface IAGnss {
    /** Access point name IP type */
    @VintfStability
    @Backing(type="int")
    enum ApnIpType {
        INVALID = 0,
        IPV4 = 1,
        IPV6 = 2,
        IPV4V6 = 3,
    }

    /**
     * Opens the AGNSS interface and provides the callback routines to the
     * implementation of this interface.
     *
     * If setCallback is not called, this interface will not respond to any
     * other method calls.
     *
     * @param callback Handle to the AGNSS status callback interface.
     */
    void setCallback(in IAGnssCallback callback);

    /**
     * Notifies that the AGNSS data connection has been closed.
     */
    void dataConnClosed();

    /**
     * Notifies that a data connection is not available for AGNSS.
     */
    void dataConnFailed();

    /**
     * Sets the hostname and port for the AGNSS server.
     *
     * @param type Specifies if SUPL or C2K.
     * @param hostname Hostname of the AGNSS server.
     * @param port Port number associated with the server.
     */
    void setServer(in AGnssType type, in @utf8InCpp String hostname, in int port);

    /**
     * Notifies GNSS that a data connection is available and sets the network handle,
     * name of the APN, and its IP type to be used for SUPL connections.
     *
     * The HAL implementation must use the network handle to set the network for the
     * SUPL connection sockets using the android_setsocknetwork function. This will ensure
     * that there is a network path to the SUPL server. The network handle can also be used
     * to get the IP address of SUPL FQDN using the android_getaddrinfofornetwork() function.
     *
     * @param networkHandle Handle representing the network for use with the NDK API.
     * @param apn Access Point Name (follows regular APN naming convention).
     * @param apnIpType Specifies IP type of APN.
     */
    void dataConnOpen(
            in long networkHandle, in @utf8InCpp String apn, in ApnIpType apnIpType);
}
