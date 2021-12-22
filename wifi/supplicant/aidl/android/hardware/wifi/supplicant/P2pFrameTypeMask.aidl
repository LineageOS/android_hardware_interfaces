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

package android.hardware.wifi.supplicant;

/**
 * Bitmask of P2P frame types.
 */
@VintfStability
@Backing(type="int")
enum P2pFrameTypeMask {
    /** P2P probe request frame */
    P2P_FRAME_PROBE_REQ_P2P = 1 << 0,
    /** P2P probe response frame */
    P2P_FRAME_PROBE_RESP_P2P = 1 << 1,
    /** P2P probe response frame from the group owner */
    P2P_FRAME_PROBE_RESP_P2P_GO = 1 << 2,
    /** Beacon frame from the group owner */
    P2P_FRAME_BEACON_P2P_GO = 1 << 3,
    /** Provision discovery request frame */
    P2P_FRAME_P2P_PD_REQ = 1 << 4,
    /** Provision discovery response frame */
    P2P_FRAME_P2P_PD_RESP = 1 << 5,
    /** Group negotiation request frame */
    P2P_FRAME_P2P_GO_NEG_REQ = 1 << 6,
    /** Group negotiation response frame */
    P2P_FRAME_P2P_GO_NEG_RESP = 1 << 7,
    /** Group negotiation confirm frame */
    P2P_FRAME_P2P_GO_NEG_CONF = 1 << 8,
    /** Invitation request frame */
    P2P_FRAME_P2P_INV_REQ = 1 << 9,
    /** Invitation response frame */
    P2P_FRAME_P2P_INV_RESP = 1 << 10,
    /** P2P Association request frame */
    P2P_FRAME_P2P_ASSOC_REQ = 1 << 11,
    /** P2P Association response frame */
    P2P_FRAME_P2P_ASSOC_RESP = 1 << 12,
}
