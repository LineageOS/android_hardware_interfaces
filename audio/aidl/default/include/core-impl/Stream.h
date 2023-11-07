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

#include <atomic>
#include <chrono>
#include <cstdlib>
#include <map>
#include <memory>
#include <optional>
#include <variant>

#include <StreamWorker.h>
#include <Utils.h>
#include <aidl/android/hardware/audio/common/SinkMetadata.h>
#include <aidl/android/hardware/audio/common/SourceMetadata.h>
#include <aidl/android/hardware/audio/core/BnStreamCommon.h>
#include <aidl/android/hardware/audio/core/BnStreamIn.h>
#include <aidl/android/hardware/audio/core/BnStreamOut.h>
#include <aidl/android/hardware/audio/core/IStreamCallback.h>
#include <aidl/android/hardware/audio/core/IStreamOutEventCallback.h>
#include <aidl/android/hardware/audio/core/StreamDescriptor.h>
#include <aidl/android/media/audio/common/AudioDevice.h>
#include <aidl/android/media/audio/common/AudioIoFlags.h>
#include <aidl/android/media/audio/common/AudioOffloadInfo.h>
#include <aidl/android/media/audio/common/MicrophoneInfo.h>
#include <error/expected_utils.h>
#include <fmq/AidlMessageQueue.h>
#include <system/thread_defs.h>
#include <utils/Errors.h>

#include "core-impl/ChildInterface.h"
#include "core-impl/SoundDose.h"
#include "core-impl/utils.h"

namespace aidl::android::hardware::audio::core {

// This class is similar to StreamDescriptor, but unlike
// the descriptor, it actually owns the objects implementing
// data exchange: FMQs etc, whereas StreamDescriptor only
// contains their descriptors.
class StreamContext {
  public:
    typedef ::android::AidlMessageQueue<
            StreamDescriptor::Command,
            ::aidl::android::hardware::common::fmq::SynchronizedReadWrite>
            CommandMQ;
    typedef ::android::AidlMessageQueue<
            StreamDescriptor::Reply, ::aidl::android::hardware::common::fmq::SynchronizedReadWrite>
            ReplyMQ;
    typedef ::android::AidlMessageQueue<
            int8_t, ::aidl::android::hardware::common::fmq::SynchronizedReadWrite>
            DataMQ;

    // Ensure that this value is not used by any of StreamDescriptor.State enums
    static constexpr StreamDescriptor::State STATE_CLOSED =
            static_cast<StreamDescriptor::State>(-1);

    struct DebugParameters {
        // An extra delay for transient states, in ms.
        int transientStateDelayMs = 0;
        // Force the "burst" command to move the SM to the TRANSFERRING state.
        bool forceTransientBurst = false;
        // Force the "drain" command to be synchronous, going directly to the IDLE state.
        bool forceSynchronousDrain = false;
    };

    StreamContext() = default;
    StreamContext(std::unique_ptr<CommandMQ> commandMQ, std::unique_ptr<ReplyMQ> replyMQ,
                  const ::aidl::android::media::audio::common::AudioFormatDescription& format,
                  const ::aidl::android::media::audio::common::AudioChannelLayout& channelLayout,
                  int sampleRate, const ::aidl::android::media::audio::common::AudioIoFlags& flags,
                  int32_t nominalLatencyMs, int32_t mixPortHandle, std::unique_ptr<DataMQ> dataMQ,
                  std::shared_ptr<IStreamCallback> asyncCallback,
                  std::shared_ptr<IStreamOutEventCallback> outEventCallback,
                  std::weak_ptr<sounddose::StreamDataProcessorInterface> streamDataProcessor,
                  DebugParameters debugParameters)
        : mCommandMQ(std::move(commandMQ)),
          mInternalCommandCookie(std::rand()),
          mReplyMQ(std::move(replyMQ)),
          mFormat(format),
          mChannelLayout(channelLayout),
          mSampleRate(sampleRate),
          mFlags(flags),
          mNominalLatencyMs(nominalLatencyMs),
          mMixPortHandle(mixPortHandle),
          mDataMQ(std::move(dataMQ)),
          mAsyncCallback(asyncCallback),
          mOutEventCallback(outEventCallback),
          mStreamDataProcessor(streamDataProcessor),
          mDebugParameters(debugParameters) {}

