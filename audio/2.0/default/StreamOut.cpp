/*
 * Copyright (C) 2016 The Android Open Source Project
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

#define LOG_TAG "StreamOutHAL"

#include <hardware/audio.h>
#include <android/log.h>

#include "StreamOut.h"

namespace android {
namespace hardware {
namespace audio {
namespace V2_0 {
namespace implementation {

StreamOut::StreamOut(audio_hw_device_t* device, audio_stream_out_t* stream)
        : mDevice(device), mStream(stream), mStreamCommon(new Stream(&stream->common)) {
}

StreamOut::~StreamOut() {
    mCallback.clear();
    mDevice->close_output_stream(mDevice, mStream);
    mStream = nullptr;
    mDevice = nullptr;
}

// Methods from ::android::hardware::audio::V2_0::IStream follow.
Return<uint64_t> StreamOut::getFrameSize()  {
    return audio_stream_out_frame_size(mStream);
}

Return<uint64_t> StreamOut::getFrameCount()  {
    return mStreamCommon->getFrameCount();
}

Return<uint64_t> StreamOut::getBufferSize()  {
    return mStreamCommon->getBufferSize();
}

Return<uint32_t> StreamOut::getSampleRate()  {
    return mStreamCommon->getSampleRate();
}

Return<void> StreamOut::getSupportedSampleRates(getSupportedSampleRates_cb _hidl_cb)  {
    return mStreamCommon->getSupportedSampleRates(_hidl_cb);
}

Return<Result> StreamOut::setSampleRate(uint32_t sampleRateHz)  {
    return mStreamCommon->setSampleRate(sampleRateHz);
}

Return<AudioChannelMask> StreamOut::getChannelMask()  {
    return mStreamCommon->getChannelMask();
}

Return<void> StreamOut::getSupportedChannelMasks(getSupportedChannelMasks_cb _hidl_cb)  {
    return mStreamCommon->getSupportedChannelMasks(_hidl_cb);
}

Return<Result> StreamOut::setChannelMask(AudioChannelMask mask)  {
    return mStreamCommon->setChannelMask(mask);
}

Return<AudioFormat> StreamOut::getFormat()  {
    return mStreamCommon->getFormat();
}

Return<void> StreamOut::getSupportedFormats(getSupportedFormats_cb _hidl_cb)  {
    return mStreamCommon->getSupportedFormats(_hidl_cb);
}

Return<Result> StreamOut::setFormat(AudioFormat format)  {
    return mStreamCommon->setFormat(format);
}

Return<void> StreamOut::getAudioProperties(getAudioProperties_cb _hidl_cb)  {
    return mStreamCommon->getAudioProperties(_hidl_cb);
}

Return<Result> StreamOut::addEffect(uint64_t effectId)  {
    return mStreamCommon->addEffect(effectId);
}

Return<Result> StreamOut::removeEffect(uint64_t effectId)  {
    return mStreamCommon->removeEffect(effectId);
}

Return<Result> StreamOut::standby()  {
    return mStreamCommon->standby();
}

Return<AudioDevice> StreamOut::getDevice()  {
    return mStreamCommon->getDevice();
}

Return<Result> StreamOut::setDevice(const DeviceAddress& address)  {
    return mStreamCommon->setDevice(address);
}

Return<Result> StreamOut::setConnectedState(const DeviceAddress& address, bool connected)  {
    return mStreamCommon->setConnectedState(address, connected);
}

Return<Result> StreamOut::setHwAvSync(uint32_t hwAvSync)  {
    return mStreamCommon->setHwAvSync(hwAvSync);
}

Return<void> StreamOut::getParameters(
        const hidl_vec<hidl_string>& keys, getParameters_cb _hidl_cb)  {
    return mStreamCommon->getParameters(keys, _hidl_cb);
}

Return<Result> StreamOut::setParameters(const hidl_vec<ParameterValue>& parameters)  {
    return mStreamCommon->setParameters(parameters);
}

Return<void> StreamOut::debugDump(const hidl_handle& fd)  {
    return mStreamCommon->debugDump(fd);
}


// Methods from ::android::hardware::audio::V2_0::IStreamOut follow.
Return<uint32_t> StreamOut::getLatency()  {
    return mStream->get_latency(mStream);
}

Return<Result> StreamOut::setVolume(float left, float right)  {
    Result retval(Result::NOT_SUPPORTED);
    if (mStream->set_volume != NULL) {
        retval = mStreamCommon->analyzeStatus(
                "set_volume", mStream->set_volume(mStream, left, right));
    }
    return retval;
}

Return<void> StreamOut::write(const hidl_vec<uint8_t>& data, write_cb _hidl_cb)  {
    // TODO(mnaganov): Replace with FMQ version.
    Result retval(Result::OK);
    uint64_t written = 0;
    ssize_t writeResult = mStream->write(mStream, &data[0], data.size());
    if (writeResult >= 0) {
        written = writeResult;
    } else {
        retval = mStreamCommon->analyzeStatus("write", writeResult);
        written = 0;
    }
    _hidl_cb(retval, written);
    return Void();
}

Return<void> StreamOut::getRenderPosition(getRenderPosition_cb _hidl_cb)  {
    uint32_t halDspFrames;
    Result retval = mStreamCommon->analyzeStatus(
            "get_render_position", mStream->get_render_position(mStream, &halDspFrames));
    _hidl_cb(retval, halDspFrames);
    return Void();
}

Return<void> StreamOut::getNextWriteTimestamp(getNextWriteTimestamp_cb _hidl_cb)  {
    Result retval(Result::NOT_SUPPORTED);
    int64_t timestampUs = 0;
    if (mStream->get_next_write_timestamp != NULL) {
        retval = mStreamCommon->analyzeStatus(
                "get_next_write_timestamp",
                mStream->get_next_write_timestamp(mStream, &timestampUs));
    }
    _hidl_cb(retval, timestampUs);
    return Void();
}

Return<Result> StreamOut::setCallback(const sp<IStreamOutCallback>& callback)  {
    if (mStream->set_callback == NULL) return Result::NOT_SUPPORTED;
    int result = mStream->set_callback(mStream, StreamOut::asyncCallback, this);
    if (result == 0) {
        mCallback = callback;
    }
    return mStreamCommon->analyzeStatus("set_callback", result);
}

Return<Result> StreamOut::clearCallback()  {
    if (mStream->set_callback == NULL) return Result::NOT_SUPPORTED;
    mCallback.clear();
    return Result::OK;
}

// static
int StreamOut::asyncCallback(stream_callback_event_t event, void*, void *cookie) {
    wp<StreamOut> weakSelf(reinterpret_cast<StreamOut*>(cookie));
    sp<StreamOut> self = weakSelf.promote();
    if (self == nullptr || self->mCallback == nullptr) return 0;
    ALOGV("asyncCallback() event %d", event);
    switch (event) {
        case STREAM_CBK_EVENT_WRITE_READY:
            self->mCallback->onWriteReady();
            break;
        case STREAM_CBK_EVENT_DRAIN_READY:
            self->mCallback->onDrainReady();
            break;
        case STREAM_CBK_EVENT_ERROR:
            self->mCallback->onError();
            break;
        default:
            ALOGW("asyncCallback() unknown event %d", event);
            break;
    }
    return 0;
}

Return<void> StreamOut::supportsPauseAndResume(supportsPauseAndResume_cb _hidl_cb)  {
    _hidl_cb(mStream->pause != NULL, mStream->resume != NULL);
    return Void();
}

Return<Result> StreamOut::pause()  {
    return mStream->pause != NULL ?
            mStreamCommon->analyzeStatus("pause", mStream->pause(mStream)) :
            Result::NOT_SUPPORTED;
}

Return<Result> StreamOut::resume()  {
    return mStream->resume != NULL ?
            mStreamCommon->analyzeStatus("resume", mStream->resume(mStream)) :
            Result::NOT_SUPPORTED;
}

Return<bool> StreamOut::supportsDrain()  {
    return mStream->drain != NULL;
}

Return<Result> StreamOut::drain(AudioDrain type)  {
    return mStream->drain != NULL ?
            mStreamCommon->analyzeStatus(
                    "drain", mStream->drain(mStream, static_cast<audio_drain_type_t>(type))) :
            Result::NOT_SUPPORTED;
}

Return<Result> StreamOut::flush()  {
    return mStream->flush != NULL ?
            mStreamCommon->analyzeStatus("flush", mStream->flush(mStream)) :
            Result::NOT_SUPPORTED;
}

Return<void> StreamOut::getPresentationPosition(getPresentationPosition_cb _hidl_cb)  {
    Result retval(Result::NOT_SUPPORTED);
    uint64_t frames = 0;
    TimeSpec timeStamp = { 0, 0 };
    if (mStream->get_presentation_position != NULL) {
        struct timespec halTimeStamp;
        retval = mStreamCommon->analyzeStatus(
                "get_presentation_position",
                mStream->get_presentation_position(mStream, &frames, &halTimeStamp));
        if (retval == Result::OK) {
            timeStamp.tvSec = halTimeStamp.tv_sec;
            timeStamp.tvNSec = halTimeStamp.tv_nsec;
        }
    }
    _hidl_cb(retval, frames, timeStamp);
    return Void();
}

}  // namespace implementation
}  // namespace V2_0
}  // namespace audio
}  // namespace hardware
}  // namespace android
