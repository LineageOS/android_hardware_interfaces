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

import android.hardware.wifi.AfcChannelAllowance;
import android.hardware.wifi.IWifiApIface;
import android.hardware.wifi.IWifiChipEventCallback;
import android.hardware.wifi.IWifiNanIface;
import android.hardware.wifi.IWifiP2pIface;
import android.hardware.wifi.IWifiRttController;
import android.hardware.wifi.IWifiStaIface;
import android.hardware.wifi.IfaceConcurrencyType;
import android.hardware.wifi.IfaceType;
import android.hardware.wifi.WifiBand;
import android.hardware.wifi.WifiChipCapabilities;
import android.hardware.wifi.WifiDebugHostWakeReasonStats;
import android.hardware.wifi.WifiDebugRingBufferStatus;
import android.hardware.wifi.WifiDebugRingBufferVerboseLevel;
import android.hardware.wifi.WifiIfaceMode;
import android.hardware.wifi.WifiRadioCombination;
import android.hardware.wifi.WifiUsableChannel;
import android.hardware.wifi.common.OuiKeyedData;

/**
 * Interface that represents a chip that must be configured as a single unit.
 */
@VintfStability
interface IWifiChip {
    /**
     * Capabilities exposed by this chip.
     */
    @VintfStability
    @Backing(type="int")
    enum FeatureSetMask {
        /**
         * Set/Reset Tx Power limits.
         */
        SET_TX_POWER_LIMIT = 1 << 0,
        /**
         * Device to Device RTT.
         */
        D2D_RTT = 1 << 1,
        /**
         * Device to AP RTT.
         */
        D2AP_RTT = 1 << 2,
        /**
         * Set/Reset Tx Power limits.
         */
        USE_BODY_HEAD_SAR = 1 << 3,
        /**
         * Set Latency Mode.
         */
        SET_LATENCY_MODE = 1 << 4,
        /**
         * Support P2P MAC randomization.
         */
        P2P_RAND_MAC = 1 << 5,
        /**
         * Chip can operate in the 60GHz band (WiGig chip).
         */
        WIGIG = 1 << 6,
        /**
         * Chip supports setting allowed channels along with PSD in 6GHz band
         * for AFC purposes.
         */
        SET_AFC_CHANNEL_ALLOWANCE = 1 << 7,
        /**
         * Chip supports Tid-To-Link mapping negotiation.
         */
        T2LM_NEGOTIATION = 1 << 8,
        /**
         * Chip supports voip mode setting.
         */
        SET_VOIP_MODE = 1 << 9,
    }

    /**
     * Set of interface concurrency types, along with the maximum number of interfaces that can have
     * one of the specified concurrency types for a given ChipConcurrencyCombination. See
     * ChipConcurrencyCombination below for examples.
     */
    @VintfStability
    parcelable ChipConcurrencyCombinationLimit {
        IfaceConcurrencyType[] types;
        int maxIfaces;
    }

    /**
     * Set of interfaces that can operate concurrently when in a given mode. See
     * ChipMode below.
     *
     * For example:
     *   [{STA} <= 2]
     *       At most two STA interfaces are supported
     *       [], [STA], [STA+STA]
     *
     *   [{STA} <= 1, {NAN} <= 1, {AP_BRIDGED} <= 1]
     *       Any combination of STA, NAN, AP_BRIDGED
     *       [], [STA], [NAN], [AP_BRIDGED], [STA+NAN], [STA+AP_BRIDGED], [NAN+AP_BRIDGED],
     *       [STA+NAN+AP_BRIDGED]
     *
     *   [{STA} <= 1, {NAN,P2P} <= 1]
     *       Optionally a STA and either NAN or P2P
     *       [], [STA], [STA+NAN], [STA+P2P], [NAN], [P2P]
     *       Not included [NAN+P2P], [STA+NAN+P2P]
     *
     *   [{STA} <= 1, {STA,NAN} <= 1]
     *       Optionally a STA and either a second STA or a NAN
     *       [], [STA], [STA+NAN], [STA+STA], [NAN]
     *       Not included [STA+STA+NAN]
     */
    @VintfStability
    parcelable ChipConcurrencyCombination {
        ChipConcurrencyCombinationLimit[] limits;
    }

    /**
     * Information about the version of the driver and firmware running this chip.
     *
     * The information in these ASCII strings are vendor specific and does not
     * need to follow any particular format. It may be dumped as part of the bug
     * report.
     */
    @VintfStability
    parcelable ChipDebugInfo {
        String driverDescription;
        String firmwareDescription;
    }

    /**
     * Set of interface types, along with the maximum number of interfaces that can have
     * one of the specified types for a given ChipIfaceCombination. See
     * ChipIfaceCombination for examples.
     */
    @VintfStability
    parcelable ChipIfaceCombinationLimit {
        IfaceType[] types;
        int maxIfaces;
    }

    /**
     * Set of interfaces that can operate concurrently when in a given mode. See
     * ChipMode below.
     *
     * For example:
     *   [{STA} <= 2]
     *       At most two STA interfaces are supported
     *       [], [STA], [STA+STA]
     *
     *   [{STA} <= 1, {NAN} <= 1, {AP} <= 1]
     *       Any combination of STA, NAN, AP
     *       [], [STA], [NAN], [AP], [STA+NAN], [STA+AP], [NAN+AP], [STA+NAN+AP]
     *
     *   [{STA} <= 1, {NAN,P2P} <= 1]
     *       Optionally a STA and either NAN or P2P
     *       [], [STA], [STA+NAN], [STA+P2P], [NAN], [P2P]
     *       Not included [NAN+P2P], [STA+NAN+P2P]
     *
     *   [{STA} <= 1, {STA,NAN} <= 1]
     *       Optionally a STA and either a second STA or a NAN
     *       [], [STA], [STA+NAN], [STA+STA], [NAN]
     *       Not included [STA+STA+NAN]
     */
    @VintfStability
    parcelable ChipIfaceCombination {
        ChipIfaceCombinationLimit[] limits;
    }

