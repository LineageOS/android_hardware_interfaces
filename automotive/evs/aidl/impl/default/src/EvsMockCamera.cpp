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

#include <aidl/android/hardware/automotive/evs/EvsResult.h>

#include <aidlcommonsupport/NativeHandle.h>
#include <android-base/logging.h>
#include <ui/GraphicBufferAllocator.h>
#include <ui/GraphicBufferMapper.h>
#include <utils/SystemClock.h>

#include <cstddef>
#include <cstdint>
#include <memory>
#include <tuple>

namespace {

using ::aidl::android::hardware::graphics::common::BufferUsage;
using ::ndk::ScopedAStatus;

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
    : mCameraInfo(camInfo) {
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

// Methods from ::aidl::android::hardware::automotive::evs::IEvsCamera follow.
ScopedAStatus EvsMockCamera::getCameraInfo(CameraDesc* _aidl_return) {
    LOG(DEBUG) << __FUNCTION__;

    // Send back our self description
    *_aidl_return = mDescription;
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

// This is the asynchronous frame generation thread that runs in parallel with the
// main serving thread.  There is one for each active camera instance.
void EvsMockCamera::generateFrames() {
    LOG(DEBUG) << "Frame generation loop started.";

    while (true) {
        const nsecs_t startTime = systemTime(SYSTEM_TIME_MONOTONIC);
        std::size_t bufferId = kInvalidBufferID;
        buffer_handle_t bufferHandle = nullptr;
        {
            std::lock_guard lock(mMutex);
            if (mStreamState != StreamState::RUNNING) {
                break;
            }
            std::tie(bufferId, bufferHandle) = useBuffer_unsafe();
        }

        if (bufferHandle != nullptr) {
            using AidlPixelFormat = ::aidl::android::hardware::graphics::common::PixelFormat;

            // Assemble the buffer description we'll transmit below
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
                                    .handle = ::android::dupToAidl(bufferHandle),
                            },
                    .bufferId = static_cast<int32_t>(bufferId),
                    .deviceId = mDescription.id,
                    .timestamp = static_cast<int64_t>(::android::elapsedRealtimeNano() *
                                                      1e+3),  // timestamps is in microseconds
            };

            // Write test data into the image buffer
            fillMockFrame(bufferHandle, reinterpret_cast<const AHardwareBuffer_Desc*>(
                                                &newBuffer.buffer.description));

            std::vector<BufferDesc> frames;
            frames.push_back(std::move(newBuffer));

            // Issue the (asynchronous) callback to the client -- can't be holding the lock
            if (mStream && mStream->deliverFrame(frames).isOk()) {
                LOG(DEBUG) << "Delivered " << bufferHandle << ", id = " << bufferId;
            } else {
                // This can happen if the client dies and is likely unrecoverable.
                // To avoid consuming resources generating failing calls, we stop sending
                // frames.  Note, however, that the stream remains in the "STREAMING" state
                // until cleaned up on the main thread.
                LOG(ERROR) << "Frame delivery call failed in the transport layer.";
                doneWithFrame(frames);
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

::android::status_t EvsMockCamera::allocateOneFrame(buffer_handle_t* handle) {
    static auto& alloc = ::android::GraphicBufferAllocator::get();
    unsigned pixelsPerLine = 0;
    const auto result = alloc.allocate(mWidth, mHeight, mFormat, 1, mUsage, handle, &pixelsPerLine,
                                       0, "EvsMockCamera");
    if (mStride < mWidth) {
        // Gralloc defines stride in terms of pixels per line
        mStride = pixelsPerLine;
    } else if (mStride != pixelsPerLine) {
        LOG(ERROR) << "We did not expect to get buffers with different strides!";
    }
    return result;
}

bool EvsMockCamera::startVideoStreamImpl_locked(
        const std::shared_ptr<evs::IEvsCameraStream>& receiver, ndk::ScopedAStatus& /* status */,
        std::unique_lock<std::mutex>& /* lck */) {
    mStream = receiver;
    mCaptureThread = std::thread([this]() { generateFrames(); });
    return true;
}

bool EvsMockCamera::stopVideoStreamImpl_locked(ndk::ScopedAStatus& /* status */,
                                               std::unique_lock<std::mutex>& lck) {
    lck.unlock();
    if (mCaptureThread.joinable()) {
        mCaptureThread.join();
    }
    lck.lock();
    return true;
}

bool EvsMockCamera::postVideoStreamStop_locked(ndk::ScopedAStatus& status,
                                               std::unique_lock<std::mutex>& lck) {
    if (!Base::postVideoStreamStop_locked(status, lck)) {
        return false;
    }
    mStream = nullptr;
    return true;
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
