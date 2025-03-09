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

#include <cmath>

#define LOG_TAG "AHAL_Stream"
#include <android-base/logging.h>
#include <audio_utils/clock.h>

#include "core-impl/Module.h"
#include "core-impl/StreamStub.h"

using aidl::android::hardware::audio::common::SinkMetadata;
using aidl::android::hardware::audio::common::SourceMetadata;
using aidl::android::media::audio::common::AudioDevice;
using aidl::android::media::audio::common::AudioOffloadInfo;
using aidl::android::media::audio::common::MicrophoneInfo;

namespace aidl::android::hardware::audio::core {

StreamStub::StreamStub(StreamContext* context, const Metadata& metadata)
    : StreamCommonImpl(context, metadata), DriverStubImpl(getContext()) {}

StreamStub::~StreamStub() {
    cleanupWorker();
}

StreamInStub::StreamInStub(StreamContext&& context, const SinkMetadata& sinkMetadata,
                           const std::vector<MicrophoneInfo>& microphones)
    : StreamIn(std::move(context), microphones), StreamStub(&mContextInstance, sinkMetadata) {}

StreamOutStub::StreamOutStub(StreamContext&& context, const SourceMetadata& sourceMetadata,
                             const std::optional<AudioOffloadInfo>& offloadInfo)
    : StreamOut(std::move(context), offloadInfo), StreamStub(&mContextInstance, sourceMetadata) {}

}  // namespace aidl::android::hardware::audio::core
