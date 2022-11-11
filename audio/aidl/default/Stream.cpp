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

void StreamWorkerCommonLogic::populateReply(StreamDescriptor::Reply* reply,
                                            bool isConnected) const {
    if (isConnected) {
        reply->status = STATUS_OK;
        reply->observable.frames = mFrameCount;
        reply->observable.timeNs = ::android::elapsedRealtimeNano();
    } else {
        reply->status = STATUS_NO_INIT;
    }
}

const std::string StreamInWorkerLogic::kThreadName = "reader";

StreamInWorkerLogic::Status StreamInWorkerLogic::cycle() {
    StreamDescriptor::Command command{};
    if (!mCommandMQ->readBlocking(&command, 1)) {
        LOG(ERROR) << __func__ << ": reading of command from MQ failed";
        mState = StreamDescriptor::State::ERROR;
        return Status::ABORT;
    }
    StreamDescriptor::Reply reply{};
    reply.status = STATUS_BAD_VALUE;
    using Tag = StreamDescriptor::Command::Tag;
    switch (command.getTag()) {
        case Tag::hal_reserved_exit:
            if (const int32_t cookie = command.get<Tag::hal_reserved_exit>();
                cookie == mInternalCommandCookie) {
                LOG(DEBUG) << __func__ << ": received EXIT command";
                setClosed();
                // This is an internal command, no need to reply.
                return Status::EXIT;
            } else {
                LOG(WARNING) << __func__ << ": EXIT command has a bad cookie: " << cookie;
            }
            break;
        case Tag::start:
            LOG(DEBUG) << __func__ << ": received START read command";
            if (mState == StreamDescriptor::State::STANDBY ||
                mState == StreamDescriptor::State::DRAINING) {
                populateReply(&reply, mIsConnected);
                mState = mState == StreamDescriptor::State::STANDBY
                                 ? StreamDescriptor::State::IDLE
                                 : StreamDescriptor::State::ACTIVE;
            } else {
                LOG(WARNING) << __func__ << ": START command can not be handled in the state "
                             << toString(mState);
                reply.status = STATUS_INVALID_OPERATION;
            }
            break;
        case Tag::burst:
            if (const int32_t fmqByteCount = command.get<Tag::burst>(); fmqByteCount >= 0) {
                LOG(DEBUG) << __func__ << ": received BURST read command for " << fmqByteCount
                           << " bytes";
                if (mState == StreamDescriptor::State::IDLE ||
                    mState == StreamDescriptor::State::ACTIVE ||
                    mState == StreamDescriptor::State::PAUSED ||
                    mState == StreamDescriptor::State::DRAINING) {
                    if (!read(fmqByteCount, &reply)) {
                        mState = StreamDescriptor::State::ERROR;
                    }
                    if (mState == StreamDescriptor::State::IDLE ||
                        mState == StreamDescriptor::State::PAUSED) {
                        mState = StreamDescriptor::State::ACTIVE;
                    } else if (mState == StreamDescriptor::State::DRAINING) {
                        // To simplify the reference code, we assume that the read operation
                        // has consumed all the data remaining in the hardware buffer.
                        // TODO: Provide parametrization on the duration of draining to test
                        //       handling of commands during the 'DRAINING' state.
                        mState = StreamDescriptor::State::STANDBY;
                    }
                } else {
                    LOG(WARNING) << __func__ << ": BURST command can not be handled in the state "
                                 << toString(mState);
                    reply.status = STATUS_INVALID_OPERATION;
                }
            } else {
                LOG(WARNING) << __func__ << ": invalid burst byte count: " << fmqByteCount;
            }
            break;
        case Tag::drain:
            LOG(DEBUG) << __func__ << ": received DRAIN read command";
            if (mState == StreamDescriptor::State::ACTIVE) {
                usleep(1000);  // Simulate a blocking call into the driver.
                populateReply(&reply, mIsConnected);
                // Can switch the state to ERROR if a driver error occurs.
                mState = StreamDescriptor::State::DRAINING;
            } else {
                LOG(WARNING) << __func__ << ": DRAIN command can not be handled in the state "
                             << toString(mState);
                reply.status = STATUS_INVALID_OPERATION;
            }
            break;
        case Tag::standby:
            LOG(DEBUG) << __func__ << ": received STANDBY read command";
            if (mState == StreamDescriptor::State::IDLE) {
                usleep(1000);  // Simulate a blocking call into the driver.
                populateReply(&reply, mIsConnected);
                // Can switch the state to ERROR if a driver error occurs.
                mState = StreamDescriptor::State::STANDBY;
            } else {
                LOG(WARNING) << __func__ << ": FLUSH command can not be handled in the state "
                             << toString(mState);
                reply.status = STATUS_INVALID_OPERATION;
            }
            break;
        case Tag::pause:
            LOG(DEBUG) << __func__ << ": received PAUSE read command";
            if (mState == StreamDescriptor::State::ACTIVE) {
                usleep(1000);  // Simulate a blocking call into the driver.
                populateReply(&reply, mIsConnected);
                // Can switch the state to ERROR if a driver error occurs.
                mState = StreamDescriptor::State::PAUSED;
            } else {
                LOG(WARNING) << __func__ << ": PAUSE command can not be handled in the state "
                             << toString(mState);
                reply.status = STATUS_INVALID_OPERATION;
            }
            break;
        case Tag::flush:
            LOG(DEBUG) << __func__ << ": received FLUSH read command";
            if (mState == StreamDescriptor::State::PAUSED) {
                usleep(1000);  // Simulate a blocking call into the driver.
                populateReply(&reply, mIsConnected);
                // Can switch the state to ERROR if a driver error occurs.
                mState = StreamDescriptor::State::STANDBY;
            } else {
                LOG(WARNING) << __func__ << ": FLUSH command can not be handled in the state "
                             << toString(mState);
                reply.status = STATUS_INVALID_OPERATION;
            }
            break;
    }
    reply.state = mState;
    LOG(DEBUG) << __func__ << ": writing reply " << reply.toString();
    if (!mReplyMQ->writeBlocking(&reply, 1)) {
        LOG(ERROR) << __func__ << ": writing of reply " << reply.toString() << " to MQ failed";
        mState = StreamDescriptor::State::ERROR;
        return Status::ABORT;
    }
    return Status::CONTINUE;
}

