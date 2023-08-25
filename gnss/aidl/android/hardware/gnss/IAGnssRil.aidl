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

package android.hardware.gnss;

import android.hardware.gnss.IAGnssRilCallback;

/**
 * Extended interface for AGNSS RIL support. An Assisted GNSS Radio Interface
 * Layer interface allows the GNSS chipset to request radio interface layer
 * information from Android platform. Examples of such information are reference
 * location, unique subscriber ID, phone number string and network availability changes.
 *
 * @hide
 */
@VintfStability
interface IAGnssRil {
    /** Network capability mode bitmask for not metered. */
    const int NETWORK_CAPABILITY_NOT_METERED = 0x01;

    /** Network capability mode bitmask for not roaming. */
    const int NETWORK_CAPABILITY_NOT_ROAMING = 0x02;

    /** AGNSS reference location type */
    @VintfStability
    @Backing(type="int")
    enum AGnssRefLocationType {
        GSM_CELLID = 1,
        UMTS_CELLID = 2,
        LTE_CELLID = 4,
        NR_CELLID = 8,
    }

    /** SET ID type*/
    @VintfStability
    @Backing(type="int")
    enum SetIdType {
        NONE = 0,
        IMSI = 1,
        MSISDM = 2,
    }

    /**
     * CellID for 2G, 3G ,LTE and NR used in AGNSS. This is defined in
     * UserPlane Location Protocol (Version 2.0.4).
     */
    @VintfStability
    parcelable AGnssRefLocationCellID {
        AGnssRefLocationType type;

        /** Mobile Country Code. */
        int mcc;

        /** Mobile Network Code .*/
        int mnc;

        /**
         * Location Area Code in 2G, 3G and LTE. In 3G lac is discarded. In LTE,
         * lac is populated with tac, to ensure that we don't break old clients that
         * might rely on the old (wrong) behavior.
         */
        int lac;

        /**
         *  Cell id in 2G. Utran Cell id in 3G. Cell Global Id EUTRA in LTE.
         *  Cell Global Id NR in 5G.
         */
        long cid;

        /** Tracking Area Code in LTE and NR. */
        int tac;

        /** Physical Cell id in LTE and NR (not used in 2G and 3G) */
        int pcid;

        /** Absolute Radio Frequency Channel Number in NR. */
        int arfcn;
    }

    /** Represents ref locations */
    @VintfStability
    parcelable AGnssRefLocation {
        AGnssRefLocationType type;

        AGnssRefLocationCellID cellID;
    }

    /** Represents network connection status and capabilities. */
    @VintfStability
    parcelable NetworkAttributes {
        /** A handle representing this Network. */
        long networkHandle;

        /**
         * True indicates that network connectivity exists and it is possible to
         * establish connections and pass data. If false, only the networkHandle field
         * is populated to indicate that this network has just disconnected.
         */
        boolean isConnected;

        /**
         * A bitfield of flags indicating the capabilities of this network. The bit masks are
         * defined in NETWORK_CAPABILITY_*.
         */
        int capabilities;

        /**
         * Telephony preferred Access Point Name to use for carrier data connection when
         * connected to a cellular network. Empty string, otherwise.
         */
        @utf8InCpp String apn;
    }

    /**
     * Opens the AGNSS interface and provides the callback routines
     * to the implementation of this interface.
     *
     * @param callback Interface for AGnssRil callbacks.
     *
     */
    void setCallback(in IAGnssRilCallback callback);

    /**
     * Sets the reference location.
     *
     * @param agnssReflocation AGNSS reference location CellID.
     *
     */
    void setRefLocation(in AGnssRefLocation agnssReflocation);

    /**
     * Sets the SET ID.
     *
     * @param type Must be populated with either IMSI or MSISDN or NONE.
     * @param setid If type is IMSI then setid is populated with
     * a string representing the unique Subscriber ID, for example, the IMSI for
     * a GMS phone. If type is MSISDN, then setid must contain
     * the phone number string for line 1. For example, the MSISDN for a GSM phone.
     * If the type is NONE, then the string must be empty.
     *
     */
    void setSetId(in SetIdType type, in @utf8InCpp String setid);

    /**
     * Notifies GNSS of network status changes.
     *
     * The framework calls this method to update the GNSS HAL implementation of network
     * state changes.
     *
     * @param attributes Updated network attributes.
     *
     */
    void updateNetworkState(in NetworkAttributes attributes);

    /**
     * Injects an SMS/WAP initiated SUPL message.
     *
     * @param msgData ASN.1 encoded SUPL INIT message. This is defined in
     * UserPlane Location Protocol (Version 2.0.4).
     * @param slotIndex Specifies the slot index (See
     *         android.telephony.SubscriptionManager#getSlotIndex()) of the SUPL connection.
     */
    void injectNiSuplMessageData(in byte[] msgData, in int slotIndex);
}
