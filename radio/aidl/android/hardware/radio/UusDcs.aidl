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

package android.hardware.radio;

/**
 * User-to-User Signaling Information data coding schemes. Possible values for Octet 3 (Protocol
 * Discriminator field) in the UUIE. The values have been specified in section 10.5.4.25 of
 * 3GPP TS 24.008
 */
@VintfStability
@Backing(type="int")
enum UusDcs {
    /**
     * User specified protocol
     */
    USP,
    /**
     * OSI higher layer protocol
     */
    OSIHLP,
    /**
     * X.244
     */
    X244,
    /**
     * Reserved for system management
     */
    RMCF,
    /**
     * IA5 characters
     */
    IA5C,
}