bool StreamInWorkerLogic::read(size_t clientSize, StreamDescriptor::Reply* reply) {
    // Can switch the state to ERROR if a driver error occurs.
    const size_t byteCount = std::min({clientSize, mDataMQ->availableToWrite(), mDataBufferSize});
    const bool isConnected = mIsConnected;
    bool fatal = false;
    // Simulate reading of data, or provide zeroes if the stream is not connected.
    for (size_t i = 0; i < byteCount; ++i) {
        using buffer_type = decltype(mDataBuffer)::element_type;
        constexpr int kBufferValueRange = std::numeric_limits<buffer_type>::max() -
                                          std::numeric_limits<buffer_type>::min() + 1;
        mDataBuffer[i] = isConnected ? (std::rand() % kBufferValueRange) +
                                               std::numeric_limits<buffer_type>::min()
                                     : 0;
    }
    usleep(3000);  // Simulate a blocking call into the driver.
    // Set 'fatal = true' if a driver error occurs.
    if (bool success = byteCount > 0 ? mDataMQ->write(&mDataBuffer[0], byteCount) : true; success) {
        LOG(DEBUG) << __func__ << ": writing of " << byteCount << " bytes into data MQ"
                   << " succeeded; connected? " << isConnected;
        // Frames are provided and counted regardless of connection status.
        reply->fmqByteCount += byteCount;
        mFrameCount += byteCount / mFrameSize;
        populateReply(reply, isConnected);
    } else {
        LOG(WARNING) << __func__ << ": writing of " << byteCount << " bytes of data to MQ failed";
        reply->status = STATUS_NOT_ENOUGH_DATA;
    }
    reply->latencyMs = Module::kLatencyMs;
    return !fatal;
}

const std::string StreamOutWorkerLogic::kThreadName = "writer";