    /**
     * A mode that the chip can be put in. A mode defines a set of constraints on
     * the interfaces that can exist while in that mode. Modes define a unit of
     * configuration where all interfaces must be torn down to switch to a
     * different mode. Some HALs may only have a single mode, but an example where
     * multiple modes would be required is if a chip has different firmwares with
     * different capabilities.
     *
     * When in a mode, it must be possible to perform any combination of creating
     * and removing interfaces as long as at least one of the
     * ChipConcurrencyCombinations is satisfied. This means that if a chip has two
     * available combinations, [{STA} <= 1] and [{AP_BRIDGED} <= 1] then it is expected
     * that exactly one STA type or one AP_BRIDGED type can be created, but it
     * is not expected that both a STA and AP_BRIDGED type  could be created. If it
     * was then there would be a single available combination
     * [{STA} <=1, {AP_BRIDGED} <= 1].
     *
     * When switching between two available combinations it is expected that
     * interfaces only supported by the initial combination must be removed until
     * the target combination is also satisfied. At that point new interfaces
     * satisfying only the target combination can be added (meaning the initial
     * combination limits will no longer satisfied). The addition of these new
     * interfaces must not impact the existence of interfaces that satisfy both
     * combinations.
     *
     * For example, a chip with available combinations:
     *     [{STA} <= 2, {NAN} <=1] and [{STA} <=1, {NAN} <= 1, {AP_BRIDGED} <= 1}]
     * If the chip currently has 3 interfaces STA, STA and NAN and wants to add an
     * AP_BRIDGED interface in place of one of the STAs, then one of the STA interfaces
     * must be removed first, and then the AP interface can be created after
     * the STA has been torn down. During this process the remaining STA and NAN
     * interfaces must not be removed/recreated.
     *
     * If a chip does not support this kind of reconfiguration in this mode then
     * the combinations must be separated into two separate modes. Before
     * switching modes, all interfaces must be torn down, the mode switch must be
     * enacted, and when it completes the new interfaces must be brought up.
     */
    @VintfStability
    parcelable ChipMode {
        /**
         * Id that can be used to put the chip in this mode.
         */
        int id;
        /**
         * A list of the possible interface concurrency type combinations that the
         * chip can have while in this mode.
         */
        ChipConcurrencyCombination[] availableCombinations;
    }

    /**
     * Wi-Fi coex channel avoidance support.
     */
    const int NO_POWER_CAP_CONSTANT = 0x7FFFFFFF;

    @VintfStability
    @Backing(type="int")
    enum CoexRestriction {
        WIFI_DIRECT = 1 << 0,
        SOFTAP = 1 << 1,
        WIFI_AWARE = 1 << 2,
    }

    /**
     * Representation of a Wi-Fi channel for Wi-Fi coex channel avoidance.
     */
    @VintfStability
    parcelable CoexUnsafeChannel {
        /*
         * Band of the channel.
         */
        WifiBand band;
        /*
         * Channel number.
         */
        int channel;
        /**
         * The power cap will be a maximum power value in dbm that is allowed to be transmitted by
         * the chip on this channel. A value of PowerCapConstant.NO_POWER_CAP means no limitation
         * on transmitted power is needed by the chip for this channel.
         */
        int powerCapDbm;
    }

    /**
     * This enum represents the different latency modes that can be set through |setLatencyMode|.
     */
    @VintfStability
    @Backing(type="int")
    enum LatencyMode {
        NORMAL = 0,
        LOW = 1,
    }

    /**
     * When there are 2 or more simultaneous STA connections, this use case hint indicates what
     * use-case is being enabled by the framework. This use case hint can be used by the firmware
     * to modify various firmware configurations like:
     *   - Allowed BSSIDs the firmware can choose for the initial connection/roaming attempts.
     *   - Duty cycle to choose for the 2 STA connections if the radio is in MCC mode.
     *   - Whether roaming, APF and other offloads need to be enabled or not.
     * Note:
     *   - This will be invoked before an active wifi connection is established on the second
     *     interface.
     *   - This use-case hint is implicitly void when the second STA interface is brought down.
     *   - When there is only 1 STA interface, we should still retain the last use case
     *     set, which must become active the next time multi STA is enabled.
     *     1. Initialize with single STA.
     *     2. Framework creates second STA.
     *     3. Framework sets use case to DUAL_STA_NON_TRANSIENT_UNBIASED.
     *     4. Framework destroys second STA. Only 1 STA remains.
     *     5. Framework recreates second STA.
     *     6. The active use case remains DUAL_STA_NON_TRANSIENT_UNBIASED (i.e. firmware should not
     *        automatically change it during period of single STA unless requested by framework).
     */
    @VintfStability
    @Backing(type="byte")
    enum MultiStaUseCase {
        /**
         * Usage:
         * - This will be sent down for make before break use-case.
         * - Platform is trying to speculatively connect to a second network and evaluate it without
         *  disrupting the primary connection.
         * Requirements for Firmware:
         * - Do not reduce the number of tx/rx chains of primary connection.
         * - If using MCC, should set the MCC duty cycle of the primary connection to be higher than
         *  the secondary connection (maybe 70/30 split).
         * - Should pick the best BSSID for the secondary STA (disregard the chip mode) independent
         *   of the primary STA:
         *    - Don’t optimize for DBS vs MCC/SCC
         * - Should not impact the primary connection’s bssid selection:
         *    - Don’t downgrade chains of the existing primary connection.
         *    - Don’t optimize for DBS vs MCC/SCC.
         */
        DUAL_STA_TRANSIENT_PREFER_PRIMARY = 0,
        /**
         * Usage:
         * - This will be sent down for any app requested peer to peer connections.
         * - In this case, both the connections need to be allocated equal resources.
         * - For the peer to peer use case, BSSID for the secondary connection will be chosen by the
         *   framework.
         *
         * Requirements for Firmware:
         * - Can choose MCC or DBS mode depending on the MCC efficiency and HW capability.
         * - If using MCC, set the MCC duty cycle of the primary connection to be equal to the
         *   secondary connection.
         * - Prefer BSSID candidates which will help provide the best "overall" performance for both
         *   the connections.
         */
        DUAL_STA_NON_TRANSIENT_UNBIASED = 1,
    }

    /**
     * List of preset wifi radio TX power levels for different scenarios.
     * The actual power values (typically varies based on the channel,
     * 802.11 connection type, number of MIMO streams, etc) for each scenario
     * is defined by the OEM as a BDF file since it varies for each wifi chip
     * vendor and device.
     */
    @VintfStability
    @Backing(type="int")
    enum TxPowerScenario {
        VOICE_CALL = 0,
        ON_HEAD_CELL_OFF = 1,
        ON_HEAD_CELL_ON = 2,
        ON_BODY_CELL_OFF = 3,
        ON_BODY_CELL_ON = 4,
    }

