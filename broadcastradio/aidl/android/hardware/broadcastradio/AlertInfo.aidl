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

import android.hardware.broadcastradio.AlertArea;
import android.hardware.broadcastradio.AlertCategory;
import android.hardware.broadcastradio.AlertCertainty;
import android.hardware.broadcastradio.AlertSeverity;
import android.hardware.broadcastradio.AlertUrgency;

/**
 * Alert information.
 *
 * <p>(see ITU-T X.1303 bis for more info).
 */
@VintfStability
@JavaDerive(equals=true, toString=true)
parcelable AlertInfo {
    /**
     * Array of categories of the subject event of the alert info.
     *
     * <p>According to ITU-T X.1303, a single alert info block may contains multiple categories.
     */
    AlertCategory[] categoryArray;

    /**
     * The urgency of the subject event of the alert info.
     *
     * <p>Urgency represents the time available to prepare for the alert.
     */
    AlertUrgency urgency;

    /**
     * The severity of the subject event of the alert info.
     *
     * <p>Severity represents the intensity of impact.
     */
    AlertSeverity severity;

    /**
     * The certainty of the subject event of the alert info.
     *
     * <p>Certainty represents confidence in the observation or prediction.
     */
    AlertCertainty certainty;

    /**
     * Textual descriptions of the subject event.
     */
    String description;

    /**
     * The array of geographic areas to which the alert info segment in which it appears applies.
     */
    AlertArea[] areas;

    /**
     * The IETF RFC 3066 language code donating the language of the alert message.
     *
     * <p>This field is optional.
     */
    @nullable String language;
}