    void fillDescriptor(StreamDescriptor* desc);
    std::shared_ptr<IStreamCallback> getAsyncCallback() const { return mAsyncCallback; }
    size_t getBufferSizeInFrames() const;
    ::aidl::android::media::audio::common::AudioChannelLayout getChannelLayout() const {
        return mChannelLayout;
    }
    CommandMQ* getCommandMQ() const { return mCommandMQ.get(); }
    DataMQ* getDataMQ() const { return mDataMQ.get(); }
    ::aidl::android::media::audio::common::AudioFormatDescription getFormat() const {
        return mFormat;
    }
    ::aidl::android::media::audio::common::AudioIoFlags getFlags() const { return mFlags; }
    bool getForceTransientBurst() const { return mDebugParameters.forceTransientBurst; }
    bool getForceSynchronousDrain() const { return mDebugParameters.forceSynchronousDrain; }
    size_t getFrameSize() const;
    int getInternalCommandCookie() const { return mInternalCommandCookie; }
    int32_t getMixPortHandle() const { return mMixPortHandle; }
    int32_t getNominalLatencyMs() const { return mNominalLatencyMs; }
    std::shared_ptr<IStreamOutEventCallback> getOutEventCallback() const {
        return mOutEventCallback;
    }
    std::weak_ptr<sounddose::StreamDataProcessorInterface> getStreamDataProcessor() const {
        return mStreamDataProcessor;
    }
    void startStreamDataProcessor();
    ReplyMQ* getReplyMQ() const { return mReplyMQ.get(); }
    int getTransientStateDelayMs() const { return mDebugParameters.transientStateDelayMs; }
    int getSampleRate() const { return mSampleRate; }
    bool isValid() const;
    // 'reset' is called on a Binder thread when closing the stream. Does not use
    // locking because it only cleans MQ pointers which were also set on the Binder thread.
    void reset();
    // 'advanceFrameCount' and 'getFrameCount' are only called on the worker thread.
    long advanceFrameCount(size_t increase) { return mFrameCount += increase; }
    long getFrameCount() const { return mFrameCount; }

  private:
    // Fields are non const to allow move assignment.
    std::unique_ptr<CommandMQ> mCommandMQ;
    int mInternalCommandCookie;  // The value used to confirm that the command was posted internally
    std::unique_ptr<ReplyMQ> mReplyMQ;
    ::aidl::android::media::audio::common::AudioFormatDescription mFormat;
    ::aidl::android::media::audio::common::AudioChannelLayout mChannelLayout;
    int mSampleRate;
    ::aidl::android::media::audio::common::AudioIoFlags mFlags;
    int32_t mNominalLatencyMs;
    int32_t mMixPortHandle;
    std::unique_ptr<DataMQ> mDataMQ;
    std::shared_ptr<IStreamCallback> mAsyncCallback;
    std::shared_ptr<IStreamOutEventCallback> mOutEventCallback;  // Only used by output streams
    std::weak_ptr<sounddose::StreamDataProcessorInterface> mStreamDataProcessor;
    DebugParameters mDebugParameters;
    long mFrameCount = 0;
};

// This interface provides operations of the stream which are executed on the worker thread.
struct DriverInterface {
    virtual ~DriverInterface() = default;
    // All the methods below are called on the worker thread.
    virtual ::android::status_t init() = 0;  // This function is only called once.
    virtual ::android::status_t drain(StreamDescriptor::DrainMode mode) = 0;
    virtual ::android::status_t flush() = 0;
    virtual ::android::status_t pause() = 0;
    virtual ::android::status_t standby() = 0;
    virtual ::android::status_t start() = 0;
    virtual ::android::status_t transfer(void* buffer, size_t frameCount, size_t* actualFrameCount,
                                         int32_t* latencyMs) = 0;
    // No need to implement 'refinePosition' unless the driver can provide more precise
    // data than just total frame count. For example, the driver may correctly account
    // for any intermediate buffers.
    virtual ::android::status_t refinePosition(StreamDescriptor::Position* /*position*/) {
        return ::android::OK;
    }
    virtual void shutdown() = 0;  // This function is only called once.
};

class StreamWorkerCommonLogic : public ::android::hardware::audio::common::StreamLogic {
  public:
    bool isClosed() const { return mState == StreamContext::STATE_CLOSED; }
    StreamDescriptor::State setClosed() {
        auto prevState = mState.exchange(StreamContext::STATE_CLOSED);
        if (prevState != StreamContext::STATE_CLOSED) {
            mStatePriorToClosing = prevState;
        }
        return mStatePriorToClosing;
    }
    void setIsConnected(bool connected) { mIsConnected = connected; }

