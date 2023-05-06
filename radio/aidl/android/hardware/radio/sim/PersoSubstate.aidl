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

package android.hardware.radio.sim;

/**
 * Additional personalization categories in addition to those specified in 3GPP TS 22.022 and
 * 3GPP2 C.S0068-0.
 * @hide
 */
@VintfStability
@Backing(type="int")
@JavaDerive(toString=true)
enum PersoSubstate {
    /**
     * Initial state
     */
    UNKNOWN,
    /**
     * In between each lock transition
     */
    IN_PROGRESS,
    /**
     * When either SIM or RUIM Perso is finished since each app must only have 1 active perso
     * involved.
     */
    READY,
    SIM_NETWORK,
    SIM_NETWORK_SUBSET,
    SIM_CORPORATE,
    SIM_SERVICE_PROVIDER,
    SIM_SIM,
    /**
     * The corresponding perso lock is blocked
     */
    SIM_NETWORK_PUK,
    SIM_NETWORK_SUBSET_PUK,
    SIM_CORPORATE_PUK,
    SIM_SERVICE_PROVIDER_PUK,
    SIM_SIM_PUK,
    RUIM_NETWORK1,
    RUIM_NETWORK2,
    RUIM_HRPD,
    RUIM_CORPORATE,
    RUIM_SERVICE_PROVIDER,
    RUIM_RUIM,
    /**
     * The corresponding perso lock is blocked
     */
    RUIM_NETWORK1_PUK,
    RUIM_NETWORK2_PUK,
    RUIM_HRPD_PUK,
    RUIM_CORPORATE_PUK,
    RUIM_SERVICE_PROVIDER_PUK,
    RUIM_RUIM_PUK,
    /**
     * The device is personalized using the content of the Service Provider Name (SPN) in the SIM
     * card.
     */
    SIM_SPN,
    SIM_SPN_PUK,
    /**
     * Service Provider and Equivalent Home PLMN. The device is personalized using both the content
     * of the GID1 (equivalent to service provider personalization) and the content of the
     * Equivalent Home PLMN (EHPLMN) in the SIM card. If the GID1 in the SIM is absent, then just
     * the content of the Equivalent Home PLMN is matched.
     */
    SIM_SP_EHPLMN,
    SIM_SP_EHPLMN_PUK,
    /**
     * Device is personalized using the first digits of the ICCID of the SIM card.
     */
    SIM_ICCID,
    SIM_ICCID_PUK,
    /**
     * Device is personalized using the content of the IMPI in the ISIM.
     */
    SIM_IMPI,
    SIM_IMPI_PUK,
    /**
     * Network Subset and Service Provider. Device is personalized using both the content of GID1
     * (equivalent to service provider personalization) and the first digits of the IMSI (equivalent
     * to network subset personalization).
     */
    SIM_NS_SP,
    SIM_NS_SP_PUK,
}
