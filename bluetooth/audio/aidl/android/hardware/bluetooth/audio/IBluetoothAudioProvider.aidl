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

package android.hardware.bluetooth.audio;

import android.hardware.bluetooth.audio.A2dpConfiguration;
import android.hardware.bluetooth.audio.A2dpConfigurationHint;
import android.hardware.bluetooth.audio.A2dpRemoteCapabilities;
import android.hardware.bluetooth.audio.A2dpStatus;
import android.hardware.bluetooth.audio.AudioConfiguration;
import android.hardware.bluetooth.audio.AudioContext;
import android.hardware.bluetooth.audio.BluetoothAudioStatus;
import android.hardware.bluetooth.audio.CodecId;
import android.hardware.bluetooth.audio.CodecParameters;
import android.hardware.bluetooth.audio.CodecSpecificCapabilitiesLtv;
import android.hardware.bluetooth.audio.CodecSpecificConfigurationLtv;
import android.hardware.bluetooth.audio.ConfigurationFlags;
import android.hardware.bluetooth.audio.IBluetoothAudioPort;
import android.hardware.bluetooth.audio.LatencyMode;
import android.hardware.bluetooth.audio.LeAudioAseConfiguration;
import android.hardware.bluetooth.audio.LeAudioBisConfiguration;
import android.hardware.bluetooth.audio.LeAudioBroadcastConfiguration.BroadcastStreamMap;
import android.hardware.bluetooth.audio.LeAudioConfiguration.StreamMap;
import android.hardware.bluetooth.audio.MetadataLtv;
import android.hardware.bluetooth.audio.Phy;
import android.hardware.common.fmq.MQDescriptor;
import android.hardware.common.fmq.SynchronizedReadWrite;

/**
 * HAL interface from the Bluetooth stack to the Audio HAL
 *
 * The Bluetooth stack calls methods in this interface to start and end audio
 * sessions and sends callback events to the Audio HAL.
 *
 */
@VintfStability
interface IBluetoothAudioProvider {
    /**
     * Ends the current session and unregisters the IBluetoothAudioPort
     * interface.
     */
    void endSession();

    /**
     * This method indicates that the Bluetooth stack is ready to stream audio.
     * It registers an instance of IBluetoothAudioPort with and provides the
     * current negotiated codec to the Audio HAL. After this method is called,
     * the Audio HAL can invoke IBluetoothAudioPort.startStream().
     *
     * Note: endSession() must be called to unregister this IBluetoothAudioPort
     *
     * @param hostIf An instance of IBluetoothAudioPort for stream control
     * @param audioConfig The audio configuration negotiated with the remote
     *    device. The PCM parameters are set if software based encoding,
     *    otherwise the correct codec configuration is used for hardware
     *    encoding.
     * @param supportedLatencyModes latency modes supported by the active
     * remote device
     *
     * @return The fast message queue for audio data from/to this
     *    provider. Audio data will be in PCM format as specified by the
     *    audioConfig.pcmConfig parameter. Invalid if streaming is offloaded
     *    from/to hardware or on failure
     */
    MQDescriptor<byte, SynchronizedReadWrite> startSession(in IBluetoothAudioPort hostIf,
            in AudioConfiguration audioConfig, in LatencyMode[] supportedLatencyModes);
    /**
     * Callback for IBluetoothAudioPort.startStream()
     *
     * @param status true for SUCCESS or false for FAILURE
     */
    void streamStarted(in BluetoothAudioStatus status);

    /**
     * Callback for IBluetoothAudioPort.suspendStream()
     *
     * @param status true for SUCCESS or false for FAILURE
     */
    void streamSuspended(in BluetoothAudioStatus status);

    /**
     * Called when the audio configuration of the stream has been changed.
     *
     * @param audioConfig The audio configuration negotiated with the remote
     *    device. The PCM parameters are set if software based encoding,
     *    otherwise the correct codec configuration is used for hardware
     *    encoding.
     */
    void updateAudioConfiguration(in AudioConfiguration audioConfig);