  protected:
    using DataBufferElement = int8_t;

    StreamWorkerCommonLogic(StreamContext* context, DriverInterface* driver)
        : mContext(context),
          mDriver(driver),
          mTransientStateDelayMs(context->getTransientStateDelayMs()) {}
    pid_t getTid() const;
    std::string init() override;
    void populateReply(StreamDescriptor::Reply* reply, bool isConnected) const;
    void populateReplyWrongState(StreamDescriptor::Reply* reply,
                                 const StreamDescriptor::Command& command) const;
    void switchToTransientState(StreamDescriptor::State state) {
        mState = state;
        mTransientStateStart = std::chrono::steady_clock::now();
    }

    // The context is only used for reading, except for updating the frame count,
    // which happens on the worker thread only.
    StreamContext* const mContext;
    DriverInterface* const mDriver;
    // This is the state the stream was in before being closed. It is retrieved by the main
    // thread after joining the worker thread.
    StreamDescriptor::State mStatePriorToClosing = StreamDescriptor::State::STANDBY;
    // Atomic fields are used both by the main and worker threads.
    std::atomic<bool> mIsConnected = false;
    static_assert(std::atomic<StreamDescriptor::State>::is_always_lock_free);
    std::atomic<StreamDescriptor::State> mState = StreamDescriptor::State::STANDBY;
    // All fields below are used on the worker thread only.
    const std::chrono::duration<int, std::milli> mTransientStateDelayMs;
    std::chrono::time_point<std::chrono::steady_clock> mTransientStateStart;
    // We use an array and the "size" field instead of a vector to be able to detect
    // memory allocation issues.
    std::unique_ptr<DataBufferElement[]> mDataBuffer;
    size_t mDataBufferSize;
};

// This interface is used to decouple stream implementations from a concrete StreamWorker
// implementation.
struct StreamWorkerInterface {
    using CreateInstance =
            std::function<StreamWorkerInterface*(StreamContext* context, DriverInterface* driver)>;
    virtual ~StreamWorkerInterface() = default;
    virtual bool isClosed() const = 0;
    virtual void setIsConnected(bool isConnected) = 0;
    virtual StreamDescriptor::State setClosed() = 0;
    virtual bool start() = 0;
    virtual pid_t getTid() = 0;
    virtual void stop() = 0;
};

template <class WorkerLogic>
class StreamWorkerImpl : public StreamWorkerInterface,
                         public ::android::hardware::audio::common::StreamWorker<WorkerLogic> {
    using WorkerImpl = ::android::hardware::audio::common::StreamWorker<WorkerLogic>;

  public:
    StreamWorkerImpl(StreamContext* context, DriverInterface* driver)
        : WorkerImpl(context, driver) {}
    bool isClosed() const override { return WorkerImpl::isClosed(); }
    void setIsConnected(bool isConnected) override { WorkerImpl::setIsConnected(isConnected); }
    StreamDescriptor::State setClosed() override { return WorkerImpl::setClosed(); }
    bool start() override {
        // This is an "audio service thread," must have elevated priority.
        return WorkerImpl::start(WorkerImpl::kThreadName, ANDROID_PRIORITY_URGENT_AUDIO);
    }
    pid_t getTid() override { return WorkerImpl::getTid(); }
    void stop() override { return WorkerImpl::stop(); }
};

class StreamInWorkerLogic : public StreamWorkerCommonLogic {
  public:
    static const std::string kThreadName;
    StreamInWorkerLogic(StreamContext* context, DriverInterface* driver)
        : StreamWorkerCommonLogic(context, driver) {}