    /**
     * Usable Wifi channels filter masks.
     */
    @VintfStability
    @Backing(type="int")
    enum UsableChannelFilter {
        /**
         * Filter Wifi channels that should be avoided due to extreme
         * cellular coexistence restrictions. Some Wifi channels can have
         * extreme interference from/to cellular due to short frequency
         * seperation with neighboring cellular channels, or when there
         * is harmonic and intermodulation interference. Channels which
         * only have some performance degradation (e.g. power back off is
         * sufficient to deal with coexistence issue) can be included and
         * should not be filtered out.
         */
        CELLULAR_COEXISTENCE = 1 << 0,
        /**
         * Filter based on concurrency state.
         * Examples:
         * - 5GHz SAP operation may be supported in standalone mode, but if
         *  there is a STA connection on a 5GHz DFS channel, none of the 5GHz
         *  channels are usable for SAP if device does not support DFS SAP mode.
         * - P2P GO may not be supported on indoor channels in the EU during
         *  standalone mode but if there is a STA connection on indoor channel.
         *  P2P GO may be supported by some vendors on the same STA channel.
         */
        CONCURRENCY = 1 << 1,
        /**
         * Filter Wifi channels that are supported for NAN 3.1 Instant communication mode.
         * This filter should only be applied to a NAN interface.
         * - If 5G is supported, then default discovery channel 149/44 is considered.
         * - If 5G is not supported, then channel 6 has to be considered.
         */
        NAN_INSTANT_MODE = 1 << 2,
    }

    /**
     * This enum represents the different VoIP mode that can be set through |setVoipMode|.
     */
    @VintfStability
    @Backing(type="int")
    enum VoipMode {
        OFF = 0,
        VOICE = 1,
    }

    /**
     * Configure the Chip.
     * This may NOT be called to reconfigure a chip due to an internal
     * limitation. Calling this when chip is already configured in a different
     * mode must trigger an ERROR_NOT_SUPPORTED failure.
     * If you want to do reconfiguration, please call |IWifi.stop| and |IWifi.start|
     * to restart Wifi HAL before calling this.
     * Any existing |IWifiIface| objects must be marked invalid after this call.
     * If this fails then the chip is now in an undefined state and
     * configureChip must be called again.
     * Must trigger |IWifiChipEventCallback.onChipReconfigured| on success.
     * Must trigger |IWifiEventCallback.onFailure| on failure.
     *
     * @param modeId Mode that the chip must switch to, corresponding to the
     *        id property of the target ChipMode.
     * @throws ServiceSpecificException with one of the following values:
     *         |WifiStatusCode.ERROR_WIFI_CHIP_INVALID|,
     *         |WifiStatusCode.ERROR_NOT_AVAILABLE|,
     *         |WifiStatusCode.ERROR_NOT_SUPPORTED|,
     *         |WifiStatusCode.ERROR_UNKNOWN|
     */
    void configureChip(in int modeId);

    /**
     * Create an AP iface on the chip.
     *
     * Depending on the mode the chip is configured in, the interface creation
     * may fail (code: |WifiStatusCode.ERROR_NOT_AVAILABLE|) if we've already
     * reached the maximum allowed (specified in |ChipIfaceCombination|) number
     * of ifaces of the AP type.
     *
     * @return AIDL interface object representing the iface if
     *         successful, null otherwise.
     * @throws ServiceSpecificException with one of the following values:
     *         |WifiStatusCode.ERROR_WIFI_CHIP_INVALID|,
     *         |WifiStatusCode.ERROR_NOT_SUPPORTED|,
     *         |WifiStatusCode.ERROR_NOT_AVAILABLE|
     */
    @PropagateAllowBlocking IWifiApIface createApIface();

    /**
     * Create a bridged AP iface on the chip.
     *
     * Depending on the mode the chip is configured in, the interface creation
     * may fail (code: |WifiStatusCode.ERROR_NOT_AVAILABLE|) if we've already
     * reached the maximum allowed (specified in |ChipIfaceCombination|) number
     * of ifaces of the AP type.
     *
     * @return AIDL interface object representing the iface if
     *         successful, null otherwise.
     * @throws ServiceSpecificException with one of the following values:
     *         |WifiStatusCode.ERROR_WIFI_CHIP_INVALID|,
     *         |WifiStatusCode.ERROR_NOT_SUPPORTED|,
     *         |WifiStatusCode.ERROR_NOT_AVAILABLE|
     */
    @PropagateAllowBlocking IWifiApIface createBridgedApIface();

    /**
     * Create a NAN iface on the chip.
     *
     * Depending on the mode the chip is configured in, the interface creation
     * may fail (code: |WifiStatusCode.ERROR_NOT_AVAILABLE|) if we've already
     * reached the maximum allowed (specified in |ChipIfaceCombination|) number
     * of ifaces of the NAN type.
     *
     * @return AIDL interface object representing the iface if
     *         successful, null otherwise.
     * @throws ServiceSpecificException with one of the following values:
     *         |WifiStatusCode.ERROR_WIFI_CHIP_INVALID|,
     *         |WifiStatusCode.ERROR_NOT_SUPPORTED|,
     *         |WifiStatusCode.ERROR_NOT_AVAILABLE|
     */
    @PropagateAllowBlocking IWifiNanIface createNanIface();

    /**
     * Create a P2P iface on the chip.
     *
     * Depending on the mode the chip is configured in, the interface creation
     * may fail (code: |WifiStatusCode.ERROR_NOT_AVAILABLE|) if we've already
     * reached the maximum allowed (specified in |ChipIfaceCombination|) number
     * of ifaces of the P2P type.
     *
     * @return AIDL interface object representing the iface if
     *         successful, null otherwise.
     * @throws ServiceSpecificException with one of the following values:
     *         |WifiStatusCode.ERROR_WIFI_CHIP_INVALID|,
     *         |WifiStatusCode.ERROR_NOT_SUPPORTED|,
     *         |WifiStatusCode.ERROR_NOT_AVAILABLE|
     */
    @PropagateAllowBlocking IWifiP2pIface createP2pIface();

