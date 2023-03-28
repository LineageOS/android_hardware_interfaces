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

#pragma once

#include "ConfigManager.h"

#include <aidl/android/hardware/automotive/evs/BnEvsCamera.h>
#include <aidl/android/hardware/automotive/evs/BufferDesc.h>
#include <aidl/android/hardware/automotive/evs/CameraDesc.h>
#include <aidl/android/hardware/automotive/evs/CameraParam.h>
#include <aidl/android/hardware/automotive/evs/EvsResult.h>
#include <aidl/android/hardware/automotive/evs/IEvsCameraStream.h>
#include <aidl/android/hardware/automotive/evs/IEvsDisplay.h>
#include <aidl/android/hardware/automotive/evs/ParameterRange.h>
#include <aidl/android/hardware/automotive/evs/Stream.h>
// #include <android-base/result.h>
#include <android/hardware_buffer.h>
#include <ui/GraphicBuffer.h>

#include <functional>
#include <thread>

namespace aidl::android::hardware::automotive::evs::implementation {

class EvsMockCamera : public evs::BnEvsCamera {
    // This prevents constructors from direct access while it allows this class to
    // be instantiated via ndk::SharedRefBase::make<>.
  private:
    struct Sigil {
        explicit Sigil() = default;
    };

  public:
    // Methods from ::android::hardware::automotive::evs::IEvsCamera follow.
    ndk::ScopedAStatus doneWithFrame(const std::vector<evs::BufferDesc>& buffers) override;
    ndk::ScopedAStatus forcePrimaryClient(
            const std::shared_ptr<evs::IEvsDisplay>& display) override;
    ndk::ScopedAStatus getCameraInfo(evs::CameraDesc* _aidl_return) override;
    ndk::ScopedAStatus getExtendedInfo(int32_t opaqueIdentifier,
                                       std::vector<uint8_t>* value) override;
    ndk::ScopedAStatus getIntParameter(evs::CameraParam id, std::vector<int32_t>* value) override;
    ndk::ScopedAStatus getIntParameterRange(evs::CameraParam id,
                                            evs::ParameterRange* _aidl_return) override;
    ndk::ScopedAStatus getParameterList(std::vector<evs::CameraParam>* _aidl_return) override;
    ndk::ScopedAStatus getPhysicalCameraInfo(const std::string& deviceId,
                                             evs::CameraDesc* _aidl_return) override;
    ndk::ScopedAStatus importExternalBuffers(const std::vector<evs::BufferDesc>& buffers,
                                             int32_t* _aidl_return) override;
    ndk::ScopedAStatus pauseVideoStream() override;
    ndk::ScopedAStatus resumeVideoStream() override;
    ndk::ScopedAStatus setExtendedInfo(int32_t opaqueIdentifier,
                                       const std::vector<uint8_t>& opaqueValue) override;
    ndk::ScopedAStatus setIntParameter(evs::CameraParam id, int32_t value,
                                       std::vector<int32_t>* effectiveValue) override;
    ndk::ScopedAStatus setPrimaryClient() override;
    ndk::ScopedAStatus setMaxFramesInFlight(int32_t bufferCount) override;
    ndk::ScopedAStatus startVideoStream(
            const std::shared_ptr<evs::IEvsCameraStream>& receiver) override;
    ndk::ScopedAStatus stopVideoStream() override;
    ndk::ScopedAStatus unsetPrimaryClient() override;

    static std::shared_ptr<EvsMockCamera> Create(const char* deviceName);
    static std::shared_ptr<EvsMockCamera> Create(
            const char* deviceName, std::unique_ptr<ConfigManager::CameraInfo>& camInfo,
            const evs::Stream* streamCfg = nullptr);
    EvsMockCamera(const EvsMockCamera&) = delete;
    EvsMockCamera& operator=(const EvsMockCamera&) = delete;

    virtual ~EvsMockCamera() override;
    void shutdown();

    const evs::CameraDesc& getDesc() { return mDescription; }

    // Constructors
    EvsMockCamera(Sigil sigil, const char* deviceName,
                  std::unique_ptr<ConfigManager::CameraInfo>& camInfo);

  private:
    // These three functions are expected to be called while mAccessLock is held
    bool setAvailableFrames_Locked(unsigned bufferCount);
    unsigned increaseAvailableFrames_Locked(unsigned numToAdd);
    unsigned decreaseAvailableFrames_Locked(unsigned numToRemove);

    void generateFrames();
    void fillMockFrame(buffer_handle_t handle, const AHardwareBuffer_Desc* pDesc);
    void returnBufferLocked(const uint32_t bufferId);
    ndk::ScopedAStatus stopVideoStream_impl();

    CameraDesc mDescription = {};  // The properties of this camera

    std::thread mCaptureThread;  // The thread we'll use to synthesize frames

    // The callback used to deliver each frame
    std::shared_ptr<evs::IEvsCameraStream> mStream;

    // Horizontal pixel count in the buffers
    uint32_t mWidth = 0;
    // Vertical pixel count in the buffers
    uint32_t mHeight = 0;
    // Values from android_pixel_format_t
    uint32_t mFormat = HAL_PIXEL_FORMAT_RGBA_8888;
    // Values from from Gralloc.h
    uint64_t mUsage =
            GRALLOC_USAGE_HW_TEXTURE | GRALLOC_USAGE_SW_READ_RARELY | GRALLOC_USAGE_SW_WRITE_OFTEN;
    // Bytes per line in the buffers
    uint32_t mStride = 0;

    struct BufferRecord {
        buffer_handle_t handle;
        bool inUse;

        explicit BufferRecord(buffer_handle_t h) : handle(h), inUse(false){};
    };

    std::vector<BufferRecord> mBuffers;  // Graphics buffers to transfer images
    unsigned mFramesAllowed;             // How many buffers are we currently using
    unsigned mFramesInUse;               // How many buffers are currently outstanding

    enum StreamStateValues {
        STOPPED,
        RUNNING,
        STOPPING,
        DEAD,
    };
    StreamStateValues mStreamState;

    // Synchronization necessary to deconflict mCaptureThread from the main service thread
    std::mutex mAccessLock;

    // Static camera module information
    std::unique_ptr<ConfigManager::CameraInfo>& mCameraInfo;

    // For the extended info
    std::unordered_map<uint32_t, std::vector<uint8_t>> mExtInfo;

    // For the camera parameters.
    struct CameraParameterDesc {
        CameraParameterDesc(int min = 0, int max = 0, int step = 0, int value = 0) {
            this->range.min = min;
            this->range.max = max;
            this->range.step = step;
            this->value = value;
        }

        ParameterRange range;
        int32_t value;
    };
    std::unordered_map<CameraParam, std::shared_ptr<CameraParameterDesc>> mParams;
    void initializeParameters();
};

}  // namespace aidl::android::hardware::automotive::evs::implementation
