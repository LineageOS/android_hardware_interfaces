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
 * List of interface concurrency types, used in reporting device concurrency capabilities.
 */
@VintfStability
@Backing(type="int")
enum IfaceConcurrencyType {
    /**
     * Concurrency type for station mode.
     */
    STA,
    /**
     * Concurrency type of single-port AP mode.
     */
    AP,
    /**
     * Concurrency type of two-port bridged AP mode.
     */
    AP_BRIDGED,
    /**
     * Concurrency type of peer-to-peer mode.
     */
    P2P,
    /**
     * Concurrency type of neighborhood area network mode.
     */
    NAN_IFACE,
}
