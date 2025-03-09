/*
 * Copyright (C) 2024 The Android Open Source Project
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

package android.hardware.broadcastradio;

/**
 * The severity of the subject event of the emergency alert message.
 *
 * <p>(see ITU-T X.1303 bis for more info).
 */
@VintfStability
@Backing(type="int")
@JavaDerive(equals=true, toString=true)
enum AlertUrgency {
    /**
     * Urgency indicating that responsive action should be taken immediately.
     */
    IMMEDIATE,

    /**
     * Urgency indicating that responsive action should be taken soon.
     */
    EXPECTED,

    /**
     * Urgency indicating that responsive action should be taken in the near future.
     */
    FUTURE,

    /**
     * Urgency indicating that responsive action is no longer required.
     */
    PAST,

    /**
     * Unknown urgency.
     */
    UNKNOWN,
}
