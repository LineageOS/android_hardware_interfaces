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

#include <algorithm>
#include <condition_variable>
#include <memory>
#include <mutex>
#include <optional>
#include <set>
#include <string>

#define LOG_TAG "VtsHalAudioCore"
#include <android-base/logging.h>

#include <aidl/Gtest.h>
#include <aidl/Vintf.h>
#include <android-base/properties.h>
#include <android/hardware/audio/core/IConfig.h>
#include <android/hardware/audio/core/IModule.h>
#include <android/media/audio/common/AudioIoFlags.h>
#include <android/media/audio/common/AudioOutputFlags.h>
#include <binder/IServiceManager.h>
#include <binder/ProcessState.h>

#include "ModuleConfig.h"

using namespace android;
using android::binder::Status;
using android::hardware::audio::common::PlaybackTrackMetadata;
using android::hardware::audio::common::RecordTrackMetadata;
using android::hardware::audio::common::SinkMetadata;
using android::hardware::audio::common::SourceMetadata;
using android::hardware::audio::core::AudioPatch;
using android::hardware::audio::core::AudioRoute;
using android::hardware::audio::core::IModule;
using android::hardware::audio::core::IStreamIn;
using android::hardware::audio::core::IStreamOut;
using android::hardware::audio::core::ModuleDebug;
using android::media::audio::common::AudioContentType;
using android::media::audio::common::AudioDevice;
using android::media::audio::common::AudioDeviceAddress;
using android::media::audio::common::AudioDeviceType;
using android::media::audio::common::AudioFormatType;
using android::media::audio::common::AudioIoFlags;
using android::media::audio::common::AudioOutputFlags;
using android::media::audio::common::AudioPort;
using android::media::audio::common::AudioPortConfig;
using android::media::audio::common::AudioPortDeviceExt;
using android::media::audio::common::AudioPortExt;
using android::media::audio::common::AudioSource;
using android::media::audio::common::AudioUsage;

template <typename T>
auto findById(std::vector<T>& v, int32_t id) {
    return std::find_if(v.begin(), v.end(), [&](const auto& e) { return e.id == id; });
}

template <typename C>
std::vector<int32_t> GetNonExistentIds(const C& allIds) {
    if (allIds.empty()) {
        return std::vector<int32_t>{-1, 0, 1};
    }
    std::vector<int32_t> nonExistentIds;
    nonExistentIds.push_back(*std::min_element(allIds.begin(), allIds.end()) - 1);
    nonExistentIds.push_back(*std::max_element(allIds.begin(), allIds.end()) + 1);
    return nonExistentIds;
}

AudioDeviceAddress GenerateUniqueDeviceAddress() {
    static int nextId = 1;
    // TODO: Use connection-specific ID.
    return AudioDeviceAddress::make<AudioDeviceAddress::Tag::id>(std::to_string(++nextId));
}

struct AidlDeathRecipient : IBinder::DeathRecipient {
    std::mutex mutex;
    std::condition_variable condition;
    bool fired = false;
    wp<IBinder> who;

    void binderDied(const wp<IBinder>& who) override {
        std::unique_lock<std::mutex> lock(mutex);
        fired = true;
        this->who = who;
        condition.notify_one();
    };

    bool waitForFired(int timeoutMs) {
        std::unique_lock<std::mutex> lock(mutex);
        condition.wait_for(lock, std::chrono::milliseconds(timeoutMs), [this]() { return fired; });
        return fired;
    }
};

template <typename T>
struct IsInput {
    constexpr operator bool() const;
};

template <>
constexpr IsInput<IStreamIn>::operator bool() const {
    return true;
}
template <>
constexpr IsInput<IStreamOut>::operator bool() const {
    return false;
}

class WithDebugFlags {
  public:
    WithDebugFlags() {}
    explicit WithDebugFlags(const ModuleDebug& initial) : mInitial(initial), mFlags(initial) {}
    explicit WithDebugFlags(const WithDebugFlags& initial)
        : mInitial(initial.mFlags), mFlags(initial.mFlags) {}
    ~WithDebugFlags() {
        if (mModule != nullptr) {
            Status status = mModule->setModuleDebug(mInitial);
            EXPECT_EQ(Status::EX_NONE, status.exceptionCode()) << status;
        }
    }
    void SetUp(IModule* module) {
        Status status = module->setModuleDebug(mFlags);
        ASSERT_EQ(Status::EX_NONE, status.exceptionCode()) << status;
    }
    ModuleDebug& flags() { return mFlags; }

  private:
    ModuleDebug mInitial;
    ModuleDebug mFlags;
    IModule* mModule = nullptr;
};

// For consistency, WithAudioPortConfig can start both with a non-existent
// port config, and with an existing one. Existence is determined by the
// id of the provided config. If it's not 0, then WithAudioPortConfig is
// essentially a no-op wrapper.
class WithAudioPortConfig {
  public:
    WithAudioPortConfig() {}
    explicit WithAudioPortConfig(const AudioPortConfig& config) : mInitialConfig(config) {}
    ~WithAudioPortConfig() {
        if (mModule != nullptr) {
            Status status = mModule->resetAudioPortConfig(getId());
            EXPECT_EQ(Status::EX_NONE, status.exceptionCode())
                    << status << "; port config id " << getId();
        }
    }
    void SetUp(IModule* module) {
        ASSERT_NE(AudioPortExt::Tag::unspecified, mInitialConfig.ext.getTag())
                << "config: " << mInitialConfig.toString();
        // Negotiation is allowed for device ports because the HAL module is
        // allowed to provide an empty profiles list for attached devices.
        ASSERT_NO_FATAL_FAILURE(
                SetUpImpl(module, mInitialConfig.ext.getTag() == AudioPortExt::Tag::device));
    }
    int32_t getId() const { return mConfig.id; }
    const AudioPortConfig& get() const { return mConfig; }

  private:
    void SetUpImpl(IModule* module, bool negotiate) {
        if (mInitialConfig.id == 0) {
            AudioPortConfig suggested;
            bool applied = false;
            Status status = module->setAudioPortConfig(mInitialConfig, &suggested, &applied);
            ASSERT_EQ(Status::EX_NONE, status.exceptionCode())
                    << status << "; Config: " << mInitialConfig.toString();
            if (!applied && negotiate) {
                mInitialConfig = suggested;
                ASSERT_NO_FATAL_FAILURE(SetUpImpl(module, false))
                        << " while applying suggested config: " << suggested.toString();
            } else {
                ASSERT_TRUE(applied) << "Suggested: " << suggested.toString();
                mConfig = suggested;
                mModule = module;
            }
        } else {
            mConfig = mInitialConfig;
        }
    }

    AudioPortConfig mInitialConfig;
    IModule* mModule = nullptr;
    AudioPortConfig mConfig;
};

class AudioCoreModule : public testing::TestWithParam<std::string> {
  public:
    void SetUp() override {
        ASSERT_NO_FATAL_FAILURE(ConnectToService());
        debug.flags().simulateDeviceConnections = true;
        ASSERT_NO_FATAL_FAILURE(debug.SetUp(module.get()));
    }

    void TearDown() override {
        if (module != nullptr) {
            Status status = module->setModuleDebug(ModuleDebug{});
            EXPECT_EQ(Status::EX_NONE, status.exceptionCode())
                    << status << " returned when resetting debug flags";
        }
    }

    void ConnectToService() {
        module = android::waitForDeclaredService<IModule>(String16(GetParam().c_str()));
        ASSERT_NE(module, nullptr);
    }

    void RestartService() {
        ASSERT_NE(module, nullptr);
        moduleConfig.reset();
        deathHandler = sp<AidlDeathRecipient>::make();
        ASSERT_EQ(NO_ERROR, IModule::asBinder(module)->linkToDeath(deathHandler));
        ASSERT_TRUE(base::SetProperty("sys.audio.restart.hal", "1"));
        EXPECT_TRUE(deathHandler->waitForFired(3000));
        deathHandler = nullptr;
        ASSERT_NO_FATAL_FAILURE(ConnectToService());
    }

