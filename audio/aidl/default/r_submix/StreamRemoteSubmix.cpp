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

#include <cmath>

#include "core-impl/StreamRemoteSubmix.h"

using aidl::android::hardware::audio::common::SinkMetadata;
using aidl::android::hardware::audio::common::SourceMetadata;
using aidl::android::hardware::audio::core::r_submix::SubmixRoute;
using aidl::android::media::audio::common::AudioDeviceAddress;
using aidl::android::media::audio::common::AudioOffloadInfo;
using aidl::android::media::audio::common::MicrophoneDynamicInfo;
using aidl::android::media::audio::common::MicrophoneInfo;

namespace aidl::android::hardware::audio::core {

StreamRemoteSubmix::StreamRemoteSubmix(StreamContext* context, const Metadata& metadata,
                                       const AudioDeviceAddress& deviceAddress)
    : StreamCommonImpl(context, metadata),
      mDeviceAddress(deviceAddress),
      mIsInput(isInput(metadata)) {
    mStreamConfig.frameSize = context->getFrameSize();
    mStreamConfig.format = context->getFormat();
    mStreamConfig.channelLayout = context->getChannelLayout();
    mStreamConfig.sampleRate = context->getSampleRate();
}

std::mutex StreamRemoteSubmix::sSubmixRoutesLock;
std::map<AudioDeviceAddress, std::shared_ptr<SubmixRoute>> StreamRemoteSubmix::sSubmixRoutes;

::android::status_t StreamRemoteSubmix::init() {
    {
        std::lock_guard guard(sSubmixRoutesLock);
        auto routeItr = sSubmixRoutes.find(mDeviceAddress);
        if (routeItr != sSubmixRoutes.end()) {
            mCurrentRoute = routeItr->second;
        }
    }
    // If route is not available for this port, add it.
    if (mCurrentRoute == nullptr) {
        // Initialize the pipe.
        mCurrentRoute = std::make_shared<SubmixRoute>();
        if (::android::OK != mCurrentRoute->createPipe(mStreamConfig)) {
            LOG(ERROR) << __func__ << ": create pipe failed";
            return ::android::NO_INIT;
        }
        {
            std::lock_guard guard(sSubmixRoutesLock);
            sSubmixRoutes.emplace(mDeviceAddress, mCurrentRoute);
        }
    } else {
        if (!mCurrentRoute->isStreamConfigValid(mIsInput, mStreamConfig)) {
            LOG(ERROR) << __func__ << ": invalid stream config";
            return ::android::NO_INIT;
        }
        sp<MonoPipe> sink = mCurrentRoute->getSink();
        if (sink == nullptr) {
            LOG(ERROR) << __func__ << ": nullptr sink when opening stream";
            return ::android::NO_INIT;
        }
        // If the sink has been shutdown or pipe recreation is forced, delete the pipe and
        // recreate it.
        if (sink->isShutdown()) {
            LOG(DEBUG) << __func__ << ": Non-nullptr shut down sink when opening stream";
            if (::android::OK != mCurrentRoute->resetPipe()) {
                LOG(ERROR) << __func__ << ": reset pipe failed";
                return ::android::NO_INIT;
            }
        }
    }

    mCurrentRoute->openStream(mIsInput);
    return ::android::OK;
}

::android::status_t StreamRemoteSubmix::drain(StreamDescriptor::DrainMode) {
    usleep(1000);
    return ::android::OK;
}

::android::status_t StreamRemoteSubmix::flush() {
    usleep(1000);
    return ::android::OK;
}

::android::status_t StreamRemoteSubmix::pause() {
    usleep(1000);
    return ::android::OK;
}

::android::status_t StreamRemoteSubmix::standby() {
    mCurrentRoute->standby(mIsInput);
    return ::android::OK;
}

::android::status_t StreamRemoteSubmix::start() {
    mCurrentRoute->exitStandby(mIsInput);
    return ::android::OK;
}

ndk::ScopedAStatus StreamRemoteSubmix::prepareToClose() {
    if (!mIsInput) {
        std::shared_ptr<SubmixRoute> route = nullptr;
        {
            std::lock_guard guard(sSubmixRoutesLock);
            auto routeItr = sSubmixRoutes.find(mDeviceAddress);
            if (routeItr != sSubmixRoutes.end()) {
                route = routeItr->second;
            }
        }
        if (route != nullptr) {
            sp<MonoPipe> sink = route->getSink();
            if (sink == nullptr) {
                ndk::ScopedAStatus::fromExceptionCode(EX_ILLEGAL_STATE);
            }
            LOG(DEBUG) << __func__ << ": shutting down MonoPipe sink";

            sink->shutdown(true);
        } else {
            LOG(DEBUG) << __func__ << ": stream already closed.";
            ndk::ScopedAStatus::fromExceptionCode(EX_ILLEGAL_STATE);
        }
    }
    return ndk::ScopedAStatus::ok();
}

// Remove references to the specified input and output streams.  When the device no longer
// references input and output streams destroy the associated pipe.
void StreamRemoteSubmix::shutdown() {
    mCurrentRoute->closeStream(mIsInput);
    // If all stream instances are closed, we can remove route information for this port.
    if (!mCurrentRoute->hasAtleastOneStreamOpen()) {
        mCurrentRoute->releasePipe();
        LOG(DEBUG) << __func__ << ": pipe destroyed";

        std::lock_guard guard(sSubmixRoutesLock);
        sSubmixRoutes.erase(mDeviceAddress);
    }
    mCurrentRoute.reset();
}

::android::status_t StreamRemoteSubmix::transfer(void* buffer, size_t frameCount,
                                                 size_t* actualFrameCount, int32_t* latencyMs) {
    *latencyMs = (getStreamPipeSizeInFrames() * MILLIS_PER_SECOND) / mStreamConfig.sampleRate;
    LOG(VERBOSE) << __func__ << ": Latency " << *latencyMs << "ms";

    sp<MonoPipe> sink = mCurrentRoute->getSink();
    if (sink != nullptr) {
        if (sink->isShutdown()) {
            sink.clear();
            LOG(VERBOSE) << __func__ << ": pipe shutdown, ignoring the transfer.";
            // the pipe has already been shutdown, this buffer will be lost but we must simulate
            // timing so we don't drain the output faster than realtime
            const size_t delayUs = static_cast<size_t>(
                    std::roundf(frameCount * MICROS_PER_SECOND / mStreamConfig.sampleRate));
            usleep(delayUs);

            *actualFrameCount = frameCount;
            return ::android::OK;
        }
    } else {
        LOG(ERROR) << __func__ << ": transfer without a pipe!";
        return ::android::UNEXPECTED_NULL;
    }
    mCurrentRoute->exitStandby(mIsInput);
    return (mIsInput ? inRead(buffer, frameCount, actualFrameCount)
                     : outWrite(buffer, frameCount, actualFrameCount));
}

::android::status_t StreamRemoteSubmix::refinePosition(StreamDescriptor::Position* position) {
    sp<MonoPipeReader> source = mCurrentRoute->getSource();
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

// Calculate the maximum size of the pipe buffer in frames for the specified stream.
size_t StreamRemoteSubmix::getStreamPipeSizeInFrames() {
    auto pipeConfig = mCurrentRoute->mPipeConfig;
    const size_t maxFrameSize = std::max(mStreamConfig.frameSize, pipeConfig.frameSize);
    return (pipeConfig.frameCount * pipeConfig.frameSize) / maxFrameSize;
}

::android::status_t StreamRemoteSubmix::outWrite(void* buffer, size_t frameCount,
                                                 size_t* actualFrameCount) {
    sp<MonoPipe> sink = mCurrentRoute->getSink();
    if (sink != nullptr) {
        if (sink->isShutdown()) {
            sink.clear();
            LOG(VERBOSE) << __func__ << ": pipe shutdown, ignoring the write.";
            // the pipe has already been shutdown, this buffer will be lost but we must
            // simulate timing so we don't drain the output faster than realtime
            const size_t delayUs = static_cast<size_t>(
                    std::roundf(frameCount * MICROS_PER_SECOND / mStreamConfig.sampleRate));
            usleep(delayUs);
            *actualFrameCount = frameCount;
            return ::android::OK;
        }
    } else {
        LOG(FATAL) << __func__ << ": without a pipe!";
        return ::android::UNKNOWN_ERROR;
    }

    const size_t availableToWrite = sink->availableToWrite();
    // NOTE: sink has been checked above and sink and source life cycles are synchronized
    sp<MonoPipeReader> source = mCurrentRoute->getSource();
    // If the write to the sink should be blocked, flush enough frames from the pipe to make space
    // to write the most recent data.
    if (!mCurrentRoute->shouldBlockWrite() && availableToWrite < frameCount) {
        static uint8_t flushBuffer[64];
        const size_t flushBufferSizeFrames = sizeof(flushBuffer) / mStreamConfig.frameSize;
        size_t framesToFlushFromSource = frameCount - availableToWrite;
        LOG(VERBOSE) << __func__ << ": flushing " << framesToFlushFromSource
                     << " frames from the pipe to avoid blocking";
        while (framesToFlushFromSource) {
            const size_t flushSize = std::min(framesToFlushFromSource, flushBufferSizeFrames);
            framesToFlushFromSource -= flushSize;
            // read does not block
            source->read(flushBuffer, flushSize);
        }
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
    sink.clear();

    if (writtenFrames < 0) {
        LOG(ERROR) << __func__ << ": failed writing to pipe with " << writtenFrames;
        *actualFrameCount = 0;
        return ::android::UNKNOWN_ERROR;
    }
    LOG(VERBOSE) << __func__ << ": wrote " << writtenFrames << "frames";
    *actualFrameCount = writtenFrames;
    return ::android::OK;
}

::android::status_t StreamRemoteSubmix::inRead(void* buffer, size_t frameCount,
                                               size_t* actualFrameCount) {
    // about to read from audio source
    sp<MonoPipeReader> source = mCurrentRoute->getSource();
    if (source == nullptr) {
        int readErrorCount = mCurrentRoute->notifyReadError();
        if (readErrorCount < kMaxReadErrorLogs) {
            LOG(ERROR) << __func__
                       << ": no audio pipe yet we're trying to read! (not all errors will be "
                          "logged)";
        } else {
            LOG(ERROR) << __func__ << ": Read errors " << readErrorCount;
        }
        const size_t delayUs = static_cast<size_t>(
                std::roundf(frameCount * MICROS_PER_SECOND / mStreamConfig.sampleRate));
        usleep(delayUs);
        memset(buffer, 0, mStreamConfig.frameSize * frameCount);
        *actualFrameCount = frameCount;
        return ::android::OK;
    }

    // read the data from the pipe
    int attempts = 0;
    const size_t delayUs = static_cast<size_t>(std::roundf(kReadAttemptSleepUs));
    char* buff = (char*)buffer;
    size_t remainingFrames = frameCount;
    int availableToRead = source->availableToRead();

    while ((remainingFrames > 0) && (availableToRead > 0) && (attempts < kMaxReadFailureAttempts)) {
        LOG(VERBOSE) << __func__ << ": frames available to read " << availableToRead;

        ssize_t framesRead = source->read(buff, remainingFrames);

        LOG(VERBOSE) << __func__ << ": frames read " << framesRead;

        if (framesRead > 0) {
            remainingFrames -= framesRead;
            buff += framesRead * mStreamConfig.frameSize;
            availableToRead -= framesRead;
            LOG(VERBOSE) << __func__ << ": (attempts = " << attempts << ") got " << framesRead
                         << " frames, remaining=" << remainingFrames;
        } else {
            attempts++;
            LOG(WARNING) << __func__ << ": read returned " << framesRead
                         << " , read failure attempts = " << attempts;
            usleep(delayUs);
        }
    }
    // done using the source
    source.clear();

    if (remainingFrames > 0) {
        const size_t remainingBytes = remainingFrames * mStreamConfig.frameSize;
        LOG(VERBOSE) << __func__ << ": clearing remaining_frames = " << remainingFrames;
        memset(((char*)buffer) + (mStreamConfig.frameSize * frameCount) - remainingBytes, 0,
               remainingBytes);
    }

    long readCounterFrames = mCurrentRoute->updateReadCounterFrames(frameCount);
    *actualFrameCount = frameCount;

    // compute how much we need to sleep after reading the data by comparing the wall clock with
    //   the projected time at which we should return.
    // wall clock after reading from the pipe
    auto recordDurationUs = std::chrono::steady_clock::now() - mCurrentRoute->getRecordStartTime();

    // readCounterFrames contains the number of frames that have been read since the beginning of
    // recording (including this call): it's converted to usec and compared to how long we've been
    // recording for, which gives us how long we must wait to sync the projected recording time, and
    // the observed recording time.
    const int projectedVsObservedOffsetUs =
            std::roundf((readCounterFrames * MICROS_PER_SECOND / mStreamConfig.sampleRate) -
                        recordDurationUs.count());

    LOG(VERBOSE) << __func__ << ": record duration " << recordDurationUs.count()
                 << " microseconds, will wait: " << projectedVsObservedOffsetUs << " microseconds";
    if (projectedVsObservedOffsetUs > 0) {
        usleep(projectedVsObservedOffsetUs);
    }
    return ::android::OK;
}

StreamInRemoteSubmix::StreamInRemoteSubmix(StreamContext&& context,
                                           const SinkMetadata& sinkMetadata,
                                           const std::vector<MicrophoneInfo>& microphones)
    : StreamIn(std::move(context), microphones), StreamSwitcher(&mContextInstance, sinkMetadata) {}

ndk::ScopedAStatus StreamInRemoteSubmix::getActiveMicrophones(
        std::vector<MicrophoneDynamicInfo>* _aidl_return) {
    LOG(DEBUG) << __func__ << ": not supported";
    *_aidl_return = std::vector<MicrophoneDynamicInfo>();
    return ndk::ScopedAStatus::ok();
}

StreamSwitcher::DeviceSwitchBehavior StreamInRemoteSubmix::switchCurrentStream(
        const std::vector<::aidl::android::media::audio::common::AudioDevice>& devices) {
    // This implementation effectively postpones stream creation until
    // receiving the first call to 'setConnectedDevices' with a non-empty list.
    if (isStubStream()) {
        if (devices.size() == 1) {
            auto deviceDesc = devices.front().type;
            if (deviceDesc.type ==
                ::aidl::android::media::audio::common::AudioDeviceType::IN_SUBMIX) {
                return DeviceSwitchBehavior::CREATE_NEW_STREAM;
            }
            LOG(ERROR) << __func__ << ": Device type " << toString(deviceDesc.type)
                       << " not supported";
        } else {
            LOG(ERROR) << __func__ << ": Only single device supported.";
        }
        return DeviceSwitchBehavior::UNSUPPORTED_DEVICES;
    }
    return DeviceSwitchBehavior::USE_CURRENT_STREAM;
}

std::unique_ptr<StreamCommonInterfaceEx> StreamInRemoteSubmix::createNewStream(
        const std::vector<::aidl::android::media::audio::common::AudioDevice>& devices,
        StreamContext* context, const Metadata& metadata) {
    return std::unique_ptr<StreamCommonInterfaceEx>(
            new InnerStreamWrapper<StreamRemoteSubmix>(context, metadata, devices.front().address));
}

StreamOutRemoteSubmix::StreamOutRemoteSubmix(StreamContext&& context,
                                             const SourceMetadata& sourceMetadata,
                                             const std::optional<AudioOffloadInfo>& offloadInfo)
    : StreamOut(std::move(context), offloadInfo),
      StreamSwitcher(&mContextInstance, sourceMetadata) {}

StreamSwitcher::DeviceSwitchBehavior StreamOutRemoteSubmix::switchCurrentStream(
        const std::vector<::aidl::android::media::audio::common::AudioDevice>& devices) {
    // This implementation effectively postpones stream creation until
    // receiving the first call to 'setConnectedDevices' with a non-empty list.
    if (isStubStream()) {
        if (devices.size() == 1) {
            auto deviceDesc = devices.front().type;
            if (deviceDesc.type ==
                ::aidl::android::media::audio::common::AudioDeviceType::OUT_SUBMIX) {
                return DeviceSwitchBehavior::CREATE_NEW_STREAM;
            }
            LOG(ERROR) << __func__ << ": Device type " << toString(deviceDesc.type)
                       << " not supported";
        } else {
            LOG(ERROR) << __func__ << ": Only single device supported.";
        }
        return DeviceSwitchBehavior::UNSUPPORTED_DEVICES;
    }
    return DeviceSwitchBehavior::USE_CURRENT_STREAM;
}

std::unique_ptr<StreamCommonInterfaceEx> StreamOutRemoteSubmix::createNewStream(
        const std::vector<::aidl::android::media::audio::common::AudioDevice>& devices,
        StreamContext* context, const Metadata& metadata) {
    return std::unique_ptr<StreamCommonInterfaceEx>(
            new InnerStreamWrapper<StreamRemoteSubmix>(context, metadata, devices.front().address));
}

}  // namespace aidl::android::hardware::audio::core
