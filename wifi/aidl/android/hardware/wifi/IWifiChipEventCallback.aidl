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

package android.hardware.wifi;

import android.hardware.wifi.IfaceType;
import android.hardware.wifi.WifiBand;
import android.hardware.wifi.WifiDebugRingBufferStatus;
import android.hardware.wifi.WifiStatusCode;

/**
 * Wifi chip event callbacks.
 */
@VintfStability
oneway interface IWifiChipEventCallback {
    /**
     * Struct describing the state of each iface operating on the radio chain
     * (hardware MAC) on the device.
     */
    @VintfStability
    parcelable IfaceInfo {
        /**
         * Name of the interface (ex. wlan0).
         */
        String name;
        /**
         * Wifi channel on which this interface is operating.
         */
        int channel;
    }

    /**
     * Struct describing the state of each hardware radio chain (hardware MAC)
     * on the device.
     */
    @VintfStability
    parcelable RadioModeInfo {
        /**
         * Identifier for this radio chain. This is vendor dependent and used
         * only for debugging purposes.
         */
        int radioId;
        /**
         * List of bands on which this radio chain is operating.
         * Can be one of:
         * a) |WifiBand.BAND_24GHZ| => 2.4Ghz.
         * b) |WifiBand.BAND_5GHZ| => 5Ghz.
         * c) |WifiBand.BAND_24GHZ_5GHZ| => 2.4Ghz + 5Ghz (Radio is time sharing
         * across the 2 bands).
         * d) |WifiBand.BAND_6GHZ| => 6Ghz.
         * e) |WifiBand.BAND_5GHZ_6GHZ| => 5Ghz + 6Ghz (Radio is time sharing
         * across the 2 bands).
         * f) |WifiBand.BAND_24GHZ_5GHZ_6GHZ| => 2.4Ghz + 5Ghz + 6Ghz (Radio is
         * time sharing across the 3 bands).
         */
        WifiBand bandInfo;
        /**
         * List of interfaces on this radio chain (hardware MAC).
         */
        IfaceInfo[] ifaceInfos;
    }

    /**
     * Callback indicating that a chip reconfiguration failed. This is a fatal
     * error and any iface objects available previously must be considered
     * invalid. The client can attempt to recover by trying to reconfigure the
     * chip again using |IWifiChip.configureChip|.
     *
     * @param status Failure reason code.
     */
    void onChipReconfigureFailure(in WifiStatusCode status);

    /**
     * Callback indicating that the chip has been reconfigured successfully. At
     * this point the interfaces available in the mode must be able to be
     * configured. When this is called, any previous iface objects must be
     * considered invalid.
     *
     * @param modeId The mode that the chip switched to, corresponding to the id
     *        property of the target ChipMode.
     */
    void onChipReconfigured(in int modeId);

    /**
     * Callback indicating that the chip has encountered a fatal error.
     * Client must not attempt to parse either the errorCode or debugData.
     * Must only be captured in a bugreport.
     *
     * @param errorCode Vendor defined error code.
     * @param debugData Vendor defined data used for debugging.
     */
    void onDebugErrorAlert(in int errorCode, in byte[] debugData);

    /**
     * Callbacks for reporting debug ring buffer data.
     *
     * The ring buffer data collection is event based:
     * - Driver calls this callback when new records are available, the
     *   |WifiDebugRingBufferStatus| passed up to framework in the callback
     *   indicates to framework if more data is available in the ring buffer.
     *   It is not expected that driver will necessarily always empty the ring
     *   immediately as data is available. Instead the driver will report data
     *   every X seconds, or if N bytes are available, based on the parameters
     *   set via |startLoggingToDebugRingBuffer|.
     * - In the case where a bug report has to be captured, the framework will
     *   require driver to upload all data immediately. This is indicated to
     *   driver when framework calls |forceDumpToDebugRingBuffer|. The driver
     *   will start sending all available data in the indicated ring by repeatedly
     *   invoking this callback.
     *
     * @param status Status of the corresponding ring buffer. This should
     *         contain the name of the ring buffer on which the data is
     *         available.
     * @param data Raw bytes of data sent by the driver. Must be dumped
     *         out to a bugreport and post processed.
     */
    void onDebugRingBufferDataAvailable(in WifiDebugRingBufferStatus status, in byte[] data);

    /**
     * Callback indicating that a new iface has been added to the chip.
     *
     * @param type Type of iface added.
     * @param name Name of iface added.
     */
    void onIfaceAdded(in IfaceType type, in String name);

    /**
     * Callback indicating that an existing iface has been removed from the chip.
     *
     * @param type Type of iface removed.
     * @param name Name of iface removed.
     */
    void onIfaceRemoved(in IfaceType type, in String name);

    /**
     * Indicates a radio mode change.
     * Radio mode change could be a result of:
     * a) Bringing up concurrent interfaces (ex. STA + AP).
     * b) Change in operating band of one of the concurrent interfaces
     * ( ex. STA connection moved from 2.4G to 5G)
     *
     * @param radioModeInfos List of RadioModeInfo structures for each
     *        radio chain (hardware MAC) on the device.
     */
    void onRadioModeChange(in RadioModeInfo[] radioModeInfos);
}