    /**
     * Called when the supported latency mode is updated.
     *
     * @param allowed If the peripheral devices can't keep up with low latency
     * mode, the API will be called with supported is false.
     */
    void setLowLatencyModeAllowed(in boolean allowed);

    /**
     * Validate and parse an A2DP Configuration,
     * shall be used with A2DP session types
     *
     * @param codecId Identify the codec
     * @param The configuration as defined by the A2DP's `Codec Specific
     *        Information Elements`, or `Vendor Specific Value` when CodecId
     *        format is set to `VENDOR`.
     * @param codecParameters result of parsing, when the validation succeeded.
     * @return A2DP Status of the parsing
     */
    A2dpStatus parseA2dpConfiguration(
            in CodecId codecId, in byte[] configuration, out CodecParameters codecParameters);

    /**
     * Return a configuration, from a list of remote Capabilites,
     * shall be used with A2DP session types
     *
     * @param remoteCapabilities The capabilities of the remote device
     * @param hint Hint on selection (audio context and/or codec)
     * @return The requested configuration. A null value value is returned
     *         when no suitable configuration has been found.
     */
    @nullable A2dpConfiguration getA2dpConfiguration(
            in List<A2dpRemoteCapabilities> remoteA2dpCapabilities, in A2dpConfigurationHint hint);

    /**
     * Set specific codec priority
     *
     *  It should be assumed that the external module will start with all its
     *  integrated codecs priority 0 by default.
     *
     * @param codecId:  codecId
     * @param priority: 0 for no priority, -1 for codec disabled,
     *                  from 1 to N, where 1 is highest.
     */
    void setCodecPriority(in CodecId codecId, int priority);

    /**
     * LE Audio device Capabilities - as defined in Bluetooth Published Audio
     * Capabilities Service specification, v1.0.1, Sec. 3.1: "Sink PAC", and
     * Sec. 3.3: "Source PAC".
     */
    @VintfStability
    parcelable LeAudioDeviceCapabilities {
        /**
         * Codec Identifier
         */
        CodecId codecId;
        /**
         * Codec capabilities, packed as LTV.
         */
        CodecSpecificCapabilitiesLtv[] codecSpecificCapabilities;
        /**
         * Vendor codec specific capabilities.
         *
         * This will not be parsed by the BT stack, but passed to the vendor
         * module who can interpret this and based on that select the proper
         * vendor specific codec configuration.
         */
        @nullable byte[] vendorCodecSpecificCapabilities;
        /**
         * Audio capabilities metadata, packed as LTV.
         */
        @nullable MetadataLtv[] metadata;
    }

    @VintfStability
    parcelable LeAudioDataPathConfiguration {
        /**
         * Vendor specific data path identifier
         */
        int dataPathId;

        /**
         * Used in the HCI_LE_Setup_ISO_Data_Path (0x006E).
         * As defined in Bluetooth Core Specification Version
         * 5.3, Vol 4, Part E, Sec. 7.8.109: "LE Setup ISO Data Path command".
         */
        @VintfStability
        parcelable IsoDataPathConfiguration {
            /**
             * Codec ID - Valid Codec Identifier matching the selected codec
             */
            CodecId codecId;
            /**
             * Whether the transparent air mode should be set as a coding format
             * in the HCI_LE_Setup_ISO_Data_Path command, indicating that the
             * codec is not in the controller.
             *
             * If set to true, 0x03 (transparent air mode) will be used as a
             * Codec_ID coding format and the `byte[] configuration` field shall
             * remain empty. Otherwise the Codec_ID field will be set to
             * according to BT specification (0xFF coding format, company ID,
             * codec ID for vendor codecs, or according to Codec_ID identifiers
             * defined in the Assigned Numbers for the non-vendor codecs).
             */
            boolean isTransparent;
            /**
             * Controller delay (in microseconds)
             */
            int controllerDelayUs;
            /**
             * Codec specific LE Audio ISO data path configuration
             * must be null when codec ID is 0x03 transparent
             */
            @nullable byte[] configuration;
        }

        /**
         * Used in HCI_Configure_Data_Path (0x0083)
         * As defined in Bluetooth Core Specification Version
         * 5.3, Vol 4, Part E, Sec. 7.3.101: "Configure Data Path command".
         */
        @VintfStability
        parcelable DataPathConfiguration {
            /**
             * Vendor specific data path configuration
             */
            @nullable byte[] configuration;
        }
        /**
         * Data path configuration
         */
        DataPathConfiguration dataPathConfiguration;
        /**
         * ISO data path configuration
         */
        IsoDataPathConfiguration isoDataPathConfiguration;
    }

