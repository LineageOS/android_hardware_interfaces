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
#include <cstdlib>
#include <map>
#include <memory>
#include <optional>
#include <variant>

#include <StreamWorker.h>
#include <aidl/android/hardware/audio/common/SinkMetadata.h>
#include <aidl/android/hardware/audio/common/SourceMetadata.h>
#include <aidl/android/hardware/audio/core/BnStreamIn.h>
#include <aidl/android/hardware/audio/core/BnStreamOut.h>
#include <aidl/android/hardware/audio/core/StreamDescriptor.h>
#include <aidl/android/media/audio/common/AudioOffloadInfo.h>
#include <fmq/AidlMessageQueue.h>
#include <system/thread_defs.h>

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

    // Ensure that this value is not used by any of StreamDescriptor.COMMAND_*
    static constexpr int COMMAND_EXIT = -1;

    StreamContext() = default;
    StreamContext(std::unique_ptr<CommandMQ> commandMQ, std::unique_ptr<ReplyMQ> replyMQ,
                  size_t frameSize, std::unique_ptr<DataMQ> dataMQ)
        : mCommandMQ(std::move(commandMQ)),
          mInternalCommandCookie(std::rand()),
          mReplyMQ(std::move(replyMQ)),
          mFrameSize(frameSize),
          mDataMQ(std::move(dataMQ)) {}
    StreamContext(StreamContext&& other)
        : mCommandMQ(std::move(other.mCommandMQ)),
          mInternalCommandCookie(other.mInternalCommandCookie),
          mReplyMQ(std::move(other.mReplyMQ)),
          mFrameSize(other.mFrameSize),
          mDataMQ(std::move(other.mDataMQ)) {}
    StreamContext& operator=(StreamContext&& other) {
        mCommandMQ = std::move(other.mCommandMQ);
        mInternalCommandCookie = other.mInternalCommandCookie;
        mReplyMQ = std::move(other.mReplyMQ);
        mFrameSize = other.mFrameSize;
        mDataMQ = std::move(other.mDataMQ);
        return *this;
    }

    void fillDescriptor(StreamDescriptor* desc);
    CommandMQ* getCommandMQ() const { return mCommandMQ.get(); }
    DataMQ* getDataMQ() const { return mDataMQ.get(); }
    size_t getFrameSize() const { return mFrameSize; }
    int getInternalCommandCookie() const { return mInternalCommandCookie; }
    ReplyMQ* getReplyMQ() const { return mReplyMQ.get(); }
    bool isValid() const;
    void reset();

  private:
    std::unique_ptr<CommandMQ> mCommandMQ;
    int mInternalCommandCookie;  // The value used to confirm that the command was posted internally
    std::unique_ptr<ReplyMQ> mReplyMQ;
    size_t mFrameSize;
    std::unique_ptr<DataMQ> mDataMQ;
};

class StreamWorkerCommonLogic : public ::android::hardware::audio::common::StreamLogic {
  public:
    void setIsConnected(bool connected) { mIsConnected = connected; }

  protected:
    explicit StreamWorkerCommonLogic(const StreamContext& context)
        : mInternalCommandCookie(context.getInternalCommandCookie()),
          mFrameSize(context.getFrameSize()),
          mCommandMQ(context.getCommandMQ()),
          mReplyMQ(context.getReplyMQ()),
          mDataMQ(context.getDataMQ()) {}
    std::string init() override;

    // Used both by the main and worker threads.
    std::atomic<bool> mIsConnected = false;
    // All fields are used on the worker thread only.
    const int mInternalCommandCookie;
    const size_t mFrameSize;
    StreamContext::CommandMQ* mCommandMQ;
    StreamContext::ReplyMQ* mReplyMQ;
    StreamContext::DataMQ* mDataMQ;
    // We use an array and the "size" field instead of a vector to be able to detect
    // memory allocation issues.
    std::unique_ptr<int8_t[]> mDataBuffer;
    size_t mDataBufferSize;
    long mFrameCount = 0;
};

class StreamInWorkerLogic : public StreamWorkerCommonLogic {
  public:
    static const std::string kThreadName;
    explicit StreamInWorkerLogic(const StreamContext& context) : StreamWorkerCommonLogic(context) {}

  protected:
    Status cycle() override;
};
using StreamInWorker = ::android::hardware::audio::common::StreamWorker<StreamInWorkerLogic>;

class StreamOutWorkerLogic : public StreamWorkerCommonLogic {
  public:
    static const std::string kThreadName;
    explicit StreamOutWorkerLogic(const StreamContext& context)
        : StreamWorkerCommonLogic(context) {}

