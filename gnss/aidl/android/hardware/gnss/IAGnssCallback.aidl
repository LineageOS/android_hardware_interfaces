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

/**
 * Callback structure for the AGNSS interface.
 *
 * @hide
 */
@VintfStability
interface IAGnssCallback {
    /** AGNSS service type */
    @VintfStability
    @Backing(type="int")
    enum AGnssType {
        // Secure User Plane Location
        SUPL = 1,
        // CDMA2000
        C2K = 2,
        // SUPL, Emergency call over IP Multimedia Subsystem
        SUPL_EIMS = 3,
        // SUPL, IP Multimedia Subsystem
        SUPL_IMS = 4,
    }

    /** AGNSS status value */
    @VintfStability
    @Backing(type="int")
    enum AGnssStatusValue {
        /** GNSS requests data connection for AGNSS. */
        REQUEST_AGNSS_DATA_CONN = 1,

        /** GNSS releases the AGNSS data connection. */
        RELEASE_AGNSS_DATA_CONN = 2,

        /** AGNSS data connection initiated */
        AGNSS_DATA_CONNECTED = 3,

        /** AGNSS data connection completed */
        AGNSS_DATA_CONN_DONE = 4,

        /** AGNSS data connection failed */
        AGNSS_DATA_CONN_FAILED = 5,
    }

    /**
     * Callback with AGNSS status information.
     *
     * The GNSS HAL implementation must use this method to request the framework to setup
     * network connection for the specified AGNSS service and to update the connection
     * status so that the framework can release the resources.
     *
     * @param type Type of AGNSS service.
     * @parama status Status of the data connection.
     */
    void agnssStatusCb(in AGnssType type, in AGnssStatusValue status);
}
