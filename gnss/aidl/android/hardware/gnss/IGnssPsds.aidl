/*
 * Copyright (C) 2020 The Android Open Source Project
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

package android.hardware.gnss;

import android.hardware.gnss.IGnssPsdsCallback;
import android.hardware.gnss.PsdsType;

/**
 * This interface is used by the GNSS HAL to request the framework to download Predicted Satellite
 * Data Service data.
 */
@VintfStability
interface IGnssPsds {

    /**
     * Inject the downloaded PSDS data into the GNSS receiver.
     *
     * @param psdsType Type of PSDS data.
     * @param psdsData GNSS PSDS data. Framework must not parse the data since the data format is
     *                 opaque to framework.
     */
    void injectPsdsData(in PsdsType psdsType, in byte[] psdsData);

    /**
     * Opens the PSDS interface and provides the callback routines to the implementation of this
     * interface.
     *
     * @param callback Handle to the IGnssPsdsCallback interface.
     */
    void setCallback(in IGnssPsdsCallback callback);
}