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

package android.hardware.radio.network;

import android.hardware.radio.network.CellInfoCdma;
import android.hardware.radio.network.CellInfoGsm;
import android.hardware.radio.network.CellInfoLte;
import android.hardware.radio.network.CellInfoNr;
import android.hardware.radio.network.CellInfoTdscdma;
import android.hardware.radio.network.CellInfoWcdma;

/** @hide */
@VintfStability
@JavaDerive(toString=true)
union CellInfoRatSpecificInfo {
    /**
     * 3gpp CellInfo types.
     */
    CellInfoGsm gsm;
    CellInfoWcdma wcdma;
    CellInfoTdscdma tdscdma;
    CellInfoLte lte;
    CellInfoNr nr;
    /**
     * 3gpp2 CellInfo types;
     */
    CellInfoCdma cdma;
}
