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

#pragma once

#include <iostream>
#include <map>
#include <memory>
#include <set>

#include <aidl/android/hardware/audio/core/BnModule.h>

#include "core-impl/ChildInterface.h"
#include "core-impl/Configuration.h"
#include "core-impl/Stream.h"

namespace aidl::android::hardware::audio::core {

class Module : public BnModule {
  public:
    // This value is used for all AudioPatches and reported by all streams.
    static constexpr int32_t kLatencyMs = 10;
    enum Type : int { DEFAULT, R_SUBMIX, STUB, USB, BLUETOOTH };
    enum BtInterface : int { BTCONF, BTA2DP, BTLE };

    static std::shared_ptr<Module> createInstance(Type type);

    explicit Module(Type type) : mType(type) {}

    typedef std::tuple<std::weak_ptr<IBluetooth>, std::weak_ptr<IBluetoothA2dp>,
                       std::weak_ptr<IBluetoothLe>>
            BtProfileHandles;

  protected:
    // The vendor extension done via inheritance can override interface methods and augment
    // a call to the base implementation.

    ndk::ScopedAStatus setModuleDebug(
            const ::aidl::android::hardware::audio::core::ModuleDebug& in_debug) override;
    ndk::ScopedAStatus getTelephony(std::shared_ptr<ITelephony>* _aidl_return) override;
    ndk::ScopedAStatus getBluetooth(std::shared_ptr<IBluetooth>* _aidl_return) override;
    ndk::ScopedAStatus getBluetoothA2dp(std::shared_ptr<IBluetoothA2dp>* _aidl_return) override;
    ndk::ScopedAStatus getBluetoothLe(std::shared_ptr<IBluetoothLe>* _aidl_return) override;
    ndk::ScopedAStatus connectExternalDevice(
            const ::aidl::android::media::audio::common::AudioPort& in_templateIdAndAdditionalData,
            ::aidl::android::media::audio::common::AudioPort* _aidl_return) override;
    ndk::ScopedAStatus disconnectExternalDevice(int32_t in_portId) override;
    ndk::ScopedAStatus getAudioPatches(std::vector<AudioPatch>* _aidl_return) override;
    ndk::ScopedAStatus getAudioPort(
            int32_t in_portId,
            ::aidl::android::media::audio::common::AudioPort* _aidl_return) override;
    ndk::ScopedAStatus getAudioPortConfigs(
            std::vector<::aidl::android::media::audio::common::AudioPortConfig>* _aidl_return)
            override;
    ndk::ScopedAStatus getAudioPorts(
            std::vector<::aidl::android::media::audio::common::AudioPort>* _aidl_return) override;
    ndk::ScopedAStatus getAudioRoutes(std::vector<AudioRoute>* _aidl_return) override;
    ndk::ScopedAStatus getAudioRoutesForAudioPort(
            int32_t in_portId,
            std::vector<::aidl::android::hardware::audio::core::AudioRoute>* _aidl_return) override;
    ndk::ScopedAStatus openInputStream(
            const ::aidl::android::hardware::audio::core::IModule::OpenInputStreamArguments&
                    in_args,
            ::aidl::android::hardware::audio::core::IModule::OpenInputStreamReturn* _aidl_return)
            override;
    ndk::ScopedAStatus openOutputStream(
            const ::aidl::android::hardware::audio::core::IModule::OpenOutputStreamArguments&
                    in_args,
            ::aidl::android::hardware::audio::core::IModule::OpenOutputStreamReturn* _aidl_return)
            override;
    ndk::ScopedAStatus getSupportedPlaybackRateFactors(
            SupportedPlaybackRateFactors* _aidl_return) override;
    ndk::ScopedAStatus setAudioPatch(const AudioPatch& in_requested,
                                     AudioPatch* _aidl_return) override;
    ndk::ScopedAStatus setAudioPortConfig(
            const ::aidl::android::media::audio::common::AudioPortConfig& in_requested,
            ::aidl::android::media::audio::common::AudioPortConfig* out_suggested,
            bool* _aidl_return) override;
    ndk::ScopedAStatus resetAudioPatch(int32_t in_patchId) override;
    ndk::ScopedAStatus resetAudioPortConfig(int32_t in_portConfigId) override;
    ndk::ScopedAStatus getMasterMute(bool* _aidl_return) override;
    ndk::ScopedAStatus setMasterMute(bool in_mute) override;
    ndk::ScopedAStatus getMasterVolume(float* _aidl_return) override;
    ndk::ScopedAStatus setMasterVolume(float in_volume) override;
    ndk::ScopedAStatus getMicMute(bool* _aidl_return) override;
    ndk::ScopedAStatus setMicMute(bool in_mute) override;
    ndk::ScopedAStatus getMicrophones(
            std::vector<::aidl::android::media::audio::common::MicrophoneInfo>* _aidl_return)
            override;
    ndk::ScopedAStatus updateAudioMode(
            ::aidl::android::media::audio::common::AudioMode in_mode) override;
    ndk::ScopedAStatus updateScreenRotation(
            ::aidl::android::hardware::audio::core::IModule::ScreenRotation in_rotation) override;
    ndk::ScopedAStatus updateScreenState(bool in_isTurnedOn) override;
    ndk::ScopedAStatus getSoundDose(std::shared_ptr<sounddose::ISoundDose>* _aidl_return) override;
    ndk::ScopedAStatus generateHwAvSyncId(int32_t* _aidl_return) override;
    ndk::ScopedAStatus getVendorParameters(const std::vector<std::string>& in_ids,
                                           std::vector<VendorParameter>* _aidl_return) override;
    ndk::ScopedAStatus setVendorParameters(const std::vector<VendorParameter>& in_parameters,
                                           bool in_async) override;
    ndk::ScopedAStatus addDeviceEffect(
            int32_t in_portConfigId,
            const std::shared_ptr<::aidl::android::hardware::audio::effect::IEffect>& in_effect)
            override;
    ndk::ScopedAStatus removeDeviceEffect(
            int32_t in_portConfigId,
            const std::shared_ptr<::aidl::android::hardware::audio::effect::IEffect>& in_effect)
            override;
    ndk::ScopedAStatus getMmapPolicyInfos(
            ::aidl::android::media::audio::common::AudioMMapPolicyType mmapPolicyType,
            std::vector<::aidl::android::media::audio::common::AudioMMapPolicyInfo>* _aidl_return)
            override;
    ndk::ScopedAStatus supportsVariableLatency(bool* _aidl_return) override;
    ndk::ScopedAStatus getAAudioMixerBurstCount(int32_t* _aidl_return) override;
    ndk::ScopedAStatus getAAudioHardwareBurstMinUsec(int32_t* _aidl_return) override;

