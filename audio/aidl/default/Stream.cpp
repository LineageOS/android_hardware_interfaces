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
#include <android/binder_ibinder_platform.h>
#include <utils/SystemClock.h>

#include <Utils.h>

#include "core-impl/Module.h"
#include "core-impl/Stream.h"

using aidl::android::hardware::audio::common::SinkMetadata;
using aidl::android::hardware::audio::common::SourceMetadata;
using aidl::android::media::audio::common::AudioDevice;
using aidl::android::media::audio::common::AudioDualMonoMode;
using aidl::android::media::audio::common::AudioLatencyMode;
using aidl::android::media::audio::common::AudioOffloadInfo;
using aidl::android::media::audio::common::AudioPlaybackRate;
using android::hardware::audio::common::getChannelCount;
using android::hardware::audio::common::getFrameSizeInBytes;

namespace aidl::android::hardware::audio::core {

void StreamContext::fillDescriptor(StreamDescriptor* desc) {
    if (mCommandMQ) {
        desc->command = mCommandMQ->dupeDesc();
    }
    if (mReplyMQ) {
        desc->reply = mReplyMQ->dupeDesc();
    }
    if (mDataMQ) {
        const size_t frameSize = getFrameSize();
        desc->frameSizeBytes = frameSize;
        desc->bufferSizeFrames = mDataMQ->getQuantumCount() * mDataMQ->getQuantumSize() / frameSize;
        desc->audio.set<StreamDescriptor::AudioBuffer::Tag::fmq>(mDataMQ->dupeDesc());
    }
}

size_t StreamContext::getFrameSize() const {
    return getFrameSizeInBytes(mFormat, mChannelLayout);
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
    if (getFrameSize() == 0) {
        LOG(ERROR) << "frame size is invalid";
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
    reply->status = STATUS_OK;
    if (isConnected) {
        reply->observable.frames = mFrameCount;
        reply->observable.timeNs = ::android::elapsedRealtimeNano();
    } else {
        reply->observable.frames = StreamDescriptor::Position::UNKNOWN;
        reply->observable.timeNs = StreamDescriptor::Position::UNKNOWN;
    }
}

void StreamWorkerCommonLogic::populateReplyWrongState(
        StreamDescriptor::Reply* reply, const StreamDescriptor::Command& command) const {
    LOG(WARNING) << "command '" << toString(command.getTag())
                 << "' can not be handled in the state " << toString(mState);
    reply->status = STATUS_INVALID_OPERATION;
}

const std::string StreamInWorkerLogic::kThreadName = "reader";

StreamInWorkerLogic::Status StreamInWorkerLogic::cycle() {
    // Note: for input streams, draining is driven by the client, thus
    // "empty buffer" condition can only happen while handling the 'burst'
    // command. Thus, unlike for output streams, it does not make sense to
    // delay the 'DRAINING' state here by 'mTransientStateDelayMs'.
    // TODO: Add a delay for transitions of async operations when/if they added.

    StreamDescriptor::Command command{};
    if (!mCommandMQ->readBlocking(&command, 1)) {
        LOG(ERROR) << __func__ << ": reading of command from MQ failed";
        mState = StreamDescriptor::State::ERROR;
        return Status::ABORT;
    }
    LOG(DEBUG) << __func__ << ": received command " << command.toString() << " in " << kThreadName;
    StreamDescriptor::Reply reply{};
    reply.status = STATUS_BAD_VALUE;
    using Tag = StreamDescriptor::Command::Tag;
    switch (command.getTag()) {
        case Tag::halReservedExit:
            if (const int32_t cookie = command.get<Tag::halReservedExit>();
                cookie == mInternalCommandCookie) {
                setClosed();
                // This is an internal command, no need to reply.
                return Status::EXIT;
            } else {
                LOG(WARNING) << __func__ << ": EXIT command has a bad cookie: " << cookie;
            }
            break;
        case Tag::getStatus:
            populateReply(&reply, mIsConnected);
            break;
        case Tag::start:
            if (mState == StreamDescriptor::State::STANDBY ||
                mState == StreamDescriptor::State::DRAINING) {
                populateReply(&reply, mIsConnected);
                mState = mState == StreamDescriptor::State::STANDBY
                                 ? StreamDescriptor::State::IDLE
                                 : StreamDescriptor::State::ACTIVE;
            } else {
                populateReplyWrongState(&reply, command);
            }
            break;
        case Tag::burst:
            if (const int32_t fmqByteCount = command.get<Tag::burst>(); fmqByteCount >= 0) {
                LOG(DEBUG) << __func__ << ": '" << toString(command.getTag()) << "' command for "
                           << fmqByteCount << " bytes";
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
                        // In a real implementation, here we would either remain in
                        // the 'DRAINING' state, or transfer to 'STANDBY' depending on the
                        // buffer state.
                        mState = StreamDescriptor::State::STANDBY;
                    }
                } else {
                    populateReplyWrongState(&reply, command);
                }
            } else {
                LOG(WARNING) << __func__ << ": invalid burst byte count: " << fmqByteCount;
            }
            break;
        case Tag::drain:
            if (command.get<Tag::drain>() == StreamDescriptor::DrainMode::DRAIN_UNSPECIFIED) {
                if (mState == StreamDescriptor::State::ACTIVE) {
                    usleep(1000);  // Simulate a blocking call into the driver.
                    populateReply(&reply, mIsConnected);
                    // Can switch the state to ERROR if a driver error occurs.
                    mState = StreamDescriptor::State::DRAINING;
                } else {
                    populateReplyWrongState(&reply, command);
                }
            } else {
                LOG(WARNING) << __func__
                             << ": invalid drain mode: " << toString(command.get<Tag::drain>());
            }
            break;
        case Tag::standby:
            if (mState == StreamDescriptor::State::IDLE) {
                usleep(1000);  // Simulate a blocking call into the driver.
                populateReply(&reply, mIsConnected);
                // Can switch the state to ERROR if a driver error occurs.
                mState = StreamDescriptor::State::STANDBY;
            } else {
                populateReplyWrongState(&reply, command);
            }
            break;
        case Tag::pause:
            if (mState == StreamDescriptor::State::ACTIVE) {
                usleep(1000);  // Simulate a blocking call into the driver.
                populateReply(&reply, mIsConnected);
                // Can switch the state to ERROR if a driver error occurs.
                mState = StreamDescriptor::State::PAUSED;
            } else {
                populateReplyWrongState(&reply, command);
            }
            break;
        case Tag::flush:
            if (mState == StreamDescriptor::State::PAUSED) {
                usleep(1000);  // Simulate a blocking call into the driver.
                populateReply(&reply, mIsConnected);
                // Can switch the state to ERROR if a driver error occurs.
                mState = StreamDescriptor::State::STANDBY;
            } else {
                populateReplyWrongState(&reply, command);
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
    if (mState == StreamDescriptor::State::DRAINING ||
        mState == StreamDescriptor::State::TRANSFERRING) {
        if (auto stateDurationMs = std::chrono::duration_cast<std::chrono::milliseconds>(
                    std::chrono::steady_clock::now() - mTransientStateStart);
            stateDurationMs >= mTransientStateDelayMs) {
            if (mAsyncCallback == nullptr) {
                // In blocking mode, mState can only be DRAINING.
                mState = StreamDescriptor::State::IDLE;
            } else {
                // In a real implementation, the driver should notify the HAL about
                // drain or transfer completion. In the stub, we switch unconditionally.
                if (mState == StreamDescriptor::State::DRAINING) {
                    mState = StreamDescriptor::State::IDLE;
                    ndk::ScopedAStatus status = mAsyncCallback->onDrainReady();
                    if (!status.isOk()) {
                        LOG(ERROR) << __func__ << ": error from onDrainReady: " << status;
                    }
                } else {
                    mState = StreamDescriptor::State::ACTIVE;
                    ndk::ScopedAStatus status = mAsyncCallback->onTransferReady();
                    if (!status.isOk()) {
                        LOG(ERROR) << __func__ << ": error from onTransferReady: " << status;
                    }
                }
            }
            if (mTransientStateDelayMs.count() != 0) {
                LOG(DEBUG) << __func__ << ": switched to state " << toString(mState)
                           << " after a timeout";
            }
        }
    }

    StreamDescriptor::Command command{};
    if (!mCommandMQ->readBlocking(&command, 1)) {
        LOG(ERROR) << __func__ << ": reading of command from MQ failed";
        mState = StreamDescriptor::State::ERROR;
        return Status::ABORT;
    }
    LOG(DEBUG) << __func__ << ": received command " << command.toString() << " in " << kThreadName;
    StreamDescriptor::Reply reply{};
    reply.status = STATUS_BAD_VALUE;
    using Tag = StreamDescriptor::Command::Tag;
    switch (command.getTag()) {
        case Tag::halReservedExit:
            if (const int32_t cookie = command.get<Tag::halReservedExit>();
                cookie == mInternalCommandCookie) {
                setClosed();
                // This is an internal command, no need to reply.
                return Status::EXIT;
            } else {
                LOG(WARNING) << __func__ << ": EXIT command has a bad cookie: " << cookie;
            }
            break;
        case Tag::getStatus:
            populateReply(&reply, mIsConnected);
            break;
        case Tag::start: {
            bool commandAccepted = true;
            switch (mState) {
                case StreamDescriptor::State::STANDBY:
                    mState = StreamDescriptor::State::IDLE;
                    break;
                case StreamDescriptor::State::PAUSED:
                    mState = StreamDescriptor::State::ACTIVE;
                    break;
                case StreamDescriptor::State::DRAIN_PAUSED:
                    switchToTransientState(StreamDescriptor::State::DRAINING);
                    break;
                case StreamDescriptor::State::TRANSFER_PAUSED:
                    switchToTransientState(StreamDescriptor::State::TRANSFERRING);
                    break;
                default:
                    populateReplyWrongState(&reply, command);
                    commandAccepted = false;
            }
            if (commandAccepted) {
                populateReply(&reply, mIsConnected);
            }
        } break;
        case Tag::burst:
            if (const int32_t fmqByteCount = command.get<Tag::burst>(); fmqByteCount >= 0) {
                LOG(DEBUG) << __func__ << ": '" << toString(command.getTag()) << "' command for "
                           << fmqByteCount << " bytes";
                if (mState != StreamDescriptor::State::ERROR &&
                    mState != StreamDescriptor::State::TRANSFERRING &&
                    mState != StreamDescriptor::State::TRANSFER_PAUSED) {
                    if (!write(fmqByteCount, &reply)) {
                        mState = StreamDescriptor::State::ERROR;
                    }
                    if (mState == StreamDescriptor::State::STANDBY ||
                        mState == StreamDescriptor::State::DRAIN_PAUSED ||
                        mState == StreamDescriptor::State::PAUSED) {
                        if (mAsyncCallback == nullptr ||
                            mState != StreamDescriptor::State::DRAIN_PAUSED) {
                            mState = StreamDescriptor::State::PAUSED;
                        } else {
                            mState = StreamDescriptor::State::TRANSFER_PAUSED;
                        }
                    } else if (mState == StreamDescriptor::State::IDLE ||
                               mState == StreamDescriptor::State::DRAINING ||
                               mState == StreamDescriptor::State::ACTIVE) {
                        if (mAsyncCallback == nullptr || reply.fmqByteCount == fmqByteCount) {
                            mState = StreamDescriptor::State::ACTIVE;
                        } else {
                            switchToTransientState(StreamDescriptor::State::TRANSFERRING);
                        }
                    }
                } else {
                    populateReplyWrongState(&reply, command);
                }
            } else {
                LOG(WARNING) << __func__ << ": invalid burst byte count: " << fmqByteCount;
            }
            break;
        case Tag::drain:
            if (command.get<Tag::drain>() == StreamDescriptor::DrainMode::DRAIN_ALL ||
                command.get<Tag::drain>() == StreamDescriptor::DrainMode::DRAIN_EARLY_NOTIFY) {
                if (mState == StreamDescriptor::State::ACTIVE ||
                    mState == StreamDescriptor::State::TRANSFERRING) {
                    usleep(1000);  // Simulate a blocking call into the driver.
                    populateReply(&reply, mIsConnected);
                    // Can switch the state to ERROR if a driver error occurs.
                    switchToTransientState(StreamDescriptor::State::DRAINING);
                } else if (mState == StreamDescriptor::State::TRANSFER_PAUSED) {
                    mState = StreamDescriptor::State::DRAIN_PAUSED;
                    populateReply(&reply, mIsConnected);
                } else {
                    populateReplyWrongState(&reply, command);
                }
            } else {
                LOG(WARNING) << __func__
                             << ": invalid drain mode: " << toString(command.get<Tag::drain>());
            }
            break;
        case Tag::standby:
            if (mState == StreamDescriptor::State::IDLE) {
                usleep(1000);  // Simulate a blocking call into the driver.
                populateReply(&reply, mIsConnected);
                // Can switch the state to ERROR if a driver error occurs.
                mState = StreamDescriptor::State::STANDBY;
            } else {
                populateReplyWrongState(&reply, command);
            }
            break;
        case Tag::pause: {
            bool commandAccepted = true;
            switch (mState) {
                case StreamDescriptor::State::ACTIVE:
                    mState = StreamDescriptor::State::PAUSED;
                    break;
                case StreamDescriptor::State::DRAINING:
                    mState = StreamDescriptor::State::DRAIN_PAUSED;
                    break;
                case StreamDescriptor::State::TRANSFERRING:
                    mState = StreamDescriptor::State::TRANSFER_PAUSED;
                    break;
                default:
                    populateReplyWrongState(&reply, command);
                    commandAccepted = false;
            }
            if (commandAccepted) {
                populateReply(&reply, mIsConnected);
            }
        } break;
        case Tag::flush:
            if (mState == StreamDescriptor::State::PAUSED ||
                mState == StreamDescriptor::State::DRAIN_PAUSED ||
                mState == StreamDescriptor::State::TRANSFER_PAUSED) {
                populateReply(&reply, mIsConnected);
                mState = StreamDescriptor::State::IDLE;
            } else {
                populateReplyWrongState(&reply, command);
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
StreamCommonImpl<Metadata, StreamWorker>::~StreamCommonImpl() {
    if (!isClosed()) {
        LOG(ERROR) << __func__ << ": stream was not closed prior to destruction, resource leak";
        stopWorker();
        // The worker and the context should clean up by themselves via destructors.
    }
}

template <class Metadata, class StreamWorker>
void StreamCommonImpl<Metadata, StreamWorker>::createStreamCommon(
        const std::shared_ptr<StreamCommonInterface>& delegate) {
    if (mCommon != nullptr) {
        LOG(FATAL) << __func__ << ": attempting to create the common interface twice";
    }
    mCommon = ndk::SharedRefBase::make<StreamCommon>(delegate);
    mCommonBinder = mCommon->asBinder();
    AIBinder_setMinSchedulerPolicy(mCommonBinder.get(), SCHED_NORMAL, ANDROID_PRIORITY_AUDIO);
}

template <class Metadata, class StreamWorker>
ndk::ScopedAStatus StreamCommonImpl<Metadata, StreamWorker>::getStreamCommon(
        std::shared_ptr<IStreamCommon>* _aidl_return) {
    if (mCommon == nullptr) {
        LOG(FATAL) << __func__ << ": the common interface was not created";
    }
    *_aidl_return = mCommon;
    LOG(DEBUG) << __func__ << ": returning " << _aidl_return->get()->asBinder().get();
    return ndk::ScopedAStatus::ok();
}

template <class Metadata, class StreamWorker>
ndk::ScopedAStatus StreamCommonImpl<Metadata, StreamWorker>::updateHwAvSyncId(
        int32_t in_hwAvSyncId) {
    LOG(DEBUG) << __func__ << ": id " << in_hwAvSyncId;
    return ndk::ScopedAStatus::fromExceptionCode(EX_UNSUPPORTED_OPERATION);
}

template <class Metadata, class StreamWorker>
ndk::ScopedAStatus StreamCommonImpl<Metadata, StreamWorker>::getVendorParameters(
        const std::vector<std::string>& in_ids, std::vector<VendorParameter>* _aidl_return) {
    LOG(DEBUG) << __func__ << ": id count: " << in_ids.size();
    (void)_aidl_return;
    return ndk::ScopedAStatus::fromExceptionCode(EX_UNSUPPORTED_OPERATION);
}

template <class Metadata, class StreamWorker>
ndk::ScopedAStatus StreamCommonImpl<Metadata, StreamWorker>::setVendorParameters(
        const std::vector<VendorParameter>& in_parameters, bool in_async) {
    LOG(DEBUG) << __func__ << ": parameters count " << in_parameters.size()
               << ", async: " << in_async;
    return ndk::ScopedAStatus::fromExceptionCode(EX_UNSUPPORTED_OPERATION);
}

template <class Metadata, class StreamWorker>
ndk::ScopedAStatus StreamCommonImpl<Metadata, StreamWorker>::addEffect(
        const std::shared_ptr<::aidl::android::hardware::audio::effect::IEffect>& in_effect) {
    if (in_effect == nullptr) {
        LOG(DEBUG) << __func__ << ": null effect";
    } else {
        LOG(DEBUG) << __func__ << ": effect Binder" << in_effect->asBinder().get();
    }
    return ndk::ScopedAStatus::fromExceptionCode(EX_UNSUPPORTED_OPERATION);
}

template <class Metadata, class StreamWorker>
ndk::ScopedAStatus StreamCommonImpl<Metadata, StreamWorker>::removeEffect(
        const std::shared_ptr<::aidl::android::hardware::audio::effect::IEffect>& in_effect) {
    if (in_effect == nullptr) {
        LOG(DEBUG) << __func__ << ": null effect";
    } else {
        LOG(DEBUG) << __func__ << ": effect Binder" << in_effect->asBinder().get();
    }
    return ndk::ScopedAStatus::fromExceptionCode(EX_UNSUPPORTED_OPERATION);
}

template <class Metadata, class StreamWorker>
ndk::ScopedAStatus StreamCommonImpl<Metadata, StreamWorker>::close() {
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
void StreamCommonImpl<Metadata, StreamWorker>::stopWorker() {
    if (auto commandMQ = mContext.getCommandMQ(); commandMQ != nullptr) {
        LOG(DEBUG) << __func__ << ": asking the worker to exit...";
        auto cmd = StreamDescriptor::Command::make<StreamDescriptor::Command::Tag::halReservedExit>(
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
ndk::ScopedAStatus StreamCommonImpl<Metadata, StreamWorker>::updateMetadata(
        const Metadata& metadata) {
    LOG(DEBUG) << __func__;
    if (!isClosed()) {
        mMetadata = metadata;
        return ndk::ScopedAStatus::ok();
    }
    LOG(ERROR) << __func__ << ": stream was closed";
    return ndk::ScopedAStatus::fromExceptionCode(EX_ILLEGAL_STATE);
}

// static
ndk::ScopedAStatus StreamIn::createInstance(const common::SinkMetadata& sinkMetadata,
                                            StreamContext context,
                                            const std::vector<MicrophoneInfo>& microphones,
                                            std::shared_ptr<StreamIn>* result) {
    auto stream = ndk::SharedRefBase::make<StreamIn>(sinkMetadata, std::move(context), microphones);
    if (auto status = stream->init(); !status.isOk()) {
        return status;
    }
    stream->createStreamCommon(stream);
    *result = std::move(stream);
    return ndk::ScopedAStatus::ok();
}

namespace {
static std::map<AudioDevice, std::string> transformMicrophones(
        const std::vector<MicrophoneInfo>& microphones) {
    std::map<AudioDevice, std::string> result;
    std::transform(microphones.begin(), microphones.end(), std::inserter(result, result.begin()),
                   [](const auto& mic) { return std::make_pair(mic.device, mic.id); });
    return result;
}
}  // namespace

StreamIn::StreamIn(const SinkMetadata& sinkMetadata, StreamContext&& context,
                   const std::vector<MicrophoneInfo>& microphones)
    : StreamCommonImpl<SinkMetadata, StreamInWorker>(sinkMetadata, std::move(context)),
      mMicrophones(transformMicrophones(microphones)) {
    LOG(DEBUG) << __func__;
}

ndk::ScopedAStatus StreamIn::getActiveMicrophones(
        std::vector<MicrophoneDynamicInfo>* _aidl_return) {
    std::vector<MicrophoneDynamicInfo> result;
    std::vector<MicrophoneDynamicInfo::ChannelMapping> channelMapping{
            getChannelCount(mContext.getChannelLayout()),
            MicrophoneDynamicInfo::ChannelMapping::DIRECT};
    for (auto it = mConnectedDevices.begin(); it != mConnectedDevices.end(); ++it) {
        if (auto micIt = mMicrophones.find(*it); micIt != mMicrophones.end()) {
            MicrophoneDynamicInfo dynMic;
            dynMic.id = micIt->second;
            dynMic.channelMapping = channelMapping;
            result.push_back(std::move(dynMic));
        }
    }
    *_aidl_return = std::move(result);
    LOG(DEBUG) << __func__ << ": returning " << ::android::internal::ToString(*_aidl_return);
    return ndk::ScopedAStatus::ok();
}

ndk::ScopedAStatus StreamIn::getMicrophoneDirection(MicrophoneDirection* _aidl_return) {
    LOG(DEBUG) << __func__;
    (void)_aidl_return;
    return ndk::ScopedAStatus::fromExceptionCode(EX_UNSUPPORTED_OPERATION);
}

ndk::ScopedAStatus StreamIn::setMicrophoneDirection(MicrophoneDirection in_direction) {
    LOG(DEBUG) << __func__ << ": direction " << toString(in_direction);
    return ndk::ScopedAStatus::fromExceptionCode(EX_UNSUPPORTED_OPERATION);
}

ndk::ScopedAStatus StreamIn::getMicrophoneFieldDimension(float* _aidl_return) {
    LOG(DEBUG) << __func__;
    (void)_aidl_return;
    return ndk::ScopedAStatus::fromExceptionCode(EX_UNSUPPORTED_OPERATION);
}

ndk::ScopedAStatus StreamIn::setMicrophoneFieldDimension(float in_zoom) {
    LOG(DEBUG) << __func__ << ": zoom " << in_zoom;
    return ndk::ScopedAStatus::fromExceptionCode(EX_UNSUPPORTED_OPERATION);
}

ndk::ScopedAStatus StreamIn::getHwGain(std::vector<float>* _aidl_return) {
    LOG(DEBUG) << __func__;
    (void)_aidl_return;
    return ndk::ScopedAStatus::fromExceptionCode(EX_UNSUPPORTED_OPERATION);
}

ndk::ScopedAStatus StreamIn::setHwGain(const std::vector<float>& in_channelGains) {
    LOG(DEBUG) << __func__ << ": gains " << ::android::internal::ToString(in_channelGains);
    return ndk::ScopedAStatus::fromExceptionCode(EX_UNSUPPORTED_OPERATION);
}

// static
ndk::ScopedAStatus StreamOut::createInstance(const SourceMetadata& sourceMetadata,
                                             StreamContext context,
                                             const std::optional<AudioOffloadInfo>& offloadInfo,
                                             std::shared_ptr<StreamOut>* result) {
    auto stream =
            ndk::SharedRefBase::make<StreamOut>(sourceMetadata, std::move(context), offloadInfo);
    if (auto status = stream->init(); !status.isOk()) {
        return status;
    }
    stream->createStreamCommon(stream);
    *result = std::move(stream);
    return ndk::ScopedAStatus::ok();
}

StreamOut::StreamOut(const SourceMetadata& sourceMetadata, StreamContext&& context,
                     const std::optional<AudioOffloadInfo>& offloadInfo)
    : StreamCommonImpl<SourceMetadata, StreamOutWorker>(sourceMetadata, std::move(context)),
      mOffloadInfo(offloadInfo) {
    LOG(DEBUG) << __func__;
}

ndk::ScopedAStatus StreamOut::getHwVolume(std::vector<float>* _aidl_return) {
    LOG(DEBUG) << __func__;
    (void)_aidl_return;
    return ndk::ScopedAStatus::fromExceptionCode(EX_UNSUPPORTED_OPERATION);
}

ndk::ScopedAStatus StreamOut::setHwVolume(const std::vector<float>& in_channelVolumes) {
    LOG(DEBUG) << __func__ << ": gains " << ::android::internal::ToString(in_channelVolumes);
    return ndk::ScopedAStatus::fromExceptionCode(EX_UNSUPPORTED_OPERATION);
}

ndk::ScopedAStatus StreamOut::getAudioDescriptionMixLevel(float* _aidl_return) {
    LOG(DEBUG) << __func__;
    (void)_aidl_return;
    return ndk::ScopedAStatus::fromExceptionCode(EX_UNSUPPORTED_OPERATION);
}

ndk::ScopedAStatus StreamOut::setAudioDescriptionMixLevel(float in_leveldB) {
    LOG(DEBUG) << __func__ << ": description mix level " << in_leveldB;
    return ndk::ScopedAStatus::fromExceptionCode(EX_UNSUPPORTED_OPERATION);
}

ndk::ScopedAStatus StreamOut::getDualMonoMode(AudioDualMonoMode* _aidl_return) {
    LOG(DEBUG) << __func__;
    (void)_aidl_return;
    return ndk::ScopedAStatus::fromExceptionCode(EX_UNSUPPORTED_OPERATION);
}

ndk::ScopedAStatus StreamOut::setDualMonoMode(AudioDualMonoMode in_mode) {
    LOG(DEBUG) << __func__ << ": dual mono mode " << toString(in_mode);
    return ndk::ScopedAStatus::fromExceptionCode(EX_UNSUPPORTED_OPERATION);
}

ndk::ScopedAStatus StreamOut::getRecommendedLatencyModes(
        std::vector<AudioLatencyMode>* _aidl_return) {
    LOG(DEBUG) << __func__;
    (void)_aidl_return;
    return ndk::ScopedAStatus::fromExceptionCode(EX_UNSUPPORTED_OPERATION);
}

ndk::ScopedAStatus StreamOut::setLatencyMode(AudioLatencyMode in_mode) {
    LOG(DEBUG) << __func__ << ": latency mode " << toString(in_mode);
    return ndk::ScopedAStatus::fromExceptionCode(EX_UNSUPPORTED_OPERATION);
}

ndk::ScopedAStatus StreamOut::getPlaybackRateParameters(AudioPlaybackRate* _aidl_return) {
    LOG(DEBUG) << __func__;
    (void)_aidl_return;
    return ndk::ScopedAStatus::fromExceptionCode(EX_UNSUPPORTED_OPERATION);
}

ndk::ScopedAStatus StreamOut::setPlaybackRateParameters(const AudioPlaybackRate& in_playbackRate) {
    LOG(DEBUG) << __func__ << ": " << in_playbackRate.toString();
    return ndk::ScopedAStatus::fromExceptionCode(EX_UNSUPPORTED_OPERATION);
}

ndk::ScopedAStatus StreamOut::selectPresentation(int32_t in_presentationId, int32_t in_programId) {
    LOG(DEBUG) << __func__ << ": presentationId " << in_presentationId << ", programId "
               << in_programId;
    return ndk::ScopedAStatus::fromExceptionCode(EX_UNSUPPORTED_OPERATION);
}

}  // namespace aidl::android::hardware::audio::core
