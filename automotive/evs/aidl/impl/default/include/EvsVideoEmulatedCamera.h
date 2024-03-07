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
#include "EvsCamera.h"

#include <aidl/android/hardware/automotive/evs/BufferDesc.h>
#include <aidl/android/hardware/automotive/evs/CameraDesc.h>
#include <aidl/android/hardware/automotive/evs/CameraParam.h>
#include <aidl/android/hardware/automotive/evs/IEvsCameraStream.h>
#include <aidl/android/hardware/automotive/evs/IEvsDisplay.h>
#include <aidl/android/hardware/automotive/evs/ParameterRange.h>
#include <aidl/android/hardware/automotive/evs/Stream.h>
#include <media/NdkMediaExtractor.h>

#include <ui/GraphicBuffer.h>

#include <cstdint>
#include <memory>
#include <thread>
#include <unordered_map>
#include <vector>

namespace aidl::android::hardware::automotive::evs::implementation {

class EvsVideoEmulatedCamera : public EvsCamera {
  private:
    using Base = EvsCamera;

  public:
    EvsVideoEmulatedCamera(Sigil sigil, const char* deviceName,
                           std::unique_ptr<ConfigManager::CameraInfo>& camInfo);

    ~EvsVideoEmulatedCamera() override = default;

    // Methods from ::android::hardware::automotive::evs::IEvsCamera follow.
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
    ndk::ScopedAStatus setExtendedInfo(int32_t opaqueIdentifier,
                                       const std::vector<uint8_t>& opaqueValue) override;
    ndk::ScopedAStatus setIntParameter(evs::CameraParam id, int32_t value,
                                       std::vector<int32_t>* effectiveValue) override;
    ndk::ScopedAStatus setPrimaryClient() override;
    ndk::ScopedAStatus unsetPrimaryClient() override;

    // Methods from EvsCameraBase follow.
    void shutdown() override;

    const evs::CameraDesc& getDesc() { return mDescription; }

    static std::shared_ptr<EvsVideoEmulatedCamera> Create(const char* deviceName);
    static std::shared_ptr<EvsVideoEmulatedCamera> Create(
            const char* deviceName, std::unique_ptr<ConfigManager::CameraInfo>& camInfo,
            const evs::Stream* streamCfg = nullptr);

  private:
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

    bool initialize();

    void generateFrames();

    void renderOneFrame();

    void initializeParameters();

    void onCodecInputAvailable(const int32_t index);

    void onCodecOutputAvailable(const int32_t index, const AMediaCodecBufferInfo& info);

    ::android::status_t allocateOneFrame(buffer_handle_t* handle) override;

    bool startVideoStreamImpl_locked(const std::shared_ptr<evs::IEvsCameraStream>& receiver,
                                     ndk::ScopedAStatus& status,
                                     std::unique_lock<std::mutex>& lck) override;

    bool stopVideoStreamImpl_locked(ndk::ScopedAStatus& status,
                                    std::unique_lock<std::mutex>& lck) override;

    bool postVideoStreamStop_locked(ndk::ScopedAStatus& status,
                                    std::unique_lock<std::mutex>& lck) override;

    // The properties of this camera.
    CameraDesc mDescription = {};

    std::thread mCaptureThread;

    // The callback used to deliver each frame
    std::shared_ptr<evs::IEvsCameraStream> mStream;

    std::string mVideoFileName;
    // Media decoder resources - Owned by mDecoderThead when thread is running.
    int mVideoFd = 0;

    struct AMediaExtractorDeleter {
        void operator()(AMediaExtractor* extractor) const { AMediaExtractor_delete(extractor); }
    };
    struct AMediaCodecDeleter {
        void operator()(AMediaCodec* codec) const { AMediaCodec_delete(codec); }
    };

    std::unique_ptr<AMediaExtractor, AMediaExtractorDeleter> mVideoExtractor;
    std::unique_ptr<AMediaCodec, AMediaCodecDeleter> mVideoCodec;

    // Horizontal pixel count in the buffers
    int32_t mWidth = 0;
    // Vertical pixel count in the buffers
    int32_t mHeight = 0;
    // Values from android_pixel_format_t
    uint32_t mFormat = 0;
    // Values from from Gralloc.h
    uint64_t mUsage = 0;
    // Bytes per line in the buffers
    uint32_t mStride = 0;

    // Camera parameters.
    std::unordered_map<CameraParam, std::shared_ptr<CameraParameterDesc>> mParams;

    // Static camera module information
    std::unique_ptr<ConfigManager::CameraInfo>& mCameraInfo;

    // For the extended info
    std::unordered_map<uint32_t, std::vector<uint8_t>> mExtInfo;
};

}  // namespace aidl::android::hardware::automotive::evs::implementation
