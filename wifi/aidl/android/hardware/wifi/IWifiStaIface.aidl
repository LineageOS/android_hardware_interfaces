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

import android.hardware.wifi.CachedScanData;
import android.hardware.wifi.IWifiStaIfaceEventCallback;
import android.hardware.wifi.StaApfPacketFilterCapabilities;
import android.hardware.wifi.StaBackgroundScanCapabilities;
import android.hardware.wifi.StaBackgroundScanParameters;
import android.hardware.wifi.StaLinkLayerStats;
import android.hardware.wifi.StaRoamingCapabilities;
import android.hardware.wifi.StaRoamingConfig;
import android.hardware.wifi.StaRoamingState;
import android.hardware.wifi.TwtCapabilities;
import android.hardware.wifi.TwtRequest;
import android.hardware.wifi.WifiBand;
import android.hardware.wifi.WifiDebugRxPacketFateReport;
import android.hardware.wifi.WifiDebugTxPacketFateReport;

/**
 * Interface used to represent a single STA iface.
 */
@VintfStability
interface IWifiStaIface {
    /**
     * Mask of capabilities supported by this iface.
     */
    @VintfStability
    @Backing(type="int")
    enum FeatureSetMask {
        /**
         * Support for APF APIs. APF (Android Packet Filter) is a
         * BPF-like packet filtering bytecode executed by the firmware.
         */
        APF = 1 << 0,
        /**
         * Support for Background Scan APIs. Background scan allows the host
         * to send a number of buckets down to the firmware. Each bucket
         * contains a set of channels, a period, and some parameters about
         * how and when to report results.
         */
        BACKGROUND_SCAN = 1 << 1,
        /**
         * Support for link layer stats APIs.
         */
        LINK_LAYER_STATS = 1 << 2,
        /**
         * Support for RSSI monitor APIs.
         */
        RSSI_MONITOR = 1 << 3,
        /**
         * Support for roaming APIs.
         */
        CONTROL_ROAMING = 1 << 4,
        /**
         * Support for Probe IE allow-listing.
         */
        PROBE_IE_ALLOWLIST = 1 << 5,
        /**
         * Support for MAC & Probe Sequence Number randomization.
         */
        SCAN_RAND = 1 << 6,
        /**
         * Support for 5 GHz Band.
         */
        STA_5G = 1 << 7,
        /**
         * Support for GAS/ANQP queries.
         */
        HOTSPOT = 1 << 8,
        /**
         * Support for Preferred Network Offload.
         */
        PNO = 1 << 9,
        /**
         * Support for Tunneled Direct Link Setup.
         */
        TDLS = 1 << 10,
        /**
         * Support for Tunneled Direct Link Setup off channel.
         */
        TDLS_OFFCHANNEL = 1 << 11,
        /**
         * Support for neighbour discovery offload.
         */
        ND_OFFLOAD = 1 << 12,
        /**
         * Support for keep alive packet offload.
         */
        KEEP_ALIVE = 1 << 13,
        /**
         * Support for configuring roaming mode.
         */
        ROAMING_MODE_CONTROL = 1 << 14,
        /**
         * Support for cached scan data report.
         */
        CACHED_SCAN_DATA = 1 << 15,
    }

    /**
     * Get the name of this iface.
     *
     * @return Name of this iface.
     * @throws ServiceSpecificException with one of the following values:
     *         |WifiStatusCode.ERROR_WIFI_IFACE_INVALID|
     */
    String getName();

    /**
     * Configure roaming control parameters.
     * Must fail if |StaIfaceCapabilityMask.CONTROL_ROAMING| is not set.
     *
     * @param config Instance of |StaRoamingConfig|.
     * @throws ServiceSpecificException with one of the following values:
     *         |WifiStatusCode.ERROR_WIFI_IFACE_INVALID|,
     *         |WifiStatusCode.ERROR_NOT_SUPPORTED|,
     *         |WifiStatusCode.ERROR_UNKNOWN|
     */
    void configureRoaming(in StaRoamingConfig config);

    /**
     * Disable link layer stats collection.
     * Must fail if |StaIfaceCapabilityMask.LINK_LAYER_STATS| is not set.
     *
     * @throws ServiceSpecificException with one of the following values:
     *         |WifiStatusCode.ERROR_WIFI_IFACE_INVALID|,
     *         |WifiStatusCode.ERROR_NOT_SUPPORTED|,
     *         |WifiStatusCode.ERROR_NOT_STARTED|,
     *         |WifiStatusCode.ERROR_NOT_AVAILABLE|,
     *         |WifiStatusCode.ERROR_UNKNOWN|
     */
    void disableLinkLayerStatsCollection();

