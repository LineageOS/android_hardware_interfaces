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

#include "EvsCamera.h"

#include <aidl/android/hardware/automotive/evs/EvsResult.h>
#include <aidlcommonsupport/NativeHandle.h>
#include <android-base/logging.h>
#include <android/hardware_buffer.h>
#include <ui/GraphicBufferAllocator.h>
#include <ui/GraphicBufferMapper.h>

#include <cstddef>
#include <mutex>

namespace aidl::android::hardware::automotive::evs::implementation {

// Arbitrary limit on number of graphics buffers allowed to be allocated
// Safeguards against unreasonable resource consumption and provides a testable limit
constexpr std::size_t kMaxBuffersInFlight = 100;

// Minimum number of buffers to run a video stream
constexpr int kMinimumBuffersInFlight = 1;

EvsCamera::~EvsCamera() {
    shutdown();
}

ndk::ScopedAStatus EvsCamera::doneWithFrame(const std::vector<evs::BufferDesc>& buffers) {
    std::lock_guard lck(mMutex);
    for (const auto& desc : buffers) {
        returnBuffer_unsafe(desc.bufferId);
    }
    return ndk::ScopedAStatus::ok();
}

ndk::ScopedAStatus EvsCamera::importExternalBuffers(const std::vector<evs::BufferDesc>& buffers,
                                                    int32_t* _aidl_return) {
    if (buffers.empty()) {
        LOG(DEBUG) << __func__
                   << ": Ignoring a request to import external buffers with an empty list.";
        return ndk::ScopedAStatus::ok();
    }
    static auto& mapper = ::android::GraphicBufferMapper::get();
    std::lock_guard lck(mMutex);
    std::size_t numBuffersToAdd = std::min(buffers.size(), kMaxBuffersInFlight - mAvailableFrames);
    if (numBuffersToAdd == 0) {
        LOG(WARNING) << __func__ << ": The number of buffers has hit the upper limit ("
                     << kMaxBuffersInFlight << "). Stop importing.";
        return ndk::ScopedAStatus::ok();
    } else if (numBuffersToAdd < buffers.size()) {
        LOG(WARNING) << "Exceeds the limit on the number of buffers. Only " << numBuffersToAdd
                     << " buffers will be imported. " << buffers.size() << " are asked.";
    }
    const size_t before = mAvailableFrames;
    for (std::size_t idx = 0; idx < numBuffersToAdd; ++idx) {
        auto& buffer = buffers[idx];
        const AHardwareBuffer_Desc* pDesc =
                reinterpret_cast<const AHardwareBuffer_Desc*>(&buffer.buffer.description);

        buffer_handle_t handleToImport = ::android::dupFromAidl(buffer.buffer.handle);
        buffer_handle_t handleToStore = nullptr;
        if (handleToImport == nullptr) {
            LOG(WARNING) << "Failed to duplicate a memory handle. Ignoring a buffer "
                         << buffer.bufferId;
            continue;
        }

        ::android::status_t result =
                mapper.importBuffer(handleToImport, pDesc->width, pDesc->height, pDesc->layers,
                                    pDesc->format, pDesc->usage, pDesc->stride, &handleToStore);
        if (result != ::android::NO_ERROR || handleToStore == nullptr ||
            !increaseAvailableFrames_unsafe(handleToStore)) {
            LOG(WARNING) << "Failed to import a buffer " << buffer.bufferId;
        }
    }
    *_aidl_return = mAvailableFrames - before;
    return ndk::ScopedAStatus::ok();
}

ndk::ScopedAStatus EvsCamera::setMaxFramesInFlight(int32_t bufferCount) {
    std::lock_guard lock(mMutex);
    if (bufferCount < 1) {
        LOG(ERROR) << "Ignoring setMaxFramesInFlight with less than one buffer requested.";
        return ndk::ScopedAStatus::fromServiceSpecificError(
                static_cast<int>(EvsResult::INVALID_ARG));
    }
    if (!setAvailableFrames_unsafe(bufferCount)) {
        LOG(ERROR) << "Failed to adjust the maximum number of frames in flight.";
        return ndk::ScopedAStatus::fromServiceSpecificError(
                static_cast<int>(EvsResult::BUFFER_NOT_AVAILABLE));
    }
    return ndk::ScopedAStatus::ok();
}

void EvsCamera::freeOneFrame(const buffer_handle_t handle) {
    static auto& alloc = ::android::GraphicBufferAllocator::get();
    alloc.free(handle);
}

bool EvsCamera::preVideoStreamStart_locked(const std::shared_ptr<evs::IEvsCameraStream>& receiver,
                                           ndk::ScopedAStatus& status,
                                           std::unique_lock<std::mutex>& /* lck */) {
    if (!receiver) {
        LOG(ERROR) << __func__ << ": Null receiver.";
        status = ndk::ScopedAStatus::fromServiceSpecificError(
                static_cast<int>(EvsResult::INVALID_ARG));
        return false;
    }

    // If we've been displaced by another owner of the camera, then we can't do anything else
    if (mStreamState == StreamState::DEAD) {
        LOG(ERROR) << __func__ << ": Ignoring when camera has been lost.";
        status = ndk::ScopedAStatus::fromServiceSpecificError(
                static_cast<int>(EvsResult::OWNERSHIP_LOST));
        return false;
    }

    if (mStreamState != StreamState::STOPPED) {
        LOG(ERROR) << __func__ << ": Ignoring when a stream is already running.";
        status = ndk::ScopedAStatus::fromServiceSpecificError(
                static_cast<int>(EvsResult::STREAM_ALREADY_RUNNING));
        return false;
    }

    // If the client never indicated otherwise, configure ourselves for a single streaming buffer
    if (mAvailableFrames < kMinimumBuffersInFlight &&
        !setAvailableFrames_unsafe(kMinimumBuffersInFlight)) {
        LOG(ERROR) << __func__ << "Failed to because we could not get a graphics buffer.";
        status = ndk::ScopedAStatus::fromServiceSpecificError(
                static_cast<int>(EvsResult::BUFFER_NOT_AVAILABLE));
        return false;
    }
    mStreamState = StreamState::RUNNING;
    return true;
}

bool EvsCamera::postVideoStreamStart_locked(
        const std::shared_ptr<evs::IEvsCameraStream>& /* receiver */,
        ndk::ScopedAStatus& /* status */, std::unique_lock<std::mutex>& /* lck */) {
    return true;
}

bool EvsCamera::preVideoStreamStop_locked(ndk::ScopedAStatus& status,
                                          std::unique_lock<std::mutex>& /* lck */) {
    if (mStreamState != StreamState::RUNNING) {
        // Terminate the stop process because a stream is not running.
        status = ndk::ScopedAStatus::ok();
        return false;
    }
    mStreamState = StreamState::STOPPING;
    return true;
}

bool EvsCamera::postVideoStreamStop_locked(ndk::ScopedAStatus& /* status */,
                                           std::unique_lock<std::mutex>& /* lck */) {
    mStreamState = StreamState::STOPPED;
    return true;
}

ndk::ScopedAStatus EvsCamera::startVideoStream(
        const std::shared_ptr<evs::IEvsCameraStream>& receiver) {
    bool needShutdown = false;
    auto status = ndk::ScopedAStatus::ok();
    {
        std::unique_lock lck(mMutex);
        if (!preVideoStreamStart_locked(receiver, status, lck)) {
            return status;
        }

        if ((!startVideoStreamImpl_locked(receiver, status, lck) ||
             !postVideoStreamStart_locked(receiver, status, lck)) &&
            !status.isOk()) {
            needShutdown = true;
        }
    }
    if (needShutdown) {
        shutdown();
    }
    return status;
}

ndk::ScopedAStatus EvsCamera::stopVideoStream() {
    bool needShutdown = false;
    auto status = ndk::ScopedAStatus::ok();
    {
        std::unique_lock lck(mMutex);
        if ((!preVideoStreamStop_locked(status, lck) || !stopVideoStreamImpl_locked(status, lck) ||
             !postVideoStreamStop_locked(status, lck)) &&
            !status.isOk()) {
            needShutdown = true;
        }
    }
    if (needShutdown) {
        shutdown();
    }
    return status;
}

ndk::ScopedAStatus EvsCamera::pauseVideoStream() {
    return ndk::ScopedAStatus::fromServiceSpecificError(static_cast<int>(EvsResult::NOT_SUPPORTED));
}

ndk::ScopedAStatus EvsCamera::resumeVideoStream() {
    return ndk::ScopedAStatus::fromServiceSpecificError(static_cast<int>(EvsResult::NOT_SUPPORTED));
}

bool EvsCamera::setAvailableFrames_unsafe(const std::size_t bufferCount) {
    if (bufferCount < 1) {
        LOG(ERROR) << "Ignoring request to set buffer count to zero.";
        return false;
    }
    if (bufferCount > kMaxBuffersInFlight) {
        LOG(ERROR) << "Rejecting buffer request in excess of internal limit";
        return false;
    }

    if (bufferCount > mAvailableFrames) {
        bool success = true;
        const std::size_t numBufferBeforeAlloc = mAvailableFrames;
        for (int numBufferToAllocate = bufferCount - mAvailableFrames;
             success && numBufferToAllocate > 0; --numBufferToAllocate) {
            buffer_handle_t handle = nullptr;
            const auto result = allocateOneFrame(&handle);
            if (result != ::android::NO_ERROR || !handle) {
                LOG(ERROR) << __func__ << ": Failed to allocate a graphics buffer. Error " << result
                           << ", handle: " << handle;
                success = false;
                break;
            }
            success &= increaseAvailableFrames_unsafe(handle);
        }
        if (!success) {
            // Rollback when failure.
            for (int numBufferToRelease = mAvailableFrames - numBufferBeforeAlloc;
                 numBufferToRelease > 0; --numBufferToRelease) {
                decreaseAvailableFrames_unsafe();
            }
            return false;
        }
    } else {
        for (int numBufferToRelease = mAvailableFrames - std::max(bufferCount, mFramesInUse);
             numBufferToRelease > 0; --numBufferToRelease) {
            decreaseAvailableFrames_unsafe();
        }
        if (mAvailableFrames > bufferCount) {
            // This shouldn't happen with a properly behaving client because the client
            // should only make this call after returning sufficient outstanding buffers
            // to allow a clean resize.
            LOG(ERROR) << "Buffer queue shrink failed, asked: " << bufferCount
                       << ", actual: " << mAvailableFrames
                       << " -- too many buffers currently in use?";
        }
    }
    return true;
}

void EvsCamera::shutdown() {
    stopVideoStream();
    std::lock_guard lck(mMutex);
    closeAllBuffers_unsafe();
    mStreamState = StreamState::DEAD;
}

void EvsCamera::closeAllBuffers_unsafe() {
    if (mFramesInUse > 0) {
        LOG(WARNING) << __func__ << ": Closing while " << mFramesInUse
                     << " frame(s) are still in use.";
    }
    for (auto& buffer : mBuffers) {
        freeOneFrame(buffer.handle);
        buffer.handle = nullptr;
    }
    mBuffers.clear();
    mBufferPosToId.clear();
    mBufferIdToPos.clear();
}

std::pair<std::size_t, buffer_handle_t> EvsCamera::useBuffer_unsafe() {
    if (mFramesInUse >= mAvailableFrames) {
        DCHECK_EQ(mFramesInUse, mAvailableFrames);
        return {kInvalidBufferID, nullptr};
    }
    const std::size_t pos = mFramesInUse++;
    auto& buffer = mBuffers[pos];
    DCHECK(!buffer.inUse);
    DCHECK(buffer.handle);
    buffer.inUse = true;
    return {mBufferPosToId[pos], buffer.handle};
}

void EvsCamera::returnBuffer_unsafe(const std::size_t id) {
    if (id >= mBuffers.size()) {
        LOG(ERROR) << __func__ << ": ID out-of-bound. id: " << id
                   << " max: " << mBuffers.size() - 1;
        return;
    }
    const std::size_t pos = mBufferIdToPos[id];

    if (!mBuffers[pos].inUse) {
        LOG(ERROR) << __func__ << ": Ignoring returning frame " << id << " which is already free.";
        return;
    }
    DCHECK_LT(pos, mFramesInUse);
    const std::size_t last_in_use_pos = --mFramesInUse;
    swapBufferFrames_unsafe(pos, last_in_use_pos);
    mBuffers[last_in_use_pos].inUse = false;
}

bool EvsCamera::increaseAvailableFrames_unsafe(const buffer_handle_t handle) {
    if (mAvailableFrames >= kMaxBuffersInFlight) {
        LOG(WARNING) << __func__ << ": The number of buffers has hit the upper limit ("
                     << kMaxBuffersInFlight << "). Stop increasing.";
        return false;
    }
    const std::size_t pos = mAvailableFrames++;
    if (mAvailableFrames > mBuffers.size()) {
        const std::size_t oldBufferSize = mBuffers.size();
        mBuffers.resize(mAvailableFrames);
        mBufferPosToId.resize(mAvailableFrames);
        mBufferIdToPos.resize(mAvailableFrames);
        // Build position/ID mapping.
        for (std::size_t idx = oldBufferSize; idx < mBuffers.size(); ++idx) {
            mBufferPosToId[idx] = idx;
            mBufferIdToPos[idx] = idx;
        }
    }
    auto& buffer = mBuffers[pos];
    DCHECK(!buffer.inUse);
    DCHECK(!buffer.handle);
    buffer.handle = handle;
    return true;
}

bool EvsCamera::decreaseAvailableFrames_unsafe() {
    if (mFramesInUse >= mAvailableFrames) {
        DCHECK_EQ(mFramesInUse, mAvailableFrames);
        return false;
    }
    const std::size_t pos = --mAvailableFrames;
    auto& buffer = mBuffers[pos];
    DCHECK(!buffer.inUse);
    DCHECK(buffer.handle);
    freeOneFrame(buffer.handle);
    buffer.handle = nullptr;
    return true;
}

void EvsCamera::swapBufferFrames_unsafe(const std::size_t pos1, const std::size_t pos2) {
    if (pos1 == pos2) {
        return;
    }
    if (pos1 >= mBuffers.size() || pos2 >= mBuffers.size()) {
        LOG(ERROR) << __func__ << ": Index out-of-bound. pos1: " << pos1 << ", pos2: " << pos2
                   << ", buffer size: " << mBuffers.size();
        return;
    }
    const std::size_t id1 = mBufferPosToId[pos1];
    const std::size_t id2 = mBufferPosToId[pos2];
    std::swap(mBufferPosToId[pos1], mBufferPosToId[pos2]);
    std::swap(mBufferIdToPos[id1], mBufferIdToPos[id2]);
    std::swap(mBuffers[pos1], mBuffers[pos2]);
}

}  // namespace aidl::android::hardware::automotive::evs::implementation
