/*
 * Copyright (C) 2025 The Android Open Source Project
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

#include <unistd.h>
#include <cstdlib>

#define LOG_TAG "AHAL_MmapStream"
#include <android-base/logging.h>
#include <audio_utils/clock.h>
#include <error/Result.h>
#include <utils/SystemClock.h>

#include "core-impl/StreamMmapStub.h"

using aidl::android::hardware::audio::common::SinkMetadata;
using aidl::android::hardware::audio::common::SourceMetadata;
using aidl::android::media::audio::common::AudioOffloadInfo;
using aidl::android::media::audio::common::MicrophoneInfo;

namespace aidl::android::hardware::audio::core {

namespace mmap {

std::string DspSimulatorLogic::init() {
    {
        std::lock_guard l(mSharedState.lock);
        mSharedState.mmapPos.timeNs = StreamDescriptor::Position::UNKNOWN;
        mSharedState.mmapPos.frames = StreamDescriptor::Position::UNKNOWN;
    }
    // Progress in buffer size chunks to make sure that VTS tolerates infrequent position updates
    // (see b/350998390).
    mCycleDurationUs = (mSharedState.bufferSizeBytes / mSharedState.frameSizeBytes) *
                       MICROS_PER_SECOND / mSharedState.sampleRate;
    return "";
}

DspSimulatorLogic::Status DspSimulatorLogic::cycle() {
    // Simulate DSP moving along in real time.
    const int64_t timeBeginNs = ::android::uptimeNanos();
    usleep(mCycleDurationUs);
    int64_t newFrames;
    std::lock_guard l(mSharedState.lock);
    if (mMemBegin != mSharedState.sharedMemory) {
        mMemBegin = mSharedState.sharedMemory;
        if (mMemBegin != nullptr) mMemPos = mMemBegin;
    }
    if (mMemBegin != nullptr) {
        mSharedState.mmapPos.timeNs = ::android::uptimeNanos();
        newFrames = (mSharedState.mmapPos.timeNs - timeBeginNs) * mSharedState.sampleRate /
                    NANOS_PER_SECOND;
        // Restore the reported frames position to ensure continuity.
        if (mSharedState.mmapPos.frames == StreamDescriptor::Position::UNKNOWN) {
            mSharedState.mmapPos.frames = mLastFrames;
        }
        mSharedState.mmapPos.frames += newFrames;
        mLastFrames = mSharedState.mmapPos.frames;
        if (mSharedState.isInput) {
            for (size_t i = 0; i < static_cast<size_t>(newFrames) * mSharedState.frameSizeBytes;
                 ++i) {
                *mMemPos++ = std::rand() % 255;
                if (mMemPos >= mMemBegin + mSharedState.bufferSizeBytes) mMemPos = mMemBegin;
            }
        }
    } else {
        LOG(WARNING) << "No shared memory but the DSP is active";
        mSharedState.mmapPos.timeNs = StreamDescriptor::Position::UNKNOWN;
        mSharedState.mmapPos.frames = StreamDescriptor::Position::UNKNOWN;
    }
    return Status::CONTINUE;
}

}  // namespace mmap

using mmap::DspSimulatorState;

DriverMmapStubImpl::DriverMmapStubImpl(const StreamContext& context)
    : DriverStubImpl(context, 0 /*asyncSleepTimeUs*/),
      mState{mIsInput, mSampleRate, static_cast<int>(mFrameSizeBytes),
             mBufferSizeFrames * mFrameSizeBytes},
      mDspWorker(mState) {
    LOG_IF(FATAL, !context.isMmap()) << "The steam must be used in MMAP mode";
}

::android::status_t DriverMmapStubImpl::init(DriverCallbackInterface* callback) {
    RETURN_STATUS_IF_ERROR(DriverStubImpl::init(callback));
    return ::android::OK;
}

::android::status_t DriverMmapStubImpl::drain(StreamDescriptor::DrainMode drainMode) {
    RETURN_STATUS_IF_ERROR(DriverStubImpl::drain(drainMode));
    mDspWorker.pause();
    return ::android::OK;
}

::android::status_t DriverMmapStubImpl::pause() {
    RETURN_STATUS_IF_ERROR(DriverStubImpl::pause());
    mDspWorker.pause();
    return ::android::OK;
}