    void ApplyEveryConfig(const std::vector<AudioPortConfig>& configs) {
        for (const auto& config : configs) {
            ASSERT_NE(0, config.portId);
            WithAudioPortConfig portConfig(config);
            ASSERT_NO_FATAL_FAILURE(portConfig.SetUp(module.get()));  // calls setAudioPortConfig
            EXPECT_EQ(config.portId, portConfig.get().portId);
            std::vector<AudioPortConfig> retrievedPortConfigs;
            Status status = module->getAudioPortConfigs(&retrievedPortConfigs);
            ASSERT_EQ(Status::EX_NONE, status.exceptionCode()) << status;
            const int32_t portConfigId = portConfig.getId();
            auto configIt = std::find_if(
                    retrievedPortConfigs.begin(), retrievedPortConfigs.end(),
                    [&portConfigId](const auto& retrConf) { return retrConf.id == portConfigId; });
            EXPECT_NE(configIt, retrievedPortConfigs.end())
                    << "Port config id returned by setAudioPortConfig: " << portConfigId
                    << " is not found in the list returned by getAudioPortConfigs";
            if (configIt != retrievedPortConfigs.end()) {
                EXPECT_EQ(portConfig.get(), *configIt)
                        << "Applied port config returned by setAudioPortConfig: "
                        << portConfig.get().toString()
                        << " is not the same as retrieved via getAudioPortConfigs: "
                        << configIt->toString();
            }
        }
    }

    template <typename Entity>
    void GetAllEntityIds(std::set<int32_t>* entityIds,
                         Status (IModule::*getter)(std::vector<Entity>*),
                         const std::string& errorMessage) {
        std::vector<Entity> entities;
        {
            Status status = (module.get()->*getter)(&entities);
            ASSERT_EQ(Status::EX_NONE, status.exceptionCode()) << status;
        }
        std::transform(entities.begin(), entities.end(),
                       std::inserter(*entityIds, entityIds->begin()),
                       [](const auto& entity) { return entity.id; });
        EXPECT_EQ(entities.size(), entityIds->size()) << errorMessage;
    }

    void GetAllPatchIds(std::set<int32_t>* patchIds) {
        return GetAllEntityIds<AudioPatch>(
                patchIds, &IModule::getAudioPatches,
                "IDs of audio patches returned by IModule.getAudioPatches are not unique");
    }

    void GetAllPortIds(std::set<int32_t>* portIds) {
        return GetAllEntityIds<AudioPort>(
                portIds, &IModule::getAudioPorts,
                "IDs of audio ports returned by IModule.getAudioPorts are not unique");
    }

    void GetAllPortConfigIds(std::set<int32_t>* portConfigIds) {
        return GetAllEntityIds<AudioPortConfig>(
                portConfigIds, &IModule::getAudioPortConfigs,
                "IDs of audio port configs returned by IModule.getAudioPortConfigs are not unique");
    }

    void SetUpModuleConfig() {
        if (moduleConfig == nullptr) {
            moduleConfig = std::make_unique<ModuleConfig>(module.get());
            ASSERT_EQ(Status::EX_NONE, moduleConfig->getStatus().exceptionCode())
                    << "ModuleConfig init error: " << moduleConfig->getError();
        }
    }

    sp<IModule> module;
    sp<AidlDeathRecipient> deathHandler;
    std::unique_ptr<ModuleConfig> moduleConfig;
    WithDebugFlags debug;
};

class WithDevicePortConnectedState {
  public:
    explicit WithDevicePortConnectedState(const AudioPort& idAndData) : mIdAndData(idAndData) {}
    WithDevicePortConnectedState(const AudioPort& id, const AudioDeviceAddress& address)
        : mIdAndData(setAudioPortAddress(id, address)) {}
    ~WithDevicePortConnectedState() {
        if (mModule != nullptr) {
            Status status = mModule->disconnectExternalDevice(getId());
            EXPECT_EQ(Status::EX_NONE, status.exceptionCode())
                    << status << " returned when disconnecting device port ID " << getId();
        }
    }
    void SetUp(IModule* module) {
        Status status = module->connectExternalDevice(mIdAndData, &mConnectedPort);
        ASSERT_EQ(Status::EX_NONE, status.exceptionCode())
                << status << " returned when connecting device port ID & data "
                << mIdAndData.toString();
        ASSERT_NE(mIdAndData.id, getId())
                << "ID of the connected port must not be the same as the ID of the template port";
        mModule = module;
    }
    int32_t getId() const { return mConnectedPort.id; }
    const AudioPort& get() { return mConnectedPort; }

  private:
    static AudioPort setAudioPortAddress(const AudioPort& id, const AudioDeviceAddress& address) {
        AudioPort result = id;
        result.ext.get<AudioPortExt::Tag::device>().device.address = address;
        return result;
    }

    const AudioPort mIdAndData;
    IModule* mModule = nullptr;
    AudioPort mConnectedPort;
};

template <typename Stream>
class WithStream {
  public:
    WithStream() {}
    explicit WithStream(const AudioPortConfig& portConfig) : mPortConfig(portConfig) {}
    ~WithStream() {
        if (mStream != nullptr) {
            Status status = mStream->close();
            EXPECT_EQ(Status::EX_NONE, status.exceptionCode())
                    << status << "; port config id " << getPortId();
        }
    }
    void SetUpPortConfig(IModule* module) { ASSERT_NO_FATAL_FAILURE(mPortConfig.SetUp(module)); }
    Status SetUpNoChecks(IModule* module) { return SetUpNoChecks(module, mPortConfig.get()); }
    Status SetUpNoChecks(IModule* module, const AudioPortConfig& portConfig);
    void SetUp(IModule* module) {
        ASSERT_NO_FATAL_FAILURE(SetUpPortConfig(module));
        Status status = SetUpNoChecks(module);
        ASSERT_EQ(Status::EX_NONE, status.exceptionCode())
                << status << "; port config id " << getPortId();
        ASSERT_NE(nullptr, mStream) << "; port config id " << getPortId();
    }
    Stream* get() const { return mStream.get(); }
    const AudioPortConfig& getPortConfig() const { return mPortConfig.get(); }
    int32_t getPortId() const { return mPortConfig.getId(); }

  private:
    WithAudioPortConfig mPortConfig;
    sp<Stream> mStream;
};

template <>
Status WithStream<IStreamIn>::SetUpNoChecks(IModule* module, const AudioPortConfig& portConfig) {
    RecordTrackMetadata trackMeta;
    trackMeta.source = AudioSource::MIC;
    trackMeta.gain = 1.0;
    trackMeta.channelMask = portConfig.channelMask.value();
    SinkMetadata metadata;
    metadata.tracks.push_back(trackMeta);
    return module->openInputStream(portConfig.id, metadata, &mStream);
}

template <>
Status WithStream<IStreamOut>::SetUpNoChecks(IModule* module, const AudioPortConfig& portConfig) {
    PlaybackTrackMetadata trackMeta;
    trackMeta.usage = AudioUsage::MEDIA;
    trackMeta.contentType = AudioContentType::MUSIC;
    trackMeta.gain = 1.0;
    trackMeta.channelMask = portConfig.channelMask.value();
    SourceMetadata metadata;
    metadata.tracks.push_back(trackMeta);
    return module->openOutputStream(portConfig.id, metadata, {}, &mStream);
}

class WithAudioPatch {
  public:
    WithAudioPatch() {}
    WithAudioPatch(const AudioPortConfig& srcPortConfig, const AudioPortConfig& sinkPortConfig)
        : mSrcPortConfig(srcPortConfig), mSinkPortConfig(sinkPortConfig) {}
    ~WithAudioPatch() {
        if (mModule != nullptr && mPatch.id != 0) {
            Status status = mModule->resetAudioPatch(mPatch.id);
            EXPECT_EQ(Status::EX_NONE, status.exceptionCode())
                    << status << "; patch id " << getId();
        }
    }
    void SetUpPortConfigs(IModule* module) {
        ASSERT_NO_FATAL_FAILURE(mSrcPortConfig.SetUp(module));
        ASSERT_NO_FATAL_FAILURE(mSinkPortConfig.SetUp(module));
    }
    Status SetUpNoChecks(IModule* module) {
        mModule = module;
        mPatch.sourcePortConfigIds = std::vector<int32_t>{mSrcPortConfig.getId()};
        mPatch.sinkPortConfigIds = std::vector<int32_t>{mSinkPortConfig.getId()};
        return mModule->setAudioPatch(mPatch, &mPatch);
    }
    void SetUp(IModule* module) {
        ASSERT_NO_FATAL_FAILURE(SetUpPortConfigs(module));
        Status status = SetUpNoChecks(module);
        ASSERT_EQ(Status::EX_NONE, status.exceptionCode())
                << status << "; source port config id " << mSrcPortConfig.getId()
                << "; sink port config id " << mSinkPortConfig.getId();
    }
    int32_t getId() const { return mPatch.id; }
    const AudioPatch& get() const { return mPatch; }

