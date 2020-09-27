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

import android.hardware.gnss.PsdsType;

/**
 * This interface is used by the GNSS HAL to request download data from Predicted Satellite Data
 * Service (PSDS).
 */
@VintfStability
interface IGnssPsdsCallback {

    /**
     * Callback to request the client to download PSDS data from one of the URLs defined in the
     * framework specified by psdsType. The URLs must be specified via properties on the vendor
     * partitions. E.g., LONGTERM_PSDS_SERVER_1, NORMAL_PSDS_SERVER, or REALTIME_PSDS_SERVER. The
     * client must download PSDS data and inject it by calling injectPsdsData().
     *
     * @param psdsType Type of PSDS data.
     */
    void downloadRequestCb(in PsdsType psdsType);
}