    /**
     * Enable link layer stats collection.
     * Must fail if |StaIfaceCapabilityMask.LINK_LAYER_STATS| is not set.
     *
     * Radio statistics (once started) must not stop until disabled.
     * Iface statistics (once started) reset and start afresh after each
     * connection until disabled.
     *
     * @param debug true to enable field debug mode, false to disable. Driver
     *        must collect all statistics regardless of performance impact.
     * @throws ServiceSpecificException with one of the following values:
     *         |WifiStatusCode.ERROR_WIFI_IFACE_INVALID|,
     *         |WifiStatusCode.ERROR_NOT_SUPPORTED|,
     *         |WifiStatusCode.ERROR_NOT_AVAILABLE|,
     *         |WifiStatusCode.ERROR_UNKNOWN|
     */
    void enableLinkLayerStatsCollection(in boolean debug);

    /**
     * Enable/Disable neighbour discovery offload functionality in the firmware.
     *
     * @param enable true to enable, false to disable.
     * @throws ServiceSpecificException with one of the following values:
     *         |WifiStatusCode.ERROR_WIFI_IFACE_INVALID|,
     *         |WifiStatusCode.ERROR_UNKNOWN|
     */
    void enableNdOffload(in boolean enable);

    /**
     * Used to query additional information about the chip's APF capabilities.
     * Must fail if |StaIfaceCapabilityMask.APF| is not set.
     *
     * @return Instance of |StaApfPacketFilterCapabilities|.
     * @throws ServiceSpecificException with one of the following values:
     *         |WifiStatusCode.ERROR_WIFI_IFACE_INVALID|,
     *         |WifiStatusCode.ERROR_NOT_SUPPORTED|,
     *         |WifiStatusCode.ERROR_NOT_AVAILABLE|,
     *         |WifiStatusCode.ERROR_UNKNOWN|
     */
    StaApfPacketFilterCapabilities getApfPacketFilterCapabilities();

    /**
     * Used to query additional information about the chip's Background Scan capabilities.
     * Must fail if |StaIfaceCapabilityMask.BACKGROUND_SCAN| is not set.
     *
     * @return Instance of |StaBackgroundScanCapabilities|
     * @throws ServiceSpecificException with one of the following values:
     *         |WifiStatusCode.ERROR_WIFI_IFACE_INVALID|,
     *         |WifiStatusCode.ERROR_NOT_SUPPORTED|,
     *         |WifiStatusCode.ERROR_NOT_AVAILABLE|,
     *         |WifiStatusCode.ERROR_UNKNOWN|
     */
    StaBackgroundScanCapabilities getBackgroundScanCapabilities();

    /**
     * Get the features supported by this STA iface.
     *
     * @return Bitset of |FeatureSetMask| values.
     * @throws ServiceSpecificException with one of the following values:
     *         |WifiStatusCode.ERROR_WIFI_IFACE_INVALID|,
     *         |WifiStatusCode.ERROR_NOT_AVAILABLE|,
     *         |WifiStatusCode.ERROR_UNKNOWN|
     */
    int getFeatureSet();

    /**
     * API to retrieve the fates of inbound packets.
     * - HAL implementation must return the fates of
     *   all the frames received for the most recent association.
     *   The fate reports must follow the same order as their respective
     *   packets.
     * - HAL implementation may choose (but is not required) to include
     *   reports for management frames.
     * - Packets reported by firmware, but not recognized by driver,
     *   must be included. However, the ordering of the corresponding
     *   reports is at the discretion of HAL implementation.
     * - Framework must be able to call this API multiple times for the same
     *   association.
     *
     * @return Vector of |WifiDebugRxPacketFateReport| instances corresponding
     *         to the packet fates.
     * @throws ServiceSpecificException with one of the following values:
     *         |WifiStatusCode.ERROR_WIFI_IFACE_INVALID|,
     *         |WifiStatusCode.ERROR_NOT_SUPPORTED|,
     *         |WifiStatusCode.ERROR_NOT_STARTED|,
     *         |WifiStatusCode.ERROR_NOT_AVAILABLE|,
     *         |WifiStatusCode.ERROR_UNKNOWN|
     */
    WifiDebugRxPacketFateReport[] getDebugRxPacketFates();

