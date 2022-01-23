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

/*
 * Presence Check Algorithm as defined in ISO/IEC 14443-4:
 * https://www.iso.org/standard/73599.html
 */
@VintfStability
@Backing(type="byte")
enum PresenceCheckAlgorithm {
    /**
     * Let the stack select an algorithm
     */
    DEFAULT = 0,
    /**
     * ISO-DEP protocol's empty I-block
     */
    I_BLOCK = 1,
    /**
     * Type - 4 tag protocol iso-dep nak presence check command is sent waiting for
     * response and notification.
     */
    ISO_DEP_NAK = 2,
}
