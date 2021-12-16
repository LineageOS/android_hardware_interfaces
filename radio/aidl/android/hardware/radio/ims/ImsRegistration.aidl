/*
 * Copyright (C) 2022 The Android Open Source Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License"),
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

package android.hardware.radio.ims;

@VintfStability
parcelable ImsRegistration {

    @VintfStability
    @Backing(type="int")
    enum State {
        /** IMS is not registered */
        NOT_REGISTERED,
        /**
         * IMS registration procedure just started.
         * It could be used by radio to start IMS establishment timer based on carrier requirements.
         */
        REGISTERING,
        /** IMS is successfully registered */
        REGISTERED,
    }

    @VintfStability
    @Backing(type="int")
    enum ImsAccessNetwork {
        /**
         * Indicates the access network where IMS is registered. If IMS is registered on
         * cellular then the radio shall wait for IMS PDN disconnection for 4 seconds
         * before performing NAS detach procedure. This requirement is not applicable for
         * if access network IWLAN and CROSS_SIM.
         */
        NONE,
        EUTRAN,
        NGRAN,
        UTRAN,
        /** The IMS registration is done over WiFi network. */
        IWLAN,
        /**
         * The radio shall not consider the registration information with this type
         * while voice domain selection is performed.
         * The IMS registration is done over internet of the default data  subscription.
         */
        CROSS_SIM,
    }

    @VintfStability
    @Backing(type="int")
    enum FailureReason {
        /** Default value */
        NONE,
        /**
         * Indicates that the IMS registration is failed with fatal error 403 or 404
         * on all P-CSCF addresses. The radio shall block the current PLMN or disable
         * the RAT as per the carrier requirements.
         */
        FATAL_ERROR,
        /**
         * Indicates that the IMS registration on current PLMN failed multiple times.
         * The radio shall block the current PLMN or disable the RAT as per the
         * carrier requirements.
         */
        REPEATED_ERROR,
        /*
         * Indicates that IMS registration has failed temporarily.
         */
        TEMPORARY_ERROR,
    }

    /** Default value */
    const int FEATURE_NONE = 0;
    /** IMS voice feature */
    const int FEATURE_VOICE = 1 << 0;
    /** IMS video feature */
    const int FEATURE_VIDEO = 1 << 1;
    /** IMS SMS feature */
    const int FEATURE_SMS = 1 << 2;

    /** Indicates the current IMS registration state. */
    State state;

    /** Indicates the IP connectivity access network where IMS features are registered. */
    ImsAccessNetwork ipcan;

    /** Indicates a failure reason for IMS registration. */
    FailureReason reason;

    /**
     * Values are bitwise ORs of FEATURE_.
     * IMS features such as VOICE, VIDEO and SMS.
     */
    int features;
}