    /**
     * Create an RTTController instance.
     *
     * RTT controller can be either:
     * a) Bound to a specific STA iface by passing in the corresponding
     * |IWifiStaIface| object in the |boundIface| param, OR
     * b) Let the implementation decide the iface to use for RTT operations
     * by passing null in the |boundIface| param.
     *
     * @param boundIface AIDL interface object representing the STA iface if
     *        the responder must be bound to a specific iface, null otherwise.
     * @throws ServiceSpecificException with one of the following values:
     *         |WifiStatusCode.ERROR_WIFI_CHIP_INVALID|
     */
    @PropagateAllowBlocking IWifiRttController createRttController(in IWifiStaIface boundIface);

    /**
     * Create a STA iface on the chip.
     *
     * Depending on the mode the chip is configured in, the interface creation
     * may fail (code: |WifiStatusCode.ERROR_NOT_AVAILABLE|) if we've already
     * reached the maximum allowed (specified in |ChipIfaceCombination|) number
     * of ifaces of the STA type.
     *
     * @return AIDL interface object representing the iface if
     *         successful, null otherwise.
     * @throws ServiceSpecificException with one of the following values:
     *         |WifiStatusCode.ERROR_WIFI_CHIP_INVALID|,
     *         |WifiStatusCode.ERROR_NOT_SUPPORTED|,
     *         |WifiStatusCode.ERROR_NOT_AVAILABLE|
     */
    @PropagateAllowBlocking IWifiStaIface createStaIface();

    /**
     * API to enable/disable alert notifications from the chip.
     * These alerts must be used to notify the framework of any fatal error events
     * that the chip encounters via |IWifiChipEventCallback.onDebugErrorAlert| method.
     *
     * @param enable true to enable, false to disable.
     * @throws ServiceSpecificException with one of the following values:
     *         |WifiStatusCode.ERROR_WIFI_CHIP_INVALID|,
     *         |WifiStatusCode.ERROR_NOT_SUPPORTED|,
     *         |WifiStatusCode.NOT_AVAILABLE|,
     *         |WifiStatusCode.UNKNOWN|
     */
    void enableDebugErrorAlerts(in boolean enable);

    /**
     * API to flush debug ring buffer data to files.
     *
     * Force flush debug ring buffer using IBase::debug.
     * This API helps to collect firmware/driver/pkt logs.
     *
     * @throws ServiceSpecificException with one of the following values:
     *         |WifiStatusCode.UNKNOWN|
     */
    void flushRingBufferToFile();

    /**
     * API to force dump data into the corresponding ring buffer.
     * This is to be invoked during bugreport collection.
     *
     * @param ringName Name of the ring for which data collection should
     *        be forced. This can be retrieved via the corresponding
     *        |WifiDebugRingBufferStatus|.
     * @throws ServiceSpecificException with one of the following values:
     *         |WifiStatusCode.ERROR_WIFI_CHIP_INVALID|,
     *         |WifiStatusCode.ERROR_NOT_SUPPORTED|,
     *         |WifiStatusCode.ERROR_NOT_STARTED|,
     *         |WifiStatusCode.NOT_AVAILABLE|,
     *         |WifiStatusCode.UNKNOWN|
     */
    void forceDumpToDebugRingBuffer(in String ringName);

    /**
     * Gets an AIDL interface object for the AP Iface corresponding
     * to the provided ifname.
     *
     * @param ifname Name of the iface.
     * @return AIDL interface object representing the iface if
     *         it exists, null otherwise.
     * @throws ServiceSpecificException with one of the following values:
     *         |WifiStatusCode.ERROR_WIFI_CHIP_INVALID|,
     *         |WifiStatusCode.ERROR_INVALID_ARGS|
     */
    @PropagateAllowBlocking IWifiApIface getApIface(in String ifname);

    /**
     * List all the AP iface names configured on the chip.
     * The corresponding |IWifiApIface| object for any iface
     * can be retrieved using the |getApIface| method.
     *
     * @return List of all AP iface names on the chip.
     * @throws ServiceSpecificException with one of the following values:
     *         |WifiStatusCode.ERROR_WIFI_CHIP_INVALID|
     */
    String[] getApIfaceNames();

    /**
     * Get the set of operation modes that the chip supports.
     *
     * @return List of modes supported by the device.
     * @throws ServiceSpecificException with one of the following values:
     *         |WifiStatusCode.ERROR_WIFI_CHIP_INVALID|
     */
    ChipMode[] getAvailableModes();

    /**
     * Get the features supported by this chip.
     *
     * @return Bitset of |FeatureSetMask| values.
     * @throws ServiceSpecificException with one of the following values:
     *         |WifiStatusCode.ERROR_WIFI_CHIP_INVALID|,
     *         |WifiStatusCode.ERROR_NOT_AVAILABLE|,
     *         |WifiStatusCode.ERROR_UNKNOWN|
     */
    int getFeatureSet();

    /**
     * API to retrieve the wifi wake up reason stats for debugging.
     * The driver is expected to start maintaining these stats once the chip
     * is configured using |configureChip|. These stats must be reset whenever
     * the chip is reconfigured or the HAL is stopped.
     *
     * @return Instance of |WifiDebugHostWakeReasonStats|.
     * @throws ServiceSpecificException with one of the following values:
     *         |WifiStatusCode.ERROR_WIFI_CHIP_INVALID|,
     *         |WifiStatusCode.ERROR_NOT_SUPPORTED|,
     *         |WifiStatusCode.NOT_AVAILABLE|,
     *         |WifiStatusCode.UNKNOWN|
     */
    WifiDebugHostWakeReasonStats getDebugHostWakeReasonStats();

    /**
     * The WiFi debug ring buffer life cycle is as follows:
     * - At initialization, the framework must call |getDebugRingBuffersStatus|.
     *   to obtain the names and list of supported ring buffers.
     *   The driver may expose several different rings, each holding a different
     *   type of data (connection events, power events, etc).
     * - When WiFi operations start, the framework must call
     *   |startLoggingToDebugRingBuffer| to trigger log collection for a specific
     *   ring. The vebose level for each ring buffer can be specified in this API.
     * - During wifi operations, the driver must periodically report per ring data
     *   to framework by invoking the
     *   |IWifiChipEventCallback.onDebugRingBufferDataAvailable| callback.
     * - When capturing a bug report, the framework must indicate to driver that
     *   all the data has to be uploaded urgently by calling |forceDumpToDebugRingBuffer|.
     *
     * The data uploaded by driver must be stored by the framework in separate files,
     * with one stream of file per ring. The framework must store the files in pcapng
     * format, allowing for easy merging and parsing with network analyzer tools.
     * TODO: Since we're no longer dumping the raw data, storing in separate
     * pcapng files for parsing later must not work anymore.
     */

