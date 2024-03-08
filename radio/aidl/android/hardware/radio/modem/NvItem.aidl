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

package android.hardware.radio.modem;

/**
 * @deprecated NV APIs are deprecated starting from Android U.
 * @hide
 */
@VintfStability
@Backing(type="int")
@JavaDerive(toString=true)
enum NvItem {
    /**
     * CDMA radio and account information (items 1-10)
     * CDMA MEID (hex)
     */
    CDMA_MEID = 1,
    /**
     * CDMA MIN (MSID)
     */
    CDMA_MIN = 2,
    /**
     * CDMA MDN
     */
    CDMA_MDN = 3,
    /**
     * CDMA access overload control
     */
    CDMA_ACCOLC = 4,
    /**
     * Carrier device provisioning (items 11-30)
     * Device MSL
     */
    DEVICE_MSL = 11,
    /**
     * RTN reconditioned status
     */
    RTN_RECONDITIONED_STATUS = 12,
    /**
     * RTN activation date
     */
    RTN_ACTIVATION_DATE = 13,
    /**
     * RTN life timer
     */
    RTN_LIFE_TIMER = 14,
    /**
     * RTN life calls
     */
    RTN_LIFE_CALLS = 15,
    /**
     * RTN life data TX
     */
    RTN_LIFE_DATA_TX = 16,
    /**
     * RTN life data RX
     */
    RTN_LIFE_DATA_RX = 17,
    /**
     * HFA in progress
     */
    OMADM_HFA_LEVEL = 18,
    /**
     * Mobile IP profile information (items 31-50)
     * NAI realm
     */
    MIP_PROFILE_NAI = 31,
    /**
     * MIP home address
     */
    MIP_PROFILE_HOME_ADDRESS = 32,
    /**
     * AAA auth
     */
    MIP_PROFILE_AAA_AUTH = 33,
    /**
     * HA auth
     */
    MIP_PROFILE_HA_AUTH = 34,
    /**
     * Primary HA address
     */
    MIP_PROFILE_PRI_HA_ADDR = 35,
    /**
     * Secondary HA address
     */
    MIP_PROFILE_SEC_HA_ADDR = 36,
    /**
     * Reverse TUN preference
     */
    MIP_PROFILE_REV_TUN_PREF = 37,
    /**
     * HA SPI
     */
    MIP_PROFILE_HA_SPI = 38,
    /**
     * AAA SPI
     */
    MIP_PROFILE_AAA_SPI = 39,
    /**
     * HA shared secret
     */
    MIP_PROFILE_MN_HA_SS = 40,
    /**
     * AAA shared secret
     */
    MIP_PROFILE_MN_AAA_SS = 41,
    /**
     * CDMA network and band config (items 51-70)
     * CDMA PRL version
     */
    CDMA_PRL_VERSION = 51,
    /**
     * CDMA band class 10
     */
    CDMA_BC10 = 52,
    /**
     * CDMA band class 14
     */
    CDMA_BC14 = 53,
    /**
     * CDMA SO68
     */
    CDMA_SO68 = 54,
    /**
     * CDMA SO73 COP0
     */
    CDMA_SO73_COP0 = 55,
    /**
     * CDMA SO73 COP1-7
     */
    CDMA_SO73_COP1TO7 = 56,
    /**
     * CDMA 1X Advanced enabled
     */
    CDMA_1X_ADVANCED_ENABLED = 57,
    /**
     * CDMA eHRPD enabled
     */
    CDMA_EHRPD_ENABLED = 58,
    /**
     * CDMA eHRPD forced
     */
    CDMA_EHRPD_FORCED = 59,
    /**
     * LTE network and band config (items 71-90)
     * LTE band 25 enabled
     */
    LTE_BAND_ENABLE_25 = 71,
    /**
     * LTE band 26 enabled
     */
    LTE_BAND_ENABLE_26 = 72,
    /**
     * LTE band 41 enabled
     */
    LTE_BAND_ENABLE_41 = 73,
    /**
     * LTE band 25 scan priority
     */
    LTE_SCAN_PRIORITY_25 = 74,
    /**
     * LTE band 26 scan priority
     */
    LTE_SCAN_PRIORITY_26 = 75,
    /**
     * LTE band 41 scan priority
     */
    LTE_SCAN_PRIORITY_41 = 76,
    /**
     * LTE hidden band 25 priority
     */
    LTE_HIDDEN_BAND_PRIORITY_25 = 77,
    /**
     * LTE hidden band 26 priority
     */
    LTE_HIDDEN_BAND_PRIORITY_26 = 78,
    /**
     * LTE hidden band 41 priority
     */
    LTE_HIDDEN_BAND_PRIORITY_41 = 79,
}