    /**
     * API to retrieve fates of outbound packets.
     * - HAL implementation must return the fates of
     *   all the frames transmitted for the most recent association.
     *   The fate reports must follow the same order as their respective
     *   packets.
     * - HAL implementation may choose (but is not required) to include
     *   reports for management frames.
     * - Packets reported by firmware, but not recognized by driver,
     *   must be included. However, the ordering of the corresponding
     *   reports is at the discretion of HAL implementation.
     * - Framework must be able to call this API multiple times for the same
     *   association.
     *
     * @return Vector of |WifiDebugTxPacketFateReport| instances corresponding
     *         to the packet fates.
     * @throws ServiceSpecificException with one of the following values:
     *         |WifiStatusCode.ERROR_WIFI_IFACE_INVALID|,
     *         |WifiStatusCode.ERROR_NOT_SUPPORTED|,
     *         |WifiStatusCode.ERROR_NOT_STARTED|,
     *         |WifiStatusCode.ERROR_NOT_AVAILABLE|,
     *         |WifiStatusCode.ERROR_UNKNOWN|
     */
    WifiDebugTxPacketFateReport[] getDebugTxPacketFates();

    /**
     * Gets the factory MAC address of the STA interface.
     *
     * @return Factory MAC address of the STA interface.
     * @throws ServiceSpecificException with one of the following values:
     *         |WifiStatusCode.ERROR_WIFI_IFACE_INVALID|,
     *         |WifiStatusCode.ERROR_UNKNOWN|
     */
    byte[6] getFactoryMacAddress();

    /**
     * Retrieve the latest link layer stats.
     * Must fail if |StaIfaceCapabilityMask.LINK_LAYER_STATS| is not set or if
     * link layer stats collection hasn't been explicitly enabled.
     *
     * @return Instance of |LinkLayerStats|.
     * @throws ServiceSpecificException with one of the following values:
     *     |WifiStatusCode.ERROR_WIFI_IFACE_INVALID|,
     *     |WifiStatusCode.ERROR_NOT_SUPPORTED|,
     *     |WifiStatusCode.ERROR_NOT_STARTED|,
     *     |WifiStatusCode.ERROR_NOT_AVAILABLE|,
     *     |WifiStatusCode.ERROR_UNKNOWN|
     */
    StaLinkLayerStats getLinkLayerStats();

    /**
     * Get roaming control capabilities.
     * Must fail if |StaIfaceCapabilityMask.CONTROL_ROAMING| is not set.
     *
     * @return Instance of |StaRoamingCapabilities|.
     * @throws ServiceSpecificException with one of the following values:
     *         |WifiStatusCode.ERROR_WIFI_IFACE_INVALID|,
     *         |WifiStatusCode.ERROR_NOT_SUPPORTED|,
     *         |WifiStatusCode.ERROR_UNKNOWN|
     */
    StaRoamingCapabilities getRoamingCapabilities();

    /**
     * Installs an APF program on this iface, replacing an existing
     * program if present.
     * Must fail if |StaIfaceCapabilityMask.APF| is not set.
     *
     * APF docs
     * ==========================================================================
     * APF functionality, instructions and bytecode/binary format is described in:
     * http://android.googlesource.com/platform/hardware/google/apf/
     * +/b75c9f3714cfae3dad3d976958e063150781437e/apf.h
     *
     * The interpreter API is described here:
     * http://android.googlesource.com/platform/hardware/google/apf/+/
     * b75c9f3714cfae3dad3d976958e063150781437e/apf_interpreter.h#32
     *
     * The assembler/generator API is described in javadocs here:
     * http://android.googlesource.com/platform/frameworks/base/+/
     * 4456f33a958a7f09e608399da83c4d12b2e7d191/services/net/java/android/net/
     * apf/ApfGenerator.java
     *
     * Disassembler usage is described here:
     * http://android.googlesource.com/platform/hardware/google/apf/+/
     * b75c9f3714cfae3dad3d976958e063150781437e/apf_disassembler.c#65
     *
     * The BPF to APF translator usage is described here:
     * http://android.googlesource.com/platform/frameworks/base/+/
     * 4456f33a958a7f09e608399da83c4d12b2e7d191/tests/net/java/android/net/
     * apf/Bpf2Apf.java
     * ==========================================================================
     *
     * @param program APF Program to be set.
     * @throws ServiceSpecificException with one of the following values:
     *         |WifiStatusCode.ERROR_WIFI_IFACE_INVALID|,
     *         |WifiStatusCode.ERROR_INVALID_ARGS|,
     *         |WifiStatusCode.ERROR_NOT_SUPPORTED|,
     *         |WifiStatusCode.ERROR_NOT_AVAILABLE|,
     *         |WifiStatusCode.ERROR_UNKNOWN|
     */
    void installApfPacketFilter(in byte[] program);