  private:
    WithAudioPortConfig mSrcPortConfig;
    WithAudioPortConfig mSinkPortConfig;
    IModule* mModule = nullptr;
    AudioPatch mPatch;
};

TEST_P(AudioCoreModule, Published) {
    // SetUp must complete with no failures.
}

TEST_P(AudioCoreModule, CanBeRestarted) {
    ASSERT_NO_FATAL_FAILURE(RestartService());
}

TEST_P(AudioCoreModule, PortIdsAreUnique) {
    std::set<int32_t> portIds;
    ASSERT_NO_FATAL_FAILURE(GetAllPortIds(&portIds));
}

TEST_P(AudioCoreModule, GetAudioPortsIsStable) {
    std::vector<AudioPort> ports1;
    {
        Status status = module->getAudioPorts(&ports1);
        ASSERT_EQ(Status::EX_NONE, status.exceptionCode()) << status;
    }
    std::vector<AudioPort> ports2;
    {
        Status status = module->getAudioPorts(&ports2);
        ASSERT_EQ(Status::EX_NONE, status.exceptionCode()) << status;
    }
    ASSERT_EQ(ports1.size(), ports2.size())
            << "Sizes of audio port arrays do not match across consequent calls to getAudioPorts";
    std::sort(ports1.begin(), ports1.end());
    std::sort(ports2.begin(), ports2.end());
    EXPECT_EQ(ports1, ports2);
}

TEST_P(AudioCoreModule, GetAudioRoutesIsStable) {
    std::vector<AudioRoute> routes1;
    {
        Status status = module->getAudioRoutes(&routes1);
        ASSERT_EQ(Status::EX_NONE, status.exceptionCode()) << status;
    }
    std::vector<AudioRoute> routes2;
    {
        Status status = module->getAudioRoutes(&routes2);
        ASSERT_EQ(Status::EX_NONE, status.exceptionCode()) << status;
    }
    ASSERT_EQ(routes1.size(), routes2.size())
            << "Sizes of audio route arrays do not match across consequent calls to getAudioRoutes";
    std::sort(routes1.begin(), routes1.end());
    std::sort(routes2.begin(), routes2.end());
    EXPECT_EQ(routes1, routes2);
}

TEST_P(AudioCoreModule, GetAudioRoutesAreValid) {
    std::vector<AudioRoute> routes;
    {
        Status status = module->getAudioRoutes(&routes);
        ASSERT_EQ(Status::EX_NONE, status.exceptionCode()) << status;
    }
    for (const auto& route : routes) {
        std::set<int32_t> sources(route.sourcePortIds.begin(), route.sourcePortIds.end());
        EXPECT_NE(0, sources.size())
                << "empty audio port sinks in the audio route: " << route.toString();
        EXPECT_EQ(sources.size(), route.sourcePortIds.size())
                << "IDs of audio port sinks are not unique in the audio route: "
                << route.toString();
    }
}

TEST_P(AudioCoreModule, GetAudioRoutesPortIdsAreValid) {
    std::set<int32_t> portIds;
    ASSERT_NO_FATAL_FAILURE(GetAllPortIds(&portIds));
    std::vector<AudioRoute> routes;
    {
        Status status = module->getAudioRoutes(&routes);
        ASSERT_EQ(Status::EX_NONE, status.exceptionCode()) << status;
    }
    for (const auto& route : routes) {
        EXPECT_EQ(1, portIds.count(route.sinkPortId))
                << route.sinkPortId << " sink port id is unknown";
        for (const auto& source : route.sourcePortIds) {
            EXPECT_EQ(1, portIds.count(source)) << source << " source port id is unknown";
        }
    }
}

TEST_P(AudioCoreModule, GetAudioRoutesForAudioPort) {
    std::set<int32_t> portIds;
    ASSERT_NO_FATAL_FAILURE(GetAllPortIds(&portIds));
    if (portIds.empty()) {
        GTEST_SKIP() << "No ports in the module.";
    }
    for (const auto portId : portIds) {
        std::vector<AudioRoute> routes;
        Status status = module->getAudioRoutesForAudioPort(portId, &routes);
        EXPECT_EQ(Status::EX_NONE, status.exceptionCode()) << status;
        for (const auto& r : routes) {
            if (r.sinkPortId != portId) {
                const auto& srcs = r.sourcePortIds;
                EXPECT_TRUE(std::find(srcs.begin(), srcs.end(), portId) != srcs.end())
                        << " port ID " << portId << " does not used by the route " << r.toString();
            }
        }
    }
    for (const auto portId : GetNonExistentIds(portIds)) {
        std::vector<AudioRoute> routes;
        Status status = module->getAudioRoutesForAudioPort(portId, &routes);
        EXPECT_EQ(Status::EX_ILLEGAL_ARGUMENT, status.exceptionCode())
                << status << " returned for port ID " << portId;
    }
}

TEST_P(AudioCoreModule, CheckDevicePorts) {
    std::vector<AudioPort> ports;
    {
        Status status = module->getAudioPorts(&ports);
        ASSERT_EQ(Status::EX_NONE, status.exceptionCode()) << status;
    }
    std::optional<int32_t> defaultOutput, defaultInput;
    std::set<AudioDevice> inputs, outputs;
    const int defaultDeviceFlag = 1 << AudioPortDeviceExt::FLAG_INDEX_DEFAULT_DEVICE;
    for (const auto& port : ports) {
        if (port.ext.getTag() != AudioPortExt::Tag::device) continue;
        const auto& devicePort = port.ext.get<AudioPortExt::Tag::device>();
        EXPECT_NE(AudioDeviceType::NONE, devicePort.device.type.type);
        EXPECT_NE(AudioDeviceType::IN_DEFAULT, devicePort.device.type.type);
        EXPECT_NE(AudioDeviceType::OUT_DEFAULT, devicePort.device.type.type);
        if (devicePort.device.type.type > AudioDeviceType::IN_DEFAULT &&
            devicePort.device.type.type < AudioDeviceType::OUT_DEFAULT) {
            EXPECT_EQ(AudioIoFlags::Tag::input, port.flags.getTag());
        } else if (devicePort.device.type.type > AudioDeviceType::OUT_DEFAULT) {
            EXPECT_EQ(AudioIoFlags::Tag::output, port.flags.getTag());
        }
        EXPECT_FALSE((devicePort.flags & defaultDeviceFlag) != 0 &&
                     !devicePort.device.type.connection.empty())
                << "Device port " << port.id
                << " must be permanently attached to be set as default";
        if ((devicePort.flags & defaultDeviceFlag) != 0) {
            if (port.flags.getTag() == AudioIoFlags::Tag::output) {
                EXPECT_FALSE(defaultOutput.has_value())
                        << "At least two output device ports are declared as default: "
                        << defaultOutput.value() << " and " << port.id;
                defaultOutput = port.id;
                EXPECT_EQ(0, outputs.count(devicePort.device))
                        << "Non-unique output device: " << devicePort.device.toString();
                outputs.insert(devicePort.device);
            } else if (port.flags.getTag() == AudioIoFlags::Tag::input) {
                EXPECT_FALSE(defaultInput.has_value())
                        << "At least two input device ports are declared as default: "
                        << defaultInput.value() << " and " << port.id;
                defaultInput = port.id;
                EXPECT_EQ(0, inputs.count(devicePort.device))
                        << "Non-unique input device: " << devicePort.device.toString();
                inputs.insert(devicePort.device);
            } else {
                FAIL() << "Invalid AudioIoFlags Tag: " << toString(port.flags.getTag());
            }
        }
    }
}

TEST_P(AudioCoreModule, CheckMixPorts) {
    std::vector<AudioPort> ports;
    {
        Status status = module->getAudioPorts(&ports);
        ASSERT_EQ(Status::EX_NONE, status.exceptionCode()) << status;
    }
    std::optional<int32_t> primaryMixPort;
    constexpr int primaryOutputFlag = 1 << static_cast<int>(AudioOutputFlags::PRIMARY);
    for (const auto& port : ports) {
        if (port.ext.getTag() != AudioPortExt::Tag::mix) continue;
        const auto& mixPort = port.ext.get<AudioPortExt::Tag::mix>();
        if (port.flags.getTag() == AudioIoFlags::Tag::output &&
            ((port.flags.get<AudioIoFlags::Tag::output>() & primaryOutputFlag) != 0)) {
            EXPECT_FALSE(primaryMixPort.has_value())
                    << "At least two mix ports have PRIMARY flag set: " << primaryMixPort.value()
                    << " and " << port.id;
            primaryMixPort = port.id;
            EXPECT_EQ(1, mixPort.maxOpenStreamCount)
                    << "Primary mix port " << port.id << " can not have maxOpenStreamCount "
                    << mixPort.maxOpenStreamCount;
        }
    }
}

