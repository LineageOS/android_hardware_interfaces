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

package android.hardware.usb;

@VintfStability
enum PowerBrickStatus {
    /**
     * Status not known.
     */
    UNKNOWN = 0,
    /**
     * Port partner is power brick.
     */
    CONNECTED = 1,
    /**
     * Port partner is not power brick.
     */
    NOT_CONNECTED = 2,
}