    // This value is used for all AudioPatches.
    static constexpr int32_t kMinimumStreamBufferSizeFrames = 256;
    // The maximum stream buffer size is 1 GiB = 2 ** 30 bytes;
    static constexpr int32_t kMaximumStreamBufferSizeBytes = 1 << 30;

  private:
    struct VendorDebug {
        static const std::string kForceTransientBurstName;
        static const std::string kForceSynchronousDrainName;
        bool forceTransientBurst = false;
        bool forceSynchronousDrain = false;
    };
    // ids of device ports created at runtime via 'connectExternalDevice'.
    // Also stores a list of ids of mix ports with dynamic profiles that were populated from
    // the connected port. This list can be empty, thus an int->int multimap can't be used.
    using ConnectedDevicePorts = std::map<int32_t, std::vector<int32_t>>;
    // Maps port ids and port config ids to patch ids.
    // Multimap because both ports and configs can be used by multiple patches.
    using Patches = std::multimap<int32_t, int32_t>;

    const Type mType;
    std::unique_ptr<internal::Configuration> mConfig;
    ModuleDebug mDebug;
    VendorDebug mVendorDebug;
    ConnectedDevicePorts mConnectedDevicePorts;
    Streams mStreams;
    Patches mPatches;
    bool mMicMute = false;
    bool mMasterMute = false;
    float mMasterVolume = 1.0f;
    ChildInterface<sounddose::ISoundDose> mSoundDose;
    std::optional<bool> mIsMmapSupported;

