/*
 * Copyright (C) 2024 The Android Open Source Project
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

#include "core-impl/Stream.h"

namespace aidl::android::hardware::audio::core {

class DriverStubImpl : virtual public DriverInterface {
  public:
    explicit DriverStubImpl(const StreamContext& context);

    ::android::status_t init() override;
    ::android::status_t drain(StreamDescriptor::DrainMode) override;
    ::android::status_t flush() override;
    ::android::status_t pause() override;
    ::android::status_t standby() override;
    ::android::status_t start() override;
    ::android::status_t transfer(void* buffer, size_t frameCount, size_t* actualFrameCount,
                                 int32_t* latencyMs) override;
    void shutdown() override;

  private:
    const size_t mBufferSizeFrames;
    const size_t mFrameSizeBytes;
    const int mSampleRate;
    const bool mIsAsynchronous;
    const bool mIsInput;
    bool mIsInitialized = false;  // Used for validating the state machine logic.
    bool mIsStandby = true;       // Used for validating the state machine logic.
    int64_t mStartTimeNs = 0;
    long mFramesSinceStart = 0;
};

}  // namespace aidl::android::hardware::audio::core