  protected:
    Status cycle() override;

  private:
    bool read(size_t clientSize, StreamDescriptor::Reply* reply);
};
using StreamInWorker = StreamWorkerImpl<StreamInWorkerLogic>;

class StreamOutWorkerLogic : public StreamWorkerCommonLogic {
  public:
    static const std::string kThreadName;
    StreamOutWorkerLogic(StreamContext* context, DriverInterface* driver)
        : StreamWorkerCommonLogic(context, driver),
          mEventCallback(context->getOutEventCallback()) {}

  protected:
    Status cycle() override;

  private:
    bool write(size_t clientSize, StreamDescriptor::Reply* reply);

    std::shared_ptr<IStreamOutEventCallback> mEventCallback;
};
using StreamOutWorker = StreamWorkerImpl<StreamOutWorkerLogic>;

// This interface provides operations of the stream which are executed on a Binder pool thread.
// These methods originate both from the AIDL interface and its implementation.
struct StreamCommonInterface {
    using ConnectedDevices = std::vector<::aidl::android::media::audio::common::AudioDevice>;
    using Metadata =
            std::variant<::aidl::android::hardware::audio::common::SinkMetadata /*IStreamIn*/,
                         ::aidl::android::hardware::audio::common::SourceMetadata /*IStreamOut*/>;

    static constexpr bool isInput(const Metadata& metadata) { return metadata.index() == 0; }

    virtual ~StreamCommonInterface() = default;
    // Methods below originate from the 'IStreamCommon' interface.
    // This is semantically equivalent to inheriting from 'IStreamCommon' with a benefit
    // that concrete stream implementations can inherit both from this interface and IStreamIn/Out.
    virtual ndk::ScopedAStatus close() = 0;
    virtual ndk::ScopedAStatus prepareToClose() = 0;
    virtual ndk::ScopedAStatus updateHwAvSyncId(int32_t in_hwAvSyncId) = 0;
    virtual ndk::ScopedAStatus getVendorParameters(const std::vector<std::string>& in_ids,
                                                   std::vector<VendorParameter>* _aidl_return) = 0;
    virtual ndk::ScopedAStatus setVendorParameters(
            const std::vector<VendorParameter>& in_parameters, bool in_async) = 0;
    virtual ndk::ScopedAStatus addEffect(
            const std::shared_ptr<::aidl::android::hardware::audio::effect::IEffect>&
                    in_effect) = 0;
    virtual ndk::ScopedAStatus removeEffect(
            const std::shared_ptr<::aidl::android::hardware::audio::effect::IEffect>&
                    in_effect) = 0;
    // Methods below are common for both 'IStreamIn' and 'IStreamOut'. Note that
    // 'updateMetadata' in them uses an individual structure which is wrapped here.
    // The 'Common' suffix is added to distinguish them from the methods from 'IStreamIn/Out'.
    virtual ndk::ScopedAStatus getStreamCommonCommon(
            std::shared_ptr<IStreamCommon>* _aidl_return) = 0;
    virtual ndk::ScopedAStatus updateMetadataCommon(const Metadata& metadata) = 0;
    // Methods below are called by implementation of 'IModule', 'IStreamIn' and 'IStreamOut'.
    virtual ndk::ScopedAStatus initInstance(
            const std::shared_ptr<StreamCommonInterface>& delegate) = 0;
    virtual const StreamContext& getContext() const = 0;
    virtual bool isClosed() const = 0;
    virtual const ConnectedDevices& getConnectedDevices() const = 0;
    virtual ndk::ScopedAStatus setConnectedDevices(
            const std::vector<::aidl::android::media::audio::common::AudioDevice>& devices) = 0;
    virtual ndk::ScopedAStatus bluetoothParametersUpdated() = 0;
};

// This is equivalent to automatically generated 'IStreamCommonDelegator' but uses
// a weak pointer to avoid creating a reference loop. The loop will occur because
// 'IStreamIn/Out.getStreamCommon' must return the same instance every time, thus
// the stream implementation must hold a strong pointer to an instance of 'IStreamCommon'.
// Also, we use 'StreamCommonInterface' here instead of 'IStreamCommon'.
class StreamCommonDelegator : public BnStreamCommon {
  public:
    explicit StreamCommonDelegator(const std::shared_ptr<StreamCommonInterface>& delegate)
        : mDelegate(delegate) {}