StreamOutWorkerLogic::Status StreamOutWorkerLogic::cycle() {
    StreamDescriptor::Command command{};
    if (!mCommandMQ->readBlocking(&command, 1)) {
        LOG(ERROR) << __func__ << ": reading of command from MQ failed";
        mState = StreamDescriptor::State::ERROR;
        return Status::ABORT;
    }
    StreamDescriptor::Reply reply{};
    reply.status = STATUS_BAD_VALUE;
    using Tag = StreamDescriptor::Command::Tag;
    switch (command.getTag()) {
        case Tag::hal_reserved_exit:
            if (const int32_t cookie = command.get<Tag::hal_reserved_exit>();
                cookie == mInternalCommandCookie) {
                LOG(DEBUG) << __func__ << ": received EXIT command";
                setClosed();
                // This is an internal command, no need to reply.
                return Status::EXIT;
            } else {
                LOG(WARNING) << __func__ << ": EXIT command has a bad cookie: " << cookie;
            }
            break;
        case Tag::start:
            LOG(DEBUG) << __func__ << ": received START write command";
            switch (mState) {
                case StreamDescriptor::State::STANDBY:
                    mState = StreamDescriptor::State::IDLE;
                    break;
                case StreamDescriptor::State::PAUSED:
                    mState = StreamDescriptor::State::ACTIVE;
                    break;
                case StreamDescriptor::State::DRAIN_PAUSED:
                    mState = StreamDescriptor::State::PAUSED;
                    break;
                default:
                    LOG(WARNING) << __func__ << ": START command can not be handled in the state "
                                 << toString(mState);
                    reply.status = STATUS_INVALID_OPERATION;
            }
            if (reply.status != STATUS_INVALID_OPERATION) {
                populateReply(&reply, mIsConnected);
            }
            break;
        case Tag::burst:
            if (const int32_t fmqByteCount = command.get<Tag::burst>(); fmqByteCount >= 0) {
                LOG(DEBUG) << __func__ << ": received BURST write command for " << fmqByteCount
                           << " bytes";
                if (mState !=
                    StreamDescriptor::State::ERROR) {  // BURST can be handled in all valid states
                    if (!write(fmqByteCount, &reply)) {
                        mState = StreamDescriptor::State::ERROR;
                    }
                    if (mState == StreamDescriptor::State::STANDBY ||
                        mState == StreamDescriptor::State::DRAIN_PAUSED) {
                        mState = StreamDescriptor::State::PAUSED;
                    } else if (mState == StreamDescriptor::State::IDLE ||
                               mState == StreamDescriptor::State::DRAINING) {
                        mState = StreamDescriptor::State::ACTIVE;
                    }  // When in 'ACTIVE' and 'PAUSED' do not need to change the state.
                } else {
                    LOG(WARNING) << __func__ << ": BURST command can not be handled in the state "
                                 << toString(mState);
                    reply.status = STATUS_INVALID_OPERATION;
                }
            } else {
                LOG(WARNING) << __func__ << ": invalid burst byte count: " << fmqByteCount;
            }
            break;
        case Tag::drain:
            LOG(DEBUG) << __func__ << ": received DRAIN write command";
            if (mState == StreamDescriptor::State::ACTIVE) {
                usleep(1000);  // Simulate a blocking call into the driver.
                populateReply(&reply, mIsConnected);
                // Can switch the state to ERROR if a driver error occurs.
                mState = StreamDescriptor::State::IDLE;
                // Since there is no actual hardware that would be draining the buffer,
                // in order to simplify the reference code, we assume that draining
                // happens instantly, thus skipping the 'DRAINING' state.
                // TODO: Provide parametrization on the duration of draining to test
                //       handling of commands during the 'DRAINING' state.
            } else {
                LOG(WARNING) << __func__ << ": DRAIN command can not be handled in the state "
                             << toString(mState);
                reply.status = STATUS_INVALID_OPERATION;
            }
            break;
        case Tag::standby:
            LOG(DEBUG) << __func__ << ": received STANDBY write command";
            if (mState == StreamDescriptor::State::IDLE) {
                usleep(1000);  // Simulate a blocking call into the driver.
                populateReply(&reply, mIsConnected);
                // Can switch the state to ERROR if a driver error occurs.
                mState = StreamDescriptor::State::STANDBY;
            } else {
                LOG(WARNING) << __func__ << ": STANDBY command can not be handled in the state "
                             << toString(mState);
                reply.status = STATUS_INVALID_OPERATION;
            }
            break;
        case Tag::pause:
            LOG(DEBUG) << __func__ << ": received PAUSE write command";
            if (mState == StreamDescriptor::State::ACTIVE ||
                mState == StreamDescriptor::State::DRAINING) {
                populateReply(&reply, mIsConnected);
                mState = mState == StreamDescriptor::State::ACTIVE
                                 ? StreamDescriptor::State::PAUSED
                                 : StreamDescriptor::State::DRAIN_PAUSED;
            } else {
                LOG(WARNING) << __func__ << ": PAUSE command can not be handled in the state "
                             << toString(mState);
                reply.status = STATUS_INVALID_OPERATION;
            }
            break;
        case Tag::flush:
            LOG(DEBUG) << __func__ << ": received FLUSH write command";
            if (mState == StreamDescriptor::State::PAUSED ||
                mState == StreamDescriptor::State::DRAIN_PAUSED) {
                populateReply(&reply, mIsConnected);
                mState = StreamDescriptor::State::IDLE;
            } else {
                LOG(WARNING) << __func__ << ": FLUSH command can not be handled in the state "
                             << toString(mState);
                reply.status = STATUS_INVALID_OPERATION;
            }
            break;
    }
    reply.state = mState;
    LOG(DEBUG) << __func__ << ": writing reply " << reply.toString();
    if (!mReplyMQ->writeBlocking(&reply, 1)) {
        LOG(ERROR) << __func__ << ": writing of reply " << reply.toString() << " to MQ failed";
        mState = StreamDescriptor::State::ERROR;
        return Status::ABORT;
    }
    return Status::CONTINUE;
}

