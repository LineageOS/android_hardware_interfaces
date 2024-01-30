/*
 * Copyright 2023 The Android Open Source Project
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

import android.hardware.radio.network.CellularIdentifier;
import android.hardware.radio.network.NasProtocolMessage;

/**
 * A single occurrence of a cellular identifier being sent in the clear pre-authentication. See
 * IRadioNetwork.setCellularIdentifierTransparencyEnabled for more details.
 *
 * @hide
 */
@JavaDerive(toString=true)
@VintfStability
parcelable CellularIdentifierDisclosure {
    // The PLMN-ID to which the UE transmitted the cellular identifier
    String plmn;
    // The type of cellular identifier that was disclosed
    CellularIdentifier identifier;
    // The NAS protocol message within which the cellular identifier was transmitted.
    NasProtocolMessage protocolMessage;
    // Whether or not this cellular identifier disclosure is in service of an emergency call.
    boolean isEmergency;
}
