/*
 * Copyright (C) 2022 The Android Open Source Project
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

package android.hardware.radio.ims;

import android.hardware.radio.RadioIndicationType;
import android.hardware.radio.ims.ConnectionFailureInfo;

/**
 * Interface declaring unsolicited radio indications for ims APIs.
 */
@VintfStability
oneway interface IRadioImsIndication {
    /**
     * Fired by radio when any IMS traffic is not sent to network due to any failure
     * on cellular networks.
     *
     * @param token The token number of the notifyImsTraffic() or performACBcheck() APIs
     * @param type Type of radio indication
     * @param info Connection failure information
     */
    void onConnectionSetupFailure(in RadioIndicationType type, int token,
            in ConnectionFailureInfo info);

    /**
     * Fired by radio in response to performAcbCheck(token, trafficType)
     * if the access class check is allowed for the requested traffic type.
     *
     * @param token The token of the operation of performAcbCheck() API
     * @param type Type of radio indication
     */
    void onAccessAllowed(in RadioIndicationType type, int token);
}
