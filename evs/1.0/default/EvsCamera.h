/*
 * Copyright (C) 2016 The Android Open Source Project
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

#ifndef ANDROID_HARDWARE_EVS_V1_0_EVSCAMERA_H
#define ANDROID_HARDWARE_EVS_V1_0_EVSCAMERA_H

#include <android/hardware/evs/1.0/types.h>
#include <android/hardware/evs/1.0/IEvsCamera.h>
#include <ui/GraphicBuffer.h>

#include <thread>

namespace android {
namespace hardware {
namespace evs {
namespace V1_0 {
namespace implementation {

class EvsCamera : public IEvsCamera {
public:
    // Methods from ::android::hardware::evs::V1_0::IEvsCamera follow.
    Return<void> getId(getId_cb id_cb)  override;
    Return<EvsResult> setMaxFramesInFlight(uint32_t bufferCount)  override;
    Return<EvsResult> startVideoStream(const ::android::sp<IEvsCameraStream>& stream) override;
    Return<EvsResult> doneWithFrame(uint32_t frameId, const hidl_handle& bufferHandle)  override;
    Return<void> stopVideoStream()  override;
    Return<int32_t> getExtendedInfo(uint32_t opaqueIdentifier)  override;
    Return<EvsResult> setExtendedInfo(uint32_t opaqueIdentifier, int32_t opaqueValue)  override;

    // Implementation details
    EvsCamera(const char* id);
    virtual ~EvsCamera() override;

    const CameraDesc& getDesc() { return mDescription; };
    void GenerateFrames();

    static const char kCameraName_Backup[];
    static const char kCameraName_RightTurn[];

private:
    CameraDesc              mDescription = {};  // The properties of this camera

    buffer_handle_t         mBuffer = nullptr;  // A graphics buffer into which we'll store images
    uint32_t                mWidth  = 0;        // number of pixels across the buffer
    uint32_t                mHeight = 0;        // number of pixels vertically in the buffer
    uint32_t                mStride = 0;        // Bytes per line in the buffer

    sp<IEvsCameraStream>    mStream = nullptr;  // The callback the user expects when a frame is ready

    std::thread             mCaptureThread;     // The thread we'll use to synthesize frames

    uint32_t                mFrameId;           // A frame counter used to identify specific frames

    enum StreamStateValues {
        STOPPED,
        RUNNING,
        STOPPING,
    };
    StreamStateValues       mStreamState;
    bool                    mFrameBusy;         // A flag telling us our one buffer is in use

    std::mutex              mAccessLock;
};

} // namespace implementation
} // namespace V1_0
} // namespace evs
} // namespace hardware
} // namespace android

#endif  // ANDROID_HARDWARE_EVS_V1_0_EVSCAMERA_H
