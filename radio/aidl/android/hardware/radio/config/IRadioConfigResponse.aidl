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

package android.hardware.radio.config;

import android.hardware.radio.config.PhoneCapability;
import android.hardware.radio.config.SimSlotStatus;

/**
 * Interface declaring response functions to solicited radio config requests.
 * @hide
 */
@VintfStability
oneway interface IRadioConfigResponse {
    /**
     * @param info Response info struct containing response type, serial no. and error
     * @param modemReducedFeatureSet1 True indicates that the modem does NOT support the following
     *        features.
     *        - Providing either LinkCapacityEstimate:secondaryDownlinkCapacityKbps
     *          or LinkCapacityEstimate:secondaryUplinkCapacityKbps when given from
     *          RadioIndication:currentLinkCapacityEstimate
     *        - Calling IRadio.setNrDualConnectivityState or querying
     *          IRadio.isNrDualConnectivityEnabled
     *        - Requesting IRadio.setDataThrottling()
     *        - Providing SlicingConfig through getSlicingConfig()
     *        - Providing PhysicalChannelConfig through
     *          IRadioIndication.currentPhysicalChannelConfigs_1_6()
     *
     * Valid errors returned:
     *   RadioError:REQUEST_NOT_SUPPORTED when android.hardware.telephony is not defined
     *   RadioError:NONE
     *   RadioError:RADIO_NOT_AVAILABLE
     *   RadioError:INTERNAL_ERR
     */
    void getHalDeviceCapabilitiesResponse(
            in android.hardware.radio.RadioResponseInfo info, in boolean modemReducedFeatureSet1);

    /**
     * @param info Response info struct containing response type, serial no. and error
     * @param numOfLiveModems <byte> indicate the number of live modems i.e. modems that
     *        are enabled and actively working as part of a working connectivity stack
     *
     * Valid errors returned:
     *   RadioError:REQUEST_NOT_SUPPORTED when android.hardware.telephony is not defined
     *   RadioError:NONE
     *   RadioError:RADIO_NOT_AVAILABLE
     */
    void getNumOfLiveModemsResponse(
            in android.hardware.radio.RadioResponseInfo info, in byte numOfLiveModems);

    /**
     * @param info Response info struct containing response type, serial no. and error
     * @param phoneCapability <PhoneCapability> it defines modem's capability for example
     *        how many logical modems it has, how many data connections it supports.
     *
     * Valid errors returned:
     *   RadioError:REQUEST_NOT_SUPPORTED when android.hardware.telephony is not defined
     *   RadioError:NONE
     *   RadioError:RADIO_NOT_AVAILABLE
     *   RadioError:INTERNAL_ERR
     */
    void getPhoneCapabilityResponse(
            in android.hardware.radio.RadioResponseInfo info, in PhoneCapability phoneCapability);

    /**
     * @param info Response info struct containing response type, serial no. and error
     * @param slotStatus Sim slot struct containing all the physical SIM slots info with size
     *        equal to the number of physical slots on the device
     *
     * Valid errors returned:
     *   RadioError:REQUEST_NOT_SUPPORTED when android.hardware.telephony.subscription is not
     *                                    defined
     *   RadioError:NONE
     *   RadioError:RADIO_NOT_AVAILABLE
     *   RadioError:NO_MEMORY
     *   RadioError:INTERNAL_ERR
     *   RadioError:MODEM_ERR
     */
    void getSimSlotsStatusResponse(
            in android.hardware.radio.RadioResponseInfo info, in SimSlotStatus[] slotStatus);

    /**
     * @param info Response info struct containing response type, serial no. and error
     *
     * Valid errors returned:
     *   RadioError:REQUEST_NOT_SUPPORTED when android.hardware.telephony is not defined
     *   RadioError:NONE
     *   RadioError:RADIO_NOT_AVAILABLE
     *   RadioError:INVALID_ARGUMENTS
     */
    void setNumOfLiveModemsResponse(in android.hardware.radio.RadioResponseInfo info);

    /**
     * @param info Response info struct containing response type, serial no. and error
     *
     * Valid errors returned:
     *   RadioError:REQUEST_NOT_SUPPORTED when android.hardware.telephony.data is not defined
     *   RadioError:NONE
     *   RadioError:RADIO_NOT_AVAILABLE
     *   RadioError:INTERNAL_ERR
     *   RadioError:INVALID_ARGUMENTS
     */
    void setPreferredDataModemResponse(in android.hardware.radio.RadioResponseInfo info);

    /**
     * @param info Response info struct containing response type, serial no. and error
     *
     * Valid errors returned:
     *   RadioError:REQUEST_NOT_SUPPORTED when android.hardware.telephony.subscription is not
     *                                    defined
     *   RadioError:NONE
     *   RadioError:RADIO_NOT_AVAILABLE
     *   RadioError:NO_MEMORY
     *   RadioError:INTERNAL_ERR
     *   RadioError:MODEM_ERR
     *   RadioError:INVALID_ARGUMENTS
     */
    void setSimSlotsMappingResponse(in android.hardware.radio.RadioResponseInfo info);
}