    /* All the LeAudioAseQosConfiguration parameters are defined by the
     * Bluetooth Audio Stream Control Service specification v.1.0, Sec. 5: "ASE
     * Control Operations".
     */
    @VintfStability
    parcelable LeAudioAseQosConfiguration {
        /**
         * SDU Interval (in microseconds) used in Set CIG Parameters command and
         * Configure QoS.
         */
        int sduIntervalUs;
        /**
         * Framing used in Set CIG Parameters command and Configure QoS
         */
        Framing framing;
        /**
         * Phy used in Set CIG Parameters command and Configure QoS
         */
        Phy[] phy;
        /**
         * Max transport latency (in milliseconds) used in Set CIG Parameters
         * command and Configure QoS.
         */
        int maxTransportLatencyMs;
        /**
         * Max SDU used in Set CIG Parameters command and Configure QoS
         */
        int maxSdu;
        /**
         * Retransmission number used in Set CIG Parameters command and
         * Configure QoS
         */
        int retransmissionNum;
    }

    /**
     * Connected Isochronous Channel arrangement within the Connected
     * Isochronous Group. As defined in Bluetooth Core Specification Version
     * 5.3, Vol 4, Part E, Sec. 7.8.97.
     */
    @VintfStability
    @Backing(type="byte")
    enum Packing {
        SEQUENTIAL = 0x00,
        INTERLEAVED = 0x01,
    }

    /**
     * Isochronous Data PDU framing parameter. As defined in Bluetooth Core
     * Specification Version 5.3, Vol 4, Part E, Sec. 7.8.97.
     */
    @VintfStability
    @Backing(type="byte")
    enum Framing {
        UNFRAMED = 0x00,
        FRAMED = 0x01,
    }

    @VintfStability
    parcelable LeAudioAseConfigurationSetting {
        /**
         * Audio Context that this configuration apply to
         */
        AudioContext audioContext;
        /**
         * Sequential or interleave packing used in Set CIG Parameters command
         */
        Packing packing;

        @VintfStability
        parcelable AseDirectionConfiguration {
            /**
             * ASE configuration
             */
            LeAudioAseConfiguration aseConfiguration;
            /**
             * QoS Configuration
             */
            @nullable LeAudioAseQosConfiguration qosConfiguration;
            /**
             * Data path configuration
             * If not provided, getLeAudioAseDatapathConfiguration() will be
             * called during the configuration, increasing the stream
             * establishment time (not recommended).
             */
            @nullable LeAudioDataPathConfiguration dataPathConfiguration;
        }
        /**
         * Sink ASEs configuration
         */
        @nullable List<AseDirectionConfiguration> sinkAseConfiguration;
        /**
         * Source ASEs configuration
         */
        @nullable List<AseDirectionConfiguration> sourceAseConfiguration;
        /**
         * Additional flags, used for configurations with special features
         */
        @nullable ConfigurationFlags flags;
    }