::android::status_t DriverMmapStubImpl::start() {
    RETURN_STATUS_IF_ERROR(DriverStubImpl::start());
    RETURN_STATUS_IF_ERROR(startWorkerIfNeeded());
    mDspWorker.resume();
    return ::android::OK;
}

::android::status_t DriverMmapStubImpl::transfer(void*, size_t frameCount, size_t*, int32_t*) {
    // Do not call into DriverStubImpl::transfer
    if (!mIsInitialized) {
        LOG(FATAL) << __func__ << ": must not happen for an uninitialized driver";
    }
    if (mIsStandby) {
        LOG(FATAL) << __func__ << ": must not happen while in standby";
    }
    if (frameCount != 0) {
        LOG(ERROR) << __func__ << ": burst value size must be 0 for MMAP";
        return ::android::BAD_VALUE;
    }
    RETURN_STATUS_IF_ERROR(startWorkerIfNeeded());
    mDspWorker.resume();
    return ::android::OK;
}

void DriverMmapStubImpl::shutdown() {
    LOG(DEBUG) << __func__ << ": stopping the DSP simulator worker";
    mDspWorker.stop();
    std::lock_guard l(mState.lock);
    releaseSharedMemory();
    DriverStubImpl::shutdown();
}

::android::status_t DriverMmapStubImpl::initSharedMemory(int ashmemFd) {
    {
        std::lock_guard l(mState.lock);
        if (ashmemFd == -1) {
            mState.sharedMemory = nullptr;
            return ::android::BAD_VALUE;
        }
        RETURN_STATUS_IF_ERROR(releaseSharedMemory());
    }
    uint8_t* sharedMemory = static_cast<uint8_t*>(::mmap(
            nullptr, mState.bufferSizeBytes, PROT_READ | PROT_WRITE, MAP_SHARED, ashmemFd, 0));
    if (sharedMemory == reinterpret_cast<uint8_t*>(MAP_FAILED) || sharedMemory == nullptr) {
        PLOG(ERROR) << "mmap failed for size " << mState.bufferSizeBytes << ", fd " << ashmemFd;
        return ::android::NO_INIT;
    }
    std::lock_guard l(mState.lock);
    mState.sharedMemory = sharedMemory;
    return ::android::OK;
}

::android::status_t DriverMmapStubImpl::releaseSharedMemory() {
    if (mState.sharedMemory != nullptr) {
        LOG(DEBUG) << __func__ << ": unmapping shared memory";
        if (munmap(mState.sharedMemory, mState.bufferSizeBytes) != 0) {
            PLOG(ERROR) << "munmap failed for size " << mState.bufferSizeBytes;
            return ::android::INVALID_OPERATION;
        }
        mState.sharedMemory = nullptr;
    }
    return ::android::OK;
}

::android::status_t DriverMmapStubImpl::startWorkerIfNeeded() {
    if (!mDspWorkerStarted) {
        // This is an "audio service thread," must have elevated priority.
        if (!mDspWorker.start("dsp_sim", ANDROID_PRIORITY_URGENT_AUDIO)) {
            return ::android::NO_INIT;
        }
        mDspWorkerStarted = true;
    }
    return ::android::OK;
}

::android::status_t DriverMmapStubImpl::refinePosition(StreamDescriptor::Position* position) {
    std::lock_guard l(mState.lock);
    *position = mState.mmapPos;
    return ::android::OK;
}

::android::status_t DriverMmapStubImpl::getMmapPositionAndLatency(
        StreamDescriptor::Position* position, int32_t* latencyMs) {
    {
        std::lock_guard l(mState.lock);
        *position = mState.mmapPos;
    }
    const size_t latencyFrames = mBufferSizeFrames / 4;
    if (position->frames != StreamDescriptor::Position::UNKNOWN) {
        position->frames += latencyFrames;
    }
    *latencyMs = latencyFrames * MILLIS_PER_SECOND / mSampleRate;
    return ::android::OK;
}

const std::string StreamMmapStub::kCreateMmapBufferName = "aosp.createMmapBuffer";