TEST_P(AudioCoreModule, GetAudioPort) {
    std::set<int32_t> portIds;
    ASSERT_NO_FATAL_FAILURE(GetAllPortIds(&portIds));
    if (portIds.empty()) {
        GTEST_SKIP() << "No ports in the module.";
    }
    for (const auto portId : portIds) {
        AudioPort port;
        Status status = module->getAudioPort(portId, &port);
        EXPECT_EQ(Status::EX_NONE, status.exceptionCode()) << status;
        EXPECT_EQ(portId, port.id);
    }
    for (const auto portId : GetNonExistentIds(portIds)) {
        AudioPort port;
        Status status = module->getAudioPort(portId, &port);
        EXPECT_EQ(Status::EX_ILLEGAL_ARGUMENT, status.exceptionCode())
                << status << " returned for port ID " << portId;
    }
}

// Verify that HAL module reports for a connected device port at least one non-dynamic profile,
// that is, a profile with actual supported configuration.
// Note: This test relies on simulation of external device connections by the HAL module.
TEST_P(AudioCoreModule, GetAudioPortWithExternalDevices) {
    ASSERT_NO_FATAL_FAILURE(SetUpModuleConfig());
    std::vector<AudioPort> ports = moduleConfig->getExternalDevicePorts();
    if (ports.empty()) {
        GTEST_SKIP() << "No external devices in the module.";
    }
    for (const auto& port : ports) {
        AudioPort portWithData = port;
        portWithData.ext.get<AudioPortExt::Tag::device>().device.address =
                GenerateUniqueDeviceAddress();
        WithDevicePortConnectedState portConnected(portWithData);
        ASSERT_NO_FATAL_FAILURE(portConnected.SetUp(module.get()));
        const int32_t connectedPortId = portConnected.getId();
        ASSERT_NE(portWithData.id, connectedPortId);
        ASSERT_EQ(portWithData.ext.getTag(), portConnected.get().ext.getTag());
        EXPECT_EQ(portWithData.ext.get<AudioPortExt::Tag::device>().device,
                  portConnected.get().ext.get<AudioPortExt::Tag::device>().device);
        // Verify that 'getAudioPort' and 'getAudioPorts' return the same connected port.
        AudioPort connectedPort;
        Status status = module->getAudioPort(connectedPortId, &connectedPort);
        EXPECT_EQ(Status::EX_NONE, status.exceptionCode())
                << status << " returned for getAudioPort port ID " << connectedPortId;
        EXPECT_EQ(portConnected.get(), connectedPort);
        const auto& portProfiles = connectedPort.profiles;
        EXPECT_NE(0, portProfiles.size())
                << "Connected port has no profiles: " << connectedPort.toString();
        const auto dynamicProfileIt =
                std::find_if(portProfiles.begin(), portProfiles.end(), [](const auto& profile) {
                    return profile.format.type == AudioFormatType::DEFAULT;
                });
        EXPECT_EQ(portProfiles.end(), dynamicProfileIt) << "Connected port contains dynamic "
                                                        << "profiles: " << connectedPort.toString();

        std::vector<AudioPort> allPorts;
        {
            Status status = module->getAudioPorts(&allPorts);
            ASSERT_EQ(Status::EX_NONE, status.exceptionCode()) << status;
        }
        const auto allPortsIt = findById(allPorts, connectedPortId);
        EXPECT_NE(allPorts.end(), allPortsIt);
        if (allPortsIt != allPorts.end()) {
            EXPECT_EQ(portConnected.get(), *allPortsIt);
        }
    }
}

TEST_P(AudioCoreModule, OpenStreamInvalidPortConfigId) {
    std::set<int32_t> portConfigIds;
    ASSERT_NO_FATAL_FAILURE(GetAllPortConfigIds(&portConfigIds));
    for (const auto portConfigId : GetNonExistentIds(portConfigIds)) {
        {
            sp<IStreamIn> stream;
            Status status = module->openInputStream(portConfigId, {}, &stream);
            EXPECT_EQ(Status::EX_ILLEGAL_ARGUMENT, status.exceptionCode())
                    << status << " openInputStream returned for port config ID " << portConfigId;
            EXPECT_EQ(nullptr, stream);
        }
        {
            sp<IStreamOut> stream;
            Status status = module->openOutputStream(portConfigId, {}, {}, &stream);
            EXPECT_EQ(Status::EX_ILLEGAL_ARGUMENT, status.exceptionCode())
                    << status << " openOutputStream returned for port config ID " << portConfigId;
            EXPECT_EQ(nullptr, stream);
        }
    }
}

TEST_P(AudioCoreModule, PortConfigIdsAreUnique) {
    std::set<int32_t> portConfigIds;
    ASSERT_NO_FATAL_FAILURE(GetAllPortConfigIds(&portConfigIds));
}

TEST_P(AudioCoreModule, PortConfigPortIdsAreValid) {
    std::set<int32_t> portIds;
    ASSERT_NO_FATAL_FAILURE(GetAllPortIds(&portIds));
    std::vector<AudioPortConfig> portConfigs;
    {
        Status status = module->getAudioPortConfigs(&portConfigs);
        ASSERT_EQ(Status::EX_NONE, status.exceptionCode()) << status;
    }
    for (const auto& config : portConfigs) {
        EXPECT_EQ(1, portIds.count(config.portId))
                << config.portId << " port id is unknown, config id " << config.id;
    }
}

TEST_P(AudioCoreModule, ResetAudioPortConfigInvalidId) {
    std::set<int32_t> portConfigIds;
    ASSERT_NO_FATAL_FAILURE(GetAllPortConfigIds(&portConfigIds));
    for (const auto portConfigId : GetNonExistentIds(portConfigIds)) {
        Status status = module->resetAudioPortConfig(portConfigId);
        EXPECT_EQ(Status::EX_ILLEGAL_ARGUMENT, status.exceptionCode())
                << status << " returned for port config ID " << portConfigId;
    }
}

// Verify that for the audio port configs provided by the HAL after init, resetting
// the config does not delete it, but brings it back to the initial config.
TEST_P(AudioCoreModule, ResetAudioPortConfigToInitialValue) {
    std::vector<AudioPortConfig> portConfigsBefore;
    {
        Status status = module->getAudioPortConfigs(&portConfigsBefore);
        ASSERT_EQ(Status::EX_NONE, status.exceptionCode()) << status;
    }
    // TODO: Change port configs according to port profiles.
    for (const auto& c : portConfigsBefore) {
        Status status = module->resetAudioPortConfig(c.id);
        EXPECT_EQ(Status::EX_NONE, status.exceptionCode())
                << status << " returned for port config ID " << c.id;
    }
    std::vector<AudioPortConfig> portConfigsAfter;
    {
        Status status = module->getAudioPortConfigs(&portConfigsAfter);
        ASSERT_EQ(Status::EX_NONE, status.exceptionCode()) << status;
    }
    for (const auto& c : portConfigsBefore) {
        auto afterIt = findById<AudioPortConfig>(portConfigsAfter, c.id);
        EXPECT_NE(portConfigsAfter.end(), afterIt)
                << " port config ID " << c.id << " was removed by reset";
        if (afterIt != portConfigsAfter.end()) {
            EXPECT_EQ(c, *afterIt);
        }
    }
}