    /**
     * Fetches a consistent snapshot of the entire APF program and working
     * memory buffer and returns it to the host. The returned buffer contains
     * both code and data. Its length must match the most recently returned
     * |StaApfPacketFilterCapabilities.maxLength|.
     *
     * While the snapshot is being fetched, the APF interpreter must not execute
     * and all incoming packets must be passed to the host as if there was no
     * APF program installed.
     *
     * Must fail with |WifiStatusCode.ERROR_NOT_SUPPORTED| if
     * |StaIfaceCapabilityMask.APF| is not set.
     *
     * @return The entire APF working memory buffer when status is
     *         |WifiStatusCode.SUCCESS|, empty otherwise.
     * @throws ServiceSpecificException with one of the following values:
     *         |WifiStatusCode.ERROR_WIFI_IFACE_INVALID|,
     *         |WifiStatusCode.ERROR_NOT_SUPPORTED|,
     *         |WifiStatusCode.ERROR_NOT_AVAILABLE|,
     *         |WifiStatusCode.ERROR_UNKNOWN|
     * @see getApfPacketFilterCapabilities()
     * @see installApfPacketFilter()
     */
    byte[] readApfPacketFilterData();

    /**
     * Requests notifications of significant events on this iface. Multiple calls
     * to this must register multiple callbacks, each of which must receive all
     * events.
     *
     * @param callback An instance of the |IWifiStaIfaceEventCallback| AIDL
     *        interface object.
     * @throws ServiceSpecificException with one of the following values:
     *         |WifiStatusCode.ERROR_WIFI_IFACE_INVALID|
     */
    void registerEventCallback(in IWifiStaIfaceEventCallback callback);

    /**
     * Changes the MAC address of the STA Interface to the given
     * MAC address.
     *
     * @param mac MAC address to change to.
     * @throws ServiceSpecificException with one of the following values:
     *         |WifiStatusCode.ERROR_WIFI_IFACE_INVALID|,
     *         |WifiStatusCode.ERROR_UNKNOWN|
     */
    void setMacAddress(in byte[6] mac);

    /**
     * Set the roaming control state with the parameters configured
     * using |configureRoaming|. Depending on the roaming state set, the
     * driver/firmware would enable/disable control over roaming decisions.
     * Must fail if |StaIfaceCapabilityMask.CONTROL_ROAMING| is not set.
     *
     * @param state State of the roaming control.
     * @throws ServiceSpecificException with one of the following values:
     *         |WifiStatusCode.ERROR_WIFI_IFACE_INVALID|,
     *         |WifiStatusCode.ERROR_NOT_SUPPORTED|,
     *         |WifiStatusCode.ERROR_BUSY|,
     *         |WifiStatusCode.ERROR_UNKNOWN|
     */
    void setRoamingState(in StaRoamingState state);

    /**
     * Turn on/off scan only mode for the interface.
     *
     * @param enable True to enable scan only mode, false to disable.
     * @throws ServiceSpecificException with one of the following values:
     *         |WifiStatusCode.ERROR_NOT_SUPPORTED|,
     *         |WifiStatusCode.ERROR_WIFI_IFACE_INVALID|,
     *         |WifiStatusCode.FAILURE_UNKNOWN|
     */
    void setScanMode(in boolean enable);

