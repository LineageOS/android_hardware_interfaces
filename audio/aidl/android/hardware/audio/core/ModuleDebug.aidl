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

package android.hardware.audio.core;

/**
 * This structure contains flags used for enabling various debugging aspects
 * in a HAL module. By default, all debugging aspects are turned off. They
 * can be enabled during xTS tests for functionality that, for example, would
 * otherwise require human intervention (e.g. connection of external devices).
 */
@JavaDerive(equals=true, toString=true)
@VintfStability
parcelable ModuleDebug {
    /**
     * When set to 'true', HAL module must simulate connection of external
     * devices. An external device becomes 'connected' after a call to
     * IModule.connectExternalDevice, simulation of connection requires:
     *  - provision of at least one non-dynamic device port profile on
     *    connection (as if it was retrieved from a connected device);
     *  - simulating successful application of port configurations for reported
     *    profiles.
     */
    boolean simulateDeviceConnections;
    /**
     * Must be non-negative. When set to non-zero, HAL module must delay
     * transition from "transient" stream states (see StreamDescriptor.aidl)
     * by the specified amount of milliseconds. The purpose of this delay
     * is to allow VTS to test sending of stream commands while the stream is
     * in a transient state. The delay must apply to newly created streams,
     * it is not required to apply the delay to already opened streams.
     *
     * Note: the drawback of enabling this delay for asynchronous (non-blocking)
     *       modes is that sending of callbacks will also be delayed, because
     *       callbacks are sent once the stream state machine exits a transient
     *       state. Thus, it's not recommended to use it with tests that require
     *       waiting for an async callback.
     */
    int streamTransientStateDelayMs;
}
