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

package android.hardware.radio.network;

/** @hide */
@VintfStability
@Backing(type="int")
@JavaDerive(toString=true)
enum IndicationFilter {
    NONE = 0,
    ALL = ~0,
    /**
     * When this bit is set, modem must send the signal strength update through
     * IRadioNetworkIndication.currentSignalStrength() when all criteria specified by
     * IRadioNetwork.setSignalStrengthReportingCriteria() are met.
     */
    SIGNAL_STRENGTH = 1 << 0,
    /**
     * When this bit is set, modem must invoke IRadioNetworkIndication.networkStateChanged() when
     * any field in the voice or data RegStateResult changes. When this bit is not set, modem must
     * suppress IRadioNetworkIndication.networkStateChanged() when there are only changes from
     * insignificant fields. Modem must invoke IRadioNetworkIndication.networkStateChanged() when
     * significant fields are updated regardless of whether this bit is set.
     *
     * The following fields in RegStateResult are considered significant: regState, rat.
     */
    FULL_NETWORK_STATE = 1 << 1,
    /**
     * When this bit is set, modem must send IRadioNetworkIndication.dataCallListChanged() whenever
     * any field in SetupDataCallResult changes. When this bit is not set, modem must suppress the
     * indication when the only changed field is 'active' (for data dormancy). For all other field
     * changes, the modem must send IRadioNetworkIndication.dataCallListChanged() regardless of
     * whether this bit is set.
     */
    DATA_CALL_DORMANCY_CHANGED = 1 << 2,
    /**
     * When this bit is set, modem must send the link capacity update through
     * IRadioNetworkIndication.currentLinkCapacityEstimate() when all criteria specified by
     * IRadioNetwork.setLinkCapacityReportingCriteria() are met.
     */
    LINK_CAPACITY_ESTIMATE = 1 << 3,
    /**
     * When this bit is set, the modem must send the physical channel configuration update through
     * IRadioNetworkIndication.currentPhysicalChannelConfigs() when the configuration has changed.
     * It is recommended that this be reported whenever link capacity or signal strength is
     * reported.
     */
    PHYSICAL_CHANNEL_CONFIG = 1 << 4,
    /**
     * Control the unsolicited sending of registration failure reports via onRegistrationFailed
     */
    REGISTRATION_FAILURE = 1 << 5,
    /**
     * Control the unsolicited sending of barring info updates via onBarringInfo
     */
    BARRING_INFO = 1 << 6,
}