    /*
     * API to get the status of all ring buffers supported by driver.
     *
     * @return Vector of |WifiDebugRingBufferStatus| corresponding to the
     *         status of each ring buffer on the device.
     * @throws ServiceSpecificException with one of the following values:
     *         |WifiStatusCode.ERROR_WIFI_CHIP_INVALID|,
     *         |WifiStatusCode.ERROR_NOT_SUPPORTED|,
     *         |WifiStatusCode.NOT_AVAILABLE|,
     *         |WifiStatusCode.UNKNOWN|
     */
    WifiDebugRingBufferStatus[] getDebugRingBuffersStatus();

    /**
     * Get the Id assigned to this chip.
     *
     * @return Assigned chip Id.
     * @throws ServiceSpecificException with one of the following values:
     *         |WifiStatusCode.ERROR_WIFI_CHIP_INVALID|
     */
    int getId();

    /**
     * Get the current mode that the chip is in.
     *
     * @return Mode that the chip is currently configured to,
     *         corresponding to the Id property of the target ChipMode.
     * @throws ServiceSpecificException with one of the following values:
     *         |WifiStatusCode.ERROR_WIFI_CHIP_INVALID|
     */
    int getMode();

    /**
     * Gets an AIDL interface object for the NAN Iface corresponding
     * to the provided ifname.
     *
     * @param ifname Name of the iface.
     * @return AIDL interface object representing the iface if
     *         it exists, null otherwise.
     * @throws ServiceSpecificException with one of the following values:
     *         |WifiStatusCode.ERROR_WIFI_CHIP_INVALID|,
     *         |WifiStatusCode.ERROR_INVALID_ARGS|
     */
    @PropagateAllowBlocking IWifiNanIface getNanIface(in String ifname);

    /**
     * List all the NAN iface names configured on the chip.
     * The corresponding |IWifiNanIface| object for any iface can
     * be retrieved using the |getNanIface| method.
     *
     * @return List of all NAN iface names on the chip.
     * @throws ServiceSpecificException with one of the following values:
     *         |WifiStatusCode.ERROR_WIFI_CHIP_INVALID|
     */
    String[] getNanIfaceNames();

    /**
     * Gets an AIDL interface object for the P2P Iface corresponding
     * to the provided ifname.
     *
     * @param ifname Name of the iface.
     * @return AIDL interface object representing the iface if
     *         it exists, null otherwise.
     * @throws ServiceSpecificException with one of the following values:
     *         |WifiStatusCode.ERROR_WIFI_CHIP_INVALID|,
     *         |WifiStatusCode.ERROR_INVALID_ARGS|
     */
    @PropagateAllowBlocking IWifiP2pIface getP2pIface(in String ifname);

    /**
     * List all the P2P iface names configured on the chip.
     * The corresponding |IWifiP2pIface| object for any iface can
     * be retrieved using the |getP2pIface| method.
     *
     * @return List of all P2P iface names on the chip.
     * @throws ServiceSpecificException with one of the following values:
     *         |WifiStatusCode.ERROR_WIFI_CHIP_INVALID|
     */
    String[] getP2pIfaceNames();

    /**
     * Gets an AIDL interface object for the STA Iface corresponding
     * to the provided ifname.
     *
     * @param ifname Name of the iface.
     * @return AIDL interface object representing the iface if
     *         it exists, null otherwise.
     * @throws ServiceSpecificException with one of the following values:
     *         |WifiStatusCode.ERROR_WIFI_CHIP_INVALID|,
     *         |WifiStatusCode.ERROR_INVALID_ARGS|
     */
    @PropagateAllowBlocking IWifiStaIface getStaIface(in String ifname);

    /**
     * List all the STA iface names configured on the chip.
     * The corresponding |IWifiStaIface| object for any iface can
     * be retrieved using the |getStaIface| method.
     *
     * @param List of all STA iface names on the chip.
     * @throws ServiceSpecificException with one of the following values:
     *         |WifiStatusCode.ERROR_WIFI_CHIP_INVALID|
     */
    String[] getStaIfaceNames();

    /**
     * Retrieve the list of all the possible radio combinations supported by this
     * chip.
     *
     * @return A list of all the possible radio combinations.
     *         For example, in case of a chip which has two radios, where one radio is
     *         capable of 2.4GHz 2X2 only and another radio which is capable of either
     *         5GHz or 6GHz 2X2, the number of possible radio combinations in this case
     *         is 5 and the possible combinations are:
     *         {{{2G 2X2}}, //Standalone 2G
     *         {{5G 2X2}}, //Standalone 5G
     *         {{6G 2X2}}, //Standalone 6G
     *         {{2G 2X2}, {5G 2X2}}, //2G+5G DBS
     *         {{2G 2X2}, {6G 2X2}}} //2G+6G DBS
     *         Note: Since this chip doesn’t support 5G+6G simultaneous operation,
     *         as there is only one radio which can support both bands, it can only
     *         do MCC 5G+6G. This table should not get populated with possible MCC
     *         configurations. This is only for simultaneous radio configurations
     *         (such as standalone, multi band simultaneous or single band simultaneous).
     * @throws ServiceSpecificException with one of the following values:
     *         |WifiStatusCode.ERROR_WIFI_CHIP_INVALID|,
     *         |WifiStatusCode.ERROR_NOT_SUPPORTED|,
     *         |WifiStatusCode.FAILURE_UNKNOWN|
     *
     */
    WifiRadioCombination[] getSupportedRadioCombinations();

    /**
     * Get capabilities supported by this chip.
     *
     * @return Chip capabilities represented by |WifiChipCapabilities|.
     * @throws ServiceSpecificException with one of the following values:
     *         |WifiStatusCode.ERROR_WIFI_CHIP_INVALID|,
     *         |WifiStatusCode.ERROR_NOT_SUPPORTED|,
     *         |WifiStatusCode.FAILURE_UNKNOWN|
     *
     */
    WifiChipCapabilities getWifiChipCapabilities();