    /**
     * ASE configuration requirements set by the BT stack.
     */
    @VintfStability
    parcelable LeAudioConfigurationRequirement {
        /**
         * Audio Contect that this requirements apply to
         */
        AudioContext audioContext;

        @VintfStability
        parcelable AseDirectionRequirement {
            /**
             * Optional ASE configurations requirements
             *
             * Note that the Host can set as many or as little parameters in
             * the `aseConfiguration.codecConfiguration` field as needed, to
             * closely or loosely specify the requirements. If any parameter
             * is not specified, the offloader can choose it freely. The
             * offloader should put all the specified parameters into the
             * `aseConfiguration.codecConfiguration` field of the returned
             * configuration to let the BT stack verify if the requirements
             * were met. The mandatory requirement set by the BT stack will be
             * the Audio Location.
             */
            LeAudioAseConfiguration aseConfiguration;
        }
        /**
         * Sink ASEs configuration setting
         */
        @nullable List<AseDirectionRequirement> sinkAseRequirement;
        /**
         * Source ASEs configuration setting
         */
        @nullable List<AseDirectionRequirement> sourceAseRequirement;
        /**
         * Additional flags, used to request configurations with special
         * features
         */
        @nullable ConfigurationFlags flags;
    }

    /**
     * Method that returns a proposed ASE configuration settings for each
     * requested audio context type
     *
     * Note: _ENCODING session provides SINK ASE configuration
     *       and _DECODING session provides SOURCE ASE configuration unless
     *       BluetoothAudioProvider sets supportsMultidirectionalCapabilities to
     *       true in ProviderInfo.
     *       If supportsMultidirectionalCapabilities is set to true then the
     *       BluetoothStack expects to get configuration list for SINK and SOURCE
     *       on either _ENCODING or _DECODING session.
     *
     * @param remoteSinkAudioCapabilities List of remote sink capabilities
     *        supported by an active group devices.
     * @param remoteSourceAudioCapabilities List of remote source capabilities
     *        supported by an active group devices.
     * @param requirements ASE configuration requirements
     *
     * @return List<LeAudioAseConfigurationSetting>
     */
    List<LeAudioAseConfigurationSetting> getLeAudioAseConfiguration(
            in @nullable List<LeAudioDeviceCapabilities> remoteSinkAudioCapabilities,
            in @nullable List<LeAudioDeviceCapabilities> remoteSourceAudioCapabilities,
            in List<LeAudioConfigurationRequirement> requirements);

    @VintfStability
    parcelable LeAudioAseQosConfigurationRequirement {
        /**
         * Audio Contect Type that this requirements apply to
         */
        AudioContext contextType;

        /**
         * QoS preferences received in Codec Configured ASE state. As defined in
         * bluetooth service specification: Audio Stream Control Service" V1.0,
         * Sec. 4.1 Audio Stream Endpoints, Table 4.3:"Additional_ASE_Parameters
         * format when ASE_State = 0x01 (Codec Configured)".
         */
        @VintfStability
        parcelable AseQosDirectionRequirement {
            /**
             * Support for unframed Isochronous Adaptation Layer PDUs.
             * When set to FRAMED, the unframed PDUs are not supported.
             */
            Framing framing;
            /**
             * Preferred value for the PHY parameter to be written by the client
             * for this ASE in the Config QoS operation
             */
            Phy[] preferredPhy;
            /**
             * Preferred value for the Retransmission Number parameter to be
             * written by the client for this ASE in the Config QoS operation.
             */
            int preferredRetransmissionNum;
            /**
             * Preferred value for the Max Transport Latency parameter to be
             * written by the client for this ASE in the Config QoS operation.
             */
            int maxTransportLatencyMs;
            /**
             * Minimum server supported Presentation Delay (in microseconds) for
             * an ASE.
             */
            int presentationDelayMinUs;
            /**
             * Maximum server supported Presentation Delay (in microseconds) for
             * an ASE.
             */
            int presentationDelayMaxUs;
            /**
             * Preferred minimum Presentation Delay (in microseconds) for an
             * ASE.
             */
            int preferredPresentationDelayMinUs;
            /**
             * Preferred maximum Presentation Delay (in microseconds) for an
             * ASE.
             */
            int preferredPresentationDelayMaxUs;

            /**
             * ASE configuration
             */
            LeAudioAseConfiguration aseConfiguration;
        }
        /**
         * Sink ASEs configuration setting
         */
        @nullable AseQosDirectionRequirement sinkAseQosRequirement;
        /**
         * Source ASEs configuration setting
         */
        @nullable AseQosDirectionRequirement sourceAseQosRequirement;
        /**
         * Additional configuration flags requirements
         */
        @nullable ConfigurationFlags flags;
    }