bool StreamOutWorkerLogic::write(size_t clientSize, StreamDescriptor::Reply* reply) {
    const size_t readByteCount = mDataMQ->availableToRead();
    // Amount of data that the HAL module is going to actually use.
    const size_t byteCount = std::min({clientSize, readByteCount, mDataBufferSize});
    bool fatal = false;
    if (bool success = readByteCount > 0 ? mDataMQ->read(&mDataBuffer[0], readByteCount) : true) {
        const bool isConnected = mIsConnected;
        LOG(DEBUG) << __func__ << ": reading of " << readByteCount << " bytes from data MQ"
                   << " succeeded; connected? " << isConnected;
        // Frames are consumed and counted regardless of connection status.
        reply->fmqByteCount += byteCount;
        mFrameCount += byteCount / mFrameSize;
        populateReply(reply, isConnected);
        usleep(3000);  // Simulate a blocking call into the driver.
        // Set 'fatal = true' if a driver error occurs.
    } else {
        LOG(WARNING) << __func__ << ": reading of " << readByteCount
                     << " bytes of data from MQ failed";
        reply->status = STATUS_NOT_ENOUGH_DATA;
    }
    reply->latencyMs = Module::kLatencyMs;
    return !fatal;
}

template <class Metadata, class StreamWorker>
StreamCommon<Metadata, StreamWorker>::~StreamCommon() {
    if (!isClosed()) {
        LOG(ERROR) << __func__ << ": stream was not closed prior to destruction, resource leak";
        stopWorker();
        // The worker and the context should clean up by themselves via destructors.
    }
}

template <class Metadata, class StreamWorker>
ndk::ScopedAStatus StreamCommon<Metadata, StreamWorker>::close() {
    LOG(DEBUG) << __func__;
    if (!isClosed()) {
        stopWorker();
        LOG(DEBUG) << __func__ << ": joining the worker thread...";
        mWorker.stop();
        LOG(DEBUG) << __func__ << ": worker thread joined";
        mContext.reset();
        mWorker.setClosed();
        return ndk::ScopedAStatus::ok();
    } else {
        LOG(ERROR) << __func__ << ": stream was already closed";
        return ndk::ScopedAStatus::fromExceptionCode(EX_ILLEGAL_STATE);
    }
}

template <class Metadata, class StreamWorker>
void StreamCommon<Metadata, StreamWorker>::stopWorker() {
    if (auto commandMQ = mContext.getCommandMQ(); commandMQ != nullptr) {
        LOG(DEBUG) << __func__ << ": asking the worker to exit...";
        auto cmd =
                StreamDescriptor::Command::make<StreamDescriptor::Command::Tag::hal_reserved_exit>(
                        mContext.getInternalCommandCookie());
        // Note: never call 'pause' and 'resume' methods of StreamWorker
        // in the HAL implementation. These methods are to be used by
        // the client side only. Preventing the worker loop from running
        // on the HAL side can cause a deadlock.
        if (!commandMQ->writeBlocking(&cmd, 1)) {
            LOG(ERROR) << __func__ << ": failed to write exit command to the MQ";
        }
        LOG(DEBUG) << __func__ << ": done";
    }
}

template <class Metadata, class StreamWorker>
ndk::ScopedAStatus StreamCommon<Metadata, StreamWorker>::updateMetadata(const Metadata& metadata) {
    LOG(DEBUG) << __func__;
    if (!isClosed()) {
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