TEST_P(AudioCoreModule, SetAudioPortConfigSuggestedConfig) {
    ASSERT_NO_FATAL_FAILURE(SetUpModuleConfig());
    auto srcMixPort = moduleConfig->getSourceMixPortForAttachedDevice();
    if (!srcMixPort.has_value()) {
        GTEST_SKIP() << "No mix port for attached output devices";
    }
    AudioPortConfig portConfig;
    AudioPortConfig suggestedConfig;
    portConfig.portId = srcMixPort.value().id;
    {
        bool applied = true;
        Status status = module->setAudioPortConfig(portConfig, &suggestedConfig, &applied);
        ASSERT_EQ(Status::EX_NONE, status.exceptionCode())
                << status << "; Config: " << portConfig.toString();
        EXPECT_FALSE(applied);
    }
    EXPECT_EQ(0, suggestedConfig.id);
    EXPECT_TRUE(suggestedConfig.sampleRate.has_value());
    EXPECT_TRUE(suggestedConfig.channelMask.has_value());
    EXPECT_TRUE(suggestedConfig.format.has_value());
    EXPECT_TRUE(suggestedConfig.flags.has_value());
    WithAudioPortConfig applied(suggestedConfig);
    ASSERT_NO_FATAL_FAILURE(applied.SetUp(module.get()));
    const AudioPortConfig& appliedConfig = applied.get();
    EXPECT_NE(0, appliedConfig.id);
    EXPECT_TRUE(appliedConfig.sampleRate.has_value());
    EXPECT_EQ(suggestedConfig.sampleRate.value(), appliedConfig.sampleRate.value());
    EXPECT_TRUE(appliedConfig.channelMask.has_value());
    EXPECT_EQ(suggestedConfig.channelMask.value(), appliedConfig.channelMask.value());
    EXPECT_TRUE(appliedConfig.format.has_value());
    EXPECT_EQ(suggestedConfig.format.value(), appliedConfig.format.value());
    EXPECT_TRUE(appliedConfig.flags.has_value());
    EXPECT_EQ(suggestedConfig.flags.value(), appliedConfig.flags.value());
}

TEST_P(AudioCoreModule, SetAllAttachedDevicePortConfigs) {
    ASSERT_NO_FATAL_FAILURE(SetUpModuleConfig());
    ASSERT_NO_FATAL_FAILURE(ApplyEveryConfig(moduleConfig->getPortConfigsForAttachedDevicePorts()));
}

// Note: This test relies on simulation of external device connections by the HAL module.
TEST_P(AudioCoreModule, SetAllExternalDevicePortConfigs) {
    ASSERT_NO_FATAL_FAILURE(SetUpModuleConfig());
    std::vector<AudioPort> ports = moduleConfig->getExternalDevicePorts();
    if (ports.empty()) {
        GTEST_SKIP() << "No external devices in the module.";
    }
    for (const auto& port : ports) {
        WithDevicePortConnectedState portConnected(port, GenerateUniqueDeviceAddress());
        ASSERT_NO_FATAL_FAILURE(portConnected.SetUp(module.get()));
        ASSERT_NO_FATAL_FAILURE(
                ApplyEveryConfig(moduleConfig->getPortConfigsForDevicePort(portConnected.get())));
    }
}

TEST_P(AudioCoreModule, SetAllStaticAudioPortConfigs) {
    ASSERT_NO_FATAL_FAILURE(SetUpModuleConfig());
    ASSERT_NO_FATAL_FAILURE(ApplyEveryConfig(moduleConfig->getPortConfigsForMixPorts()));
}

TEST_P(AudioCoreModule, SetAudioPortConfigInvalidPortId) {
    std::set<int32_t> portIds;
    ASSERT_NO_FATAL_FAILURE(GetAllPortIds(&portIds));
    for (const auto portId : GetNonExistentIds(portIds)) {
        AudioPortConfig portConfig, suggestedConfig;
        bool applied;
        portConfig.portId = portId;
        Status status = module->setAudioPortConfig(portConfig, &suggestedConfig, &applied);
        EXPECT_EQ(Status::EX_ILLEGAL_ARGUMENT, status.exceptionCode())
                << status << " returned for port ID " << portId;
        EXPECT_FALSE(suggestedConfig.format.has_value());
        EXPECT_FALSE(suggestedConfig.channelMask.has_value());
        EXPECT_FALSE(suggestedConfig.sampleRate.has_value());
    }
}

TEST_P(AudioCoreModule, SetAudioPortConfigInvalidPortConfigId) {
    std::set<int32_t> portConfigIds;
    ASSERT_NO_FATAL_FAILURE(GetAllPortConfigIds(&portConfigIds));
    for (const auto portConfigId : GetNonExistentIds(portConfigIds)) {
        AudioPortConfig portConfig, suggestedConfig;
        bool applied;
        portConfig.id = portConfigId;
        Status status = module->setAudioPortConfig(portConfig, &suggestedConfig, &applied);
        EXPECT_EQ(Status::EX_ILLEGAL_ARGUMENT, status.exceptionCode())
                << status << " returned for port config ID " << portConfigId;
        EXPECT_FALSE(suggestedConfig.format.has_value());
        EXPECT_FALSE(suggestedConfig.channelMask.has_value());
        EXPECT_FALSE(suggestedConfig.sampleRate.has_value());
    }
}

TEST_P(AudioCoreModule, TryConnectMissingDevice) {
    ASSERT_NO_FATAL_FAILURE(SetUpModuleConfig());
    std::vector<AudioPort> ports = moduleConfig->getExternalDevicePorts();
    if (ports.empty()) {
        GTEST_SKIP() << "No external devices in the module.";
    }
    AudioPort ignored;
    WithDebugFlags doNotSimulateConnections(debug);
    doNotSimulateConnections.flags().simulateDeviceConnections = false;
    ASSERT_NO_FATAL_FAILURE(doNotSimulateConnections.SetUp(module.get()));
    for (const auto& port : ports) {
        AudioPort portWithData = port;
        portWithData.ext.get<AudioPortExt::Tag::device>().device.address =
                GenerateUniqueDeviceAddress();
        Status status = module->connectExternalDevice(portWithData, &ignored);
        EXPECT_EQ(Status::EX_ILLEGAL_STATE, status.exceptionCode())
                << status << " returned for static port " << portWithData.toString();
    }
}

TEST_P(AudioCoreModule, TryChangingConnectionSimulationMidway) {
    ASSERT_NO_FATAL_FAILURE(SetUpModuleConfig());
    std::vector<AudioPort> ports = moduleConfig->getExternalDevicePorts();
    if (ports.empty()) {
        GTEST_SKIP() << "No external devices in the module.";
    }
    WithDevicePortConnectedState portConnected(*ports.begin(), GenerateUniqueDeviceAddress());
    ASSERT_NO_FATAL_FAILURE(portConnected.SetUp(module.get()));
    ModuleDebug midwayDebugChange = debug.flags();
    midwayDebugChange.simulateDeviceConnections = false;
    Status status = module->setModuleDebug(midwayDebugChange);
    EXPECT_EQ(Status::EX_ILLEGAL_STATE, status.exceptionCode())
            << status << " returned when trying to disable connections simulation "
            << "while having a connected device";
}

TEST_P(AudioCoreModule, ConnectDisconnectExternalDeviceInvalidPorts) {
    AudioPort ignored;
    std::set<int32_t> portIds;
    ASSERT_NO_FATAL_FAILURE(GetAllPortIds(&portIds));
    for (const auto portId : GetNonExistentIds(portIds)) {
        AudioPort invalidPort;
        invalidPort.id = portId;
        Status status = module->connectExternalDevice(invalidPort, &ignored);
        EXPECT_EQ(Status::EX_ILLEGAL_ARGUMENT, status.exceptionCode())
                << status << " returned for port ID " << portId << " when setting CONNECTED state";
        status = module->disconnectExternalDevice(portId);
        EXPECT_EQ(Status::EX_ILLEGAL_ARGUMENT, status.exceptionCode())
                << status << " returned for port ID " << portId
                << " when setting DISCONNECTED state";
    }

    std::vector<AudioPort> ports;
    {
        Status status = module->getAudioPorts(&ports);
        ASSERT_EQ(Status::EX_NONE, status.exceptionCode()) << status;
    }
    for (const auto& port : ports) {
        if (port.ext.getTag() != AudioPortExt::Tag::device) {
            Status status = module->connectExternalDevice(port, &ignored);
            EXPECT_EQ(Status::EX_ILLEGAL_ARGUMENT, status.exceptionCode())
                    << status << " returned for non-device port ID " << port.id
                    << " when setting CONNECTED state";
            status = module->disconnectExternalDevice(port.id);
            EXPECT_EQ(Status::EX_ILLEGAL_ARGUMENT, status.exceptionCode())
                    << status << " returned for non-device port ID " << port.id
                    << " when setting DISCONNECTED state";
        } else {
            const auto& devicePort = port.ext.get<AudioPortExt::Tag::device>();
            if (devicePort.device.type.connection.empty()) {
                Status status = module->connectExternalDevice(port, &ignored);
                EXPECT_EQ(Status::EX_ILLEGAL_ARGUMENT, status.exceptionCode())
                        << status << " returned for permanently attached device port ID " << port.id
                        << " when setting CONNECTED state";
                status = module->disconnectExternalDevice(port.id);
                EXPECT_EQ(Status::EX_ILLEGAL_ARGUMENT, status.exceptionCode())
                        << status << " returned for permanently attached device port ID " << port.id
                        << " when setting DISCONNECTED state";
            }
        }
    }
}

