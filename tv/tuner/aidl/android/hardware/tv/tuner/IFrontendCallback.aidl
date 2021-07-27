/*
 * Copyright 2021 The Android Open Source Project
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

package android.hardware.tv.tuner;

import android.hardware.tv.tuner.FrontendEventType;
import android.hardware.tv.tuner.FrontendScanMessage;
import android.hardware.tv.tuner.FrontendScanMessageType;

/**
 * This interface is used by the HAL to notify the fronted event and scan messages
 * back to the client, the cient implements the interfaces and passes a handle to
 * the HAL.
 * @hide
 */
@VintfStability
oneway interface IFrontendCallback {
    /**
     * Notify the client that a new event happened on the frontend.
     *
     * @param frontendEventType the event type.
     */
    void onEvent(in FrontendEventType frontendEventType);

    /**
     * The callback function that must be called by HAL implementation to notify
     * the client of scan messages.
     *
     * @param type the type of scan message.
     * @param message the scan message sent by HAL to the client.
     */
    void onScanMessage(in FrontendScanMessageType type,
        in FrontendScanMessage message);
}
