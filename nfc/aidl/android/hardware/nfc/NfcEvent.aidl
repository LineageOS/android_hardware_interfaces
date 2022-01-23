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

/**
 * Nfc event to notify nfc status change.
 */
@VintfStability
@Backing(type="int")
enum NfcEvent {
    /**
     * Open complete event to notify upper layer when NFC HAL and NFCC
     * initialization complete.
     */
    OPEN_CPLT = 0,
    /**
     * Close complete event to notify upper layer when HAL close is done.
     */
    CLOSE_CPLT = 1,
    /**
     * Post init complete event to notify upper layer when post init operations
     * are done.
     */
    POST_INIT_CPLT = 2,
    /**
     * Pre-discover complete event to notify upper layer when pre-discover
     * operations are done.
     */
    PRE_DISCOVER_CPLT = 3,
    /**
     * HCI network reset event to notify upplayer when HCI network needs to
     * be re-initialized in case of an error.
     */
    HCI_NETWORK_RESET = 4,
    /**
     * Error event to notify upper layer when there's an unknown error.
     */
    ERROR = 5,
}