// Note: This test relies on simulation of external device connections by the HAL module.
TEST_P(AudioCoreModule, ConnectDisconnectExternalDeviceTwice) {
    ASSERT_NO_FATAL_FAILURE(SetUpModuleConfig());
    AudioPort ignored;
    std::vector<AudioPort> ports = moduleConfig->getExternalDevicePorts();
    if (ports.empty()) {
        GTEST_SKIP() << "No external devices in the module.";
    }
    for (const auto& port : ports) {
        Status status = module->disconnectExternalDevice(port.id);
        EXPECT_EQ(Status::EX_ILLEGAL_ARGUMENT, status.exceptionCode())
                << status << " returned when disconnecting already disconnected device port ID "
                << port.id;
        AudioPort portWithData = port;
        portWithData.ext.get<AudioPortExt::Tag::device>().device.address =
                GenerateUniqueDeviceAddress();
        WithDevicePortConnectedState portConnected(portWithData);
        ASSERT_NO_FATAL_FAILURE(portConnected.SetUp(module.get()));
        status = module->connectExternalDevice(portConnected.get(), &ignored);
        EXPECT_EQ(Status::EX_ILLEGAL_ARGUMENT, status.exceptionCode())
                << status << " returned when trying to connect a connected device port "
                << portConnected.get().toString();
        status = module->connectExternalDevice(portWithData, &ignored);
        EXPECT_EQ(Status::EX_ILLEGAL_STATE, status.exceptionCode())
                << status << " returned when connecting again the external device "
                << portWithData.ext.get<AudioPortExt::Tag::device>().device.toString();
        if (status.exceptionCode() == Status::EX_NONE) {
            ADD_FAILURE() << "Returned connected port " << ignored.toString() << " for template "
                          << portWithData.toString();
        }
    }
}

// Note: This test relies on simulation of external device connections by the HAL module.
TEST_P(AudioCoreModule, DisconnectExternalDeviceNonResetPortConfig) {
    ASSERT_NO_FATAL_FAILURE(SetUpModuleConfig());
    std::vector<AudioPort> ports = moduleConfig->getExternalDevicePorts();
    if (ports.empty()) {
        GTEST_SKIP() << "No external devices in the module.";
    }
    for (const auto& port : ports) {
        WithDevicePortConnectedState portConnected(port, GenerateUniqueDeviceAddress());
        ASSERT_NO_FATAL_FAILURE(portConnected.SetUp(module.get()));
        const auto portConfig = moduleConfig->getSingleConfigForDevicePort(portConnected.get());
        {
            WithAudioPortConfig config(portConfig);
            // Note: if SetUp fails, check the status of 'GetAudioPortWithExternalDevices' test.
            // Our test assumes that 'getAudioPort' returns at least one profile, and it
            // is not a dynamic profile.
            ASSERT_NO_FATAL_FAILURE(config.SetUp(module.get()));
            Status status = module->disconnectExternalDevice(portConnected.getId());
            EXPECT_EQ(Status::EX_ILLEGAL_STATE, status.exceptionCode())
                    << status << " returned when trying to disconnect device port ID " << port.id
                    << " with active configuration " << config.getId();
        }
    }
}

TEST_P(AudioCoreModule, ExternalDevicePortRoutes) {
    ASSERT_NO_FATAL_FAILURE(SetUpModuleConfig());
    std::vector<AudioPort> ports = moduleConfig->getExternalDevicePorts();
    if (ports.empty()) {
        GTEST_SKIP() << "No external devices in the module.";
    }
    for (const auto& port : ports) {
        std::vector<AudioRoute> routesBefore;
        {
            Status status = module->getAudioRoutes(&routesBefore);
            ASSERT_EQ(Status::EX_NONE, status.exceptionCode()) << status;
        }

        int32_t connectedPortId;
        {
            WithDevicePortConnectedState portConnected(port, GenerateUniqueDeviceAddress());
            ASSERT_NO_FATAL_FAILURE(portConnected.SetUp(module.get()));
            connectedPortId = portConnected.getId();
            std::vector<AudioRoute> connectedPortRoutes;
            {
                Status status =
                        module->getAudioRoutesForAudioPort(connectedPortId, &connectedPortRoutes);
                ASSERT_EQ(Status::EX_NONE, status.exceptionCode())
                        << status << " returned when retrieving routes for connected port id "
                        << connectedPortId;
            }
            // There must be routes for the port to be useful.
            if (connectedPortRoutes.empty()) {
                std::vector<AudioRoute> allRoutes;
                Status status = module->getAudioRoutes(&allRoutes);
                ASSERT_EQ(Status::EX_NONE, status.exceptionCode()) << status;
                ADD_FAILURE() << " no routes returned for the connected port "
                              << portConnected.get().toString()
                              << "; all routes: " << android::internal::ToString(allRoutes);
            }
        }
        std::vector<AudioRoute> ignored;
        Status status = module->getAudioRoutesForAudioPort(connectedPortId, &ignored);
        ASSERT_EQ(Status::EX_ILLEGAL_ARGUMENT, status.exceptionCode())
                << status << " returned when retrieving routes for released connected port id "
                << connectedPortId;

        std::vector<AudioRoute> routesAfter;
        {
            Status status = module->getAudioRoutes(&routesAfter);
            ASSERT_EQ(Status::EX_NONE, status.exceptionCode()) << status;
        }
        ASSERT_EQ(routesBefore.size(), routesAfter.size())
                << "Sizes of audio route arrays do not match after creating and "
                << "releasing a connected port";
        std::sort(routesBefore.begin(), routesBefore.end());
        std::sort(routesAfter.begin(), routesAfter.end());
        EXPECT_EQ(routesBefore, routesAfter);
    }
}

template <typename Stream>
class AudioStream : public AudioCoreModule {
  public:
    static std::string direction(bool capitalize);

    void SetUp() override {
        ASSERT_NO_FATAL_FAILURE(AudioCoreModule::SetUp());
        ASSERT_NO_FATAL_FAILURE(SetUpModuleConfig());
    }

    void CloseTwice() {
        const auto portConfig = moduleConfig->getSingleConfigForMixPort(IsInput<Stream>());
        if (!portConfig.has_value()) {
            GTEST_SKIP() << "No mix port for attached devices";
        }
        sp<Stream> heldStream;
        {
            WithStream<Stream> stream(portConfig.value());
            ASSERT_NO_FATAL_FAILURE(stream.SetUp(module.get()));
            heldStream = stream.get();
        }
        Status status = heldStream->close();
        EXPECT_EQ(Status::EX_ILLEGAL_STATE, status.exceptionCode())
                << status << " when closing the stream twice";
    }

    void OpenAllConfigs() {
        const auto allPortConfigs = moduleConfig->getPortConfigsForMixPorts(IsInput<Stream>());
        for (const auto& portConfig : allPortConfigs) {
            WithStream<Stream> stream(portConfig);
            ASSERT_NO_FATAL_FAILURE(stream.SetUp(module.get()));
        }
    }

    void OpenOverMaxCount() {
        constexpr bool isInput = IsInput<Stream>();
        auto ports = moduleConfig->getMixPorts(isInput);
        bool hasSingleRun = false;
        for (const auto& port : ports) {
            const size_t maxStreamCount = port.ext.get<AudioPortExt::Tag::mix>().maxOpenStreamCount;
            if (maxStreamCount == 0 ||
                moduleConfig->getAttachedDevicesPortsForMixPort(isInput, port).empty()) {
                // No restrictions or no permanently attached devices.
                continue;
            }
            auto portConfigs = moduleConfig->getPortConfigsForMixPorts(isInput, port);
            if (portConfigs.size() < maxStreamCount + 1) {
                // Not able to open a sufficient number of streams for this port.
                continue;
            }
            hasSingleRun = true;
            std::optional<WithStream<Stream>> streamWraps[maxStreamCount + 1];
            for (size_t i = 0; i <= maxStreamCount; ++i) {
                streamWraps[i].emplace(portConfigs[i]);
                WithStream<Stream>& stream = streamWraps[i].value();
                if (i < maxStreamCount) {
                    ASSERT_NO_FATAL_FAILURE(stream.SetUp(module.get()));
                } else {
                    ASSERT_NO_FATAL_FAILURE(stream.SetUpPortConfig(module.get()));
                    Status status = stream.SetUpNoChecks(module.get());
                    EXPECT_EQ(Status::EX_ILLEGAL_STATE, status.exceptionCode())
                            << status << " open" << direction(true)
                            << "Stream"
                               " returned for port config ID "
                            << stream.getPortId() << ", maxOpenStreamCount is " << maxStreamCount;
                }
            }
        }
        if (!hasSingleRun) {
            GTEST_SKIP() << "Not enough " << direction(false)
                         << " ports to test max open stream count";
        }
    }

