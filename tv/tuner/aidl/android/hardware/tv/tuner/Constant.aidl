/*
 * Copyright 2021 The Android Open Source Project
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

package android.hardware.tv.tuner;

/**
 * @hide
 */
@VintfStability
@Backing(type="int")
enum Constant {
    /**
     * An invalid packet ID in transport stream according to ISO/IEC 13818-1.
     */
    INVALID_TS_PID = 0xFFFF,

    /**
     * An invalid Stream ID.
     */
    INVALID_STREAM_ID = 0xFFFF,

    /**
     * An invalid Filter ID.
     */
    INVALID_FILTER_ID = 0xFFFFFFFF,

    /**
     * An invalid AV sync hardware ID.
     */
    INVALID_AV_SYNC_ID = 0xFFFFFFFF,

    /**
     * An invalid mpuSequenceNumber.
     */
    INVALID_MMTP_RECORD_EVENT_MPT_SEQUENCE_NUM = 0xFFFFFFFF,

    /**
     * An invalid first macroblock address.
     */
    INVALID_FIRST_MACROBLOCK_IN_SLICE = 0xFFFFFFFF,

    /**
     * An invalid frenquency that can be used as the default value of the frontend setting.
     */
    INVALID_FRONTEND_SETTING_FREQUENCY = 0xFFFFFFFF,

    /**
     * An invalid context id that can be used as the default value of the unconfigured id. It can
     * be used to reset the configured ip context id.
     */
    INVALID_IP_FILTER_CONTEXT_ID = 0xFFFFFFFF,

    /**
     * An invalid local transport stream id used as the return value on a failed operation of
     * IFrontend.linkCiCam.
     */
    INVALID_LTS_ID = 0xFFFFFFFF,

    /**
     * An invalid frontend ID.
     */
    INVALID_FRONTEND_ID = 0xFFFFFFFF,

    /**
     * An invalid LNB ID.
     */
    INVALID_LNB_ID = 0xFFFFFFFF,

    /**
     * An invalid key token. It is used to remove the current key from the descrambler.
     */
    INVALID_KEYTOKEN = 0x00,

     /**
     * An invalid section filter version number.
     */
    INVALID_TABINFO_VERSION = 0xFFFFFFFF,
}