    /**
     * A directional pair for QoS configuration. Either one or both directions
     * can be set, depending on the audio context and the requirements provided
     * to getLeAudioAseQosConfiguration().
     */
    @VintfStability
    parcelable LeAudioAseQosConfigurationPair {
        @nullable LeAudioAseQosConfiguration sinkQosConfiguration;
        @nullable LeAudioAseQosConfiguration sourceQosConfiguration;
    }

    /**
     * Method that returns an ASE QoS configuration settings for the given ASE
     * configuration,taking an ASE preferenced QoS parameters. It should be used
     * to negotiaite the QoS parameters, when the initialy received QoS
     * parameters are not within the boundaries received from the remote device
     * after configuring the ASEs.
     *
     * @param qosRequirement ASE QoS configurations requirements
     *
     * @return LeAudioAseQosConfigurationPair
     */
    LeAudioAseQosConfigurationPair getLeAudioAseQosConfiguration(
            in LeAudioAseQosConfigurationRequirement qosRequirement);

    /**
     * Audio data path configuration.
     */
    parcelable LeAudioDataPathConfigurationPair {
        /* Host to Controller data path */
        @nullable LeAudioDataPathConfiguration inputConfig;
        /* Controller to Host data path */
        @nullable LeAudioDataPathConfiguration outputConfig;
    }

    /**
     * Used to get a data path configuration which dynamically depends on CIS
     * connection handles in StreamMap. This is used if non-dynamic data path
     * was not provided in LeAudioAseConfigurationSetting. Calling this during
     * the unicast audio stream establishment might slightly delay the stream
     * start.
     */
    LeAudioDataPathConfigurationPair getLeAudioAseDatapathConfiguration(
            in AudioContext context, in StreamMap[] streamMap);

    /*
     * Audio Stream Endpoint state used to report Metadata changes on the remote
     * device audio endpoints.
     */
    @VintfStability
    @Backing(type="byte")
    enum AseState {
        ENABLING = 0x00,
        STREAMING = 0x01,
        DISABLING = 0x02,
    }

    /**
     * Used to report metadata changes to the provider. This allows for a
     * pseudo communication channel between the remote device and the provider,
     * using the vendor specific metadata of the changing ASE state.
     * It is used only when ASE is using configurations marked with the
     * `PROVIDE_ASE_METADATA` flag.
     */
    void onSinkAseMetadataChanged(
            in AseState state, int cigId, int cisId, in @nullable MetadataLtv[] metadata);
    void onSourceAseMetadataChanged(
            in AseState state, int cigId, int cisId, in @nullable MetadataLtv[] metadata);

    /**
     * Broadcast quality index
     */
    @VintfStability
    @Backing(type="byte")
    enum BroadcastQuality {
        STANDARD,
        HIGH,
    }

    /**
     * It is used in LeAudioBroadcastConfigurationRequirement
     */
    @VintfStability
    parcelable LeAudioBroadcastSubgroupConfigurationRequirement {
        /**
         * Streaming Audio Context for the given subgroup.
         * This can serve as a hint for selecting the proper configuration by
         * the offloader.
         */
        AudioContext context;
        /**
         * Streaming Broadcast Audio Quality
         */
        BroadcastQuality quality;
        /**
         * Number of BISes for the given subgroup
         */
        int bisNumPerSubgroup;
    }