StreamMmapStub::StreamMmapStub(StreamContext* context, const Metadata& metadata)
    : StreamCommonImpl(context, metadata), DriverMmapStubImpl(getContext()) {}

StreamMmapStub::~StreamMmapStub() {
    cleanupWorker();
}

ndk::ScopedAStatus StreamMmapStub::getVendorParameters(const std::vector<std::string>& in_ids,
                                                       std::vector<VendorParameter>* _aidl_return) {
    std::vector<std::string> unprocessedIds;
    for (const auto& id : in_ids) {
        if (id == kCreateMmapBufferName) {
            LOG(DEBUG) << __func__ << ": " << id;
            MmapBufferDescriptor mmapDesc;
            RETURN_STATUS_IF_ERROR(createMmapBuffer(&mmapDesc));
            VendorParameter createMmapBuffer{.id = id};
            createMmapBuffer.ext.setParcelable(mmapDesc);
            LOG(DEBUG) << __func__ << ": returning " << mmapDesc.toString();
            _aidl_return->push_back(std::move(createMmapBuffer));
        } else {
            unprocessedIds.push_back(id);
        }
    }
    if (!unprocessedIds.empty()) {
        return StreamCommonImpl::getVendorParameters(unprocessedIds, _aidl_return);
    }
    return ndk::ScopedAStatus::ok();
}

ndk::ScopedAStatus StreamMmapStub::setVendorParameters(
        const std::vector<VendorParameter>& in_parameters, bool in_async) {
    std::vector<VendorParameter> unprocessedParameters;
    for (const auto& param : in_parameters) {
        if (param.id == kCreateMmapBufferName) {
            LOG(DEBUG) << __func__ << ": " << param.id;
            // The value is irrelevant. The fact that this parameter can be "set" is an
            // indication that the method can be used by the client via 'getVendorParameters'.
        } else {
            unprocessedParameters.push_back(param);
        }
    }
    if (!unprocessedParameters.empty()) {
        return StreamCommonImpl::setVendorParameters(unprocessedParameters, in_async);
    }
    return ndk::ScopedAStatus::ok();
}

ndk::ScopedAStatus StreamMmapStub::createMmapBuffer(MmapBufferDescriptor* desc) {
    const size_t bufferSizeFrames = mContext.getBufferSizeInFrames();
    const size_t bufferSizeBytes = static_cast<size_t>(bufferSizeFrames) * mContext.getFrameSize();
    const std::string regionName =
            std::string("mmap-sim-") + std::to_string(mContext.getMixPortHandle());
    int fd = ashmem_create_region(regionName.c_str(), bufferSizeBytes);
    if (fd < 0) {
        PLOG(ERROR) << __func__ << ": failed to create shared memory region of " << bufferSizeBytes
                    << " bytes";
        return ndk::ScopedAStatus::fromExceptionCode(EX_ILLEGAL_STATE);
    }
    mSharedMemoryFd = ndk::ScopedFileDescriptor(fd);
    if (initSharedMemory(mSharedMemoryFd.get()) != ::android::OK) {
        return ndk::ScopedAStatus::fromExceptionCode(EX_ILLEGAL_STATE);
    }
    desc->sharedMemory.fd = mSharedMemoryFd.dup();
    desc->sharedMemory.size = bufferSizeBytes;
    desc->burstSizeFrames = bufferSizeFrames / 4;
    desc->flags = 1 << MmapBufferDescriptor::FLAG_INDEX_APPLICATION_SHAREABLE;
    LOG(DEBUG) << __func__ << ": " << desc->toString();
    return ndk::ScopedAStatus::ok();
}

StreamInMmapStub::StreamInMmapStub(StreamContext&& context, const SinkMetadata& sinkMetadata,
                                   const std::vector<MicrophoneInfo>& microphones)
    : StreamIn(std::move(context), microphones), StreamMmapStub(&mContextInstance, sinkMetadata) {}

StreamOutMmapStub::StreamOutMmapStub(StreamContext&& context, const SourceMetadata& sourceMetadata,
                                     const std::optional<AudioOffloadInfo>& offloadInfo)
    : StreamOut(std::move(context), offloadInfo),
      StreamMmapStub(&mContextInstance, sourceMetadata) {}

}  // namespace aidl::android::hardware::audio::core
