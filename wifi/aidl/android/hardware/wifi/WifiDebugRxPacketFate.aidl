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

package android.hardware.wifi;

/**
 * Enum describing the fate of the RX packets.
 */
@VintfStability
@Backing(type="int")
enum WifiDebugRxPacketFate {
    /**
     * Valid and delivered to network stack (e.g., netif_rx()).
     */
    SUCCESS,
    /**
     * Queued within firmware, but not yet sent to driver.
     */
    FW_QUEUED,
    /**
     * Dropped by firmware due to host-programmable filters.
     */
    FW_DROP_FILTER,
    /**
     * Dropped by firmware as invalid (e.g. bad checksum, decrypt failed,
     * or invalid for current state).
     */
    FW_DROP_INVALID,
    /**
     * Dropped by firmware due to lack of buffer space.
     */
    FW_DROP_NOBUFS,
    /**
     * Dropped by firmware for any other reason.
     */
    FW_DROP_OTHER,
    /**
     * Queued within driver, not yet delivered to network stack.
     */
    DRV_QUEUED,
    /**
     * Dropped by driver due to filter rules.
     */
    DRV_DROP_FILTER,
    /**
     * Dropped by driver as invalid (e.g. not permitted in current state).
     */
    DRV_DROP_INVALID,
    /**
     * Dropped by driver due to lack of buffer space.
     */
    DRV_DROP_NOBUFS,
    /**
     * Dropped by driver for any other reason.
     */
    DRV_DROP_OTHER,
}
