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

#pragma once

#include <map>
#include <optional>
#include <variant>

#include <aidl/android/hardware/audio/common/SinkMetadata.h>
#include <aidl/android/hardware/audio/common/SourceMetadata.h>
#include <aidl/android/hardware/audio/core/BnStreamIn.h>
#include <aidl/android/hardware/audio/core/BnStreamOut.h>
#include <aidl/android/media/audio/common/AudioOffloadInfo.h>

#include "core-impl/utils.h"

namespace aidl::android::hardware::audio::core {

class StreamIn : public BnStreamIn {
    ndk::ScopedAStatus close() override;
    ndk::ScopedAStatus updateMetadata(
            const ::aidl::android::hardware::audio::common::SinkMetadata& in_sinkMetadata) override;

  public:
    explicit StreamIn(const ::aidl::android::hardware::audio::common::SinkMetadata& sinkMetadata);
    bool isClosed() const { return mIsClosed; }

  private:
    ::aidl::android::hardware::audio::common::SinkMetadata mMetadata;
    bool mIsClosed = false;
};

class StreamOut : public BnStreamOut {
    ndk::ScopedAStatus close() override;
    ndk::ScopedAStatus updateMetadata(
            const ::aidl::android::hardware::audio::common::SourceMetadata& in_sourceMetadata)
            override;

  public:
    StreamOut(const ::aidl::android::hardware::audio::common::SourceMetadata& sourceMetadata,
              const std::optional<::aidl::android::media::audio::common::AudioOffloadInfo>&
                      offloadInfo);
    bool isClosed() const { return mIsClosed; }

  private:
    ::aidl::android::hardware::audio::common::SourceMetadata mMetadata;
    std::optional<::aidl::android::media::audio::common::AudioOffloadInfo> mOffloadInfo;
    bool mIsClosed = false;
};

class StreamWrapper {
  public:
    explicit StreamWrapper(std::shared_ptr<StreamIn> streamIn) : mStream(streamIn) {}
    explicit StreamWrapper(std::shared_ptr<StreamOut> streamOut) : mStream(streamOut) {}
    bool isStreamOpen() const {
        return std::visit(
                [](auto&& ws) -> bool {
                    auto s = ws.lock();
                    return s && !s->isClosed();
                },
                mStream);
    }

  private:
    std::variant<std::weak_ptr<StreamIn>, std::weak_ptr<StreamOut>> mStream;
};

class Streams {
  public:
    Streams() = default;
    Streams(const Streams&) = delete;
    Streams& operator=(const Streams&) = delete;
    size_t count(int32_t id) {
        // Streams do not remove themselves from the collection on close.
        erase_if(mStreams, [](const auto& pair) { return !pair.second.isStreamOpen(); });
        return mStreams.count(id);
    }
    void insert(int32_t portId, int32_t portConfigId, StreamWrapper sw) {
        mStreams.insert(std::pair{portConfigId, sw});
        mStreams.insert(std::pair{portId, sw});
    }

  private:
    // Maps port ids and port config ids to streams. Multimap because a port
    // (not port config) can have multiple streams opened on it.
    std::multimap<int32_t, StreamWrapper> mStreams;
};

}  // namespace aidl::android::hardware::audio::core