  private:
    ndk::ScopedAStatus close() override {
        auto delegate = mDelegate.lock();
        return delegate != nullptr ? delegate->close()
                                   : ndk::ScopedAStatus::fromExceptionCode(EX_ILLEGAL_STATE);
    }
    ndk::ScopedAStatus prepareToClose() override {
        auto delegate = mDelegate.lock();
        return delegate != nullptr ? delegate->prepareToClose()
                                   : ndk::ScopedAStatus::fromExceptionCode(EX_ILLEGAL_STATE);
    }
    ndk::ScopedAStatus updateHwAvSyncId(int32_t in_hwAvSyncId) override {
        auto delegate = mDelegate.lock();
        return delegate != nullptr ? delegate->updateHwAvSyncId(in_hwAvSyncId)
                                   : ndk::ScopedAStatus::fromExceptionCode(EX_ILLEGAL_STATE);
    }
    ndk::ScopedAStatus getVendorParameters(const std::vector<std::string>& in_ids,
                                           std::vector<VendorParameter>* _aidl_return) override {
        auto delegate = mDelegate.lock();
        return delegate != nullptr ? delegate->getVendorParameters(in_ids, _aidl_return)
                                   : ndk::ScopedAStatus::fromExceptionCode(EX_ILLEGAL_STATE);
    }
    ndk::ScopedAStatus setVendorParameters(const std::vector<VendorParameter>& in_parameters,
                                           bool in_async) override {
        auto delegate = mDelegate.lock();
        return delegate != nullptr ? delegate->setVendorParameters(in_parameters, in_async)
                                   : ndk::ScopedAStatus::fromExceptionCode(EX_ILLEGAL_STATE);
    }
    ndk::ScopedAStatus addEffect(
            const std::shared_ptr<::aidl::android::hardware::audio::effect::IEffect>& in_effect)
            override {
        auto delegate = mDelegate.lock();
        return delegate != nullptr ? delegate->addEffect(in_effect)
                                   : ndk::ScopedAStatus::fromExceptionCode(EX_ILLEGAL_STATE);
    }
    ndk::ScopedAStatus removeEffect(
            const std::shared_ptr<::aidl::android::hardware::audio::effect::IEffect>& in_effect)
            override {
        auto delegate = mDelegate.lock();
        return delegate != nullptr ? delegate->removeEffect(in_effect)
                                   : ndk::ScopedAStatus::fromExceptionCode(EX_ILLEGAL_STATE);
    }
    // It is possible that on the client side the proxy for IStreamCommon will outlive
    // the IStream* instance, and the server side IStream* instance will get destroyed
    // while this IStreamCommon instance is still alive.
    std::weak_ptr<StreamCommonInterface> mDelegate;
};

// The implementation of DriverInterface must be provided by each concrete stream implementation.
// Note that StreamCommonImpl does not own the context. This is to support swapping on the fly
// implementations of the stream while keeping the same IStreamIn/Out instance. It's that instance
// who must be owner of the context.
class StreamCommonImpl : virtual public StreamCommonInterface, virtual public DriverInterface {
  public:
    StreamCommonImpl(StreamContext* context, const Metadata& metadata,
                     const StreamWorkerInterface::CreateInstance& createWorker)
        : mContext(*context), mMetadata(metadata), mWorker(createWorker(context, this)) {}
    StreamCommonImpl(StreamContext* context, const Metadata& metadata)
        : StreamCommonImpl(
                  context, metadata,
                  isInput(metadata) ? getDefaultInWorkerCreator() : getDefaultOutWorkerCreator()) {}
    ~StreamCommonImpl();

