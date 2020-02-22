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

#include "SurroundView2dSession.h"

#include <utils/Log.h>
#include <utils/SystemClock.h>

namespace android {
namespace hardware {
namespace automotive {
namespace sv {
namespace V1_0 {
namespace implementation {

SurroundView2dSession::SurroundView2dSession() :
    mStreamState(STOPPED) {
    mEvsCameraIds = {"0" , "1", "2", "3"};

    mConfig.width = 640;
    mConfig.blending = SvQuality::HIGH;

    framesRecord.frames.svBuffers.resize(1);
    framesRecord.frames.svBuffers[0].viewId = 0;
    framesRecord.frames.svBuffers[0].hardwareBuffer.nativeHandle =
        new native_handle_t();
    framesRecord.frames.svBuffers[0].hardwareBuffer.description[0] =
        mConfig.width;
    framesRecord.frames.svBuffers[0].hardwareBuffer.description[1] =
        mConfig.width * 3 / 4;
}

// Methods from ::android::hardware::automotive::sv::V1_0::ISurroundViewSession
Return<SvResult> SurroundView2dSession::startStream(
    const sp<ISurroundViewStream>& stream) {
    ALOGD("SurroundView2dSession::startStream");
    std::lock_guard<std::mutex> lock(mAccessLock);

    if (mStreamState != STOPPED) {
        ALOGE("ignoring startVideoStream call"
              "when a stream is already running.");
        return SvResult::INTERNAL_ERROR;
    }

    mStream = stream;

    ALOGD("Notify SvEvent::STREAM_STARTED");
    mStream->notify(SvEvent::STREAM_STARTED);

    // Start the frame generation thread
    mStreamState = RUNNING;
    mCaptureThread = std::thread([this](){ generateFrames(); });

    return SvResult::OK;
}

Return<void> SurroundView2dSession::stopStream() {
    ALOGD("SurroundView2dSession::stopStream");
    std::unique_lock <std::mutex> lock(mAccessLock);

    if (mStreamState == RUNNING) {
        // Tell the GenerateFrames loop we want it to stop
        mStreamState = STOPPING;

        // Block outside the mutex until the "stop" flag has been acknowledged
        // We won't send any more frames, but the client might still get some
        // already in flight
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

Return<void> SurroundView2dSession::doneWithFrames(
    const SvFramesDesc& svFramesDesc){
    ALOGD("SurroundView2dSession::doneWithFrames");
    std::unique_lock <std::mutex> lock(mAccessLock);

    framesRecord.inUse = false;

    (void)svFramesDesc;
    return android::hardware::Void();
}

// Methods from ISurroundView2dSession follow.
Return<void> SurroundView2dSession::get2dMappingInfo(
    get2dMappingInfo_cb _hidl_cb) {
    ALOGD("SurroundView2dSession::get2dMappingInfo");
    std::unique_lock <std::mutex> lock(mAccessLock);

    Sv2dMappingInfo info;
    info.width = 8; // keeps ratio to 4:3
    info.height = 6;
    info.center.isValid = true;
    info.center.x = 0;
    info.center.y = 0;
    _hidl_cb(info);
    return android::hardware::Void();
}

Return<SvResult> SurroundView2dSession::set2dConfig(
    const Sv2dConfig& sv2dConfig) {
    ALOGD("SurroundView2dSession::setConfig");
    std::unique_lock <std::mutex> lock(mAccessLock);

    mConfig.width = sv2dConfig.width;
    mConfig.blending = sv2dConfig.blending;
    ALOGD("Notify SvEvent::CONFIG_UPDATED");
    mStream->notify(SvEvent::CONFIG_UPDATED);

    return SvResult::OK;
}

Return<void> SurroundView2dSession::get2dConfig(get2dConfig_cb _hidl_cb) {
    ALOGD("SurroundView2dSession::getConfig");
    std::unique_lock <std::mutex> lock(mAccessLock);

    _hidl_cb(mConfig);
    return android::hardware::Void();
}

Return<void> SurroundView2dSession::projectCameraPoints(
        const hidl_vec<Point2dInt>& points2dCamera,
        const hidl_string& cameraId,
        projectCameraPoints_cb _hidl_cb) {
    ALOGD("SurroundView2dSession::projectCameraPoints");
    std::unique_lock <std::mutex> lock(mAccessLock);

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
        _hidl_cb(hidl_vec<Point2dFloat>());
        return android::hardware::Void();
    }

    hidl_vec<Point2dFloat> outPoints;
    outPoints.resize(points2dCamera.size());

    int width = mConfig.width;
    int height = mConfig.width * 3 / 4;
    for (int i=0; i<points2dCamera.size(); i++) {
        // Assuming all the points in the image frame can be projected into 2d
        // Surround View space. Otherwise cannot.
        if (points2dCamera[i].x < 0 || points2dCamera[i].y > width-1 ||
            points2dCamera[i].x < 0 || points2dCamera[i].y > height-1) {
            ALOGW("SurroundView2dSession::projectCameraPoints "
                  "gets invalid 2d camera points. Ignored");
            outPoints[i].isValid = false;
            outPoints[i].x = 10000;
            outPoints[i].y = 10000;
        } else {
            outPoints[i].isValid = true;
            outPoints[i].x = 0;
            outPoints[i].y = 0;
        }
    }

    _hidl_cb(outPoints);
    return android::hardware::Void();
}

void SurroundView2dSession::generateFrames() {
    ALOGD("SurroundView2dSession::generateFrames");

    int sequenceId = 0;

    while(true) {
        {
            std::lock_guard<std::mutex> lock(mAccessLock);

            if (mStreamState != RUNNING) {
                // Break out of our main thread loop
                break;
            }

            framesRecord.frames.svBuffers[0].hardwareBuffer.description[0] =
                mConfig.width;
            framesRecord.frames.svBuffers[0].hardwareBuffer.description[1] =
                mConfig.width * 3 / 4;
        }

        usleep(100 * 1000);

        framesRecord.frames.timestampNs = elapsedRealtimeNano();
        framesRecord.frames.sequenceId = sequenceId++;

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

    // If we've been asked to stop, send an event to signal the actual
    // end of stream
    ALOGD("Notify SvEvent::STREAM_STOPPED");
    mStream->notify(SvEvent::STREAM_STOPPED);
}

}  // namespace implementation
}  // namespace V1_0
}  // namespace sv
}  // namespace automotive
}  // namespace hardware
}  // namespace android