    /**
     * Start a background scan using the given cmdId as an identifier. Only one
     * active background scan need be supported.
     * Must fail if |StaIfaceCapabilityMask.BACKGROUND_SCAN| is not set.
     *
     * When this is called all requested buckets must be scanned, starting the
     * beginning of the cycle.
     *
     * For example:
     * If there are two buckets specified
     *  - Bucket 1: period=10s
     *  - Bucket 2: period=20s
     *  - Bucket 3: period=30s
     * Then the following scans must occur
     *  - t=0  buckets 1, 2, and 3 are scanned
     *  - t=10 bucket 1 is scanned
     *  - t=20 bucket 1 and 2 are scanned
     *  - t=30 bucket 1 and 3 are scanned
     *  - t=40 bucket 1 and 2 are scanned
     *  - t=50 bucket 1 is scanned
     *  - t=60 buckets 1, 2, and 3 are scanned
     *  - and the pattern repeats
     *
     * If any scan does not occur or is incomplete (error, interrupted, etc),
     * then a cached scan result must still be recorded with the
     * WIFI_SCAN_FLAG_INTERRUPTED flag set.
     *
     * @param cmdId Command Id to use for this invocation.
     * @param params Background scan parameters.
     * @throws ServiceSpecificException with one of the following values:
     *         |WifiStatusCode.ERROR_WIFI_IFACE_INVALID|,
     *         |WifiStatusCode.ERROR_INVALID_ARGS|,
     *         |WifiStatusCode.ERROR_NOT_SUPPORTED|,
     *         |WifiStatusCode.ERROR_NOT_AVAILABLE|,
     *         |WifiStatusCode.ERROR_UNKNOWN|
     */
    void startBackgroundScan(in int cmdId, in StaBackgroundScanParameters params);

    /**
     * API to start packet fate monitoring.
     * - Once started, monitoring must remain active until HAL is stopped or the
     *   chip is reconfigured.
     * - When HAL is unloaded, all packet fate buffers must be cleared.
     * - The packet fates are used to monitor the state of packets transmitted/
     *   received during association.
     *
     * @throws ServiceSpecificException with one of the following values:
     *         |WifiStatusCode.ERROR_WIFI_IFACE_INVALID|,
     *         |WifiStatusCode.ERROR_NOT_SUPPORTED|,
     *         |WifiStatusCode.ERROR_NOT_AVAILABLE|,
     *         |WifiStatusCode.ERROR_UNKNOWN|
     */
    void startDebugPacketFateMonitoring();

    /**
     * Start RSSI monitoring on the currently connected access point.
     * Once the monitoring is enabled, the
     * |IWifiStaIfaceEventCallback.onRssiThresholdBreached| callback must be
     * invoked to indicate if the RSSI goes above |maxRssi| or below |minRssi|.
     * Must fail if |StaIfaceCapabilityMask.RSSI_MONITOR| is not set.
     *
     * @param cmdId Command Id to use for this invocation.
     * @param maxRssi Maximum RSSI threshold.
     * @param minRssi Minimum RSSI threshold.
     * @throws ServiceSpecificException with one of the following values:
     *         |WifiStatusCode.ERROR_WIFI_IFACE_INVALID|,
     *         |WifiStatusCode.ERROR_ARGS_INVALID|,
     *         |WifiStatusCode.ERROR_NOT_SUPPORTED|,
     *         |WifiStatusCode.ERROR_NOT_AVAILABLE|,
     *         |WifiStatusCode.ERROR_UNKNOWN|
     */
    void startRssiMonitoring(in int cmdId, in int maxRssi, in int minRssi);

    /**
     * Start sending the specified keep alive packets periodically.
     *
     * @param cmdId Command Id to use for this invocation.
     * @param ipPacketData IP packet contents to be transmitted.
     * @param etherType 16 bit ether type to be set in the ethernet frame
     *        transmitted.
     * @param srcAddress Source MAC address of the packet.
     * @param dstAddress Destination MAC address of the packet.
     * @param periodInMs Interval at which this packet must be transmitted.
     * @throws ServiceSpecificException with one of the following values:
     *         |WifiStatusCode.ERROR_WIFI_IFACE_INVALID|,
     *         |WifiStatusCode.ERROR_NOT_SUPPORTED|,
     *         |WifiStatusCode.ERROR_NOT_AVAILABLE|,
     *         |WifiStatusCode.ERROR_UNKNOWN|
     */
    void startSendingKeepAlivePackets(in int cmdId, in byte[] ipPacketData, in char etherType,
            in byte[6] srcAddress, in byte[6] dstAddress, in int periodInMs);

