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

#include "EvsMockCamera.h"
#include "ConfigManager.h"
#include "EvsEnumerator.h"

#include <aidlcommonsupport/NativeHandle.h>
#include <ui/GraphicBufferAllocator.h>
#include <ui/GraphicBufferMapper.h>
#include <utils/SystemClock.h>

#include <memory>

namespace {

using ::aidl::android::hardware::graphics::common::BufferUsage;
using ::ndk::ScopedAStatus;

// Arbitrary limit on number of graphics buffers allowed to be allocated
// Safeguards against unreasonable resource consumption and provides a testable limit
constexpr unsigned kMaxBuffersInFlight = 100;

// Minimum number of buffers to run a video stream
constexpr int kMinimumBuffersInFlight = 1;

// Colors for the colorbar test pattern in ABGR format
constexpr uint32_t kColors[] = {
        0xFFFFFFFF,  // white
        0xFF00FFFF,  // yellow
        0xFFFFFF00,  // cyan
        0xFF00FF00,  // green
        0xFFFF00FF,  // fuchsia
        0xFF0000FF,  // red
        0xFFFF0000,  // blue
        0xFF000000,  // black
};
constexpr size_t kNumColors = sizeof(kColors) / sizeof(kColors[0]);

}  // namespace

namespace aidl::android::hardware::automotive::evs::implementation {

EvsMockCamera::EvsMockCamera([[maybe_unused]] Sigil sigil, const char* id,
                             std::unique_ptr<ConfigManager::CameraInfo>& camInfo)
    : mFramesAllowed(0), mFramesInUse(0), mStreamState(STOPPED), mCameraInfo(camInfo) {
    LOG(DEBUG) << __FUNCTION__;

    /* set a camera id */
    mDescription.id = id;

    /* set camera metadata */
    if (camInfo) {
        uint8_t* ptr = reinterpret_cast<uint8_t*>(camInfo->characteristics);
        const size_t len = get_camera_metadata_size(camInfo->characteristics);
        mDescription.metadata.insert(mDescription.metadata.end(), ptr, ptr + len);
    }

    // Initialize parameters.
    initializeParameters();
}

EvsMockCamera::~EvsMockCamera() {
    LOG(DEBUG) << __FUNCTION__;
    shutdown();
}

void EvsMockCamera::initializeParameters() {
    mParams.emplace(
            CameraParam::BRIGHTNESS,
            new CameraParameterDesc(/* min= */ 0, /* max= */ 255, /* step= */ 1, /* value= */ 255));
    mParams.emplace(
            CameraParam::CONTRAST,
            new CameraParameterDesc(/* min= */ 0, /* max= */ 255, /* step= */ 1, /* value= */ 255));
    mParams.emplace(
            CameraParam::SHARPNESS,
            new CameraParameterDesc(/* min= */ 0, /* max= */ 255, /* step= */ 1, /* value= */ 255));
}

// This gets called if another caller "steals" ownership of the camera
void EvsMockCamera::shutdown() {
    LOG(DEBUG) << __FUNCTION__;

    // Make sure our output stream is cleaned up
    // (It really should be already)
    stopVideoStream_impl();

    // Claim the lock while we work on internal state
    std::lock_guard lock(mAccessLock);

    // Drop all the graphics buffers we've been using
    if (mBuffers.size() > 0) {
        ::android::GraphicBufferAllocator& alloc(::android::GraphicBufferAllocator::get());
        for (auto&& rec : mBuffers) {
            if (rec.inUse) {
                LOG(WARNING) << "WARNING: releasing a buffer remotely owned.";
            }
            alloc.free(rec.handle);
            rec.handle = nullptr;
        }
        mBuffers.clear();
    }

    // Put this object into an unrecoverable error state since somebody else
    // is going to own the underlying camera now
    mStreamState = DEAD;
}

// Methods from ::aidl::android::hardware::automotive::evs::IEvsCamera follow.
ScopedAStatus EvsMockCamera::getCameraInfo(CameraDesc* _aidl_return) {
    LOG(DEBUG) << __FUNCTION__;

    // Send back our self description
    *_aidl_return = mDescription;
    return ScopedAStatus::ok();
}

ScopedAStatus EvsMockCamera::setMaxFramesInFlight(int32_t bufferCount) {
    LOG(DEBUG) << __FUNCTION__ << ", bufferCount = " << bufferCount;
    ;

    std::lock_guard lock(mAccessLock);

    // If we've been displaced by another owner of the camera, then we can't do anything else
    if (mStreamState == DEAD) {
        LOG(ERROR) << "Ignoring setMaxFramesInFlight call when camera has been lost.";
        return ScopedAStatus::fromServiceSpecificError(static_cast<int>(EvsResult::OWNERSHIP_LOST));
    }

    // We cannot function without at least one video buffer to send data
    if (bufferCount < 1) {
        LOG(ERROR) << "Ignoring setMaxFramesInFlight with less than one buffer requested.";
        return ScopedAStatus::fromServiceSpecificError(static_cast<int>(EvsResult::INVALID_ARG));
    }

    // Update our internal state
    if (!setAvailableFrames_Locked(bufferCount)) {
        LOG(ERROR) << "Failed to adjust the maximum number of frames in flight.";
        return ScopedAStatus::fromServiceSpecificError(
                static_cast<int>(EvsResult::BUFFER_NOT_AVAILABLE));
    }

    return ScopedAStatus::ok();
}

ScopedAStatus EvsMockCamera::startVideoStream(const std::shared_ptr<IEvsCameraStream>& cb) {
    LOG(DEBUG) << __FUNCTION__;

    if (!cb) {
        LOG(ERROR) << "A given stream callback is invalid.";
        return ScopedAStatus::fromServiceSpecificError(static_cast<int>(EvsResult::INVALID_ARG));
    }

    std::lock_guard lock(mAccessLock);

    // If we've been displaced by another owner of the camera, then we can't do anything else
    if (mStreamState == DEAD) {
        LOG(ERROR) << "Ignoring startVideoStream call when camera has been lost.";
        return ScopedAStatus::fromServiceSpecificError(static_cast<int>(EvsResult::OWNERSHIP_LOST));
    }

    if (mStreamState != STOPPED) {
        LOG(ERROR) << "Ignoring startVideoStream call when a stream is already running.";
        return ScopedAStatus::fromServiceSpecificError(
                static_cast<int>(EvsResult::STREAM_ALREADY_RUNNING));
    }

    // If the client never indicated otherwise, configure ourselves for a single streaming buffer
    if (mFramesAllowed < kMinimumBuffersInFlight &&
        !setAvailableFrames_Locked(kMinimumBuffersInFlight)) {
        LOG(ERROR) << "Failed to start stream because we couldn't get a graphics buffer";
        return ScopedAStatus::fromServiceSpecificError(
                static_cast<int>(EvsResult::BUFFER_NOT_AVAILABLE));
    }

    // Record the user's callback for use when we have a frame ready
    mStream = cb;

    // Start the frame generation thread
    mStreamState = RUNNING;
    mCaptureThread = std::thread([this]() { generateFrames(); });

    return ScopedAStatus::ok();
}

ScopedAStatus EvsMockCamera::doneWithFrame(const std::vector<BufferDesc>& list) {
    std::lock_guard lock(mAccessLock);
    for (const auto& desc : list) {
        returnBufferLocked(desc.bufferId);
    }

    return ScopedAStatus::ok();
}

ScopedAStatus EvsMockCamera::stopVideoStream() {
    LOG(DEBUG) << __FUNCTION__;
    return stopVideoStream_impl();
}

ScopedAStatus EvsMockCamera::stopVideoStream_impl() {
    std::unique_lock lock(mAccessLock);

    if (mStreamState != RUNNING) {
        // Safely return here because a stream is not running.
        return ScopedAStatus::ok();
    }

    // Tell the GenerateFrames loop we want it to stop
    mStreamState = STOPPING;

    // Block outside the mutex until the "stop" flag has been acknowledged
    // We won't send any more frames, but the client might still get some already in flight
    LOG(DEBUG) << "Waiting for stream thread to end...";
    lock.unlock();
    if (mCaptureThread.joinable()) {
        mCaptureThread.join();
    }
    lock.lock();

    mStreamState = STOPPED;
    mStream = nullptr;
    LOG(DEBUG) << "Stream marked STOPPED.";

    return ScopedAStatus::ok();
}

ScopedAStatus EvsMockCamera::getExtendedInfo(int32_t opaqueIdentifier,
                                             std::vector<uint8_t>* opaqueValue) {
    const auto it = mExtInfo.find(opaqueIdentifier);
    if (it == mExtInfo.end()) {
        return ScopedAStatus::fromServiceSpecificError(static_cast<int>(EvsResult::INVALID_ARG));
    } else {
        *opaqueValue = mExtInfo[opaqueIdentifier];
    }

    return ScopedAStatus::ok();
}

ScopedAStatus EvsMockCamera::setExtendedInfo(int32_t opaqueIdentifier,
                                             const std::vector<uint8_t>& opaqueValue) {
    mExtInfo.insert_or_assign(opaqueIdentifier, opaqueValue);
    return ScopedAStatus::ok();
}

ScopedAStatus EvsMockCamera::getPhysicalCameraInfo([[maybe_unused]] const std::string& id,
                                                   CameraDesc* _aidl_return) {
    LOG(DEBUG) << __FUNCTION__;

    // This method works exactly same as getCameraInfo() in EVS HW module.
    *_aidl_return = mDescription;
    return ScopedAStatus::ok();
}

ScopedAStatus EvsMockCamera::pauseVideoStream() {
    return ScopedAStatus::fromServiceSpecificError(static_cast<int>(EvsResult::NOT_SUPPORTED));
}

ScopedAStatus EvsMockCamera::resumeVideoStream() {
    return ScopedAStatus::fromServiceSpecificError(static_cast<int>(EvsResult::NOT_SUPPORTED));
}

ScopedAStatus EvsMockCamera::setPrimaryClient() {
    /* Because EVS HW module reference implementation expects a single client at
     * a time, this returns a success code always.
     */
    return ScopedAStatus::ok();
}

ScopedAStatus EvsMockCamera::forcePrimaryClient(const std::shared_ptr<IEvsDisplay>&) {
    /* Because EVS HW module reference implementation expects a single client at
     * a time, this returns a success code always.
     */
    return ScopedAStatus::ok();
}

ScopedAStatus EvsMockCamera::unsetPrimaryClient() {
    /* Because EVS HW module reference implementation expects a single client at
     * a time, there is no chance that this is called by the secondary client and
     * therefore returns a success code always.
     */
    return ScopedAStatus::ok();
}

ScopedAStatus EvsMockCamera::getParameterList(std::vector<CameraParam>* _aidl_return) {
    if (mCameraInfo) {
        _aidl_return->resize(mCameraInfo->controls.size());
        auto idx = 0;
        for (auto& [name, range] : mCameraInfo->controls) {
            (*_aidl_return)[idx++] = name;
        }
    }

    return ScopedAStatus::ok();
}

ScopedAStatus EvsMockCamera::getIntParameterRange(CameraParam id, ParameterRange* _aidl_return) {
    auto it = mParams.find(id);
    if (it == mParams.end()) {
        return ScopedAStatus::fromServiceSpecificError(static_cast<int>(EvsResult::NOT_SUPPORTED));
    }

    _aidl_return->min = it->second->range.min;
    _aidl_return->max = it->second->range.max;
    _aidl_return->step = it->second->range.step;
    return ScopedAStatus::ok();
}

ScopedAStatus EvsMockCamera::setIntParameter(CameraParam id, int32_t value,
                                             std::vector<int32_t>* effectiveValue) {
    auto it = mParams.find(id);
    if (it == mParams.end()) {
        return ScopedAStatus::fromServiceSpecificError(static_cast<int>(EvsResult::NOT_SUPPORTED));
    }

    // Rounding down to the closest value.
    int32_t candidate = value / it->second->range.step * it->second->range.step;
    if (candidate < it->second->range.min || candidate > it->second->range.max) {
        return ScopedAStatus::fromServiceSpecificError(static_cast<int>(EvsResult::INVALID_ARG));
    }

    it->second->value = candidate;
    effectiveValue->push_back(candidate);
    return ScopedAStatus::ok();
}

ScopedAStatus EvsMockCamera::getIntParameter(CameraParam id, std::vector<int32_t>* value) {
    auto it = mParams.find(id);
    if (it == mParams.end()) {
        return ScopedAStatus::fromServiceSpecificError(static_cast<int>(EvsResult::NOT_SUPPORTED));
    }

    value->push_back(it->second->value);
    return ScopedAStatus::ok();
}

ScopedAStatus EvsMockCamera::importExternalBuffers(const std::vector<BufferDesc>& buffers,
                                                   int32_t* _aidl_return) {
    size_t numBuffersToAdd = buffers.size();
    if (numBuffersToAdd < 1) {
        LOG(DEBUG) << "Ignoring a request to import external buffers with an empty list.";
        return ScopedAStatus::ok();
    }

    std::lock_guard lock(mAccessLock);
    if (numBuffersToAdd > (kMaxBuffersInFlight - mFramesAllowed)) {
        numBuffersToAdd -= (kMaxBuffersInFlight - mFramesAllowed);
        LOG(WARNING) << "Exceed the limit on the number of buffers. " << numBuffersToAdd
                     << " buffers will be imported only.";
    }

    ::android::GraphicBufferMapper& mapper = ::android::GraphicBufferMapper::get();
    const size_t before = mFramesAllowed;
    for (size_t i = 0; i < numBuffersToAdd; ++i) {
        auto& b = buffers[i];
        const AHardwareBuffer_Desc* pDesc =
                reinterpret_cast<const AHardwareBuffer_Desc*>(&b.buffer.description);

        buffer_handle_t handleToImport = ::android::dupFromAidl(b.buffer.handle);
        buffer_handle_t handleToStore = nullptr;
        if (handleToImport == nullptr) {
            LOG(WARNING) << "Failed to duplicate a memory handle. Ignoring a buffer " << b.bufferId;
            continue;
        }

        ::android::status_t result =
                mapper.importBuffer(handleToImport, pDesc->width, pDesc->height, pDesc->layers,
                                    pDesc->format, pDesc->usage, pDesc->stride, &handleToStore);
        if (result != ::android::NO_ERROR || handleToStore == nullptr) {
            LOG(WARNING) << "Failed to import a buffer " << b.bufferId;
            continue;
        }

        bool stored = false;
        for (auto&& rec : mBuffers) {
            if (rec.handle != nullptr) {
                continue;
            }

            // Use this existing entry.
            rec.handle = handleToStore;
            rec.inUse = false;
            stored = true;
            break;
        }

        if (!stored) {
            // Add a BufferRecord wrapping this handle to our set of available buffers.
            mBuffers.push_back(BufferRecord(handleToStore));
        }
        ++mFramesAllowed;
    }

    *_aidl_return = mFramesAllowed - before;
    return ScopedAStatus::ok();
}

bool EvsMockCamera::setAvailableFrames_Locked(unsigned bufferCount) {
    if (bufferCount < 1) {
        LOG(ERROR) << "Ignoring request to set buffer count to zero";
        return false;
    }
    if (bufferCount > kMaxBuffersInFlight) {
        LOG(ERROR) << "Rejecting buffer request in excess of internal limit";
        return false;
    }

    // Is an increase required?
    if (mFramesAllowed < bufferCount) {
        // An increase is required
        auto needed = bufferCount - mFramesAllowed;
        LOG(INFO) << "Allocating " << needed << " buffers for camera frames";

        auto added = increaseAvailableFrames_Locked(needed);
        if (added != needed) {
            // If we didn't add all the frames we needed, then roll back to the previous state
            LOG(ERROR) << "Rolling back to previous frame queue size";
            decreaseAvailableFrames_Locked(added);
            return false;
        }
    } else if (mFramesAllowed > bufferCount) {
        // A decrease is required
        auto framesToRelease = mFramesAllowed - bufferCount;
        LOG(INFO) << "Returning " << framesToRelease << " camera frame buffers";

        auto released = decreaseAvailableFrames_Locked(framesToRelease);
        if (released != framesToRelease) {
            // This shouldn't happen with a properly behaving client because the client
            // should only make this call after returning sufficient outstanding buffers
            // to allow a clean resize.
            LOG(ERROR) << "Buffer queue shrink failed -- too many buffers currently in use?";
        }
    }

    return true;
}

unsigned EvsMockCamera::increaseAvailableFrames_Locked(unsigned numToAdd) {
    // Acquire the graphics buffer allocator
    ::android::GraphicBufferAllocator& alloc(::android::GraphicBufferAllocator::get());

    unsigned added = 0;
    while (added < numToAdd) {
        unsigned pixelsPerLine = 0;
        buffer_handle_t memHandle = nullptr;
        auto result = alloc.allocate(mWidth, mHeight, mFormat, 1, mUsage, &memHandle,
                                     &pixelsPerLine, 0, "EvsMockCamera");
        if (result != ::android::NO_ERROR) {
            LOG(ERROR) << "Error " << result << " allocating " << mWidth << " x " << mHeight
                       << " graphics buffer";
            break;
        }
        if (memHandle == nullptr) {
            LOG(ERROR) << "We didn't get a buffer handle back from the allocator";
            break;
        }
        if (mStride > 0) {
            if (mStride != pixelsPerLine) {
                LOG(ERROR) << "We did not expect to get buffers with different strides!";
            }
        } else {
            // Gralloc defines stride in terms of pixels per line
            mStride = pixelsPerLine;
        }

        // Find a place to store the new buffer
        auto stored = false;
        for (auto&& rec : mBuffers) {
            if (rec.handle == nullptr) {
                // Use this existing entry
                rec.handle = memHandle;
                rec.inUse = false;
                stored = true;
                break;
            }
        }
        if (!stored) {
            // Add a BufferRecord wrapping this handle to our set of available buffers
            mBuffers.push_back(BufferRecord(memHandle));
        }

        ++mFramesAllowed;
        ++added;
    }

    return added;
}

unsigned EvsMockCamera::decreaseAvailableFrames_Locked(unsigned numToRemove) {
    // Acquire the graphics buffer allocator
    ::android::GraphicBufferAllocator& alloc(::android::GraphicBufferAllocator::get());

    unsigned removed = 0;
    for (auto&& rec : mBuffers) {
        // Is this record not in use, but holding a buffer that we can free?
        if ((rec.inUse == false) && (rec.handle != nullptr)) {
            // Release buffer and update the record so we can recognize it as "empty"
            alloc.free(rec.handle);
            rec.handle = nullptr;

            --mFramesAllowed;
            ++removed;

            if (removed == numToRemove) {
                break;
            }
        }
    }

    return removed;
}

// This is the asynchronous frame generation thread that runs in parallel with the
// main serving thread.  There is one for each active camera instance.
void EvsMockCamera::generateFrames() {
    LOG(DEBUG) << "Frame generation loop started.";

    unsigned idx = 0;
    while (true) {
        bool timeForFrame = false;
        const nsecs_t startTime = systemTime(SYSTEM_TIME_MONOTONIC);

        // Lock scope for updating shared state
        {
            std::lock_guard lock(mAccessLock);

            if (mStreamState != RUNNING) {
                // Break out of our main thread loop
                break;
            }

            // Are we allowed to issue another buffer?
            if (mFramesInUse >= mFramesAllowed) {
                // Can't do anything right now -- skip this frame
                LOG(WARNING) << "Skipped a frame because too many are in flight.";
            } else {
                // Identify an available buffer to fill
                for (idx = 0; idx < mBuffers.size(); idx++) {
                    if (!mBuffers[idx].inUse) {
                        if (mBuffers[idx].handle != nullptr) {
                            // Found an available record, so stop looking
                            break;
                        }
                    }
                }
                if (idx >= mBuffers.size()) {
                    // This shouldn't happen since we already checked mFramesInUse vs mFramesAllowed
                    ALOGE("Failed to find an available buffer slot\n");
                } else {
                    // We're going to make the frame busy
                    mBuffers[idx].inUse = true;
                    mFramesInUse++;
                    timeForFrame = true;
                }
            }
        }

        if (timeForFrame) {
            using AidlPixelFormat = ::aidl::android::hardware::graphics::common::PixelFormat;

            // Assemble the buffer description we'll transmit below
            buffer_handle_t memHandle = mBuffers[idx].handle;
            BufferDesc newBuffer = {
                    .buffer =
                            {
                                    .description =
                                            {
                                                    .width = static_cast<int32_t>(mWidth),
                                                    .height = static_cast<int32_t>(mHeight),
                                                    .layers = 1,
                                                    .format = static_cast<AidlPixelFormat>(mFormat),
                                                    .usage = static_cast<BufferUsage>(mUsage),
                                                    .stride = static_cast<int32_t>(mStride),
                                            },
                                    .handle = ::android::dupToAidl(memHandle),
                            },
                    .bufferId = static_cast<int32_t>(idx),
                    .deviceId = mDescription.id,
                    .timestamp = static_cast<int64_t>(::android::elapsedRealtimeNano() *
                                                      1e+3),  // timestamps is in microseconds
            };

            // Write test data into the image buffer
            fillMockFrame(memHandle, reinterpret_cast<const AHardwareBuffer_Desc*>(
                                             &newBuffer.buffer.description));

            // Issue the (asynchronous) callback to the client -- can't be holding the lock
            auto flag = false;
            if (mStream) {
                std::vector<BufferDesc> frames;
                frames.push_back(std::move(newBuffer));
                flag = mStream->deliverFrame(frames).isOk();
            }

            if (flag) {
                LOG(DEBUG) << "Delivered " << memHandle << ", id = " << mBuffers[idx].handle;
            } else {
                // This can happen if the client dies and is likely unrecoverable.
                // To avoid consuming resources generating failing calls, we stop sending
                // frames.  Note, however, that the stream remains in the "STREAMING" state
                // until cleaned up on the main thread.
                LOG(ERROR) << "Frame delivery call failed in the transport layer.";

                // Since we didn't actually deliver it, mark the frame as available
                std::lock_guard<std::mutex> lock(mAccessLock);
                mBuffers[idx].inUse = false;
                mFramesInUse--;
            }
        }

        // We arbitrarily choose to generate frames at 15 fps to ensure we pass the 10fps test
        // requirement
        static const int kTargetFrameRate = 15;
        static const nsecs_t kTargetFrameIntervalUs = 1000 * 1000 / kTargetFrameRate;
        const nsecs_t now = systemTime(SYSTEM_TIME_MONOTONIC);
        const nsecs_t elapsedTimeUs = (now - startTime) / 1000;
        const nsecs_t sleepDurationUs = kTargetFrameIntervalUs - elapsedTimeUs;
        if (sleepDurationUs > 0) {
            usleep(sleepDurationUs);
        }
    }

    // If we've been asked to stop, send an event to signal the actual end of stream
    EvsEventDesc event = {
            .aType = EvsEventType::STREAM_STOPPED,
    };
    if (!mStream->notify(event).isOk()) {
        ALOGE("Error delivering end of stream marker");
    }

    return;
}

void EvsMockCamera::fillMockFrame(buffer_handle_t handle, const AHardwareBuffer_Desc* pDesc) {
    // Lock our output buffer for writing
    uint32_t* pixels = nullptr;
    ::android::GraphicBufferMapper& mapper = ::android::GraphicBufferMapper::get();
    mapper.lock(handle, GRALLOC_USAGE_SW_WRITE_OFTEN | GRALLOC_USAGE_SW_READ_NEVER,
                ::android::Rect(pDesc->width, pDesc->height), (void**)&pixels);

    // If we failed to lock the pixel buffer, we're about to crash, but log it first
    if (!pixels) {
        ALOGE("Camera failed to gain access to image buffer for writing");
        return;
    }

    // Fill in the test pixels; the colorbar in ABGR format
    for (unsigned row = 0; row < pDesc->height; row++) {
        for (unsigned col = 0; col < pDesc->width; col++) {
            const uint32_t index = col * kNumColors / pDesc->width;
            pixels[col] = kColors[index];
        }
        // Point to the next row
        // NOTE:  stride retrieved from gralloc is in units of pixels
        pixels = pixels + pDesc->stride;
    }

    // Release our output buffer
    mapper.unlock(handle);
}

void EvsMockCamera::returnBufferLocked(const uint32_t bufferId) {
    if (bufferId >= mBuffers.size()) {
        ALOGE("ignoring doneWithFrame called with invalid bufferId %d (max is %zu)", bufferId,
              mBuffers.size() - 1);
        return;
    }

    if (!mBuffers[bufferId].inUse) {
        ALOGE("ignoring doneWithFrame called on frame %d which is already free", bufferId);
        return;
    }

    // Mark the frame as available
    mBuffers[bufferId].inUse = false;
    mFramesInUse--;

    // If this frame's index is high in the array, try to move it down
    // to improve locality after mFramesAllowed has been reduced.
    if (bufferId >= mFramesAllowed) {
        // Find an empty slot lower in the array (which should always exist in this case)
        for (auto&& rec : mBuffers) {
            if (rec.handle == nullptr) {
                rec.handle = mBuffers[bufferId].handle;
                mBuffers[bufferId].handle = nullptr;
                break;
            }
        }
    }
}

std::shared_ptr<EvsMockCamera> EvsMockCamera::Create(const char* deviceName) {
    std::unique_ptr<ConfigManager::CameraInfo> nullCamInfo = nullptr;

    return Create(deviceName, nullCamInfo);
}

std::shared_ptr<EvsMockCamera> EvsMockCamera::Create(
        const char* deviceName, std::unique_ptr<ConfigManager::CameraInfo>& camInfo,
        [[maybe_unused]] const Stream* streamCfg) {
    std::shared_ptr<EvsMockCamera> c =
            ndk::SharedRefBase::make<EvsMockCamera>(Sigil{}, deviceName, camInfo);
    if (!c) {
        LOG(ERROR) << "Failed to instantiate EvsMockCamera.";
        return nullptr;
    }

    // Use the first resolution from the list for the testing
    // TODO(b/214835237): Uses a given Stream configuration to choose the best
    // stream configuration.
    auto it = camInfo->streamConfigurations.begin();
    c->mWidth = it->second.width;
    c->mHeight = it->second.height;
    c->mDescription.vendorFlags = 0xFFFFFFFF;  // Arbitrary test value

    c->mFormat = HAL_PIXEL_FORMAT_RGBA_8888;
    c->mUsage = GRALLOC_USAGE_HW_TEXTURE | GRALLOC_USAGE_HW_CAMERA_WRITE |
                GRALLOC_USAGE_SW_READ_RARELY | GRALLOC_USAGE_SW_WRITE_RARELY;

    return c;
}

}  // namespace aidl::android::hardware::automotive::evs::implementation