    /**
     * Retrieve a list of usable Wifi channels for the specified band &
     * operational modes.
     *
     * The list of usable Wifi channels in a given band depends on factors
     * like current country code, operational mode (e.g. STA, SAP, WFD-CLI,
     * WFD-GO, TDLS, NAN) and other restrictons due to DFS, cellular coexistence
     * and concurrency state of the device.
     *
     * @param band |WifiBand| for which list of usable channels is requested.
     * @param ifaceModeMask Bitmask of the modes represented by |WifiIfaceMode|.
     *        Bitmask respresents all the modes that the caller is interested
     *        in (e.g. STA, SAP, CLI, GO, TDLS, NAN). E.g. If the caller is
     *        interested in knowing usable channels for P2P CLI, P2P GO & NAN,
     *        ifaceModeMask would be set to
     *        IFACE_MODE_P2P_CLIENT|IFACE_MODE_P2P_GO|IFACE_MODE_NAN.
     * @param filterMask Bitmask of filters represented by
     *        |UsableChannelFilter|. Specifies whether driver should filter
     *        channels based on additional criteria. If no filter is specified,
     *        then the driver should return usable channels purely based on
     *        regulatory constraints.
     * @return List of channels represented by |WifiUsableChannel|.
     *         Each entry represents a channel frequency, bandwidth and
     *         bitmask of modes (e.g. STA, SAP, CLI, GO, TDLS, NAN) that are
     *         allowed on that channel. E.g. If only STA mode can be supported
     *         on an indoor channel, only the IFACE_MODE_STA bit would be set
     *         for that channel. If 5GHz SAP cannot be supported, then none of
     *         the 5GHz channels will have IFACE_MODE_SOFTAP bit set.
     *         Note: Bits do not represent concurrency state. Each bit only
     *         represents whether a particular mode is allowed on that channel.
     * @throws ServiceSpecificException with one of the following values:
     *         |WifiStatusCode.ERROR_NOT_SUPPORTED|,
     *         |WifiStatusCode.ERROR_INVALID_ARGS|,
     *         |WifiStatusCode.FAILURE_UNKNOWN|
     */
    WifiUsableChannel[] getUsableChannels(
            in WifiBand band, in int ifaceModeMask, in int filterMask);

    /*
     * Set the max power level the chip is allowed to transmit on for 6Ghz AFC.
     * @param afcChannelAllowance Specifies the power limitations for 6Ghz AFC.
     * @throws ServiceSpecificException with one of the following values:
     *         |WifiStatusCode.ERROR_NOT_SUPPORTED|
     */
    void setAfcChannelAllowance(in AfcChannelAllowance afcChannelAllowance);

    /**
     * Requests notifications of significant events on this chip. Multiple calls
     * to this must register multiple callbacks, each of which must receive all
     * events.
     *
     * @param callback An instance of the |IWifiChipEventCallback| AIDL interface
     *        object.
     * @throws ServiceSpecificException with one of the following values:
     *         |WifiStatusCode.ERROR_NOT_SUPPORTED|,
     *         |WifiStatusCode.ERROR_WIFI_CHIP_INVALID|
     */
    void registerEventCallback(in IWifiChipEventCallback callback);

    /**
     * Removes the AP Iface with the provided ifname.
     * Any further calls on the corresponding |IWifiApIface| AIDL interface
     * object must fail.
     *
     * @param ifname Name of the iface.
     * @throws ServiceSpecificException with one of the following values:
     *         |WifiStatusCode.ERROR_WIFI_CHIP_INVALID|,
     *         |WifiStatusCode.ERROR_INVALID_ARGS|
     */
    void removeApIface(in String ifname);

    /**
     * Removes an instance of AP iface with name |ifaceInstanceName| from the
     * bridge AP with name |brIfaceName|.
     *
     * Use the API |removeApIface| with the brIfaceName to remove the bridge iface.
     *
     * @param brIfaceName Name of the bridged AP iface.
     * @param ifaceInstanceName Name of the AP instance. The empty instance is
     *        invalid.
     * @throws ServiceSpecificException with one of the following values:
     *         |WifiStatusCode.ERROR_WIFI_CHIP_INVALID|,
     *         |WifiStatusCode.ERROR_INVALID_ARGS|,
     *         |WifiStatusCode.ERROR_NOT_AVAILABLE|
     */
    void removeIfaceInstanceFromBridgedApIface(in String brIfaceName, in String ifaceInstanceName);

    /**
     * Removes the NAN Iface with the provided ifname.
     * Any further calls on the corresponding |IWifiNanIface| AIDL interface
     * object must fail.
     *
     * @param ifname Name of the iface.
     * @throws ServiceSpecificException with one of the following values:
     *         |WifiStatusCode.ERROR_WIFI_CHIP_INVALID|,
     *         |WifiStatusCode.ERROR_INVALID_ARGS|
     */
    void removeNanIface(in String ifname);

    /**
     * Removes the P2P Iface with the provided ifname.
     * Any further calls on the corresponding |IWifiP2pIface| AIDL interface
     * object must fail.
     *
     * @param ifname Name of the iface.
     * @throws ServiceSpecificException with one of the following values:
     *         |WifiStatusCode.ERROR_WIFI_CHIP_INVALID|,
     *         |WifiStatusCode.ERROR_INVALID_ARGS|
     */
    void removeP2pIface(in String ifname);

    /**
     * Removes the STA Iface with the provided ifname.
     * Any further calls on the corresponding |IWifiStaIface| AIDL interface
     * object must fail.
     *
     * @param ifname Name of the iface.
     * @throws ServiceSpecificException with one of the following values:
     *         |WifiStatusCode.ERROR_WIFI_CHIP_INVALID|,
     *         |WifiStatusCode.ERROR_INVALID_ARGS|
     */
    void removeStaIface(in String ifname);

    /**
     * Request information about the chip.
     *
     * @return Instance of |ChipDebugInfo|.
     * @throws ServiceSpecificException with one of the following values:
     *         |WifiStatusCode.ERROR_WIFI_CHIP_INVALID|,
     *         |WifiStatusCode.ERROR_NOT_AVAILABLE|,
     *         |WifiStatusCode.ERROR_UNKNOWN|
     */
    ChipDebugInfo requestChipDebugInfo();

