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

package android.hardware.nfc;

import android.hardware.nfc.INfcClientCallback;
import android.hardware.nfc.NfcCloseType;
import android.hardware.nfc.NfcConfig;
import android.hardware.nfc.NfcStatus;

@VintfStability
interface INfc {
    /**
     * Opens the NFC controller device and performs initialization.
     * This may include patch download and other vendor-specific initialization.
     *
     * If open completes successfully, the controller must be ready to perform NCI
     * initialization - ie accept CORE_RESET and subsequent commands through the write()
     * call.
     *
     * Returns ok() if initialization starts successfully.
     * If open() returns ok(), the NCI stack must wait for a NfcEvent.OPEN_CPLT
     * callback before continuing.
     * If a NfcEvent.OPEN_CPLT callback received with status NfcStatus::OK, the controller
     * must be ready to perform  NCI initialization - ie accept CORE_RESET and subsequent
     * commands through the write() call.
     */
    void open(in INfcClientCallback clientCallback);

    /**
     * Close the NFC HAL and setup NFC controller close type.
     * Associated resources such as clientCallback must be released.
     * The clientCallback reference from open() must be invalid after close().
     * If close() returns ok(), the NCI stack must wait for a NfcEvent.CLOSE_CPLT
     * callback before continuing.
     * Returns an error if close may be called more than once.
     * Calls to any other method which expect a callback after this must return
     * a service-specific error NfcStatus::FAILED.
     * HAL close automatically if the client drops the reference to the HAL or
     * crashes.
     */
    void close(in NfcCloseType type);

    /**
     * coreInitialized() is called after the CORE_INIT_RSP is received from the
     * NFCC. At this time, the HAL can do any chip-specific configuration.
     *
     * If coreInitialized() returns ok(), the NCI stack must wait for a
     * NfcEvent.POST_INIT_CPLT before continuing.
     * If coreInitialized() returns an error, the NCI stack must continue immediately.
     *
     * coreInitialized() must be called after open() registers the clientCallback
     * or return a service-specific error NfcStatus::FAILED directly.
     *
     */
    void coreInitialized();

    /**
     * Clears the NFC chip.
     *
     * Must be called during factory reset and/or before the first time the HAL is
     * initialized after a factory reset.
     */
    void factoryReset();

    /**
     * Fetches vendor specific configurations.
     * @return NfcConfig indicates support for certain features and
     * populates the vendor specific configs.
     */
    NfcConfig getConfig();

    /**
     * Restart controller by power cyle;
     * It's similar to open but just reset the controller without initialize all the
     * resources.
     *
     * If powerCycle() returns ok(), the NCI stack must wait for a NfcEvent.OPEN_CPLT
     * before continuing.
     *
     * powerCycle() must be called after open() registers the clientCallback
     * or return a service-specific error NfcStatus::FAILED directly.
     */
    void powerCycle();

    /**
     * preDiscover is called every time before starting RF discovery.
     * It is a good place to do vendor-specific configuration that must be
     * performed every time RF discovery is about to be started.
     *
     * If preDiscover() returns ok(), the NCI stack must wait for a
     * NfcEvent.PREDISCOVER_CPLT before continuing.
     *
     * preDiscover() must be called after open() registers the clientCallback
     * or return a service-specific error NfcStatus::FAILED directly.
     *
     * If preDiscover() reports an error, the NCI stack must start RF discovery immediately.
     */
    void preDiscover();

    /**
     * Performs an NCI write.
     *
     * This method may queue writes and return immediately. The only
     * requirement is that the writes are executed in order.
     *
     * @param data
     * Data packet to transmit NCI Commands and Data Messages over write.
     * Detailed format is defined in NFC Controller Interface (NCI) Technical Specification.
     * https://nfc-forum.org/our-work/specification-releases/
     *
     * @return number of bytes written to the NFCC.
     */
    int write(in byte[] data);

    /**
     * Set the logging flag for NFC HAL to enable it's verbose logging.
     * If verbose logging is not supported, the call must not have any effect on logging verbosity.
     * However, isVerboseLoggingEnabled() must still return the value set by the last call to
     * setEnableVerboseLogging().
     * @param enable for setting the verbose logging flag to HAL
     */
    void setEnableVerboseLogging(in boolean enable);

    /**
     * Get the verbose logging flag value from NFC HAL.
     * @return true if verbose logging flag value is enabled, false if disabled.
     */
    boolean isVerboseLoggingEnabled();
}
