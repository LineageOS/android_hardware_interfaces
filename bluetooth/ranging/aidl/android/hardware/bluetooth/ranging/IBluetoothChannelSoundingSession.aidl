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

package android.hardware.bluetooth.ranging;

import android.hardware.bluetooth.ranging.ChannelSoudingRawData;
import android.hardware.bluetooth.ranging.Reason;
import android.hardware.bluetooth.ranging.ResultType;
import android.hardware.bluetooth.ranging.VendorSpecificData;

/**
 * Session of Channel Sounding get from IBluetoothChannelSounding.openSession().
 * Used by the Bluetooth stack to get preferred config from HAL and provide raw ranging data to
 * the HAL.
 */
@VintfStability
interface IBluetoothChannelSoundingSession {
    /**
     * API to get vendor-specifc replies
     *
     * @return an array of vendor-specifc data
     */
    @nullable VendorSpecificData[] getVendorSpecificReplies();

    /**
     * API to obtain supported result types. The Bluetooth stack should use this function to check
     * for supported result types and ignore unsupported types in the RangingResult.
     *
     * @return an array of vendor-specifc data
     */
    ResultType[] getSupportedResultTypes();

    /**
     * Indicate whether the HAL would like to receive raw data of abort procedures.
     * If this function returns true, the Bluetooth stack should pass the data to the HAL using
     * the writeRawData() function, even if the CS procedure is aborted.
     *
     * @return true if the HAL would like to receive raw data of abort procedures.
     */
    boolean isAbortedProcedureRequired();

    /**
     * API to provide raw ranging data to the HAL. The HAL converts this data into meaningful
     * ranging results using a proprietary algorithm and then calls back to the Bluetooth stack via
     * IBluetoothChannelSoundingSessionCallback.onResult().
     */
    void writeRawData(in ChannelSoudingRawData rawData);

    /**
     * Close the current session. Object is no longer useful after this method.
     */
    void close(Reason reason);
}