    ndk::ScopedAStatus close() override;
    ndk::ScopedAStatus prepareToClose() override;
    ndk::ScopedAStatus updateHwAvSyncId(int32_t in_hwAvSyncId) override;
    ndk::ScopedAStatus getVendorParameters(const std::vector<std::string>& in_ids,
                                           std::vector<VendorParameter>* _aidl_return) override;
    ndk::ScopedAStatus setVendorParameters(const std::vector<VendorParameter>& in_parameters,
                                           bool in_async) override;
    ndk::ScopedAStatus addEffect(
            const std::shared_ptr<::aidl::android::hardware::audio::effect::IEffect>& in_effect)
            override;
    ndk::ScopedAStatus removeEffect(
            const std::shared_ptr<::aidl::android::hardware::audio::effect::IEffect>& in_effect)
            override;

    ndk::ScopedAStatus getStreamCommonCommon(std::shared_ptr<IStreamCommon>* _aidl_return) override;
    ndk::ScopedAStatus updateMetadataCommon(const Metadata& metadata) override;

    ndk::ScopedAStatus initInstance(
            const std::shared_ptr<StreamCommonInterface>& delegate) override;
    const StreamContext& getContext() const override { return mContext; }
    bool isClosed() const override { return mWorker->isClosed(); }
    const ConnectedDevices& getConnectedDevices() const override { return mConnectedDevices; }
    ndk::ScopedAStatus setConnectedDevices(
            const std::vector<::aidl::android::media::audio::common::AudioDevice>& devices)
            override;
    ndk::ScopedAStatus bluetoothParametersUpdated() override;

  protected:
    static StreamWorkerInterface::CreateInstance getDefaultInWorkerCreator() {
        return [](StreamContext* ctx, DriverInterface* driver) -> StreamWorkerInterface* {
            return new StreamInWorker(ctx, driver);
        };
    }
    static StreamWorkerInterface::CreateInstance getDefaultOutWorkerCreator() {
        return [](StreamContext* ctx, DriverInterface* driver) -> StreamWorkerInterface* {
            return new StreamOutWorker(ctx, driver);
        };
    }

    virtual void onClose(StreamDescriptor::State statePriorToClosing) = 0;
    void stopWorker();

    const StreamContext& mContext;
    Metadata mMetadata;
    std::unique_ptr<StreamWorkerInterface> mWorker;
    ChildInterface<StreamCommonDelegator> mCommon;
    ConnectedDevices mConnectedDevices;
};

// Note: 'StreamIn/Out' can not be used on their own. Instead, they must be used for defining
// concrete input/output stream implementations.
class StreamIn : virtual public StreamCommonInterface, public BnStreamIn {
  protected:
    void defaultOnClose();

