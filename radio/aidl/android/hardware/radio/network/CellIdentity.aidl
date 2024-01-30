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

import android.hardware.radio.network.CellIdentityCdma;
import android.hardware.radio.network.CellIdentityGsm;
import android.hardware.radio.network.CellIdentityLte;
import android.hardware.radio.network.CellIdentityNr;
import android.hardware.radio.network.CellIdentityTdscdma;
import android.hardware.radio.network.CellIdentityWcdma;

/**
 * A union representing the CellIdentity of a single cell.
 * @hide
 */
@VintfStability
@JavaDerive(toString=true)
union CellIdentity {
    boolean noinit;
    CellIdentityGsm gsm;
    CellIdentityWcdma wcdma;
    CellIdentityTdscdma tdscdma;
    CellIdentityCdma cdma;
    CellIdentityLte lte;
    CellIdentityNr nr;
}