    /**
     * Request vendor debug info from the driver.
     *
     * @return Vector of bytes retrieved from the driver.
     * @throws ServiceSpecificException with one of the following values:
     *         |WifiStatusCode.ERROR_WIFI_CHIP_INVALID|,
     *         |WifiStatusCode.ERROR_NOT_AVAILABLE|,
     *         |WifiStatusCode.ERROR_UNKNOWN|
     */
    byte[] requestDriverDebugDump();

    /**
     * Request vendor debug info from the firmware.
     *
     * @return Vector of bytes retrieved from the firmware.
     * @throws ServiceSpecificException with one of the following values:
     *         |WifiStatusCode.ERROR_WIFI_CHIP_INVALID|,
     *         |WifiStatusCode.ERROR_NOT_AVAILABLE|,
     *         |WifiStatusCode.ERROR_UNKNOWN|
     */
    byte[] requestFirmwareDebugDump();

    /**
     * API to reset TX power levels.
     * This is used to indicate the end of the previously selected TX power
     * scenario and let the wifi chip fall back to the default power values.
     *
     * @throws ServiceSpecificException with one of the following values:
     *         |WifiStatusCode.ERROR_WIFI_CHIP_INVALID|,
     *         |WifiStatusCode.ERROR_NOT_SUPPORTED|,
     *         |WifiStatusCode.NOT_AVAILABLE|,
     *         |WifiStatusCode.UNKNOWN|
     */
    void resetTxPowerScenario();

    /**
     * API to select one of the preset TX power scenarios.
     *
     * The framework must invoke this method with the appropriate scenario to let
     * the wifi chip change its transmitting power levels.
     * OEM's should define various power profiles for each of the scenarios
     * above (defined in |TxPowerScenario|) in a vendor extension.
     *
     * @param scenario One of the preselected scenarios defined in
     *        |TxPowerScenario|.
     * @throws ServiceSpecificException with one of the following values:
     *         |WifiStatusCode.ERROR_WIFI_CHIP_INVALID|,
     *         |WifiStatusCode.ERROR_NOT_SUPPORTED|,
     *         |WifiStatusCode.NOT_AVAILABLE|,
     *         |WifiStatusCode.UNKNOWN|
     */
    void selectTxPowerScenario(in TxPowerScenario scenario);

    /**
     * Invoked to indicate that the provided |CoexUnsafeChannels| should be avoided with the
     * specified restrictions.
     *
     * Channel avoidance is a suggestion and should be done on a best-effort approach. If a provided
     * channel is used, then the specified power cap should be applied.
     *
     * In addition, hard restrictions on the Wifi modes may be indicated by |CoexRestriction| bits
     * (WIFI_DIRECT, SOFTAP, WIFI_AWARE) in the |restrictions| bitfield. If a hard restriction is
     * provided, then the channels should be completely avoided for the provided Wifi modes instead
     * of by best-effort.
     *
     * @param unsafeChannels List of |CoexUnsafeChannels| to avoid.
     * @param restrictions Bitset of |CoexRestriction| values indicating Wifi interfaces to
     *         completely avoid.
     * @throws ServiceSpecificException with one of the following values:
     *         |WifiStatusCode.ERROR_WIFI_CHIP_INVALID|,
     *         |WifiStatusCode.ERROR_INVALID_ARGS|,
     */
    void setCoexUnsafeChannels(in CoexUnsafeChannel[] unsafeChannels, in int restrictions);

    /**
     * Set country code for this Wifi chip.
     *
     * Country code is global setting across the Wifi chip and not Wifi
     * interface (STA or AP) specific.
     *
     * @param code 2 byte country code (as defined in ISO 3166) to set.
     * @throws ServiceSpecificException with one of the following values:
     *         |WifiStatusCode.FAILURE_UNKNOWN|,
     *         |WifiStatusCode.FAILURE_IFACE_INVALID|
     */
    void setCountryCode(in byte[2] code);

    /**
     * API to set the wifi latency mode
     *
     * The latency mode is a hint to the HAL to enable or disable Wi-Fi latency
     * optimization. The optimization should be enabled if the mode is set to |LOW|
     * and should be disabled if the mode is set to |NORMAL|.
     * Wi-Fi latency optimization may trade-off latency against other Wi-Fi
     * functionality such as scanning, roaming, etc. but it should not result in
     * completely halting this functionality.
     *
     * The low latency mode targets applications such as gaming and virtual reality.
     */
    void setLatencyMode(in LatencyMode mode);

    /**
     * Invoked to indicate that the provided iface is the primary STA iface when more
     * than 1 STA ifaces are concurrently active.
     * Notes:
     * - If the wifi firmware/chip cannot support multiple instances of any offload
     *   (like roaming, APF, rssi threshold, etc), the firmware should ensure that these
     *   offloads are at least enabled for the primary interface. If the new primary interface is
     *   already connected to a network, the firmware must switch all the offloads on
     *   this new interface without disconnecting.
     * - When there is only 1 STA interface, the firmware must still retain the last primary
     *   connection, which must become active the next time multi STA is enabled.
     *
     * @param ifname Name of the STA iface.
     * @throws ServiceSpecificException with one of the following values:
     *         |WifiStatusCode.ERROR_WIFI_CHIP_INVALID|,
     *         |WifiStatusCode.ERROR_INVALID_ARGS|
     */
    void setMultiStaPrimaryConnection(in String ifName);

    /**
     * Invoked to indicate the STA + STA use-case that is active.
     *
     * Refer to documentation of |MultiStaUseCase| for details.
     *
     * @param useCase Use case that is active.
     * @throws ServiceSpecificException with one of the following values:
     *         |WifiStatusCode.ERROR_WIFI_CHIP_INVALID|,
     *         |WifiStatusCode.ERROR_INVALID_ARGS|
     */
    void setMultiStaUseCase(in MultiStaUseCase useCase);