    ndk::ScopedAStatus getStreamCommon(std::shared_ptr<IStreamCommon>* _aidl_return) override {
        return getStreamCommonCommon(_aidl_return);
    }
    ndk::ScopedAStatus updateMetadata(const ::aidl::android::hardware::audio::common::SinkMetadata&
                                              in_sinkMetadata) override {
        return updateMetadataCommon(in_sinkMetadata);
    }
    ndk::ScopedAStatus getActiveMicrophones(
            std::vector<::aidl::android::media::audio::common::MicrophoneDynamicInfo>* _aidl_return)
            override;
    ndk::ScopedAStatus getMicrophoneDirection(MicrophoneDirection* _aidl_return) override;
    ndk::ScopedAStatus setMicrophoneDirection(MicrophoneDirection in_direction) override;
    ndk::ScopedAStatus getMicrophoneFieldDimension(float* _aidl_return) override;
    ndk::ScopedAStatus setMicrophoneFieldDimension(float in_zoom) override;
    ndk::ScopedAStatus getHwGain(std::vector<float>* _aidl_return) override;
    ndk::ScopedAStatus setHwGain(const std::vector<float>& in_channelGains) override;

    friend class ndk::SharedRefBase;

    StreamIn(StreamContext&& context,
             const std::vector<::aidl::android::media::audio::common::MicrophoneInfo>& microphones);

    StreamContext mContextInstance;
    const std::map<::aidl::android::media::audio::common::AudioDevice, std::string> mMicrophones;
};

class StreamInHwGainHelper {
  protected:
    explicit StreamInHwGainHelper(const StreamContext* context);

    ndk::ScopedAStatus getHwGainImpl(std::vector<float>* _aidl_return);
    ndk::ScopedAStatus setHwGainImpl(const std::vector<float>& in_channelGains);

    const size_t mChannelCount;
    std::vector<float> mHwGains;
};

class StreamOut : virtual public StreamCommonInterface, public BnStreamOut {
  protected:
    void defaultOnClose();

    ndk::ScopedAStatus getStreamCommon(std::shared_ptr<IStreamCommon>* _aidl_return) override {
        return getStreamCommonCommon(_aidl_return);
    }
    ndk::ScopedAStatus updateMetadata(
            const ::aidl::android::hardware::audio::common::SourceMetadata& in_sourceMetadata)
            override {
        return updateMetadataCommon(in_sourceMetadata);
    }
    ndk::ScopedAStatus updateOffloadMetadata(
            const ::aidl::android::hardware::audio::common::AudioOffloadMetadata&
                    in_offloadMetadata) override;
    ndk::ScopedAStatus getHwVolume(std::vector<float>* _aidl_return) override;
    ndk::ScopedAStatus setHwVolume(const std::vector<float>& in_channelVolumes) override;
    ndk::ScopedAStatus getAudioDescriptionMixLevel(float* _aidl_return) override;
    ndk::ScopedAStatus setAudioDescriptionMixLevel(float in_leveldB) override;
    ndk::ScopedAStatus getDualMonoMode(
            ::aidl::android::media::audio::common::AudioDualMonoMode* _aidl_return) override;
    ndk::ScopedAStatus setDualMonoMode(
            ::aidl::android::media::audio::common::AudioDualMonoMode in_mode) override;
    ndk::ScopedAStatus getRecommendedLatencyModes(
            std::vector<::aidl::android::media::audio::common::AudioLatencyMode>* _aidl_return)
            override;
    ndk::ScopedAStatus setLatencyMode(
            ::aidl::android::media::audio::common::AudioLatencyMode in_mode) override;
    ndk::ScopedAStatus getPlaybackRateParameters(
            ::aidl::android::media::audio::common::AudioPlaybackRate* _aidl_return) override;
    ndk::ScopedAStatus setPlaybackRateParameters(
            const ::aidl::android::media::audio::common::AudioPlaybackRate& in_playbackRate)
            override;
    ndk::ScopedAStatus selectPresentation(int32_t in_presentationId, int32_t in_programId) override;

    friend class ndk::SharedRefBase;

    StreamOut(StreamContext&& context,
              const std::optional<::aidl::android::media::audio::common::AudioOffloadInfo>&
                      offloadInfo);

    StreamContext mContextInstance;
    const std::optional<::aidl::android::media::audio::common::AudioOffloadInfo> mOffloadInfo;
    std::optional<::aidl::android::hardware::audio::common::AudioOffloadMetadata> mOffloadMetadata;
};

class StreamOutHwVolumeHelper {
  protected:
    explicit StreamOutHwVolumeHelper(const StreamContext* context);

