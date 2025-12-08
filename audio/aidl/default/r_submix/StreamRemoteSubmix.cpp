/*
 * Copyright (C) 2023 The Android Open Source Project
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

#define LOG_TAG "AHAL_StreamRemoteSubmix"
#include <android-base/logging.h>
#include <audio_utils/clock.h>
#include <error/Result.h>
#include <error/expected_utils.h>

#include "core-impl/StreamRemoteSubmix.h"

using aidl::android::hardware::audio::common::SinkMetadata;
using aidl::android::hardware::audio::common::SourceMetadata;
using aidl::android::hardware::audio::core::r_submix::SubmixRoute;
using aidl::android::media::audio::common::AudioDeviceAddress;
using aidl::android::media::audio::common::AudioDeviceType;
using aidl::android::media::audio::common::AudioOffloadInfo;
using aidl::android::media::audio::common::MicrophoneDynamicInfo;
using aidl::android::media::audio::common::MicrophoneInfo;
using android::MonoPipe;
using android::MonoPipeReader;
using android::sp;

namespace aidl::android::hardware::audio::core {

StreamRemoteSubmix::StreamRemoteSubmix(StreamContext* context, const Metadata& metadata)
    : StreamCommonImpl(context, metadata),
      mIsInput(isInput(metadata)),
      mStreamConfig{.sampleRate = context->getSampleRate(),
                    .format = context->getFormat(),
                    .channelLayout = context->getChannelLayout(),
                    .frameSize = context->getFrameSize(),
                    .frameCount = context->getBufferSizeInFrames()},
      mReadAttemptSleepUs(getDurationInUsForFrameCount(r_submix::kReadAttemptSleepFrames)) {}

StreamRemoteSubmix::~StreamRemoteSubmix() {
    cleanupWorker();
}

::android::status_t StreamRemoteSubmix::init(DriverCallbackInterface*) {
    return ::android::OK;
}

::android::status_t StreamRemoteSubmix::drain(StreamDescriptor::DrainMode) {
    return ::android::OK;
}

::android::status_t StreamRemoteSubmix::flush() {
    // TODO(b/372951987): consider if this needs to be done from 'StreamInWorkerLogic::cycle'.
    return mIsInput ? standby() : ::android::OK;
}

::android::status_t StreamRemoteSubmix::pause() {
    return ::android::OK;
}

::android::status_t StreamRemoteSubmix::standby() {
    std::lock_guard guard(mLock);
    if (mCurrentRoute) mCurrentRoute->standby(mIsInput);
    return ::android::OK;
}

::android::status_t StreamRemoteSubmix::start() {
    {
        std::lock_guard guard(mLock);
        if (mCurrentRoute) mCurrentRoute->exitStandby(mIsInput);
    }
    mStartTimeNs = ::android::uptimeNanos();
    mFramesSinceStart = 0;
    return ::android::OK;
}

// Remove references to the specified input and output streams.  When the device no longer
// references input and output streams destroy the associated pipe.
void StreamRemoteSubmix::shutdown() {
    std::shared_ptr<r_submix::SubmixRoute> currentRoute;
    {
        std::lock_guard guard(mLock);
        mCurrentRoute.swap(currentRoute);
    }
    if (!currentRoute) {
        LOG(DEBUG) << __func__ << ": no current route";
        return;
    }
    currentRoute->closeStream(mIsInput);
    // If all stream instances are closed, we can remove route information for this port.
    if (!currentRoute->hasAtleastOneStreamOpen()) {
        currentRoute->releasePipe();
        LOG(DEBUG) << __func__ << ": pipe " << currentRoute->getDeviceAddress().toString()
                   << " destroyed";
        currentRoute->remove();
    } else {
        LOG(DEBUG) << __func__ << ": pipe " << currentRoute->getDeviceAddress().toString()
                   << " status: " << currentRoute->dump();
    }
}

::android::status_t StreamRemoteSubmix::transfer(void* buffer, size_t frameCount,
                                                 size_t* actualFrameCount, int32_t* latencyMs) {
    std::shared_ptr<r_submix::SubmixRoute> currentRoute;
    {
        std::lock_guard guard(mLock);
        currentRoute = mCurrentRoute;
    }
    *latencyMs = getDurationInUsForFrameCount(getStreamPipeSizeInFrames(currentRoute)) / 1000;
    LOG(VERBOSE) << __func__ << ": Latency " << *latencyMs << "ms";
    ::android::status_t status = ::android::OK;
    if (currentRoute) {
        currentRoute->exitStandby(mIsInput);
        if (!mSkipNextTransfer) {
            status = mIsInput ? inRead(currentRoute, buffer, frameCount, actualFrameCount)
                              : outWrite(currentRoute, buffer, frameCount, actualFrameCount);
            if ((status != ::android::OK && mIsInput) ||
                ((status != ::android::OK && status != ::android::DEAD_OBJECT) && !mIsInput)) {
                return status;
            }
        } else {
            LOG(VERBOSE) << __func__ << ": Skipping transfer";
            if (mIsInput) memset(buffer, 0, mStreamConfig.frameSize * frameCount);
            *actualFrameCount = frameCount;
        }
    } else {
        LOG(WARNING) << __func__ << ": no current route";
        if (mIsInput) {
            memset(buffer, 0, mStreamConfig.frameSize * frameCount);
        }
        *actualFrameCount = frameCount;
    }
    mFramesSinceStart += *actualFrameCount;
    // If there is no route, always block, otherwise:
    //  - Input streams always need to block, output streams need to block when there is no sink.
    //  - When the sink exists, more sophisticated blocking algorithm is implemented by MonoPipe.
    if (mSkipNextTransfer || (currentRoute && !mIsInput && status != ::android::DEAD_OBJECT)) {
        mSkipNextTransfer = false;
        return ::android::OK;
    }
    const long bufferDurationUs =
            (*actualFrameCount) * MICROS_PER_SECOND / mContext.getSampleRate();
    const auto totalDurationUs = (::android::uptimeNanos() - mStartTimeNs) / NANOS_PER_MICROSECOND;
    const long totalOffsetUs = getDurationInUsForFrameCount(mFramesSinceStart) - totalDurationUs;
    LOG(VERBOSE) << __func__ << ": totalOffsetUs " << totalOffsetUs;
    if (totalOffsetUs > 0) {
        const long sleepTimeUs = std::max(0L, std::min(totalOffsetUs, bufferDurationUs));
        LOG(VERBOSE) << __func__ << ": sleeping for " << sleepTimeUs << " us";
        usleep(sleepTimeUs);
    } else if (totalOffsetUs <= -(bufferDurationUs / 2)) {
        LOG(VERBOSE) << __func__ << ": skipping next transfer";
        mSkipNextTransfer = true;
    }
    return ::android::OK;
}

::android::status_t StreamRemoteSubmix::refinePosition(StreamDescriptor::Position* position) {
    std::shared_ptr<r_submix::SubmixRoute> currentRoute;
    {
        std::lock_guard guard(mLock);
        currentRoute = mCurrentRoute;
    }
    if (!currentRoute) {
        return ::android::OK;
    }
    sp<MonoPipeReader> source = currentRoute->getSource();
    if (source == nullptr) {
        return ::android::NO_INIT;
    }
    const ssize_t framesInPipe = source->availableToRead();
    if (framesInPipe <= 0) {
        // No need to update the position frames
        return ::android::OK;
    }
    if (mIsInput) {
        position->frames += framesInPipe;
    } else if (position->frames >= framesInPipe) {
        position->frames -= framesInPipe;
    }
    return ::android::OK;
}

long StreamRemoteSubmix::getDurationInUsForFrameCount(size_t frameCount) const {
    return frameCount * MICROS_PER_SECOND / mStreamConfig.sampleRate;
}

// Calculate the maximum size of the pipe buffer in frames for the specified stream.
size_t StreamRemoteSubmix::getStreamPipeSizeInFrames(
        const std::shared_ptr<r_submix::SubmixRoute>& currentRoute) {
    if (!currentRoute) return r_submix::kDefaultPipeSizeInFrames;
    auto pipeConfig = currentRoute->getPipeConfig();
    const size_t maxFrameSize = std::max(mStreamConfig.frameSize, pipeConfig.frameSize);
    return (pipeConfig.frameCount * pipeConfig.frameSize) / maxFrameSize;
}

::android::status_t StreamRemoteSubmix::outWrite(
        const std::shared_ptr<r_submix::SubmixRoute>& currentRoute, void* buffer, size_t frameCount,
        size_t* actualFrameCount) {
    sp<MonoPipe> sink = currentRoute->getSink();
    if (sink != nullptr) {
        if (sink->isShutdown()) {
            sink.clear();
            if (++mWriteShutdownCount < kMaxErrorLogs) {
                LOG(DEBUG) << __func__ << ": pipe shutdown, ignoring the write. (limited logging)";
            }
            *actualFrameCount = frameCount;
            return ::android::DEAD_OBJECT;  // Induce wait in `transfer`.
        }
    } else {
        LOG(FATAL) << __func__ << ": without a pipe!";
        return ::android::UNKNOWN_ERROR;
    }
    mWriteShutdownCount = 0;

    LOG(VERBOSE) << __func__ << ": " << currentRoute->getDeviceAddress().toString() << ", "
                 << frameCount << " frames";

    const bool shouldBlockWrite = currentRoute->shouldBlockWrite();
    size_t availableToWrite = sink->availableToWrite();
    // NOTE: sink has been checked above and sink and source life cycles are synchronized
    sp<MonoPipeReader> source = currentRoute->getSource();
    // If the write to the sink should be blocked, flush enough frames from the pipe to make space
    // to write the most recent data.
    if (!shouldBlockWrite && availableToWrite < frameCount) {
        static uint8_t flushBuffer[64];
        const size_t flushBufferSizeFrames = sizeof(flushBuffer) / mStreamConfig.frameSize;
        size_t framesToFlushFromSource = frameCount - availableToWrite;
        LOG(DEBUG) << __func__ << ": flushing " << framesToFlushFromSource
                   << " frames from the pipe to avoid blocking";
        while (framesToFlushFromSource) {
            const size_t flushSize = std::min(framesToFlushFromSource, flushBufferSizeFrames);
            framesToFlushFromSource -= flushSize;
            // read does not block
            source->read(flushBuffer, flushSize);
        }
    }
    availableToWrite = sink->availableToWrite();

    if (!shouldBlockWrite && frameCount > availableToWrite) {
        LOG(WARNING) << __func__ << ": writing " << availableToWrite << " vs. requested "
                     << frameCount;
        // Truncate the request to avoid blocking.
        frameCount = availableToWrite;
    }
    ssize_t writtenFrames = sink->write(buffer, frameCount);
    if (writtenFrames < 0) {
        if (writtenFrames == (ssize_t)::android::NEGOTIATE) {
            LOG(ERROR) << __func__ << ": write to pipe returned NEGOTIATE";
            sink.clear();
            *actualFrameCount = 0;
            return ::android::UNKNOWN_ERROR;
        } else {
            // write() returned UNDERRUN or WOULD_BLOCK, retry
            LOG(ERROR) << __func__ << ": write to pipe returned unexpected " << writtenFrames;
            writtenFrames = sink->write(buffer, frameCount);
        }
    }

    if (writtenFrames < 0) {
        LOG(ERROR) << __func__ << ": failed writing to pipe with " << writtenFrames;
        *actualFrameCount = 0;
        return ::android::UNKNOWN_ERROR;
    }
    if (writtenFrames > 0 && frameCount > (size_t)writtenFrames) {
        LOG(WARNING) << __func__ << ": wrote " << writtenFrames << " vs. requested " << frameCount;
    }
    *actualFrameCount = writtenFrames;
    return ::android::OK;
}

::android::status_t StreamRemoteSubmix::inRead(
        const std::shared_ptr<r_submix::SubmixRoute>& currentRoute, void* buffer, size_t frameCount,
        size_t* actualFrameCount) {
    // Try to wait as long as possible for the audio duration, but leave some time for the call to
    // 'transfer' to complete. 'mReadAttemptSleepUs' is a good constant for this purpose because it
    // is by definition "strictly inferior" to the typical buffer duration.
    const long durationUs =
            std::max(0L, getDurationInUsForFrameCount(frameCount) - mReadAttemptSleepUs * 2);
    const int64_t deadlineTimeNs = ::android::uptimeNanos() + durationUs * NANOS_PER_MICROSECOND;

    // in any case, it is emulated that data for the entire buffer was available
    memset(buffer, 0, mStreamConfig.frameSize * frameCount);
    *actualFrameCount = frameCount;

    // about to read from audio source
    sp<MonoPipeReader> source = currentRoute->getSource();
    if (source == nullptr) {
        if (++mReadErrorCount < kMaxErrorLogs) {
            LOG(ERROR) << __func__
                       << ": no audio pipe yet we're trying to read! (not all errors will be "
                          "logged)";
        }
        return ::android::OK;
    }
    // get and hold the sink because 'MonoPipeReader' does not hold a strong pointer to it.
    sp<MonoPipe> sink = currentRoute->getSink();
    if (sink == nullptr) {
        if (++mReadErrorCount < kMaxErrorLogs) {
            LOG(ERROR) << __func__
                       << ": the sink has been released! (not all errors will be logged)";
        }
        return ::android::OK;
    }
    mReadErrorCount = 0;

    LOG(VERBOSE) << __func__ << ": " << currentRoute->getDeviceAddress().toString() << ", "
                 << frameCount << " frames";

    // read the data from the pipe
    char* buff = (char*)buffer;
    size_t actuallyRead = 0;
    long remainingFrames = frameCount;
    while (remainingFrames > 0) {
        ssize_t framesRead = source->read(buff, remainingFrames);
        LOG(VERBOSE) << __func__ << ": frames read " << framesRead;
        if (framesRead > 0) {
            remainingFrames -= framesRead;
            buff += framesRead * mStreamConfig.frameSize;
            LOG(VERBOSE) << __func__ << ": got " << framesRead
                         << " frames, remaining =" << remainingFrames;
            actuallyRead += framesRead;
        }
        if (::android::uptimeNanos() >= deadlineTimeNs) break;
        if (framesRead <= 0) {
            LOG(VERBOSE) << __func__ << ": read returned " << framesRead
                         << ", read failure, sleeping for " << mReadAttemptSleepUs << " us";
            usleep(mReadAttemptSleepUs);
        }
    }
    if (actuallyRead < frameCount) {
        if (++mReadFailureCount < r_submix::kMaxReadFailureAttempts) {
            LOG(WARNING) << __func__ << ": read " << actuallyRead << " vs. requested " << frameCount
                         << " (not all errors will be logged)";
        }
    } else {
        mReadFailureCount = 0;
    }
    currentRoute->updateReadCounterFrames(*actualFrameCount);
    return ::android::OK;
}

std::shared_ptr<r_submix::SubmixRoute> StreamRemoteSubmix::prepareCurrentRoute(
        const ::aidl::android::media::audio::common::AudioDeviceAddress& deviceAddress) {
    if (deviceAddress == AudioDeviceAddress{}) {
        return nullptr;
    }
    auto currentRoute = SubmixRoute::findOrCreateRoute(deviceAddress, mStreamConfig);
    if (currentRoute == nullptr) return nullptr;
    if (!currentRoute->isStreamConfigValid(mIsInput, mStreamConfig)) {
        LOG(ERROR) << __func__ << ": invalid stream config";
        return nullptr;
    }
    sp<MonoPipe> sink = currentRoute->getSink();
    if (sink == nullptr) {
        LOG(ERROR) << __func__ << ": nullptr sink when opening stream";
        return nullptr;
    }
    if ((!mIsInput || currentRoute->isStreamInOpen()) && sink->isShutdown()) {
        LOG(DEBUG) << __func__ << ": shut down sink when opening stream";
        if (::android::OK != currentRoute->resetPipe()) {
            LOG(ERROR) << __func__ << ": reset pipe failed";
            return nullptr;
        }
    }
    currentRoute->openStream(mIsInput);
    return currentRoute;
}

ndk::ScopedAStatus StreamRemoteSubmix::prepareToClose() {
    std::shared_ptr<r_submix::SubmixRoute> currentRoute;
    {
        std::lock_guard guard(mLock);
        currentRoute = mCurrentRoute;
    }
    if (currentRoute != nullptr) {
        if (!mIsInput) {
            // The client already considers this stream as closed, release the output end.
            currentRoute->closeStream(mIsInput);
        }
    } else {
        LOG(DEBUG) << __func__ << ": stream already closed";
        return ndk::ScopedAStatus::fromExceptionCode(EX_ILLEGAL_STATE);
    }
    return ndk::ScopedAStatus::ok();
}

ndk::ScopedAStatus StreamRemoteSubmix::setConnectedDevices(const ConnectedDevices& devices) {
    LOG(DEBUG) << __func__ << ": ioHandle: " << mContext.getMixPortHandle()
               << ", devices: " << ::android::internal::ToString(devices);
    if (devices.size() > 1) {
        LOG(ERROR) << __func__ << ": Only single device supported, got " << devices.size();
        return ndk::ScopedAStatus::fromExceptionCode(EX_UNSUPPORTED_OPERATION);
    }
    AudioDeviceAddress newAddress;
    if (!devices.empty()) {
        if (auto deviceDesc = devices.front().type;
            (mIsInput && deviceDesc.type != AudioDeviceType::IN_SUBMIX) ||
            (!mIsInput && deviceDesc.type != AudioDeviceType::OUT_SUBMIX)) {
            LOG(ERROR) << __func__ << ": Device type " << toString(deviceDesc.type)
                       << " not supported";
            return ndk::ScopedAStatus::fromExceptionCode(EX_UNSUPPORTED_OPERATION);
        }
        newAddress = devices.front().address;
        if (newAddress != AudioDeviceAddress{}) {
            auto existingRoute = SubmixRoute::findRoute(newAddress);
            if (existingRoute != nullptr) {
                if (!existingRoute->isStreamConfigValid(mIsInput, mStreamConfig)) {
                    LOG(ERROR) << __func__ << ": invalid stream config";
                    return ndk::ScopedAStatus::fromExceptionCode(EX_ILLEGAL_ARGUMENT);
                }
            }
        }
    }
    RETURN_STATUS_IF_ERROR(StreamCommonImpl::setConnectedDevices(devices));
    auto newCurrentRoute = prepareCurrentRoute(newAddress);
    if (newCurrentRoute) {
        std::lock_guard guard(mLock);
        mCurrentRoute = newCurrentRoute;
        LOG(DEBUG) << __func__ << ": connected to " << newAddress.toString();
    } else {
        // Do not update `mCurrentRoute`, it will be cleaned up by the worker thread.
        LOG(DEBUG) << __func__ << ": disconnected";
    }
    return ndk::ScopedAStatus::ok();
}

StreamInRemoteSubmix::StreamInRemoteSubmix(StreamContext&& context,
                                           const SinkMetadata& sinkMetadata,
                                           const std::vector<MicrophoneInfo>& microphones)
    : StreamIn(std::move(context), microphones),
      StreamRemoteSubmix(&mContextInstance, sinkMetadata) {}

ndk::ScopedAStatus StreamInRemoteSubmix::getActiveMicrophones(
        std::vector<MicrophoneDynamicInfo>* _aidl_return) {
    LOG(DEBUG) << __func__ << ": not supported";
    *_aidl_return = std::vector<MicrophoneDynamicInfo>();
    return ndk::ScopedAStatus::ok();
}

StreamOutRemoteSubmix::StreamOutRemoteSubmix(StreamContext&& context,
                                             const SourceMetadata& sourceMetadata,
                                             const std::optional<AudioOffloadInfo>& offloadInfo)
    : StreamOut(std::move(context), offloadInfo),
      StreamRemoteSubmix(&mContextInstance, sourceMetadata) {}

}  // namespace aidl::android::hardware::audio::core