    /**
     * API to trigger the debug data collection.
     *
     * @param ringName Name of the ring for which data collection
     *        shall start. This can be retrieved via the corresponding
     *        |WifiDebugRingBufferStatus|.
     * @param verboseLevel Verbose level for logging.
     * @parm maxIntervalInSec Maximum interval in seconds for driver to invoke
     *       |onDebugRingBufferData|, ignore if zero.
     * @parm minDataSizeInBytes: Minimum data size in buffer for driver to invoke
     *       |onDebugRingBufferData|, ignore if zero.
     * @throws ServiceSpecificException with one of the following values:
     *         |WifiStatusCode.ERROR_WIFI_CHIP_INVALID|,
     *         |WifiStatusCode.ERROR_NOT_SUPPORTED|,
     *         |WifiStatusCode.NOT_AVAILABLE|,
     *         |WifiStatusCode.UNKNOWN|
     */
    void startLoggingToDebugRingBuffer(in String ringName,
            in WifiDebugRingBufferVerboseLevel verboseLevel, in int maxIntervalInSec,
            in int minDataSizeInBytes);

    /**
     * API to stop the debug data collection for all ring buffers.
     *
     * @throws ServiceSpecificException with one of the following values:
     *         |WifiStatusCode.ERROR_WIFI_CHIP_INVALID|,
     *         |WifiStatusCode.ERROR_NOT_SUPPORTED|,
     *         |WifiStatusCode.NOT_AVAILABLE|,
     *         |WifiStatusCode.UNKNOWN|
     */
    void stopLoggingToDebugRingBuffer();

    /**
     * Trigger subsystem restart.
     *
     * If the framework detects a problem (e.g. connection failure),
     * it must call this function to attempt recovery.
     *
     * When the wifi HAL receives |triggerSubsystemRestart|, it must restart
     * the wlan subsystem, especially the wlan firmware.
     *
     * Regarding the callback function for subsystem restart, refer to documentation of
     * |IWifiEventCallback.onSubsystemRestart| for details.
     *
     * @throws ServiceSpecificException with one of the following values:
     *         |WifiStatusCode.ERROR_WIFI_CHIP_INVALID|,
     *         |WifiStatusCode.ERROR_NOT_AVAILABLE|,
     *         |WifiStatusCode.ERROR_UNKNOWN|
     */
    void triggerSubsystemRestart();

    /**
     * Channel category mask.
     */
    @VintfStability
    @Backing(type="int")
    enum ChannelCategoryMask {
        INDOOR_CHANNEL = 1 << 0,
        DFS_CHANNEL = 1 << 1,
    }

    /**
     * API to enable or disable the feature of allowing current STA-connected channel for WFA GO,
     * SAP and Aware when the regulatory allows.
     * If the channel category is enabled and allowed by the regulatory, the HAL method
     * getUsableChannels() will contain the current STA-connected channel if that channel belongs
     * to that category.
     * @param channelCategoryEnableFlag Bitmask of |ChannelCategoryMask| values.
     *        For each bit, 1 enables the channel category and 0 disables that channel category.
     * @throws ServiceSpecificException with one of the following values:
     *         |WifiStatusCode.ERROR_WIFI_CHIP_INVALID|,
     *         |WifiStatusCode.ERROR_NOT_SUPPORTED|,
     *         |WifiStatusCode.FAILURE_UNKNOWN|
     */
    void enableStaChannelForPeerNetwork(in int channelCategoryEnableFlag);

    /**
     * Multi-Link Operation modes.
     */
    @VintfStability
    @Backing(type="int")
    enum ChipMloMode {
        /**
         * Default mode for Multi-Link Operation.
         */
        DEFAULT = 0,
        /**
         * Low latency mode for Multi-link operation.
         */
        LOW_LATENCY = 1,
        /**
         * High throughput mode for Multi-link operation.
         */
        HIGH_THROUGHPUT = 2,
        /**
         * Low power mode for Multi-link operation.
         */
        LOW_POWER = 3,
    }

    /**
     * Set mode for Multi-Link Operation. Various modes are defined by the enum |ChipMloMode|.
     *
     * @param mode MLO mode as defined by the enum |ChipMloMode|
     * @throws ServiceSpecificException with one of the following values:
     *         |WifiStatusCode.ERROR_WIFI_IFACE_INVALID|,
     *         |WifiStatusCode.ERROR_NOT_SUPPORTED|,
     *         |WifiStatusCode.ERROR_UNKNOWN|
     *
     */
    void setMloMode(in ChipMloMode mode);

    /**
     * Create an AP or bridged AP iface on the chip using vendor-provided configuration parameters.
     *
     * Depending on the mode the chip is configured in, the interface creation
     * may fail (code: |WifiStatusCode.ERROR_NOT_AVAILABLE|) if we've already
     * reached the maximum allowed (specified in |ChipIfaceCombination|) number
     * of ifaces of the AP or AP_BRIDGED type.
     *
     * @param  iface IfaceConcurrencyType to be created. Takes one of
               |IfaceConcurrencyType.AP| or |IfaceConcurrencyType.AP_BRIDGED|
     * @param  vendorData Vendor-provided configuration data as a list of |OuiKeyedData|.
     * @return AIDL interface object representing the iface if
     *         successful, null otherwise.
     * @throws ServiceSpecificException with one of the following values:
     *         |WifiStatusCode.ERROR_WIFI_CHIP_INVALID|,
     *         |WifiStatusCode.ERROR_NOT_SUPPORTED|,
     *         |WifiStatusCode.ERROR_NOT_AVAILABLE|,
     *         |WifiStatusCode.ERROR_INVALID_ARGS|
     */
    @PropagateAllowBlocking
    IWifiApIface createApOrBridgedApIface(
            in IfaceConcurrencyType iface, in OuiKeyedData[] vendorData);

    /**
     * API to set the wifi VoIP mode.
     *
     * The VoIP mode is a hint to the HAL to enable or disable Wi-Fi VoIP
     * optimization. The optimization should be enabled if the mode is NOT set to |OFF|.
     * Furthermore, HAL should implement relevant optimization techniques based on the
     * current operational mode.
     *
     * Note: Wi-Fi VoIP optimization may trade-off power against Wi-Fi
     * performance but it provides better voice quility.
     *
     * @param mode Voip mode as defined by the enum |VoipMode|
     * @throws ServiceSpecificException with one of the following values:
     *         |WifiStatusCode.ERROR_WIFI_CHIP_INVALID|,
     *         |WifiStatusCode.ERROR_INVALID_ARGS|,
     *         |WifiStatusCode.ERROR_UNKNOWN|
     */
    void setVoipMode(in VoipMode mode);
}
