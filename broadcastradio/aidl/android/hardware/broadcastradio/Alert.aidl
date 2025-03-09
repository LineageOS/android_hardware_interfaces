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

import android.hardware.broadcastradio.AlertInfo;
import android.hardware.broadcastradio.AlertMessageType;
import android.hardware.broadcastradio.AlertStatus;

/**
 * Emergency Alert Message.
 *
 * <p>Alert message can be sent from a radio station of technologies such as HD radio to
 * the radio users for some emergency events (see ITU-T X.1303 bis for more info).
 */
@VintfStability
@JavaDerive(equals=true, toString=true)
parcelable Alert {
    /**
     * The status of the alert message.
     */
    AlertStatus status;

    /**
     * The message type of the alert message.
     */
    AlertMessageType messageType;

    /**
     * Array of alert information.
     */
    AlertInfo[] infoArray;
}