    void OpenInvalidDirection() {
        // Important! The direction of the port config must be reversed.
        const auto portConfig = moduleConfig->getSingleConfigForMixPort(!IsInput<Stream>());
        if (!portConfig.has_value()) {
            GTEST_SKIP() << "No mix port for attached devices";
        }
        WithStream<Stream> stream(portConfig.value());
        ASSERT_NO_FATAL_FAILURE(stream.SetUpPortConfig(module.get()));
        Status status = stream.SetUpNoChecks(module.get());
        EXPECT_EQ(Status::EX_ILLEGAL_ARGUMENT, status.exceptionCode())
                << status << " open" << direction(true) << "Stream returned for port config ID "
                << stream.getPortId();
        EXPECT_EQ(nullptr, stream.get());
    }

    void OpenTwiceSamePortConfig() {
        const auto portConfig = moduleConfig->getSingleConfigForMixPort(IsInput<Stream>());
        if (!portConfig.has_value()) {
            GTEST_SKIP() << "No mix port for attached devices";
        }
        EXPECT_NO_FATAL_FAILURE(OpenTwiceSamePortConfigImpl(portConfig.value()));
    }

    void ResetPortConfigWithOpenStream() {
        const auto portConfig = moduleConfig->getSingleConfigForMixPort(IsInput<Stream>());
        if (!portConfig.has_value()) {
            GTEST_SKIP() << "No mix port for attached devices";
        }
        WithStream<Stream> stream(portConfig.value());
        ASSERT_NO_FATAL_FAILURE(stream.SetUp(module.get()));
        Status status = module->resetAudioPortConfig(stream.getPortId());
        EXPECT_EQ(Status::EX_ILLEGAL_STATE, status.exceptionCode())
                << status << " returned for port config ID " << stream.getPortId();
    }

    void OpenTwiceSamePortConfigImpl(const AudioPortConfig& portConfig) {
        WithStream<Stream> stream1(portConfig);
        ASSERT_NO_FATAL_FAILURE(stream1.SetUp(module.get()));
        WithStream<Stream> stream2;
        Status status = stream2.SetUpNoChecks(module.get(), stream1.getPortConfig());
        EXPECT_EQ(Status::EX_ILLEGAL_STATE, status.exceptionCode())
                << status << " when opening " << direction(false)
                << " stream twice for the same port config ID " << stream1.getPortId();
    }
};
using AudioStreamIn = AudioStream<IStreamIn>;
using AudioStreamOut = AudioStream<IStreamOut>;

template <>
std::string AudioStreamIn::direction(bool capitalize) {
    return capitalize ? "Input" : "input";
}
template <>
std::string AudioStreamOut::direction(bool capitalize) {
    return capitalize ? "Output" : "output";
}

#define TEST_IO_STREAM(method_name)                                                \
    TEST_P(AudioStreamIn, method_name) { ASSERT_NO_FATAL_FAILURE(method_name()); } \
    TEST_P(AudioStreamOut, method_name) { ASSERT_NO_FATAL_FAILURE(method_name()); }

TEST_IO_STREAM(CloseTwice);
TEST_IO_STREAM(OpenAllConfigs);
TEST_IO_STREAM(OpenInvalidDirection);
TEST_IO_STREAM(OpenOverMaxCount);
TEST_IO_STREAM(OpenTwiceSamePortConfig);
TEST_IO_STREAM(ResetPortConfigWithOpenStream);

TEST_P(AudioStreamOut, OpenTwicePrimary) {
    const auto mixPorts = moduleConfig->getMixPorts(false);
    auto primaryPortIt = std::find_if(mixPorts.begin(), mixPorts.end(), [](const AudioPort& port) {
        constexpr int primaryOutputFlag = 1 << static_cast<int>(AudioOutputFlags::PRIMARY);
        return port.flags.getTag() == AudioIoFlags::Tag::output &&
               ((port.flags.get<AudioIoFlags::Tag::output>() & primaryOutputFlag) != 0);
    });
    if (primaryPortIt == mixPorts.end()) {
        GTEST_SKIP() << "No primary mix port";
    }
    if (moduleConfig->getAttachedSinkDevicesPortsForMixPort(*primaryPortIt).empty()) {
        GTEST_SKIP() << "Primary mix port can not be routed to any of attached devices";
    }
    const auto portConfig = moduleConfig->getSingleConfigForMixPort(false, *primaryPortIt);
    ASSERT_TRUE(portConfig.has_value()) << "No profiles specified for the primary mix port";
    EXPECT_NO_FATAL_FAILURE(OpenTwiceSamePortConfigImpl(portConfig.value()));
}

// Tests specific to audio patches. The fixure class is named 'AudioModulePatch'
// to avoid clashing with 'AudioPatch' class.
class AudioModulePatch : public AudioCoreModule {
  public:
    static std::string direction(bool isInput, bool capitalize) {
        return isInput ? (capitalize ? "Input" : "input") : (capitalize ? "Output" : "output");
    }

    void SetUp() override {
        ASSERT_NO_FATAL_FAILURE(AudioCoreModule::SetUp());
        ASSERT_NO_FATAL_FAILURE(SetUpModuleConfig());
    }

    void SetInvalidPatchHelper(int32_t expectedException, const std::vector<int32_t>& sources,
                               const std::vector<int32_t>& sinks) {
        AudioPatch patch;
        patch.sourcePortConfigIds = sources;
        patch.sinkPortConfigIds = sinks;
        Status status = module->setAudioPatch(patch, &patch);
        ASSERT_EQ(expectedException, status.exceptionCode())
                << status << ": patch source ids: " << android::internal::ToString(sources)
                << "; sink ids: " << android::internal::ToString(sinks);
    }

    void ResetPortConfigUsedByPatch(bool isInput) {
        auto srcSinkGroups = moduleConfig->getRoutableSrcSinkGroups(isInput);
        if (srcSinkGroups.empty()) {
            GTEST_SKIP() << "No routes to any attached " << direction(isInput, false) << " devices";
        }
        auto srcSinkGroup = *srcSinkGroups.begin();
        auto srcSink = *srcSinkGroup.second.begin();
        WithAudioPatch patch(srcSink.first, srcSink.second);
        ASSERT_NO_FATAL_FAILURE(patch.SetUp(module.get()));
        std::vector<int32_t> sourceAndSinkPortConfigIds(patch.get().sourcePortConfigIds);
        sourceAndSinkPortConfigIds.insert(sourceAndSinkPortConfigIds.end(),
                                          patch.get().sinkPortConfigIds.begin(),
                                          patch.get().sinkPortConfigIds.end());
        for (const auto portConfigId : sourceAndSinkPortConfigIds) {
            Status status = module->resetAudioPortConfig(portConfigId);
            EXPECT_EQ(Status::EX_ILLEGAL_STATE, status.exceptionCode())
                    << status << " returned for port config ID " << portConfigId;
        }
    }