    ndk::ScopedAStatus getHwVolumeImpl(std::vector<float>* _aidl_return);
    ndk::ScopedAStatus setHwVolumeImpl(const std::vector<float>& in_channelVolumes);

    const size_t mChannelCount;
    std::vector<float> mHwVolumes;
};

// The recommended way to create a stream instance.
// 'StreamImpl' is the concrete stream implementation, 'StreamInOrOut' is either 'StreamIn' or
// 'StreamOut', the rest are the arguments forwarded to the constructor of 'StreamImpl'.
template <class StreamImpl, class StreamInOrOut, class... Args>
ndk::ScopedAStatus createStreamInstance(std::shared_ptr<StreamInOrOut>* result, Args&&... args) {
    std::shared_ptr<StreamInOrOut> stream =
            ::ndk::SharedRefBase::make<StreamImpl>(std::forward<Args>(args)...);
    RETURN_STATUS_IF_ERROR(stream->initInstance(stream));
    *result = std::move(stream);
    return ndk::ScopedAStatus::ok();
}

class StreamWrapper {
  public:
    explicit StreamWrapper(const std::shared_ptr<StreamIn>& streamIn)
        : mStream(streamIn), mStreamBinder(streamIn->asBinder()) {}
    explicit StreamWrapper(const std::shared_ptr<StreamOut>& streamOut)
        : mStream(streamOut), mStreamBinder(streamOut->asBinder()) {}
    ndk::SpAIBinder getBinder() const { return mStreamBinder; }
    bool isStreamOpen() const {
        auto s = mStream.lock();
        return s && !s->isClosed();
    }
    ndk::ScopedAStatus setConnectedDevices(
            const std::vector<::aidl::android::media::audio::common::AudioDevice>& devices) {
        auto s = mStream.lock();
        if (s) return s->setConnectedDevices(devices);
        return ndk::ScopedAStatus::ok();
    }
    ndk::ScopedAStatus bluetoothParametersUpdated() {
        auto s = mStream.lock();
        if (s) return s->bluetoothParametersUpdated();
        return ndk::ScopedAStatus::ok();
    }

  private:
    std::weak_ptr<StreamCommonInterface> mStream;
    ndk::SpAIBinder mStreamBinder;
};

class Streams {
  public:
    Streams() = default;
    Streams(const Streams&) = delete;
    Streams& operator=(const Streams&) = delete;
    size_t count(int32_t id) {
        // Streams do not remove themselves from the collection on close.
        erase_if(mStreams, [](const auto& pair) { return !pair.second.isStreamOpen(); });
        return mStreams.count(id);
    }
    void insert(int32_t portId, int32_t portConfigId, StreamWrapper sw) {
        mStreams.insert(std::pair{portConfigId, sw});
        mStreams.insert(std::pair{portId, std::move(sw)});
    }
    ndk::ScopedAStatus setStreamConnectedDevices(
            int32_t portConfigId,
            const std::vector<::aidl::android::media::audio::common::AudioDevice>& devices) {
        if (auto it = mStreams.find(portConfigId); it != mStreams.end()) {
            return it->second.setConnectedDevices(devices);
        }
        return ndk::ScopedAStatus::ok();
    }
    ndk::ScopedAStatus bluetoothParametersUpdated() {
        bool isOk = true;
        for (auto& it : mStreams) {
            if (!it.second.bluetoothParametersUpdated().isOk()) isOk = false;
        }
        return isOk ? ndk::ScopedAStatus::ok()
                    : ndk::ScopedAStatus::fromExceptionCode(EX_UNSUPPORTED_OPERATION);
    }

  private:
    // Maps port ids and port config ids to streams. Multimap because a port
    // (not port config) can have multiple streams opened on it.
    std::multimap<int32_t, StreamWrapper> mStreams;
};

}  // namespace aidl::android::hardware::audio::core
