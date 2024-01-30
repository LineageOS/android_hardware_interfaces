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
enum PhoneRestrictedState {
    /**
     * No restriction at all including voice/SMS/USSD/SS/AV64 and packet data.
     */
    NONE = 0x00,
    /**
     * Block emergency call due to restriction. But allow all normal voice/SMS/USSD/SS/AV64.
     */
    CS_EMERGENCY = 0x01,
    /**
     * Block all normal voice/SMS/USSD/SS/AV64 due to restriction. Only Emergency call allowed.
     */
    CS_NORMAL = 0x02,
    /**
     * Block all voice/SMS/USSD/SS/AV64 including emergency call due to restriction.
     */
    CS_ALL = 0x04,
    /**
     * Block packet data access due to restriction.
     */
    PS_ALL = 0x10,
}