  protected:
    Status cycle() override;
};
using StreamOutWorker = ::android::hardware::audio::common::StreamWorker<StreamOutWorkerLogic>;

template <class Metadata, class StreamWorker>
class StreamCommon {
  public:
    ndk::ScopedAStatus close();
    ndk::ScopedAStatus init() {
        return mWorker.start(StreamWorker::kThreadName, ANDROID_PRIORITY_AUDIO)
                       ? ndk::ScopedAStatus::ok()
                       : ndk::ScopedAStatus::fromExceptionCode(EX_ILLEGAL_STATE);
    }
    bool isClosed() const { return mIsClosed; }
    void setIsConnected(bool connected) { mWorker.setIsConnected(connected); }
    ndk::ScopedAStatus updateMetadata(const Metadata& metadata);

  protected:
    StreamCommon(const Metadata& metadata, StreamContext context)
        : mMetadata(metadata), mContext(std::move(context)), mWorker(mContext) {}
    ~StreamCommon();
    void stopWorker();

    Metadata mMetadata;
    StreamContext mContext;
    StreamWorker mWorker;
    // This variable is checked in the destructor which can be called on an arbitrary Binder thread,
    // thus we need to ensure that any changes made by other threads are sequentially consistent.
    std::atomic<bool> mIsClosed = false;
};

class StreamIn
    : public StreamCommon<::aidl::android::hardware::audio::common::SinkMetadata, StreamInWorker>,
      public BnStreamIn {
    ndk::ScopedAStatus close() override {
        return StreamCommon<::aidl::android::hardware::audio::common::SinkMetadata,
                            StreamInWorker>::close();
    }
    ndk::ScopedAStatus updateMetadata(const ::aidl::android::hardware::audio::common::SinkMetadata&
                                              in_sinkMetadata) override {
        return StreamCommon<::aidl::android::hardware::audio::common::SinkMetadata,
                            StreamInWorker>::updateMetadata(in_sinkMetadata);
    }

  public:
    StreamIn(const ::aidl::android::hardware::audio::common::SinkMetadata& sinkMetadata,
             StreamContext context);
};

class StreamOut : public StreamCommon<::aidl::android::hardware::audio::common::SourceMetadata,
                                      StreamOutWorker>,
                  public BnStreamOut {
    ndk::ScopedAStatus close() override {
        return StreamCommon<::aidl::android::hardware::audio::common::SourceMetadata,
                            StreamOutWorker>::close();
    }
    ndk::ScopedAStatus updateMetadata(
            const ::aidl::android::hardware::audio::common::SourceMetadata& in_sourceMetadata)
            override {
        return StreamCommon<::aidl::android::hardware::audio::common::SourceMetadata,
                            StreamOutWorker>::updateMetadata(in_sourceMetadata);
    }

  public:
    StreamOut(const ::aidl::android::hardware::audio::common::SourceMetadata& sourceMetadata,
              StreamContext context,
              const std::optional<::aidl::android::media::audio::common::AudioOffloadInfo>&
                      offloadInfo);

  private:
    std::optional<::aidl::android::media::audio::common::AudioOffloadInfo> mOffloadInfo;
};

class StreamWrapper {
  public:
    explicit StreamWrapper(std::shared_ptr<StreamIn> streamIn) : mStream(streamIn) {}
    explicit StreamWrapper(std::shared_ptr<StreamOut> streamOut) : mStream(streamOut) {}
    bool isStreamOpen() const {
        return std::visit(
                [](auto&& ws) -> bool {
                    auto s = ws.lock();
                    return s && !s->isClosed();
                },
                mStream);
    }
    void setStreamIsConnected(bool connected) {
        std::visit(
                [&](auto&& ws) {
                    auto s = ws.lock();
                    if (s) s->setIsConnected(connected);
                },
                mStream);
    }

  private:
    std::variant<std::weak_ptr<StreamIn>, std::weak_ptr<StreamOut>> mStream;
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
    void setStreamIsConnected(int32_t portConfigId, bool connected) {
        if (auto it = mStreams.find(portConfigId); it != mStreams.end()) {
            it->second.setStreamIsConnected(connected);
        }
    }

  private:
    // Maps port ids and port config ids to streams. Multimap because a port
    // (not port config) can have multiple streams opened on it.
    std::multimap<int32_t, StreamWrapper> mStreams;
};

}  // namespace aidl::android::hardware::audio::core
