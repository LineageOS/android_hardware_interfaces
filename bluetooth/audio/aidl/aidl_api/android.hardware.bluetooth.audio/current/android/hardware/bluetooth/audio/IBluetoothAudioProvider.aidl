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
///////////////////////////////////////////////////////////////////////////////
// THIS FILE IS IMMUTABLE. DO NOT EDIT IN ANY CASE.                          //
///////////////////////////////////////////////////////////////////////////////

// This file is a snapshot of an AIDL file. Do not edit it manually. There are
// two cases:
// 1). this is a frozen version file - do not edit this in any case.
// 2). this is a 'current' file. If you make a backwards compatible change to
//     the interface (from the latest frozen version), the build system will
//     prompt you to update this file with `m <name>-update-api`.
//
// You must not make a backward incompatible change to any AIDL file built
// with the aidl_interface module type with versions property set. The module
// type is used to build AIDL files in a way that they can be used across
// independently updatable components of the system. If a device is shipped
// with such a backward incompatible change, it has a high risk of breaking
// later when a module using the interface is updated, e.g., Mainline modules.

package android.hardware.bluetooth.audio;
@VintfStability
interface IBluetoothAudioProvider {
  void endSession();
  android.hardware.common.fmq.MQDescriptor<byte,android.hardware.common.fmq.SynchronizedReadWrite> startSession(in android.hardware.bluetooth.audio.IBluetoothAudioPort hostIf, in android.hardware.bluetooth.audio.AudioConfiguration audioConfig, in android.hardware.bluetooth.audio.LatencyMode[] supportedLatencyModes);
  void streamStarted(in android.hardware.bluetooth.audio.BluetoothAudioStatus status);
  void streamSuspended(in android.hardware.bluetooth.audio.BluetoothAudioStatus status);
  void updateAudioConfiguration(in android.hardware.bluetooth.audio.AudioConfiguration audioConfig);
  void setLowLatencyModeAllowed(in boolean allowed);
  android.hardware.bluetooth.audio.A2dpStatus parseA2dpConfiguration(in android.hardware.bluetooth.audio.CodecId codecId, in byte[] configuration, out android.hardware.bluetooth.audio.CodecParameters codecParameters);
  @nullable android.hardware.bluetooth.audio.A2dpConfiguration getA2dpConfiguration(in List<android.hardware.bluetooth.audio.A2dpRemoteCapabilities> remoteA2dpCapabilities, in android.hardware.bluetooth.audio.A2dpConfigurationHint hint);
  void setCodecPriority(in android.hardware.bluetooth.audio.CodecId codecId, int priority);
  List<android.hardware.bluetooth.audio.IBluetoothAudioProvider.LeAudioAseConfigurationSetting> getLeAudioAseConfiguration(in @nullable List<android.hardware.bluetooth.audio.IBluetoothAudioProvider.LeAudioDeviceCapabilities> remoteSinkAudioCapabilities, in @nullable List<android.hardware.bluetooth.audio.IBluetoothAudioProvider.LeAudioDeviceCapabilities> remoteSourceAudioCapabilities, in List<android.hardware.bluetooth.audio.IBluetoothAudioProvider.LeAudioConfigurationRequirement> requirements);
  android.hardware.bluetooth.audio.IBluetoothAudioProvider.LeAudioAseQosConfigurationPair getLeAudioAseQosConfiguration(in android.hardware.bluetooth.audio.IBluetoothAudioProvider.LeAudioAseQosConfigurationRequirement qosRequirement);
  android.hardware.bluetooth.audio.IBluetoothAudioProvider.LeAudioDataPathConfigurationPair getLeAudioAseDatapathConfiguration(in android.hardware.bluetooth.audio.AudioContext context, in android.hardware.bluetooth.audio.LeAudioConfiguration.StreamMap[] streamMap);
  void onSinkAseMetadataChanged(in android.hardware.bluetooth.audio.IBluetoothAudioProvider.AseState state, int cigId, int cisId, in @nullable android.hardware.bluetooth.audio.MetadataLtv[] metadata);
  void onSourceAseMetadataChanged(in android.hardware.bluetooth.audio.IBluetoothAudioProvider.AseState state, int cigId, int cisId, in @nullable android.hardware.bluetooth.audio.MetadataLtv[] metadata);
  android.hardware.bluetooth.audio.IBluetoothAudioProvider.LeAudioBroadcastConfigurationSetting getLeAudioBroadcastConfiguration(in @nullable List<android.hardware.bluetooth.audio.IBluetoothAudioProvider.LeAudioDeviceCapabilities> remoteSinkAudioCapabilities, in android.hardware.bluetooth.audio.IBluetoothAudioProvider.LeAudioBroadcastConfigurationRequirement requirement);
  android.hardware.bluetooth.audio.IBluetoothAudioProvider.LeAudioDataPathConfiguration getLeAudioBroadcastDatapathConfiguration(in android.hardware.bluetooth.audio.AudioContext context, in android.hardware.bluetooth.audio.LeAudioBroadcastConfiguration.BroadcastStreamMap[] streamMap);
  @VintfStability
  parcelable LeAudioDeviceCapabilities {
    android.hardware.bluetooth.audio.CodecId codecId;
    android.hardware.bluetooth.audio.CodecSpecificCapabilitiesLtv[] codecSpecificCapabilities;
    @nullable byte[] vendorCodecSpecificCapabilities;
    @nullable android.hardware.bluetooth.audio.MetadataLtv[] metadata;
  }
  @VintfStability
  parcelable LeAudioDataPathConfiguration {
    int dataPathId;
    android.hardware.bluetooth.audio.IBluetoothAudioProvider.LeAudioDataPathConfiguration.DataPathConfiguration dataPathConfiguration;
    android.hardware.bluetooth.audio.IBluetoothAudioProvider.LeAudioDataPathConfiguration.IsoDataPathConfiguration isoDataPathConfiguration;
    @VintfStability
    parcelable IsoDataPathConfiguration {
      android.hardware.bluetooth.audio.CodecId codecId;
      boolean isTransparent;
      int controllerDelayUs;
      @nullable byte[] configuration;
    }
    @VintfStability
    parcelable DataPathConfiguration {
      @nullable byte[] configuration;
    }
  }
  @VintfStability
  parcelable LeAudioAseQosConfiguration {
    int sduIntervalUs;
    android.hardware.bluetooth.audio.IBluetoothAudioProvider.Framing framing;
    android.hardware.bluetooth.audio.Phy[] phy;
    int maxTransportLatencyMs;
    int maxSdu;
    int retransmissionNum;
  }
  @Backing(type="byte") @VintfStability
  enum Packing {
    SEQUENTIAL = 0x00,
    INTERLEAVED = 0x01,
  }
  @Backing(type="byte") @VintfStability
  enum Framing {
    UNFRAMED = 0x00,
    FRAMED = 0x01,
  }
  @VintfStability
  parcelable LeAudioAseConfigurationSetting {
    android.hardware.bluetooth.audio.AudioContext audioContext;
    android.hardware.bluetooth.audio.IBluetoothAudioProvider.Packing packing;
    @nullable List<android.hardware.bluetooth.audio.IBluetoothAudioProvider.LeAudioAseConfigurationSetting.AseDirectionConfiguration> sinkAseConfiguration;
    @nullable List<android.hardware.bluetooth.audio.IBluetoothAudioProvider.LeAudioAseConfigurationSetting.AseDirectionConfiguration> sourceAseConfiguration;
    @nullable android.hardware.bluetooth.audio.ConfigurationFlags flags;
    @VintfStability
    parcelable AseDirectionConfiguration {
      android.hardware.bluetooth.audio.LeAudioAseConfiguration aseConfiguration;
      @nullable android.hardware.bluetooth.audio.IBluetoothAudioProvider.LeAudioAseQosConfiguration qosConfiguration;
      @nullable android.hardware.bluetooth.audio.IBluetoothAudioProvider.LeAudioDataPathConfiguration dataPathConfiguration;
    }
  }
  @VintfStability
  parcelable LeAudioConfigurationRequirement {
    android.hardware.bluetooth.audio.AudioContext audioContext;
    @nullable List<android.hardware.bluetooth.audio.IBluetoothAudioProvider.LeAudioConfigurationRequirement.AseDirectionRequirement> sinkAseRequirement;
    @nullable List<android.hardware.bluetooth.audio.IBluetoothAudioProvider.LeAudioConfigurationRequirement.AseDirectionRequirement> sourceAseRequirement;
    @nullable android.hardware.bluetooth.audio.ConfigurationFlags flags;
    @VintfStability
    parcelable AseDirectionRequirement {
      android.hardware.bluetooth.audio.LeAudioAseConfiguration aseConfiguration;
    }
  }
  @VintfStability
  parcelable LeAudioAseQosConfigurationRequirement {
    android.hardware.bluetooth.audio.AudioContext contextType;
    @nullable android.hardware.bluetooth.audio.IBluetoothAudioProvider.LeAudioAseQosConfigurationRequirement.AseQosDirectionRequirement sinkAseQosRequirement;
    @nullable android.hardware.bluetooth.audio.IBluetoothAudioProvider.LeAudioAseQosConfigurationRequirement.AseQosDirectionRequirement sourceAseQosRequirement;
    @nullable android.hardware.bluetooth.audio.ConfigurationFlags flags;
    @VintfStability
    parcelable AseQosDirectionRequirement {
      android.hardware.bluetooth.audio.IBluetoothAudioProvider.Framing framing;
      android.hardware.bluetooth.audio.Phy[] preferredPhy;
      int preferredRetransmissionNum;
      int maxTransportLatencyMs;
      int presentationDelayMinUs;
      int presentationDelayMaxUs;
      int preferredPresentationDelayMinUs;
      int preferredPresentationDelayMaxUs;
      android.hardware.bluetooth.audio.LeAudioAseConfiguration aseConfiguration;
    }
  }
  @VintfStability
  parcelable LeAudioAseQosConfigurationPair {
    @nullable android.hardware.bluetooth.audio.IBluetoothAudioProvider.LeAudioAseQosConfiguration sinkQosConfiguration;
    @nullable android.hardware.bluetooth.audio.IBluetoothAudioProvider.LeAudioAseQosConfiguration sourceQosConfiguration;
  }
  parcelable LeAudioDataPathConfigurationPair {
    @nullable android.hardware.bluetooth.audio.IBluetoothAudioProvider.LeAudioDataPathConfiguration inputConfig;
    @nullable android.hardware.bluetooth.audio.IBluetoothAudioProvider.LeAudioDataPathConfiguration outputConfig;
  }
  @Backing(type="byte") @VintfStability
  enum AseState {
    ENABLING = 0x00,
    STREAMING = 0x01,
    DISABLING = 0x02,
  }
  @Backing(type="byte") @VintfStability
  enum BroadcastQuality {
    STANDARD,
    HIGH,
  }
  @VintfStability
  parcelable LeAudioBroadcastSubgroupConfigurationRequirement {
    android.hardware.bluetooth.audio.AudioContext context;
    android.hardware.bluetooth.audio.IBluetoothAudioProvider.BroadcastQuality quality;
    int bisNumPerSubgroup;
  }
  @VintfStability
  parcelable LeAudioBroadcastConfigurationRequirement {
    List<android.hardware.bluetooth.audio.IBluetoothAudioProvider.LeAudioBroadcastSubgroupConfigurationRequirement> subgroupConfigurationRequirements;
  }
  @VintfStability
  parcelable LeAudioSubgroupBisConfiguration {
    int numBis;
    android.hardware.bluetooth.audio.LeAudioBisConfiguration bisConfiguration;
  }
  @VintfStability
  parcelable LeAudioBroadcastSubgroupConfiguration {
    List<android.hardware.bluetooth.audio.IBluetoothAudioProvider.LeAudioSubgroupBisConfiguration> bisConfigurations;
    @nullable byte[] vendorCodecConfiguration;
  }
  @VintfStability
  parcelable LeAudioBroadcastConfigurationSetting {
    int sduIntervalUs;
    int numBis;
    int maxSduOctets;
    int maxTransportLatencyMs;
    int retransmitionNum;
    android.hardware.bluetooth.audio.Phy[] phy;
    android.hardware.bluetooth.audio.IBluetoothAudioProvider.Packing packing;
    android.hardware.bluetooth.audio.IBluetoothAudioProvider.Framing framing;
    @nullable android.hardware.bluetooth.audio.IBluetoothAudioProvider.LeAudioDataPathConfiguration dataPathConfiguration;
    List<android.hardware.bluetooth.audio.IBluetoothAudioProvider.LeAudioBroadcastSubgroupConfiguration> subgroupsConfigurations;
  }
}
