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

#include "EvsCameraBase.h"

#include <aidl/android/hardware/automotive/evs/IEvsCameraStream.h>
#include <cutils/native_handle.h>

#include <cstddef>
#include <mutex>
#include <utility>
#include <vector>

namespace aidl::android::hardware::automotive::evs::implementation {

class EvsCamera : public EvsCameraBase {
  private:
    using Base = EvsCameraBase;
    using Self = EvsCamera;

  public:
    using Base::Base;

    ~EvsCamera() override;

    // Methods from ::android::hardware::automotive::evs::IEvsCamera follow.
    ndk::ScopedAStatus doneWithFrame(const std::vector<evs::BufferDesc>& buffers) override;

    ndk::ScopedAStatus importExternalBuffers(const std::vector<evs::BufferDesc>& buffers,
                                             int32_t* _aidl_return) override;

    ndk::ScopedAStatus setMaxFramesInFlight(int32_t bufferCount) override;

    ndk::ScopedAStatus startVideoStream(
            const std::shared_ptr<evs::IEvsCameraStream>& receiver) override;

    ndk::ScopedAStatus stopVideoStream() override;

    ndk::ScopedAStatus pauseVideoStream() override;

    ndk::ScopedAStatus resumeVideoStream() override;

  protected:
    virtual ::android::status_t allocateOneFrame(buffer_handle_t* handle) = 0;

    virtual void freeOneFrame(const buffer_handle_t handle);

    virtual bool preVideoStreamStart_locked(const std::shared_ptr<evs::IEvsCameraStream>& receiver,
                                            ndk::ScopedAStatus& status,
                                            std::unique_lock<std::mutex>& lck);

    virtual bool startVideoStreamImpl_locked(const std::shared_ptr<evs::IEvsCameraStream>& receiver,
                                             ndk::ScopedAStatus& status,
                                             std::unique_lock<std::mutex>& lck) = 0;

    virtual bool postVideoStreamStart_locked(const std::shared_ptr<evs::IEvsCameraStream>& receiver,
                                             ndk::ScopedAStatus& status,
                                             std::unique_lock<std::mutex>& lck);

    virtual bool preVideoStreamStop_locked(ndk::ScopedAStatus& status,
                                           std::unique_lock<std::mutex>& lck);

    virtual bool stopVideoStreamImpl_locked(ndk::ScopedAStatus& status,
                                            std::unique_lock<std::mutex>& lck) = 0;

    virtual bool postVideoStreamStop_locked(ndk::ScopedAStatus& status,
                                            std::unique_lock<std::mutex>& lck);

    void shutdown() override;

    void closeAllBuffers_unsafe();

    // Returns (ID, handle) if succeeds. (kInvalidBufferID, nullptr) otherwise.
    [[nodiscard]] std::pair<std::size_t, buffer_handle_t> useBuffer_unsafe();

    void returnBuffer_unsafe(const std::size_t id);

    bool increaseAvailableFrames_unsafe(const buffer_handle_t handle);

    bool decreaseAvailableFrames_unsafe();

    bool setAvailableFrames_unsafe(const std::size_t bufferCount);

    void swapBufferFrames_unsafe(const std::size_t pos1, const std::size_t pos2);

    struct BufferRecord {
        BufferRecord() = default;
        BufferRecord(const BufferRecord&) = default;
        BufferRecord(BufferRecord&&) = default;
        BufferRecord& operator=(const BufferRecord&) = default;
        BufferRecord& operator=(BufferRecord&&) = default;
        ~BufferRecord() = default;

        explicit BufferRecord(buffer_handle_t h) : handle(h) {}

        buffer_handle_t handle{nullptr};
        bool inUse{false};
    };

    enum class StreamState {
        STOPPED = 0,
        RUNNING = 1,
        STOPPING = 2,
        DEAD = 3,
    };

    StreamState mStreamState{StreamState::STOPPED};

    std::mutex mMutex;

    // Graphics buffers to transfer images, always in the order of:
    // In use buffers ... available buffers ... unavailable (unallocated) buffers.
    std::vector<BufferRecord> mBuffers;

    // Double-mapping between buffer position and ID.
    std::vector<std::size_t> mBufferPosToId;
    std::vector<std::size_t> mBufferIdToPos;

    std::size_t mAvailableFrames{0};
    std::size_t mFramesInUse{0};

    // We use all 1's as a reserved invalid buffer ID.
    static constexpr std::size_t kInvalidBufferID = ~static_cast<std::size_t>(0);

  public:
    static bool IsBufferIDValid(const std::size_t bufferId) { return ~bufferId; }
};

}  // namespace aidl::android::hardware::automotive::evs::implementation
