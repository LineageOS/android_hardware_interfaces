/*
 * Copyright (C) 2025 The Android Open Source Project
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

#include <mutex>
#include <string>

#include "core-impl/DriverStubImpl.h"
#include "core-impl/Stream.h"

namespace aidl::android::hardware::audio::core {

namespace mmap {

struct DspSimulatorState {
    const bool isInput;
    const int sampleRate;
    const int frameSizeBytes;
    const size_t bufferSizeBytes;
    std::mutex lock;
    // The lock is also used to prevent un-mapping while the memory is in use.
    uint8_t* sharedMemory GUARDED_BY(lock) = nullptr;
    StreamDescriptor::Position mmapPos GUARDED_BY(lock);
};

class DspSimulatorLogic : public ::android::hardware::audio::common::StreamLogic {
  protected:
    explicit DspSimulatorLogic(DspSimulatorState& sharedState) : mSharedState(sharedState) {}
    std::string init() override;
    Status cycle() override;

  private:
    DspSimulatorState& mSharedState;
    uint32_t mCycleDurationUs = 0;
    uint8_t* mMemBegin = nullptr;
    uint8_t* mMemPos = nullptr;
    int64_t mLastFrames = 0;
};

class DspSimulatorWorker
    : public ::android::hardware::audio::common::StreamWorker<DspSimulatorLogic> {
  public:
    explicit DspSimulatorWorker(DspSimulatorState& sharedState)
        : ::android::hardware::audio::common::StreamWorker<DspSimulatorLogic>(sharedState) {}
};

}  // namespace mmap

class DriverMmapStubImpl : public DriverStubImpl {
  public:
    explicit DriverMmapStubImpl(const StreamContext& context);
    ::android::status_t init(DriverCallbackInterface* callback) override;
    ::android::status_t drain(StreamDescriptor::DrainMode drainMode) override;
    ::android::status_t flush() override;
    ::android::status_t pause() override;
    ::android::status_t standby() override;
    ::android::status_t start() override;
    ::android::status_t transfer(void* buffer, size_t frameCount, size_t* actualFrameCount,
                                 int32_t* latencyMs) override;
    void shutdown() override;
    ::android::status_t refinePosition(StreamDescriptor::Position* position) override;
    ::android::status_t getMmapPositionAndLatency(StreamDescriptor::Position* position,
                                                  int32_t* latency) override;

  protected:
    ::android::status_t initSharedMemory(int ashmemFd);

  private:
    ::android::status_t releaseSharedMemory() REQUIRES(mState.lock);
    ::android::status_t startWorkerIfNeeded();

    mmap::DspSimulatorState mState;
    mmap::DspSimulatorWorker mDspWorker;
    bool mDspWorkerStarted = false;
};

class StreamMmapStub : public StreamCommonImpl, public DriverMmapStubImpl {
  public:
    static const std::string kCreateMmapBufferName;

    StreamMmapStub(StreamContext* context, const Metadata& metadata);
    ~StreamMmapStub();

    ndk::ScopedAStatus getVendorParameters(const std::vector<std::string>& in_ids,
                                           std::vector<VendorParameter>* _aidl_return) override;
    ndk::ScopedAStatus setVendorParameters(const std::vector<VendorParameter>& in_parameters,
                                           bool in_async) override;
    ndk::ScopedAStatus createMmapBuffer(MmapBufferDescriptor* _aidl_return) override;

  private:
    ndk::ScopedFileDescriptor mSharedMemoryFd;
};

class StreamInMmapStub final : public StreamIn, public StreamMmapStub {
  public:
    friend class ndk::SharedRefBase;
    StreamInMmapStub(
            StreamContext&& context,
            const ::aidl::android::hardware::audio::common::SinkMetadata& sinkMetadata,
            const std::vector<::aidl::android::media::audio::common::MicrophoneInfo>& microphones);

  private:
    void onClose(StreamDescriptor::State) override { defaultOnClose(); }
};

class StreamOutMmapStub final : public StreamOut, public StreamMmapStub {
  public:
    friend class ndk::SharedRefBase;
    StreamOutMmapStub(
            StreamContext&& context,
            const ::aidl::android::hardware::audio::common::SourceMetadata& sourceMetadata,
            const std::optional<::aidl::android::media::audio::common::AudioOffloadInfo>&
                    offloadInfo);

  private:
    void onClose(StreamDescriptor::State) override { defaultOnClose(); }
};

}  // namespace aidl::android::hardware::audio::core