    /**
     * Stop the current background scan.
     * Must fail if |StaIfaceCapabilityMask.BACKGROUND_SCAN| is not set.
     *
     * @param cmdId Command Id corresponding to the request.
     * @throws ServiceSpecificException with one of the following values:
     *         |WifiStatusCode.ERROR_WIFI_IFACE_INVALID|,
     *         |WifiStatusCode.ERROR_NOT_SUPPORTED|,
     *         |WifiStatusCode.ERROR_NOT_STARTED|,
     *         |WifiStatusCode.ERROR_NOT_AVAILABLE|,
     *         |WifiStatusCode.ERROR_UNKNOWN|
     */
    void stopBackgroundScan(in int cmdId);

    /**
     * Stop RSSI monitoring.
     * Must fail if |StaIfaceCapabilityMask.RSSI_MONITOR| is not set.
     *
     * @param cmdId Command Id corresponding to the request.
     * @throws ServiceSpecificException with one of the following values:
     *         |WifiStatusCode.ERROR_WIFI_IFACE_INVALID|,
     *         |WifiStatusCode.ERROR_NOT_SUPPORTED|,
     *         |WifiStatusCode.ERROR_NOT_STARTED|,
     *         |WifiStatusCode.ERROR_NOT_AVAILABLE|,
     *         |WifiStatusCode.ERROR_UNKNOWN|
     */
    void stopRssiMonitoring(in int cmdId);

    /**
     * Stop sending the specified keep alive packets.
     *
     * @param cmdId Command Id corresponding to the request.
     * @throws ServiceSpecificException with one of the following values:
     *         |WifiStatusCode.ERROR_WIFI_IFACE_INVALID|,
     *         |WifiStatusCode.ERROR_NOT_SUPPORTED|,
     *         |WifiStatusCode.ERROR_NOT_AVAILABLE|,
     *         |WifiStatusCode.ERROR_UNKNOWN|
     */
    void stopSendingKeepAlivePackets(in int cmdId);

    /**
     * Set maximum acceptable DTIM multiplier to hardware driver.
     * Any multiplier larger than this maximum value must not be accepted since it will cause
     * packet loss higher than what the system can accept, which will cause unexpected behavior
     * for apps, and may interrupt the network connection.
     *
     * When STA is in the power saving mode and system is suspended,
     * the wake up interval will be set to:
     *              1) multiplier * DTIM period if multiplier > 0.
     *              2) the driver default value if multiplier <= 0.
     * Some implementations may apply an additional cap to wake up interval in the case of 1).
     *
     * @param multiplier integer maximum DTIM multiplier value to set.
     * @throws ServiceSpecificException with one of the following values:
     *         |WifiStatusCode.ERROR_WIFI_IFACE_INVALID|,
     *         |WifiStatusCode.ERROR_NOT_SUPPORTED|,
     *         |WifiStatusCode.ERROR_UNKNOWN|
     */
    void setDtimMultiplier(in int multiplier);

    /**
     * Get the cached scan data.
     *
     * @return Instance of |CachedScanData|.
     * @throws ServiceSpecificException with one of the following values:
     *         |WifiStatusCode.ERROR_WIFI_IFACE_INVALID|,
     *         |WifiStatusCode.ERROR_NOT_SUPPORTED|,
     *         |WifiStatusCode.ERROR_NOT_AVAILABLE|,
     *         |WifiStatusCode.ERROR_UNKNOWN|
     */
    CachedScanData getCachedScanData();

    /**
     * Get Target Wake Time (TWT) local device capabilities for the station interface.
     *
     * @return Instance of |TwtCapabilities|.
     * @throws ServiceSpecificException with one of the following values:
     *         |WifiStatusCode.ERROR_WIFI_IFACE_INVALID|,
     *         |WifiStatusCode.ERROR_NOT_SUPPORTED|,
     *         |WifiStatusCode.ERROR_NOT_AVAILABLE|,
     *         |WifiStatusCode.ERROR_UNKNOWN|
     */
    TwtCapabilities twtGetCapabilities();

    /**
     * Setup a Target Wake Time (TWT) session.
     *
     * Supported only if |TwtCapabilities.isTwtRequesterSupported| is set. Results in asynchronous
     * callback |IWifiStaIfaceEventCallback.onTwtSessionCreate| on success or
     * |IWifiStaIfaceEventCallback.onTwtFailure| on failure.
     *
     * @param cmdId Command Id to use for this invocation. The value 0 is reserved.
     * @param twtRequest TWT Request parameters.
     *  @throws ServiceSpecificException with one of the following values:
     *         |WifiStatusCode.ERROR_WIFI_IFACE_INVALID|,
     *         |WifiStatusCode.ERROR_NOT_SUPPORTED|,
     *         |WifiStatusCode.ERROR_UNKNOWN|
     */
    void twtSessionSetup(in int cmdId, in TwtRequest twtRequest);

