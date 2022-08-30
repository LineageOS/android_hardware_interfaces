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

#define LOG_TAG "AHAL_Stream"
#include <android-base/logging.h>
#include <utils/SystemClock.h>

#include "core-impl/Module.h"
#include "core-impl/Stream.h"

using aidl::android::hardware::audio::common::SinkMetadata;
using aidl::android::hardware::audio::common::SourceMetadata;
using aidl::android::media::audio::common::AudioOffloadInfo;

namespace aidl::android::hardware::audio::core {

void StreamContext::fillDescriptor(StreamDescriptor* desc) {
    if (mCommandMQ) {
        desc->command = mCommandMQ->dupeDesc();
    }
    if (mReplyMQ) {
        desc->reply = mReplyMQ->dupeDesc();
    }
    if (mDataMQ) {
        desc->frameSizeBytes = mFrameSize;
        desc->bufferSizeFrames =
                mDataMQ->getQuantumCount() * mDataMQ->getQuantumSize() / mFrameSize;
        desc->audio.set<StreamDescriptor::AudioBuffer::Tag::fmq>(mDataMQ->dupeDesc());
    }
}

bool StreamContext::isValid() const {
    if (mCommandMQ && !mCommandMQ->isValid()) {
        LOG(ERROR) << "command FMQ is invalid";
        return false;
    }
    if (mReplyMQ && !mReplyMQ->isValid()) {
        LOG(ERROR) << "reply FMQ is invalid";
        return false;
    }
    if (mFrameSize == 0) {
        LOG(ERROR) << "frame size is not set";
        return false;
    }
    if (mDataMQ && !mDataMQ->isValid()) {
        LOG(ERROR) << "data FMQ is invalid";
        return false;
    }
    return true;
}

void StreamContext::reset() {
    mCommandMQ.reset();
    mReplyMQ.reset();
    mDataMQ.reset();
}

std::string StreamWorkerCommonLogic::init() {
    if (mCommandMQ == nullptr) return "Command MQ is null";
    if (mReplyMQ == nullptr) return "Reply MQ is null";
    if (mDataMQ == nullptr) return "Data MQ is null";
    if (sizeof(decltype(mDataBuffer)::element_type) != mDataMQ->getQuantumSize()) {
        return "Unexpected Data MQ quantum size: " + std::to_string(mDataMQ->getQuantumSize());
    }
    mDataBufferSize = mDataMQ->getQuantumCount() * mDataMQ->getQuantumSize();
    mDataBuffer.reset(new (std::nothrow) int8_t[mDataBufferSize]);
    if (mDataBuffer == nullptr) {
        return "Failed to allocate data buffer for element count " +
               std::to_string(mDataMQ->getQuantumCount()) +
               ", size in bytes: " + std::to_string(mDataBufferSize);
    }
    return "";
}

const std::string StreamInWorkerLogic::kThreadName = "reader";

StreamInWorkerLogic::Status StreamInWorkerLogic::cycle() {
    StreamDescriptor::Command command{};
    if (!mCommandMQ->readBlocking(&command, 1)) {
        LOG(ERROR) << __func__ << ": reading of command from MQ failed";
        return Status::ABORT;
    }
    StreamDescriptor::Reply reply{};
    if (command.code == StreamContext::COMMAND_EXIT &&
        command.fmqByteCount == mInternalCommandCookie) {
        LOG(DEBUG) << __func__ << ": received EXIT command";
        // This is an internal command, no need to reply.
        return Status::EXIT;
    } else if (command.code == StreamDescriptor::COMMAND_BURST && command.fmqByteCount >= 0) {
        LOG(DEBUG) << __func__ << ": received BURST read command for " << command.fmqByteCount
                   << " bytes";
        usleep(3000);  // Simulate a blocking call into the driver.
        const size_t byteCount = std::min({static_cast<size_t>(command.fmqByteCount),
                                           mDataMQ->availableToWrite(), mDataBufferSize});
        const bool isConnected = mIsConnected;
        // Simulate reading of data, or provide zeroes if the stream is not connected.
        for (size_t i = 0; i < byteCount; ++i) {
            using buffer_type = decltype(mDataBuffer)::element_type;
            constexpr int kBufferValueRange = std::numeric_limits<buffer_type>::max() -
                                              std::numeric_limits<buffer_type>::min() + 1;
            mDataBuffer[i] = isConnected ? (std::rand() % kBufferValueRange) +
                                                   std::numeric_limits<buffer_type>::min()
                                         : 0;
        }
        bool success = byteCount > 0 ? mDataMQ->write(&mDataBuffer[0], byteCount) : true;
        if (success) {
            LOG(DEBUG) << __func__ << ": writing of " << byteCount << " bytes into data MQ"
                       << " succeeded; connected? " << isConnected;
            // Frames are provided and counted regardless of connection status.
            reply.fmqByteCount = byteCount;
            mFrameCount += byteCount / mFrameSize;
            if (isConnected) {
                reply.status = STATUS_OK;
                reply.observable.frames = mFrameCount;
                reply.observable.timeNs = ::android::elapsedRealtimeNano();
            } else {
                reply.status = STATUS_INVALID_OPERATION;
            }
        } else {
            LOG(WARNING) << __func__ << ": writing of " << byteCount
                         << " bytes of data to MQ failed";
            reply.status = STATUS_NOT_ENOUGH_DATA;
        }
        reply.latencyMs = Module::kLatencyMs;
    } else {
        LOG(WARNING) << __func__ << ": invalid command (" << command.toString()
                     << ") or count: " << command.fmqByteCount;
        reply.status = STATUS_BAD_VALUE;
    }
    LOG(DEBUG) << __func__ << ": writing reply " << reply.toString();
    if (!mReplyMQ->writeBlocking(&reply, 1)) {
        LOG(ERROR) << __func__ << ": writing of reply " << reply.toString() << " to MQ failed";
        return Status::ABORT;
    }
    return Status::CONTINUE;
}

const std::string StreamOutWorkerLogic::kThreadName = "writer";

StreamOutWorkerLogic::Status StreamOutWorkerLogic::cycle() {
    StreamDescriptor::Command command{};
    if (!mCommandMQ->readBlocking(&command, 1)) {
        LOG(ERROR) << __func__ << ": reading of command from MQ failed";
        return Status::ABORT;
    }
    StreamDescriptor::Reply reply{};
    if (command.code == StreamContext::COMMAND_EXIT &&
        command.fmqByteCount == mInternalCommandCookie) {
        LOG(DEBUG) << __func__ << ": received EXIT command";
        // This is an internal command, no need to reply.
        return Status::EXIT;
    } else if (command.code == StreamDescriptor::COMMAND_BURST && command.fmqByteCount >= 0) {
        LOG(DEBUG) << __func__ << ": received BURST write command for " << command.fmqByteCount
                   << " bytes";
        const size_t byteCount = std::min({static_cast<size_t>(command.fmqByteCount),
                                           mDataMQ->availableToRead(), mDataBufferSize});
        bool success = byteCount > 0 ? mDataMQ->read(&mDataBuffer[0], byteCount) : true;
        if (success) {
            const bool isConnected = mIsConnected;
            LOG(DEBUG) << __func__ << ": reading of " << byteCount << " bytes from data MQ"
                       << " succeeded; connected? " << isConnected;
            // Frames are consumed and counted regardless of connection status.
            reply.fmqByteCount = byteCount;
            mFrameCount += byteCount / mFrameSize;
            if (isConnected) {
                reply.status = STATUS_OK;
                reply.observable.frames = mFrameCount;
                reply.observable.timeNs = ::android::elapsedRealtimeNano();
            } else {
                reply.status = STATUS_INVALID_OPERATION;
            }
            usleep(3000);  // Simulate a blocking call into the driver.
        } else {
            LOG(WARNING) << __func__ << ": reading of " << byteCount
                         << " bytes of data from MQ failed";
            reply.status = STATUS_NOT_ENOUGH_DATA;
        }
        reply.latencyMs = Module::kLatencyMs;
    } else {
        LOG(WARNING) << __func__ << ": invalid command (" << command.toString()
                     << ") or count: " << command.fmqByteCount;
        reply.status = STATUS_BAD_VALUE;
    }
    LOG(DEBUG) << __func__ << ": writing reply " << reply.toString();
    if (!mReplyMQ->writeBlocking(&reply, 1)) {
        LOG(ERROR) << __func__ << ": writing of reply " << reply.toString() << " to MQ failed";
        return Status::ABORT;
    }
    return Status::CONTINUE;
}

template <class Metadata, class StreamWorker>
StreamCommon<Metadata, StreamWorker>::~StreamCommon() {
    if (!mIsClosed) {
        LOG(ERROR) << __func__ << ": stream was not closed prior to destruction, resource leak";
        stopWorker();
        // The worker and the context should clean up by themselves via destructors.
    }
}

template <class Metadata, class StreamWorker>
ndk::ScopedAStatus StreamCommon<Metadata, StreamWorker>::close() {
    LOG(DEBUG) << __func__;
    if (!mIsClosed) {
        stopWorker();
        LOG(DEBUG) << __func__ << ": joining the worker thread...";
        mWorker.stop();
        LOG(DEBUG) << __func__ << ": worker thread joined";
        mContext.reset();
        mIsClosed = true;
        return ndk::ScopedAStatus::ok();
    } else {
        LOG(ERROR) << __func__ << ": stream was already closed";
        return ndk::ScopedAStatus::fromExceptionCode(EX_ILLEGAL_STATE);
    }
}

template <class Metadata, class StreamWorker>
void StreamCommon<Metadata, StreamWorker>::stopWorker() {
    if (auto commandMQ = mContext.getCommandMQ(); commandMQ != nullptr) {
        LOG(DEBUG) << __func__ << ": asking the worker to stop...";
        StreamDescriptor::Command cmd;
        cmd.code = StreamContext::COMMAND_EXIT;
        cmd.fmqByteCount = mContext.getInternalCommandCookie();
        // FIXME: This can block in the case when the client wrote a command
        // while the stream worker's cycle is not running. Need to revisit
        // when implementing standby and pause/resume.
        if (!commandMQ->writeBlocking(&cmd, 1)) {
            LOG(ERROR) << __func__ << ": failed to write exit command to the MQ";
        }
        LOG(DEBUG) << __func__ << ": done";
    }
}

template <class Metadata, class StreamWorker>
ndk::ScopedAStatus StreamCommon<Metadata, StreamWorker>::updateMetadata(const Metadata& metadata) {
    LOG(DEBUG) << __func__;
    if (!mIsClosed) {
        mMetadata = metadata;
        return ndk::ScopedAStatus::ok();
    }
    LOG(ERROR) << __func__ << ": stream was closed";
    return ndk::ScopedAStatus::fromExceptionCode(EX_ILLEGAL_STATE);
}

StreamIn::StreamIn(const SinkMetadata& sinkMetadata, StreamContext context)
    : StreamCommon<SinkMetadata, StreamInWorker>(sinkMetadata, std::move(context)) {
    LOG(DEBUG) << __func__;
}

StreamOut::StreamOut(const SourceMetadata& sourceMetadata, StreamContext context,
                     const std::optional<AudioOffloadInfo>& offloadInfo)
    : StreamCommon<SourceMetadata, StreamOutWorker>(sourceMetadata, std::move(context)),
      mOffloadInfo(offloadInfo) {
    LOG(DEBUG) << __func__;
}

}  // namespace aidl::android::hardware::audio::core
