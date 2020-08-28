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

#pragma once

#include <android/hardware/automotive/sv/1.0/types.h>
#include <android/hardware/automotive/sv/1.0/ISurroundViewStream.h>
#include <android/hardware/automotive/sv/1.0/ISurroundView2dSession.h>
#include <hidl/MQDescriptor.h>
#include <hidl/Status.h>

#include <thread>

using namespace ::android::hardware::automotive::sv::V1_0;
using ::android::hardware::Return;
using ::android::hardware::Void;
using ::android::hardware::hidl_vec;
using ::android::sp;
using ::std::mutex;

namespace android {
namespace hardware {
namespace automotive {
namespace sv {
namespace V1_0 {
namespace implementation {

class SurroundView2dSession : public ISurroundView2dSession {
public:
    SurroundView2dSession();

    // Methods from ::android::hardware::automotive::sv::V1_0::ISurroundViewSession.
    Return<SvResult> startStream(
        const sp<ISurroundViewStream>& stream) override;
    Return<void> stopStream() override;
    Return<void> doneWithFrames(const SvFramesDesc& svFramesDesc) override;

    // Methods from ISurroundView2dSession follow.
    Return<void> get2dMappingInfo(get2dMappingInfo_cb _hidl_cb) override;
    Return<SvResult> set2dConfig(const Sv2dConfig& sv2dConfig) override;
    Return<void> get2dConfig(get2dConfig_cb _hidl_cb) override;
    Return<void> projectCameraPoints(
        const hidl_vec<Point2dInt>& points2dCamera,
        const hidl_string& cameraId,
        projectCameraPoints_cb _hidl_cb) override;

    // TODO(tanmayp): Make private and add set/get method.
    // Stream subscribed for the session.
    sp<ISurroundViewStream> mStream;

private:
    void generateFrames();

    enum StreamStateValues {
        STOPPED,
        RUNNING,
        STOPPING,
        DEAD,
    };
    StreamStateValues mStreamState;

    Sv2dConfig mConfig;

    std::thread mCaptureThread; // The thread we'll use to synthesize frames

    struct FramesRecord {
        SvFramesDesc frames;
        bool inUse = false;
    };

    FramesRecord framesRecord;

    // Synchronization necessary to deconflict mCaptureThread from the main service thread
    std::mutex mAccessLock;

    std::vector<std::string> mEvsCameraIds;
};

}  // namespace implementation
}  // namespace V1_0
}  // namespace sv
}  // namespace automotive
}  // namespace hardware
}  // namespace android