  protected:
    // The following virtual functions are intended for vendor extension via inheritance.

    virtual ndk::ScopedAStatus createInputStream(
            StreamContext&& context,
            const ::aidl::android::hardware::audio::common::SinkMetadata& sinkMetadata,
            const std::vector<::aidl::android::media::audio::common::MicrophoneInfo>& microphones,
            std::shared_ptr<StreamIn>* result) = 0;
    virtual ndk::ScopedAStatus createOutputStream(
            StreamContext&& context,
            const ::aidl::android::hardware::audio::common::SourceMetadata& sourceMetadata,
            const std::optional<::aidl::android::media::audio::common::AudioOffloadInfo>&
                    offloadInfo,
            std::shared_ptr<StreamOut>* result) = 0;
    // If the module is unable to populate the connected device port correctly, the returned error
    // code must correspond to the errors of `IModule.connectedExternalDevice` method.
    virtual ndk::ScopedAStatus populateConnectedDevicePort(
            ::aidl::android::media::audio::common::AudioPort* audioPort);
    // If the module finds that the patch endpoints configurations are not matched, the returned
    // error code must correspond to the errors of `IModule.setAudioPatch` method.
    virtual ndk::ScopedAStatus checkAudioPatchEndpointsMatch(
            const std::vector<::aidl::android::media::audio::common::AudioPortConfig*>& sources,
            const std::vector<::aidl::android::media::audio::common::AudioPortConfig*>& sinks);
    virtual void onExternalDeviceConnectionChanged(
            const ::aidl::android::media::audio::common::AudioPort& audioPort, bool connected);
    virtual ndk::ScopedAStatus onMasterMuteChanged(bool mute);
    virtual ndk::ScopedAStatus onMasterVolumeChanged(float volume);
    virtual std::unique_ptr<internal::Configuration> initializeConfig();

    // Utility and helper functions accessible to subclasses.
    ndk::ScopedAStatus bluetoothParametersUpdated();
    void cleanUpPatch(int32_t patchId);
    ndk::ScopedAStatus createStreamContext(
            int32_t in_portConfigId, int64_t in_bufferSizeFrames,
            std::shared_ptr<IStreamCallback> asyncCallback,
            std::shared_ptr<IStreamOutEventCallback> outEventCallback,
            ::aidl::android::hardware::audio::core::StreamContext* out_context);
    std::vector<::aidl::android::media::audio::common::AudioDevice> findConnectedDevices(
            int32_t portConfigId);
    std::set<int32_t> findConnectedPortConfigIds(int32_t portConfigId);
    ndk::ScopedAStatus findPortIdForNewStream(
            int32_t in_portConfigId, ::aidl::android::media::audio::common::AudioPort** port);
    virtual BtProfileHandles getBtProfileManagerHandles();
    internal::Configuration& getConfig();
    const ConnectedDevicePorts& getConnectedDevicePorts() const { return mConnectedDevicePorts; }
    bool getMasterMute() const { return mMasterMute; }
    bool getMasterVolume() const { return mMasterVolume; }
    bool getMicMute() const { return mMicMute; }
    const Patches& getPatches() const { return mPatches; }
    const Streams& getStreams() const { return mStreams; }
    Type getType() const { return mType; }
    bool isMmapSupported();
    template <typename C>
    std::set<int32_t> portIdsFromPortConfigIds(C portConfigIds);
    void registerPatch(const AudioPatch& patch);
    ndk::ScopedAStatus updateStreamsConnectedState(const AudioPatch& oldPatch,
                                                   const AudioPatch& newPatch);
};

std::ostream& operator<<(std::ostream& os, Module::Type t);

}  // namespace aidl::android::hardware::audio::core
