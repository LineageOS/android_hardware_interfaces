/*
 * Copyright (C) 2022 The Android Open Source Project
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

#ifndef HARDWARE_INTERFACES_CAMERA_PROVIDER_AIDL_VTS_DEVICECB_H_
#define HARDWARE_INTERFACES_CAMERA_PROVIDER_AIDL_VTS_DEVICECB_H_

#include <camera_aidl_test.h>

#include <unordered_map>

#include <CameraMetadata.h>
#include <aidl/android/hardware/camera/device/BnCameraDeviceCallback.h>
#include <aidl/android/hardware/camera/device/NotifyMsg.h>

using ::aidl::android::hardware::camera::device::BnCameraDeviceCallback;
using ::aidl::android::hardware::camera::device::BufferRequest;
using ::aidl::android::hardware::camera::device::BufferRequestStatus;
using ::aidl::android::hardware::camera::device::CaptureResult;
using ::aidl::android::hardware::camera::device::HalStream;
using ::aidl::android::hardware::camera::device::NotifyMsg;
using ::aidl::android::hardware::camera::device::PhysicalCameraMetadata;
using ::aidl::android::hardware::camera::device::Stream;
using ::aidl::android::hardware::camera::device::StreamBuffer;
using ::aidl::android::hardware::camera::device::StreamBufferRet;
using ::aidl::android::hardware::common::NativeHandle;

using ::ndk::ScopedAStatus;

class CameraAidlTest;

class DeviceCb : public BnCameraDeviceCallback {
  public:
    DeviceCb(CameraAidlTest* parent, camera_metadata_t* staticMeta);
    ScopedAStatus notify(const std::vector<NotifyMsg>& msgs) override;
    ScopedAStatus processCaptureResult(const std::vector<CaptureResult>& results) override;
    ScopedAStatus requestStreamBuffers(const std::vector<BufferRequest>& bufReqs,
                                       std::vector<StreamBufferRet>* buffers,
                                       BufferRequestStatus* _aidl_return) override;
    ScopedAStatus returnStreamBuffers(const std::vector<StreamBuffer>& buffers) override;

    void setCurrentStreamConfig(const std::vector<Stream>& streams,
                                const std::vector<HalStream>& halStreams);

    void waitForBuffersReturned();

  private:
    bool processCaptureResultLocked(const CaptureResult& results,
                                    std::vector<PhysicalCameraMetadata> physicalCameraMetadata);
    ScopedAStatus notifyHelper(const std::vector<NotifyMsg>& msgs,
                               const std::vector<std::pair<bool, nsecs_t>>& readoutTimestamps);

    CameraAidlTest* mParent;  // Parent object

    camera_metadata_t* mStaticMetadata;
    bool hasOutstandingBuffersLocked();

    /* members for requestStreamBuffers() and returnStreamBuffers()*/
    std::mutex mLock;  // protecting members below
    bool mUseHalBufManager = false;
    std::vector<Stream> mStreams;
    std::vector<HalStream> mHalStreams;
    int64_t mNextBufferId = 1;
    using OutstandingBuffers = std::unordered_map<uint64_t, buffer_handle_t>;
    // size == mStreams.size(). Tracking each streams outstanding buffers
    std::vector<OutstandingBuffers> mOutstandingBufferIds;
    std::condition_variable mFlushedCondition;
};

#endif  // HARDWARE_INTERFACES_CAMERA_PROVIDER_AIDL_VTS_DEVICECB_H_
