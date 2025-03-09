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
 * The category of the subject event of the emergency alert message.
 *
 * <p>(see ITU-T X.1303 bis for more info).
 */
@VintfStability
@Backing(type="int")
@JavaDerive(equals=true, toString=true)
enum AlertCategory {
    /**
     * Alert category related to geophysical (inc. landslide).
     */
    GEO,

    /**
     * Alert category related to meteorological (inc. flood).
     */
    MET,

    /**
     * Alert category related to general emergency and public safety.
     */
    SAFETY,

    /**
     * Alert category related to law enforcement, military, homeland and local/private security.
     */
    SECURITY,

    /**
     * Alert category related to rescue and recovery.
     */
    RESCUE,

    /**
     * Alert category related to fire suppression and rescue.
     */
    FIRE,

    /**
     * Alert category related to medical and public health.
     */
    HEALTH,

    /**
     * Alert category related to pollution and other environmental.
     */
    ENV,

    /**
     * Alert category related to public and private transportation.
     */
    TRANSPORT,

    /**
     * Utility, telecommunication, other non-transport infrastructure.
     */
    INFRA,

    /**
     * Alert category related to chemical, biological, radiological, nuclear or high-yield
     * explosive threat or attack.
     */
    CBRNE,

    /**
     * Alert category related to other events.
     */
    OTHER,
}
