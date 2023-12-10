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

#define LOG_TAG "AHAL_SubmixRoute"
#include <android-base/logging.h>
#include <media/AidlConversionCppNdk.h>

#include <Utils.h>

#include "SubmixRoute.h"

using aidl::android::hardware::audio::common::getChannelCount;

namespace aidl::android::hardware::audio::core::r_submix {

// Verify a submix input or output stream can be opened.
bool SubmixRoute::isStreamConfigValid(bool isInput, const AudioConfig& streamConfig) {
    // If the stream is already open, don't open it again.
    // ENABLE_LEGACY_INPUT_OPEN is default behaviour
    if (!isInput && isStreamOutOpen()) {
        LOG(ERROR) << __func__ << ": output stream already open.";
        return false;
    }
    // If either stream is open, verify the existing pipe config matches the stream config.
    if (hasAtleastOneStreamOpen() && !isStreamConfigCompatible(streamConfig)) {
        return false;
    }
    return true;
}

// Compare this stream config with existing pipe config, returning false if they do *not*
// match, true otherwise.
bool SubmixRoute::isStreamConfigCompatible(const AudioConfig& streamConfig) {
    if (streamConfig.channelLayout != mPipeConfig.channelLayout) {
        LOG(ERROR) << __func__ << ": channel count mismatch, stream channels = "
                   << streamConfig.channelLayout.toString()
                   << " pipe config channels = " << mPipeConfig.channelLayout.toString();
        return false;
    }
    if (streamConfig.sampleRate != mPipeConfig.sampleRate) {
        LOG(ERROR) << __func__
                   << ": sample rate mismatch, stream sample rate = " << streamConfig.sampleRate
                   << " pipe config sample rate = " << mPipeConfig.sampleRate;
        return false;
    }
    if (streamConfig.format != mPipeConfig.format) {
        LOG(ERROR) << __func__
                   << ": format mismatch, stream format = " << streamConfig.format.toString()
                   << " pipe config format = " << mPipeConfig.format.toString();
        return false;
    }
    return true;
}

bool SubmixRoute::hasAtleastOneStreamOpen() {
    std::lock_guard guard(mLock);
    return (mStreamInOpen || mStreamOutOpen);
}

// We DO NOT block if:
// - no peer input stream is present
// - the peer input is in standby AFTER having been active.
// We DO block if:
// - the input was never activated to avoid discarding first frames in the pipe in case capture
// start was delayed
bool SubmixRoute::shouldBlockWrite() {
    std::lock_guard guard(mLock);
    return (mStreamInOpen || (mStreamInStandby && (mReadCounterFrames != 0)));
}

int SubmixRoute::notifyReadError() {
    std::lock_guard guard(mLock);
    return ++mReadErrorCount;
}

long SubmixRoute::updateReadCounterFrames(size_t frameCount) {
    std::lock_guard guard(mLock);
    mReadCounterFrames += frameCount;
    return mReadCounterFrames;
}

void SubmixRoute::openStream(bool isInput) {
    std::lock_guard guard(mLock);
    if (isInput) {
        if (mStreamInOpen) {
            mInputRefCount++;
        } else {
            mInputRefCount = 1;
            mStreamInOpen = true;
        }
        mStreamInStandby = true;
        mReadCounterFrames = 0;
        mReadErrorCount = 0;
    } else {
        mStreamOutOpen = true;
    }
}

void SubmixRoute::closeStream(bool isInput) {
    std::lock_guard guard(mLock);
    if (isInput) {
        mInputRefCount--;
        if (mInputRefCount == 0) {
            mStreamInOpen = false;
            if (mSink != nullptr) {
                mSink->shutdown(true);
            }
        }
    } else {
        mStreamOutOpen = false;
    }
}

// If SubmixRoute doesn't exist for a port, create a pipe for the submix audio device of size
// buffer_size_frames and store config of the submix audio device.
::android::status_t SubmixRoute::createPipe(const AudioConfig& streamConfig) {
    const int channelCount = getChannelCount(streamConfig.channelLayout);
    const audio_format_t audioFormat = VALUE_OR_RETURN_STATUS(
            aidl2legacy_AudioFormatDescription_audio_format_t(streamConfig.format));
    const ::android::NBAIO_Format format =
            ::android::Format_from_SR_C(streamConfig.sampleRate, channelCount, audioFormat);
    const ::android::NBAIO_Format offers[1] = {format};
    size_t numCounterOffers = 0;

    const size_t pipeSizeInFrames =
            r_submix::kDefaultPipeSizeInFrames *
            ((float)streamConfig.sampleRate / r_submix::kDefaultSampleRateHz);
    LOG(VERBOSE) << __func__ << ": creating pipe, rate : " << streamConfig.sampleRate
                 << ", pipe size : " << pipeSizeInFrames;

    // Create a MonoPipe with optional blocking set to true.
    sp<MonoPipe> sink = sp<MonoPipe>::make(pipeSizeInFrames, format, true /*writeCanBlock*/);
    if (sink == nullptr) {
        LOG(FATAL) << __func__ << ": sink is null";
        return ::android::UNEXPECTED_NULL;
    }

    // Negotiation between the source and sink cannot fail as the device open operation
    // creates both ends of the pipe using the same audio format.
    ssize_t index = sink->negotiate(offers, 1, nullptr, numCounterOffers);
    if (index != 0) {
        LOG(FATAL) << __func__ << ": Negotiation for the sink failed, index = " << index;
        return ::android::BAD_INDEX;
    }
    sp<MonoPipeReader> source = sp<MonoPipeReader>::make(sink.get());
    if (source == nullptr) {
        LOG(FATAL) << __func__ << ": source is null";
        return ::android::UNEXPECTED_NULL;
    }
    numCounterOffers = 0;
    index = source->negotiate(offers, 1, nullptr, numCounterOffers);
    if (index != 0) {
        LOG(FATAL) << __func__ << ": Negotiation for the source failed, index = " << index;
        return ::android::BAD_INDEX;
    }
    LOG(VERBOSE) << __func__ << ": created pipe";

    mPipeConfig = streamConfig;
    mPipeConfig.frameCount = sink->maxFrames();

    LOG(VERBOSE) << __func__ << ": Pipe frame size : " << mPipeConfig.frameSize
                 << ", pipe frames : " << mPipeConfig.frameCount;

    // Save references to the source and sink.
    {
        std::lock_guard guard(mLock);
        mSink = std::move(sink);
        mSource = std::move(source);
    }

    return ::android::OK;
}

// Release references to the sink and source.
void SubmixRoute::releasePipe() {
    std::lock_guard guard(mLock);
    mSink.clear();
    mSource.clear();
}

::android::status_t SubmixRoute::resetPipe() {
    releasePipe();
    return createPipe(mPipeConfig);
}

void SubmixRoute::standby(bool isInput) {
    std::lock_guard guard(mLock);

    if (isInput) {
        mStreamInStandby = true;
    } else if (!mStreamOutStandby) {
        mStreamOutStandby = true;
        mStreamOutStandbyTransition = true;
    }
}

void SubmixRoute::exitStandby(bool isInput) {
    std::lock_guard guard(mLock);

    if (isInput) {
        if (mStreamInStandby || mStreamOutStandbyTransition) {
            mStreamInStandby = false;
            mStreamOutStandbyTransition = false;
            // keep track of when we exit input standby (== first read == start "real recording")
            // or when we start recording silence, and reset projected time
            mRecordStartTime = std::chrono::steady_clock::now();
            mReadCounterFrames = 0;
        }
    } else {
        if (mStreamOutStandby) {
            mStreamOutStandby = false;
            mStreamOutStandbyTransition = true;
        }
    }
}

}  // namespace aidl::android::hardware::audio::core::r_submix
