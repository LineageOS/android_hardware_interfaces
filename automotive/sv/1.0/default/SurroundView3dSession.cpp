/*
 * Copyright (C) 2020 The Android Open Source Project
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

#include "SurroundView3dSession.h"

#include <set>

#include <utils/Log.h>
#include <utils/SystemClock.h>

#include <android/hidl/memory/1.0/IMemory.h>
#include <hidlmemory/mapping.h>

using ::android::hidl::memory::V1_0::IMemory;
using ::android::hardware::hidl_memory;

namespace android {
namespace hardware {
namespace automotive {
namespace sv {
namespace V1_0 {
namespace implementation {

SurroundView3dSession::SurroundView3dSession() :
    mStreamState(STOPPED){

    mEvsCameraIds = {"0" , "1", "2", "3"};

    mConfig.width = 640;
    mConfig.height = 480;
    mConfig.carDetails = SvQuality::HIGH;

    framesRecord.frames.svBuffers.resize(1);
    framesRecord.frames.svBuffers[0].viewId = 0;
    framesRecord.frames.svBuffers[0].hardwareBuffer.nativeHandle = new native_handle_t();
    framesRecord.frames.svBuffers[0].hardwareBuffer.description[0] = mConfig.width;
    framesRecord.frames.svBuffers[0].hardwareBuffer.description[1] = mConfig.height;
}

// Methods from ::android::hardware::automotive::sv::V1_0::ISurroundViewSession.
Return<SvResult> SurroundView3dSession::startStream(
    const sp<ISurroundViewStream>& stream) {
    ALOGD("SurroundView3dSession::startStream");
    std::lock_guard<std::mutex> lock(mAccessLock);

    if (mStreamState != STOPPED) {
        ALOGE("ignoring startVideoStream call when a stream is already running.");
        return SvResult::INTERNAL_ERROR;
    }

    if (mViews.empty()) {
        ALOGE("No views have been set for current Surround View 3d Session. "
              "Please call setViews before starting the stream.");
        return SvResult::VIEW_NOT_SET;
    }

    mStream = stream;

    ALOGD("Notify SvEvent::STREAM_STARTED");
    mStream->notify(SvEvent::STREAM_STARTED);

    // Start the frame generation thread
    mStreamState = RUNNING;
    mCaptureThread = std::thread([this](){ generateFrames(); });

    return SvResult::OK;
}

Return<void> SurroundView3dSession::stopStream() {
    ALOGD("SurroundView3dSession::stopStream");
    std::unique_lock <std::mutex> lock(mAccessLock);

    if (mStreamState == RUNNING) {
        // Tell the GenerateFrames loop we want it to stop
        mStreamState = STOPPING;

        // Block outside the mutex until the "stop" flag has been acknowledged
        // We won't send any more frames, but the client might still get some already in flight
        ALOGD("Waiting for stream thread to end...");
        lock.unlock();
        mCaptureThread.join();
        lock.lock();

        mStreamState = STOPPED;
        mStream = nullptr;
        ALOGD("Stream marked STOPPED.");
    }

    return android::hardware::Void();
}

Return<void> SurroundView3dSession::doneWithFrames(
    const SvFramesDesc& svFramesDesc){
    ALOGD("SurroundView3dSession::doneWithFrames");
    std::unique_lock <std::mutex> lock(mAccessLock);

    framesRecord.inUse = false;

    (void)svFramesDesc;
    return android::hardware::Void();
}

// Methods from ISurroundView3dSession follow.
Return<SvResult> SurroundView3dSession::setViews(const hidl_vec<View3d>& views) {
    ALOGD("SurroundView3dSession::stopStream");
    std::unique_lock <std::mutex> lock(mAccessLock);

    mViews.resize(views.size());
    for (int i=0; i<views.size(); i++) {
        mViews[i] = views[i];
    }

    return SvResult::OK;
}

Return<SvResult> SurroundView3dSession::set3dConfig(const Sv3dConfig& sv3dConfig) {
    ALOGD("SurroundView3dSession::set3dConfig");
    std::unique_lock <std::mutex> lock(mAccessLock);

    mConfig.width = sv3dConfig.width;
    mConfig.height = sv3dConfig.height;
    mConfig.carDetails = sv3dConfig.carDetails;
    ALOGD("Notify SvEvent::CONFIG_UPDATED");
    mStream->notify(SvEvent::CONFIG_UPDATED);

    return SvResult::OK;
}

Return<void> SurroundView3dSession::get3dConfig(get3dConfig_cb _hidl_cb) {
    ALOGD("SurroundView3dSession::get3dConfig");
    std::unique_lock <std::mutex> lock(mAccessLock);

    _hidl_cb(mConfig);
    return android::hardware::Void();
}

bool VerifyOverlayData(const OverlaysData& overlaysData) {
    // Check size of shared memory matches overlaysMemoryDesc.
    const int kVertexSize = 16;
    const int kIdSize = 2;
    int memDescSize = 0;
    for (auto overlayMemDesc : overlaysData.overlaysMemoryDesc) {
        memDescSize += kIdSize + kVertexSize * overlayMemDesc.verticesCount;
    }
    if (memDescSize != overlaysData.overlaysMemory.size()) {
        ALOGE("shared memory and overlaysMemoryDesc size mismatch.");
        return false;
    }

    // Map memory.
    sp<IMemory> pSharedMemory = mapMemory(overlaysData.overlaysMemory);
    if(pSharedMemory.get() == nullptr) {
        ALOGE("mapMemory failed.");
        return false;
    }

    // Get Data pointer.
    uint8_t* pData = (uint8_t*)((void*)pSharedMemory->getPointer());
    if (pData == nullptr) {
        ALOGE("Shared memory getPointer() failed.");
        return false;
    }

    int idOffset = 0;
    std::set<uint16_t> overlayIdSet;
    for (auto overlayMemDesc : overlaysData.overlaysMemoryDesc) {

        if (overlayIdSet.find(overlayMemDesc.id) != overlayIdSet.end()) {
            ALOGE("Duplicate id within memory descriptor.");
            return false;
        }
        overlayIdSet.insert(overlayMemDesc.id);

        if(overlayMemDesc.verticesCount < 3) {
            ALOGE("Less than 3 vertices.");
            return false;
        }

        if (overlayMemDesc.overlayPrimitive == OverlayPrimitive::TRIANGLES &&
                overlayMemDesc.verticesCount % 3 != 0) {
            ALOGE("Triangles primitive does not have vertices multiple of 3.");
            return false;
        }

        uint16_t overlayId = *((uint16_t*)(pData + idOffset));

        if (overlayId != overlayMemDesc.id) {
            ALOGE("Overlay id mismatch %d , %d", overlayId, overlayMemDesc.id);
            return false;
        }

        idOffset += kIdSize + (kVertexSize * overlayMemDesc.verticesCount);
    }

    return true;
}

Return<SvResult>  SurroundView3dSession::updateOverlays(
        const OverlaysData& overlaysData) {

    if(!VerifyOverlayData(overlaysData)) {
        ALOGE("VerifyOverlayData failed.");
        return SvResult::INVALID_ARG;
    }

    return SvResult::OK;
}

Return<void> SurroundView3dSession::projectCameraPointsTo3dSurface(
    const hidl_vec<Point2dInt>& cameraPoints,
    const hidl_string& cameraId,
    projectCameraPointsTo3dSurface_cb _hidl_cb) {

    std::vector<Point3dFloat> points3d;
    bool cameraIdFound = false;
    for (auto evsCameraId : mEvsCameraIds) {
      if (cameraId == evsCameraId) {
          cameraIdFound = true;
          ALOGI("Camera id found.");
          break;
      }
    }

    if (!cameraIdFound) {
        ALOGE("Camera id not found.");
        _hidl_cb(points3d);
        return android::hardware::Void();
    }

    for (const auto cameraPoint : cameraPoints) {
        Point3dFloat point3d;
        point3d.isValid = true;

        if (cameraPoint.x < 0 || cameraPoint.x >= mConfig.width-1 ||
                cameraPoint.y < 0 || cameraPoint.y >= mConfig.height-1) {
            ALOGE("Camera point out of bounds.");
            point3d.isValid = false;
        }
        points3d.push_back(point3d);
    }
    _hidl_cb(points3d);
    return android::hardware::Void();
}

void SurroundView3dSession::generateFrames() {
    ALOGD("SurroundView3dSession::generateFrames");

    int sequenceId = 0;

    while(true) {
        {
            std::lock_guard<std::mutex> lock(mAccessLock);

            if (mStreamState != RUNNING) {
                // Break out of our main thread loop
                break;
            }
        }

        usleep(100 * 1000);

        framesRecord.frames.timestampNs = elapsedRealtimeNano();
        framesRecord.frames.sequenceId = sequenceId++;

        framesRecord.frames.svBuffers.resize(mViews.size());
        for (int i=0; i<mViews.size(); i++) {
            framesRecord.frames.svBuffers[i].viewId = mViews[i].viewId;
            framesRecord.frames.svBuffers[i].hardwareBuffer.nativeHandle = new native_handle_t();
            framesRecord.frames.svBuffers[i].hardwareBuffer.description[0] = mConfig.width; // width
            framesRecord.frames.svBuffers[i].hardwareBuffer.description[1] = mConfig.height; // height
        }

        {
            std::lock_guard<std::mutex> lock(mAccessLock);

            if (framesRecord.inUse) {
                ALOGD("Notify SvEvent::FRAME_DROPPED");
                mStream->notify(SvEvent::FRAME_DROPPED);
            } else {
                framesRecord.inUse = true;
                mStream->receiveFrames(framesRecord.frames);
            }
        }
    }

    // If we've been asked to stop, send an event to signal the actual end of stream
    ALOGD("Notify SvEvent::STREAM_STOPPED");
    mStream->notify(SvEvent::STREAM_STOPPED);
}

}  // namespace implementation
}  // namespace V1_0
}  // namespace sv
}  // namespace automotive
}  // namespace hardware
}  // namespace android

