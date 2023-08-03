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

#include <mutex>

#include <audio_utils/clock.h>

#include <media/nbaio/MonoPipe.h>
#include <media/nbaio/MonoPipeReader.h>

#include <aidl/android/media/audio/common/AudioChannelLayout.h>

#include "core-impl/Stream.h"

using aidl::android::media::audio::common::AudioChannelLayout;
using aidl::android::media::audio::common::AudioFormatDescription;
using aidl::android::media::audio::common::AudioFormatType;
using aidl::android::media::audio::common::PcmType;
using ::android::MonoPipe;
using ::android::MonoPipeReader;
using ::android::sp;

namespace aidl::android::hardware::audio::core::r_submix {

static constexpr int kDefaultSampleRateHz = 48000;
// Size at default sample rate
// NOTE: This value will be rounded up to the nearest power of 2 by MonoPipe().
static constexpr int kDefaultPipeSizeInFrames = (1024 * 4);

// Configuration of the audio stream.
struct AudioConfig {
    int sampleRate = kDefaultSampleRateHz;
    AudioFormatDescription format =
            AudioFormatDescription{.type = AudioFormatType::PCM, .pcm = PcmType::INT_16_BIT};
    AudioChannelLayout channelLayout =
            AudioChannelLayout::make<AudioChannelLayout::Tag::layoutMask>(
                    AudioChannelLayout::LAYOUT_STEREO);
    size_t frameSize;
    size_t frameCount;
};

class SubmixRoute {
  public:
    AudioConfig mPipeConfig;

    bool isStreamInOpen() {
        std::lock_guard guard(mLock);
        return mStreamInOpen;
    }
    bool getStreamInStandby() {
        std::lock_guard guard(mLock);
        return mStreamInStandby;
    }
    bool isStreamOutOpen() {
        std::lock_guard guard(mLock);
        return mStreamOutOpen;
    }
    bool getStreamOutStandby() {
        std::lock_guard guard(mLock);
        return mStreamOutStandby;
    }
    long getReadCounterFrames() {
        std::lock_guard guard(mLock);
        return mReadCounterFrames;
    }
    int getReadErrorCount() {
        std::lock_guard guard(mLock);
        return mReadErrorCount;
    }
    std::chrono::time_point<std::chrono::steady_clock> getRecordStartTime() {
        std::lock_guard guard(mLock);
        return mRecordStartTime;
    }
    sp<MonoPipe> getSink() {
        std::lock_guard guard(mLock);
        return mSink;
    }
    sp<MonoPipeReader> getSource() {
        std::lock_guard guard(mLock);
        return mSource;
    }

    bool isStreamConfigValid(bool isInput, const AudioConfig& streamConfig);
    void closeStream(bool isInput);
    ::android::status_t createPipe(const AudioConfig& streamConfig);
    void exitStandby(bool isInput);
    bool hasAtleastOneStreamOpen();
    int notifyReadError();
    void openStream(bool isInput);
    void releasePipe();
    ::android::status_t resetPipe();
    bool shouldBlockWrite();
    void standby(bool isInput);
    long updateReadCounterFrames(size_t frameCount);

  private:
    bool isStreamConfigCompatible(const AudioConfig& streamConfig);

    std::mutex mLock;

    bool mStreamInOpen GUARDED_BY(mLock) = false;
    int mInputRefCount GUARDED_BY(mLock) = 0;
    bool mStreamInStandby GUARDED_BY(mLock) = true;
    bool mStreamOutStandbyTransition GUARDED_BY(mLock) = false;
    bool mStreamOutOpen GUARDED_BY(mLock) = false;
    bool mStreamOutStandby GUARDED_BY(mLock) = true;
    // how many frames have been requested to be read since standby
    long mReadCounterFrames GUARDED_BY(mLock) = 0;
    int mReadErrorCount GUARDED_BY(mLock) = 0;
    // wall clock when recording starts
    std::chrono::time_point<std::chrono::steady_clock> mRecordStartTime GUARDED_BY(mLock);

    // Pipe variables: they handle the ring buffer that "pipes" audio:
    //  - from the submix virtual audio output == what needs to be played
    //    remotely, seen as an output for the client
    //  - to the virtual audio source == what is captured by the component
    //    which "records" the submix / virtual audio source, and handles it as needed.
    // A usecase example is one where the component capturing the audio is then sending it over
    // Wifi for presentation on a remote Wifi Display device (e.g. a dongle attached to a TV, or a
    // TV with Wifi Display capabilities), or to a wireless audio player.
    sp<MonoPipe> mSink GUARDED_BY(mLock);
    sp<MonoPipeReader> mSource GUARDED_BY(mLock);
};

}  // namespace aidl::android::hardware::audio::core::r_submix