    /**
     * Update a Target Wake Time (TWT) session.
     *
     * Supported only if the TWT session can be updated. See |TwtSession.isUpdatable|. Results in
     * asynchronous callback |IWifiStaIfaceEventCallback.onTwtSessionUpdate| on success or
     * |IWifiStaIfaceEventCallback.onTwtFailure| on failure.
     *
     * @param cmdId Command Id to use for this invocation. The value 0 is reserved.
     * @param sessionId TWT session id.
     * @param twtRequest TWT Request parameters.
     * @throws ServiceSpecificException with one of the following values:
     *         |WifiStatusCode.ERROR_WIFI_IFACE_INVALID|,
     *         |WifiStatusCode.ERROR_NOT_SUPPORTED|,
     *         |WifiStatusCode.ERROR_UNKNOWN|
     */
    void twtSessionUpdate(in int cmdId, in int sessionId, in TwtRequest twtRequest);

    /**
     * Suspend a Target Wake Time (TWT) session until a resume is called.
     *
     * Supported only if the TWT session supports suspend and resume. See
     * |TwtSession.isSuspendable|. Results in asynchronous callback
     * |IWifiStaIfaceEventCallback.onTwtSessionSuspend| on success or
     * |IWifiStaIfaceEventCallback.onTwtFailure| on failure.
     *
     * @param cmdId Command Id to use for this invocation. The value 0 is reserved.
     * @param sessionId TWT session id.
     * @throws ServiceSpecificException with one of the following values:
     *         |WifiStatusCode.ERROR_WIFI_IFACE_INVALID|,
     *         |WifiStatusCode.ERROR_NOT_SUPPORTED|,
     *         |WifiStatusCode.ERROR_UNKNOWN|
     */
    void twtSessionSuspend(in int cmdId, in int sessionId);

    /**
     * Resume a Target Wake Time (TWT) session which is suspended.
     *
     * Supported only if the TWT session supports suspend and resume. See
     * |TwtSession.isSuspendable|. Results in asynchronous callback
     * |IWifiStaIfaceEventCallback.onTwtSessionResume| on success or
     * |IWifiStaIfaceEventCallback.onTwtFailure| on failure.
     *
     * @param cmdId Command Id to use for this invocation. The value 0 is reserved.
     * @param sessionId TWT session id.
     * @throws ServiceSpecificException with one of the following values:
     *         |WifiStatusCode.ERROR_WIFI_IFACE_INVALID|,
     *         |WifiStatusCode.ERROR_NOT_SUPPORTED|,
     *         |WifiStatusCode.ERROR_UNKNOWN|
     */
    void twtSessionResume(in int cmdId, in int sessionId);

    /**
     * Teardown a Target Wake Time (TWT) session.
     *
     * Results in asynchronous callback |IWifiStaIfaceEventCallback.onTwtSessionTeardown| on
     * success or |IWifiStaIfaceEventCallback.onTwtFailure| on failure.
     *
     * @param cmdId Command Id to use for this invocation. The value 0 is reserved.
     * @param sessionId TWT session id.
     * @throws ServiceSpecificException with one of the following values:
     *         |WifiStatusCode.ERROR_WIFI_IFACE_INVALID|,
     *         |WifiStatusCode.ERROR_NOT_SUPPORTED|,
     *         |WifiStatusCode.ERROR_UNKNOWN|
     */
    void twtSessionTeardown(in int cmdId, in int sessionId);

    /**
     * Get stats for a Target Wake Time (TWT) session.
     *
     * Results in asynchronous callback |IWifiStaIfaceEventCallback.onTwtSessionStats| on success
     * or |IWifiStaIfaceEventCallback.onTwtFailure| on failure.
     *
     * @param cmdId Command Id to use for this invocation. The value 0 is reserved.
     * @param sessionId TWT session id.
     * @throws ServiceSpecificException with one of the following values:
     *         |WifiStatusCode.ERROR_WIFI_IFACE_INVALID|,
     *         |WifiStatusCode.ERROR_NOT_SUPPORTED|,
     *         |WifiStatusCode.ERROR_UNKNOWN|
     */
    void twtSessionGetStats(in int cmdId, in int sessionId);
}
