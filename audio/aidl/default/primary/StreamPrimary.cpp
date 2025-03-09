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

#define LOG_TAG "AHAL_StreamPrimary"

#include <cstdio>

#include <android-base/logging.h>
#include <android-base/parseint.h>
#include <android-base/properties.h>
#include <audio_utils/clock.h>
#include <error/Result.h>
#include <error/expected_utils.h>

#include "core-impl/StreamPrimary.h"

using aidl::android::hardware::audio::common::SinkMetadata;
using aidl::android::hardware::audio::common::SourceMetadata;
using aidl::android::media::audio::common::AudioDevice;
using aidl::android::media::audio::common::AudioDeviceAddress;
using aidl::android::media::audio::common::AudioDeviceDescription;
using aidl::android::media::audio::common::AudioDeviceType;
using aidl::android::media::audio::common::AudioOffloadInfo;
using aidl::android::media::audio::common::MicrophoneInfo;
using android::base::GetBoolProperty;

namespace aidl::android::hardware::audio::core {

StreamPrimary::StreamPrimary(StreamContext* context, const Metadata& metadata)
    : StreamAlsa(context, metadata, 3 /*readWriteRetries*/),
      mIsAsynchronous(!!getContext().getAsyncCallback()),
      mStubDriver(getContext()) {
    context->startStreamDataProcessor();
}

::android::status_t StreamPrimary::init() {
    RETURN_STATUS_IF_ERROR(mStubDriver.init());
    return StreamAlsa::init();
}

::android::status_t StreamPrimary::drain(StreamDescriptor::DrainMode mode) {
    return isStubStreamOnWorker() ? mStubDriver.drain(mode) : StreamAlsa::drain(mode);
}

::android::status_t StreamPrimary::flush() {
    RETURN_STATUS_IF_ERROR(isStubStreamOnWorker() ? mStubDriver.flush() : StreamAlsa::flush());
    // TODO(b/372951987): consider if this needs to be done from 'StreamInWorkerLogic::cycle'.
    return mIsInput ? standby() : ::android::OK;
}

::android::status_t StreamPrimary::pause() {
    return isStubStreamOnWorker() ? mStubDriver.pause() : StreamAlsa::pause();
}

::android::status_t StreamPrimary::standby() {
    return isStubStreamOnWorker() ? mStubDriver.standby() : StreamAlsa::standby();
}

::android::status_t StreamPrimary::start() {
    bool isStub = true, shutdownAlsaStream = false;
    {
        std::lock_guard l(mLock);
        isStub = mAlsaDeviceId == kStubDeviceId;
        shutdownAlsaStream =
                mCurrAlsaDeviceId != mAlsaDeviceId && mCurrAlsaDeviceId != kStubDeviceId;
        mCurrAlsaDeviceId = mAlsaDeviceId;
    }
    if (shutdownAlsaStream) {
        StreamAlsa::shutdown();  // Close currently opened ALSA devices.
    }
    if (isStub) {
        return mStubDriver.start();
    }
    RETURN_STATUS_IF_ERROR(StreamAlsa::start());
    mStartTimeNs = ::android::uptimeNanos();
    mFramesSinceStart = 0;
    mSkipNextTransfer = false;
    return ::android::OK;
}

::android::status_t StreamPrimary::transfer(void* buffer, size_t frameCount,
                                            size_t* actualFrameCount, int32_t* latencyMs) {
    if (isStubStreamOnWorker()) {
        return mStubDriver.transfer(buffer, frameCount, actualFrameCount, latencyMs);
    }
    // This is a workaround for the emulator implementation which has a host-side buffer
    // and is not being able to achieve real-time behavior similar to ADSPs (b/302587331).
    if (!mSkipNextTransfer) {
        RETURN_STATUS_IF_ERROR(
                StreamAlsa::transfer(buffer, frameCount, actualFrameCount, latencyMs));
    } else {
        LOG(DEBUG) << __func__ << ": skipping transfer (" << frameCount << " frames)";
        *actualFrameCount = frameCount;
        if (mIsInput) memset(buffer, 0, frameCount * mFrameSizeBytes);
        mSkipNextTransfer = false;
    }
    if (!mIsAsynchronous) {
        const long bufferDurationUs =
                (*actualFrameCount) * MICROS_PER_SECOND / mContext.getSampleRate();
        const auto totalDurationUs =
                (::android::uptimeNanos() - mStartTimeNs) / NANOS_PER_MICROSECOND;
        mFramesSinceStart += *actualFrameCount;
        const long totalOffsetUs =
                mFramesSinceStart * MICROS_PER_SECOND / mContext.getSampleRate() - totalDurationUs;
        LOG(VERBOSE) << __func__ << ": totalOffsetUs " << totalOffsetUs;
        if (totalOffsetUs > 0) {
            const long sleepTimeUs = std::min(totalOffsetUs, bufferDurationUs);
            LOG(VERBOSE) << __func__ << ": sleeping for " << sleepTimeUs << " us";
            usleep(sleepTimeUs);
        } else {
            mSkipNextTransfer = true;
        }
    } else {
        LOG(VERBOSE) << __func__ << ": asynchronous transfer";
    }
    return ::android::OK;
}

::android::status_t StreamPrimary::refinePosition(StreamDescriptor::Position*) {
    // Since not all data is actually sent to the HAL, use the position maintained by Stream class
    // which accounts for all frames passed from / to the client.
    return ::android::OK;
}

void StreamPrimary::shutdown() {
    StreamAlsa::shutdown();
    mStubDriver.shutdown();
}

ndk::ScopedAStatus StreamPrimary::setConnectedDevices(const ConnectedDevices& devices) {
    LOG(DEBUG) << __func__ << ": " << ::android::internal::ToString(devices);
    if (devices.size() > 1) {
        LOG(ERROR) << __func__ << ": primary stream can only be connected to one device, got: "
                   << devices.size();
        return ndk::ScopedAStatus::fromExceptionCode(EX_UNSUPPORTED_OPERATION);
    }
    {
        const bool useStubDriver = devices.empty() || useStubStream(mIsInput, devices[0]);
        std::lock_guard l(mLock);
        mAlsaDeviceId = useStubDriver ? kStubDeviceId : getCardAndDeviceId(devices);
    }
    if (!devices.empty()) {
        auto streamDataProcessor = getContext().getStreamDataProcessor().lock();
        if (streamDataProcessor != nullptr) {
            streamDataProcessor->setAudioDevice(devices[0]);
        }
    }
    return StreamAlsa::setConnectedDevices(devices);
}

std::vector<alsa::DeviceProfile> StreamPrimary::getDeviceProfiles() {
    return {alsa::DeviceProfile{.card = mCurrAlsaDeviceId.first,
                                .device = mCurrAlsaDeviceId.second,
                                .direction = mIsInput ? PCM_IN : PCM_OUT,
                                .isExternal = false}};
}

bool StreamPrimary::isStubStream() {
    std::lock_guard l(mLock);
    return mAlsaDeviceId == kStubDeviceId;
}

// static
StreamPrimary::AlsaDeviceId StreamPrimary::getCardAndDeviceId(
        const std::vector<AudioDevice>& devices) {
    if (devices.empty() || devices[0].address.getTag() != AudioDeviceAddress::id) {
        return kDefaultCardAndDeviceId;
    }
    std::string deviceAddress = devices[0].address.get<AudioDeviceAddress::id>();
    AlsaDeviceId cardAndDeviceId;
    if (const size_t suffixPos = deviceAddress.rfind("CARD_");
        suffixPos == std::string::npos ||
        sscanf(deviceAddress.c_str() + suffixPos, "CARD_%d_DEV_%d", &cardAndDeviceId.first,
               &cardAndDeviceId.second) != 2) {
        return kDefaultCardAndDeviceId;
    }
    LOG(DEBUG) << __func__ << ": parsed with card id " << cardAndDeviceId.first << ", device id "
               << cardAndDeviceId.second;
    return cardAndDeviceId;
}

// static
bool StreamPrimary::useStubStream(
        bool isInput, const ::aidl::android::media::audio::common::AudioDevice& device) {
    static const bool kSimulateInput =
            GetBoolProperty("ro.boot.audio.tinyalsa.simulate_input", false);
    static const bool kSimulateOutput =
            GetBoolProperty("ro.boot.audio.tinyalsa.ignore_output", false);
    if (isInput) {
        return kSimulateInput || device.type.type == AudioDeviceType::IN_TELEPHONY_RX ||
               device.type.type == AudioDeviceType::IN_FM_TUNER ||
               device.type.connection == AudioDeviceDescription::CONNECTION_BUS /*deprecated */;
    }
    return kSimulateOutput || device.type.type == AudioDeviceType::OUT_TELEPHONY_TX ||
           device.type.connection == AudioDeviceDescription::CONNECTION_BUS /*deprecated*/;
}

StreamInPrimary::StreamInPrimary(StreamContext&& context, const SinkMetadata& sinkMetadata,
                                 const std::vector<MicrophoneInfo>& microphones)
    : StreamIn(std::move(context), microphones),
      StreamPrimary(&mContextInstance, sinkMetadata),
      StreamInHwGainHelper(&mContextInstance) {}

ndk::ScopedAStatus StreamInPrimary::getHwGain(std::vector<float>* _aidl_return) {
    if (isStubStream()) {
        return ndk::ScopedAStatus::fromExceptionCode(EX_UNSUPPORTED_OPERATION);
    }
    if (mHwGains.empty()) {
        float gain;
        RETURN_STATUS_IF_ERROR(primary::PrimaryMixer::getInstance().getMicGain(&gain));
        _aidl_return->resize(mChannelCount, gain);
        RETURN_STATUS_IF_ERROR(setHwGainImpl(*_aidl_return));
    }
    return getHwGainImpl(_aidl_return);
}

ndk::ScopedAStatus StreamInPrimary::setHwGain(const std::vector<float>& in_channelGains) {
    if (isStubStream()) {
        LOG(DEBUG) << __func__ << ": gains " << ::android::internal::ToString(in_channelGains);
        return ndk::ScopedAStatus::fromExceptionCode(EX_UNSUPPORTED_OPERATION);
    }
    auto currentGains = mHwGains;
    RETURN_STATUS_IF_ERROR(setHwGainImpl(in_channelGains));
    if (in_channelGains.size() < 1) {
        LOG(FATAL) << __func__ << ": unexpected gain vector size: " << in_channelGains.size();
    }
    if (auto status = primary::PrimaryMixer::getInstance().setMicGain(in_channelGains[0]);
        !status.isOk()) {
        mHwGains = currentGains;
        return status;
    }
    float gain;
    RETURN_STATUS_IF_ERROR(primary::PrimaryMixer::getInstance().getMicGain(&gain));
    // Due to rounding errors, round trip conversions between percents and indexed values may not
    // match.
    if (gain != in_channelGains[0]) {
        LOG(WARNING) << __func__ << ": unmatched gain: set: " << in_channelGains[0]
                     << ", from mixer: " << gain;
    }
    return ndk::ScopedAStatus::ok();
}

StreamOutPrimary::StreamOutPrimary(StreamContext&& context, const SourceMetadata& sourceMetadata,
                                   const std::optional<AudioOffloadInfo>& offloadInfo)
    : StreamOut(std::move(context), offloadInfo),
      StreamPrimary(&mContextInstance, sourceMetadata),
      StreamOutHwVolumeHelper(&mContextInstance) {}

ndk::ScopedAStatus StreamOutPrimary::getHwVolume(std::vector<float>* _aidl_return) {
    if (isStubStream()) {
        return ndk::ScopedAStatus::fromExceptionCode(EX_UNSUPPORTED_OPERATION);
    }
    if (mHwVolumes.empty()) {
        RETURN_STATUS_IF_ERROR(primary::PrimaryMixer::getInstance().getVolumes(_aidl_return));
        _aidl_return->resize(mChannelCount);
        RETURN_STATUS_IF_ERROR(setHwVolumeImpl(*_aidl_return));
    }
    return getHwVolumeImpl(_aidl_return);
}

ndk::ScopedAStatus StreamOutPrimary::setHwVolume(const std::vector<float>& in_channelVolumes) {
    if (isStubStream()) {
        LOG(DEBUG) << __func__ << ": volumes " << ::android::internal::ToString(in_channelVolumes);
        return ndk::ScopedAStatus::fromExceptionCode(EX_UNSUPPORTED_OPERATION);
    }
    auto currentVolumes = mHwVolumes;
    RETURN_STATUS_IF_ERROR(setHwVolumeImpl(in_channelVolumes));
    if (auto status = primary::PrimaryMixer::getInstance().setVolumes(in_channelVolumes);
        !status.isOk()) {
        mHwVolumes = currentVolumes;
        return status;
    }
    std::vector<float> volumes;
    RETURN_STATUS_IF_ERROR(primary::PrimaryMixer::getInstance().getVolumes(&volumes));
    // Due to rounding errors, round trip conversions between percents and indexed values may not
    // match.
    if (volumes != in_channelVolumes) {
        LOG(WARNING) << __func__ << ": unmatched volumes: set: "
                     << ::android::internal::ToString(in_channelVolumes)
                     << ", from mixer: " << ::android::internal::ToString(volumes);
    }
    return ndk::ScopedAStatus::ok();
}

}  // namespace aidl::android::hardware::audio::core