    void SetInvalidPatch(bool isInput) {
        auto srcSinkPair = moduleConfig->getRoutableSrcSinkPair(isInput);
        if (!srcSinkPair.has_value()) {
            GTEST_SKIP() << "No routes to any attached " << direction(isInput, false) << " devices";
        }
        WithAudioPortConfig srcPortConfig(srcSinkPair.value().first);
        ASSERT_NO_FATAL_FAILURE(srcPortConfig.SetUp(module.get()));
        WithAudioPortConfig sinkPortConfig(srcSinkPair.value().second);
        ASSERT_NO_FATAL_FAILURE(sinkPortConfig.SetUp(module.get()));
        {  // Check that the pair can actually be used for setting up a patch.
            WithAudioPatch patch(srcPortConfig.get(), sinkPortConfig.get());
            ASSERT_NO_FATAL_FAILURE(patch.SetUp(module.get()));
        }
        EXPECT_NO_FATAL_FAILURE(
                SetInvalidPatchHelper(Status::EX_ILLEGAL_ARGUMENT, {}, {sinkPortConfig.getId()}));
        EXPECT_NO_FATAL_FAILURE(SetInvalidPatchHelper(
                Status::EX_ILLEGAL_ARGUMENT, {srcPortConfig.getId(), srcPortConfig.getId()},
                {sinkPortConfig.getId()}));
        EXPECT_NO_FATAL_FAILURE(
                SetInvalidPatchHelper(Status::EX_ILLEGAL_ARGUMENT, {srcPortConfig.getId()}, {}));
        EXPECT_NO_FATAL_FAILURE(
                SetInvalidPatchHelper(Status::EX_ILLEGAL_ARGUMENT, {srcPortConfig.getId()},
                                      {sinkPortConfig.getId(), sinkPortConfig.getId()}));

        std::set<int32_t> portConfigIds;
        ASSERT_NO_FATAL_FAILURE(GetAllPortConfigIds(&portConfigIds));
        for (const auto portConfigId : GetNonExistentIds(portConfigIds)) {
            EXPECT_NO_FATAL_FAILURE(SetInvalidPatchHelper(
                    Status::EX_ILLEGAL_ARGUMENT, {portConfigId}, {sinkPortConfig.getId()}));
            EXPECT_NO_FATAL_FAILURE(SetInvalidPatchHelper(Status::EX_ILLEGAL_ARGUMENT,
                                                          {srcPortConfig.getId()}, {portConfigId}));
        }
    }

    void SetNonRoutablePatch(bool isInput) {
        auto srcSinkPair = moduleConfig->getNonRoutableSrcSinkPair(isInput);
        if (!srcSinkPair.has_value()) {
            GTEST_SKIP() << "All possible source/sink pairs are routable";
        }
        WithAudioPatch patch(srcSinkPair.value().first, srcSinkPair.value().second);
        ASSERT_NO_FATAL_FAILURE(patch.SetUpPortConfigs(module.get()));
        Status status = patch.SetUpNoChecks(module.get());
        EXPECT_EQ(Status::EX_ILLEGAL_ARGUMENT, status.exceptionCode())
                << status << ": when setting up a patch from "
                << srcSinkPair.value().first.toString() << " to "
                << srcSinkPair.value().second.toString() << " that does not have a route";
    }

    void SetPatch(bool isInput) {
        auto srcSinkGroups = moduleConfig->getRoutableSrcSinkGroups(isInput);
        if (srcSinkGroups.empty()) {
            GTEST_SKIP() << "No routes to any attached " << direction(isInput, false) << " devices";
        }
        for (const auto& srcSinkGroup : srcSinkGroups) {
            const auto& route = srcSinkGroup.first;
            std::vector<WithAudioPatch> patches;
            for (const auto& srcSink : srcSinkGroup.second) {
                if (!route.isExclusive) {
                    patches.emplace_back(srcSink.first, srcSink.second);
                    EXPECT_NO_FATAL_FAILURE(patches[patches.size() - 1].SetUp(module.get()));
                } else {
                    WithAudioPatch patch(srcSink.first, srcSink.second);
                    EXPECT_NO_FATAL_FAILURE(patch.SetUp(module.get()));
                }
            }
        }
    }

    void UpdatePatch(bool isInput) {
        auto srcSinkGroups = moduleConfig->getRoutableSrcSinkGroups(isInput);
        if (srcSinkGroups.empty()) {
            GTEST_SKIP() << "No routes to any attached " << direction(isInput, false) << " devices";
        }
        for (const auto& srcSinkGroup : srcSinkGroups) {
            for (const auto& srcSink : srcSinkGroup.second) {
                WithAudioPatch patch(srcSink.first, srcSink.second);
                ASSERT_NO_FATAL_FAILURE(patch.SetUp(module.get()));
                AudioPatch ignored;
                EXPECT_NO_FATAL_FAILURE(module->setAudioPatch(patch.get(), &ignored));
            }
        }
    }

    void UpdateInvalidPatchId(bool isInput) {
        auto srcSinkGroups = moduleConfig->getRoutableSrcSinkGroups(isInput);
        if (srcSinkGroups.empty()) {
            GTEST_SKIP() << "No routes to any attached " << direction(isInput, false) << " devices";
        }
        // First, set up a patch to ensure that its settings are accepted.
        auto srcSinkGroup = *srcSinkGroups.begin();
        auto srcSink = *srcSinkGroup.second.begin();
        WithAudioPatch patch(srcSink.first, srcSink.second);
        ASSERT_NO_FATAL_FAILURE(patch.SetUp(module.get()));
        // Then use the same patch setting, except for having an invalid ID.
        std::set<int32_t> patchIds;
        ASSERT_NO_FATAL_FAILURE(GetAllPatchIds(&patchIds));
        for (const auto patchId : GetNonExistentIds(patchIds)) {
            AudioPatch patchWithNonExistendId = patch.get();
            patchWithNonExistendId.id = patchId;
            Status status = module->setAudioPatch(patchWithNonExistendId, &patchWithNonExistendId);
            EXPECT_EQ(Status::EX_ILLEGAL_ARGUMENT, status.exceptionCode())
                    << status << " returned for patch ID " << patchId;
        }
    }
};

// Not all tests require both directions, so parametrization would require
// more abstractions.
#define TEST_PATCH_BOTH_DIRECTIONS(method_name)                                                  \
    TEST_P(AudioModulePatch, method_name##Input) { ASSERT_NO_FATAL_FAILURE(method_name(true)); } \
    TEST_P(AudioModulePatch, method_name##Output) { ASSERT_NO_FATAL_FAILURE(method_name(false)); }

TEST_PATCH_BOTH_DIRECTIONS(ResetPortConfigUsedByPatch);
TEST_PATCH_BOTH_DIRECTIONS(SetInvalidPatch);
TEST_PATCH_BOTH_DIRECTIONS(SetNonRoutablePatch);
TEST_PATCH_BOTH_DIRECTIONS(SetPatch);
TEST_PATCH_BOTH_DIRECTIONS(UpdateInvalidPatchId);
TEST_PATCH_BOTH_DIRECTIONS(UpdatePatch);

TEST_P(AudioModulePatch, ResetInvalidPatchId) {
    std::set<int32_t> patchIds;
    ASSERT_NO_FATAL_FAILURE(GetAllPatchIds(&patchIds));
    for (const auto patchId : GetNonExistentIds(patchIds)) {
        Status status = module->resetAudioPatch(patchId);
        EXPECT_EQ(Status::EX_ILLEGAL_ARGUMENT, status.exceptionCode())
                << status << " returned for patch ID " << patchId;
    }
}

INSTANTIATE_TEST_SUITE_P(AudioCoreModuleTest, AudioCoreModule,
                         testing::ValuesIn(android::getAidlHalInstanceNames(IModule::descriptor)),
                         android::PrintInstanceNameToString);
GTEST_ALLOW_UNINSTANTIATED_PARAMETERIZED_TEST(AudioCoreModule);
INSTANTIATE_TEST_SUITE_P(AudioStreamInTest, AudioStreamIn,
                         testing::ValuesIn(android::getAidlHalInstanceNames(IModule::descriptor)),
                         android::PrintInstanceNameToString);
GTEST_ALLOW_UNINSTANTIATED_PARAMETERIZED_TEST(AudioStreamIn);
INSTANTIATE_TEST_SUITE_P(AudioStreamOutTest, AudioStreamOut,
                         testing::ValuesIn(android::getAidlHalInstanceNames(IModule::descriptor)),
                         android::PrintInstanceNameToString);
GTEST_ALLOW_UNINSTANTIATED_PARAMETERIZED_TEST(AudioStreamOut);
INSTANTIATE_TEST_SUITE_P(AudioPatchTest, AudioModulePatch,
                         testing::ValuesIn(android::getAidlHalInstanceNames(IModule::descriptor)),
                         android::PrintInstanceNameToString);
GTEST_ALLOW_UNINSTANTIATED_PARAMETERIZED_TEST(AudioModulePatch);

int main(int argc, char** argv) {
    ::testing::InitGoogleTest(&argc, argv);
    ProcessState::self()->setThreadPoolMaxThreadCount(1);
    ProcessState::self()->startThreadPool();
    return RUN_ALL_TESTS();
}
