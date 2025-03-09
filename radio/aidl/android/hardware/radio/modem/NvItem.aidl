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
 * @hide
 */
@VintfStability
@Backing(type="int")
@JavaDerive(toString=true)
enum NvItem {
    /**
     * @deprecated NV APIs are deprecated starting from Android U.
     */
    INVALID = 0,
    /**
     * CDMA radio and account information (items 1-10)
     * CDMA MEID (hex)
     * @deprecated NV APIs are deprecated starting from Android U.
     */
    CDMA_MEID = 1,
    /**
     * CDMA MIN (MSID)
     * @deprecated NV APIs are deprecated starting from Android U.
     */
    CDMA_MIN = 2,
    /**
     * CDMA MDN
     * @deprecated NV APIs are deprecated starting from Android U.
     */
    CDMA_MDN = 3,
    /**
     * CDMA access overload control
     * @deprecated NV APIs are deprecated starting from Android U.
     */
    CDMA_ACCOLC = 4,
    /**
     * Carrier device provisioning (items 11-30)
     * Device MSL
     * @deprecated NV APIs are deprecated starting from Android U.
     */
    DEVICE_MSL = 11,
    /**
     * RTN reconditioned status
     * @deprecated NV APIs are deprecated starting from Android U.
     */
    RTN_RECONDITIONED_STATUS = 12,
    /**
     * RTN activation date
     * @deprecated NV APIs are deprecated starting from Android U.
     */
    RTN_ACTIVATION_DATE = 13,
    /**
     * RTN life timer
     * @deprecated NV APIs are deprecated starting from Android U.
     */
    RTN_LIFE_TIMER = 14,
    /**
     * RTN life calls
     * @deprecated NV APIs are deprecated starting from Android U.
     */
    RTN_LIFE_CALLS = 15,
    /**
     * RTN life data TX
     * @deprecated NV APIs are deprecated starting from Android U.
     */
    RTN_LIFE_DATA_TX = 16,
    /**
     * RTN life data RX
     * @deprecated NV APIs are deprecated starting from Android U.
     */
    RTN_LIFE_DATA_RX = 17,
    /**
     * HFA in progress
     * @deprecated NV APIs are deprecated starting from Android U.
     */
    OMADM_HFA_LEVEL = 18,
    /**
     * Mobile IP profile information (items 31-50)
     * NAI realm
     * @deprecated NV APIs are deprecated starting from Android U.
     */
    MIP_PROFILE_NAI = 31,
    /**
     * MIP home address
     * @deprecated NV APIs are deprecated starting from Android U.
     */
    MIP_PROFILE_HOME_ADDRESS = 32,
    /**
     * AAA auth
     * @deprecated NV APIs are deprecated starting from Android U.
     */
    MIP_PROFILE_AAA_AUTH = 33,
    /**
     * HA auth
     * @deprecated NV APIs are deprecated starting from Android U.
     */
    MIP_PROFILE_HA_AUTH = 34,
    /**
     * Primary HA address
     * @deprecated NV APIs are deprecated starting from Android U.
     */
    MIP_PROFILE_PRI_HA_ADDR = 35,
    /**
     * Secondary HA address
     * @deprecated NV APIs are deprecated starting from Android U.
     */
    MIP_PROFILE_SEC_HA_ADDR = 36,
    /**
     * Reverse TUN preference
     * @deprecated NV APIs are deprecated starting from Android U.
     */
    MIP_PROFILE_REV_TUN_PREF = 37,
    /**
     * HA SPI
     * @deprecated NV APIs are deprecated starting from Android U.
     */
    MIP_PROFILE_HA_SPI = 38,
    /**
     * AAA SPI
     * @deprecated NV APIs are deprecated starting from Android U.
     */
    MIP_PROFILE_AAA_SPI = 39,
    /**
     * HA shared secret
     * @deprecated NV APIs are deprecated starting from Android U.
     */
    MIP_PROFILE_MN_HA_SS = 40,
    /**
     * AAA shared secret
     * @deprecated NV APIs are deprecated starting from Android U.
     */
    MIP_PROFILE_MN_AAA_SS = 41,
    /**
     * CDMA network and band config (items 51-70)
     * CDMA PRL version
     * @deprecated NV APIs are deprecated starting from Android U.
     */
    CDMA_PRL_VERSION = 51,
    /**
     * CDMA band class 10
     * @deprecated NV APIs are deprecated starting from Android U.
     */
    CDMA_BC10 = 52,
    /**
     * CDMA band class 14
     * @deprecated NV APIs are deprecated starting from Android U.
     */
    CDMA_BC14 = 53,
    /**
     * CDMA SO68
     * @deprecated NV APIs are deprecated starting from Android U.
     */
    CDMA_SO68 = 54,
    /**
     * CDMA SO73 COP0
     * @deprecated NV APIs are deprecated starting from Android U.
     */
    CDMA_SO73_COP0 = 55,
    /**
     * CDMA SO73 COP1-7
     * @deprecated NV APIs are deprecated starting from Android U.
     */
    CDMA_SO73_COP1TO7 = 56,
    /**
     * CDMA 1X Advanced enabled
     * @deprecated NV APIs are deprecated starting from Android U.
     */
    CDMA_1X_ADVANCED_ENABLED = 57,
    /**
     * CDMA eHRPD enabled
     * @deprecated NV APIs are deprecated starting from Android U.
     */
    CDMA_EHRPD_ENABLED = 58,
    /**
     * CDMA eHRPD forced
     * @deprecated NV APIs are deprecated starting from Android U.
     */
    CDMA_EHRPD_FORCED = 59,
    /**
     * LTE network and band config (items 71-90)
     * LTE band 25 enabled
     * @deprecated NV APIs are deprecated starting from Android U.
     */
    LTE_BAND_ENABLE_25 = 71,
    /**
     * LTE band 26 enabled
     * @deprecated NV APIs are deprecated starting from Android U.
     */
    LTE_BAND_ENABLE_26 = 72,
    /**
     * LTE band 41 enabled
     * @deprecated NV APIs are deprecated starting from Android U.
     */
    LTE_BAND_ENABLE_41 = 73,
    /**
     * LTE band 25 scan priority
     * @deprecated NV APIs are deprecated starting from Android U.
     */
    LTE_SCAN_PRIORITY_25 = 74,
    /**
     * LTE band 26 scan priority
     * @deprecated NV APIs are deprecated starting from Android U.
     */
    LTE_SCAN_PRIORITY_26 = 75,
    /**
     * LTE band 41 scan priority
     * @deprecated NV APIs are deprecated starting from Android U.
     */
    LTE_SCAN_PRIORITY_41 = 76,
    /**
     * LTE hidden band 25 priority
     * @deprecated NV APIs are deprecated starting from Android U.
     */
    LTE_HIDDEN_BAND_PRIORITY_25 = 77,
    /**
     * LTE hidden band 26 priority
     * @deprecated NV APIs are deprecated starting from Android U.
     */
    LTE_HIDDEN_BAND_PRIORITY_26 = 78,
    /**
     * LTE hidden band 41 priority
     * @deprecated NV APIs are deprecated starting from Android U.
     */
    LTE_HIDDEN_BAND_PRIORITY_41 = 79,
}