    /**
     * It is used in getLeAudioBroadcastConfiguration method
     * If any group id is provided, the Provider should check Pacs capabilities
     * of the group(s) and provide Broadcast configuration supported by the
     * group.
     */
    @VintfStability
    parcelable LeAudioBroadcastConfigurationRequirement {
        List<LeAudioBroadcastSubgroupConfigurationRequirement> subgroupConfigurationRequirements;
    }

    /**
     * Subgroup BIS configuration
     *
     */
    @VintfStability
    parcelable LeAudioSubgroupBisConfiguration {
        /**
         * The number of BISes with the given configuration
         */
        int numBis;
        /**
         * LE Audio BIS configuration for the `numBis` number of BISes
         */
        LeAudioBisConfiguration bisConfiguration;
    }

    /**
     * Subgroup configuration with a list of BIS configurations
     *
     */
    @VintfStability
    parcelable LeAudioBroadcastSubgroupConfiguration {
        List<LeAudioSubgroupBisConfiguration> bisConfigurations;

        /**
         * Vendor specific codec configuration for all the BISes inside this
         * subgroup. Only the vendor specific part is needed, since the BT stack
         * can derive the common subgroup configuration by intersecting the LTV
         * formatted configuration of every BIS inside the subgroup.
         * This will not be parsed by the BT stack but will be set as the codec
         * specific configuration for the ongoing audio stream at the subgroup
         * level of the audio announcement,The remote device will receive this
         * information when being configured for receiveing a brodcast audio
         * stream.
         */
        @nullable byte[] vendorCodecConfiguration;
    }

    /**
     * LeAudioBroadcastConfigurationSetting is a result of
     * getLeAudioBroadcastConfiguration. It is used in HCI_LE_Create_BIG command
     * and for creating the Broadcast Announcements.
     *
     */
    @VintfStability
    parcelable LeAudioBroadcastConfigurationSetting {
        /**
         * SDU Interval (in microseconds) used in LE Create BIG command
         */
        int sduIntervalUs;
        /**
         * Total number of BISes in the BIG
         */
        int numBis;
        /**
         *  Maximum size of an SDU in octets
         */
        int maxSduOctets;
        /**
         * Maximum transport latency (in milliseconds)
         */
        int maxTransportLatencyMs;
        /**
         * The number of times every PDU should be retransmitted
         */
        int retransmitionNum;
        /**
         * A list of PHYs used for transmission of PDUs of BISes in the BIG.
         */
        Phy[] phy;
        /**
         * The preferred method of arranging subevents of multiple BISes
         */
        Packing packing;
        /**
         * format for sending BIS Data PDUs
         */
        Framing framing;

        /**
         * Data path configuration
         * If not provided, getLeAudioBroadcastDatapathConfiguration() will be
         * called during the configuration, increasing the stream establishment
         * time (not recommended).
         */
        @nullable LeAudioDataPathConfiguration dataPathConfiguration;

        /**
         * A list of subgroup configurations in the broadcast.
         */
        List<LeAudioBroadcastSubgroupConfiguration> subgroupsConfigurations;
    }

    /**
     * Get Broadcast configuration. Output of this function will be used
     * in HCI_LE_Create_BIG  (0x0068) command and also to create BIG INFO
     *
     */
    LeAudioBroadcastConfigurationSetting getLeAudioBroadcastConfiguration(
            in @nullable List<LeAudioDeviceCapabilities> remoteSinkAudioCapabilities,
            in LeAudioBroadcastConfigurationRequirement requirement);

    /**
     * Used to get a data path configuration which dynamically depends on BIS
     * handles in BroadcastStreamMap. This is used if non-dynamic data path was
     * not provided in LeAudioBroadcastConfigurationSetting. Calling this during
     * the broadcast audio stream establishment might slightly delay the stream
     * start.
     */
    LeAudioDataPathConfiguration getLeAudioBroadcastDatapathConfiguration(
            in AudioContext context, in BroadcastStreamMap[] streamMap);
}
