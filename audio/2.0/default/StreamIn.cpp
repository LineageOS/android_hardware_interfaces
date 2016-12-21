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

#define LOG_TAG "StreamInHAL"

#include <hardware/audio.h>
#include <android/log.h>

#include "StreamIn.h"

namespace android {
namespace hardware {
namespace audio {
namespace V2_0 {
namespace implementation {

StreamIn::StreamIn(audio_hw_device_t* device, audio_stream_in_t* stream)
        : mDevice(device), mStream(stream),
          mStreamCommon(new Stream(&stream->common)),
          mStreamMmap(new StreamMmap<audio_stream_in_t>(stream)) {
}

StreamIn::~StreamIn() {
    mDevice->close_input_stream(mDevice, mStream);
    mStream = nullptr;
    mDevice = nullptr;
}

// Methods from ::android::hardware::audio::V2_0::IStream follow.
Return<uint64_t> StreamIn::getFrameSize()  {
    return audio_stream_in_frame_size(mStream);
}

Return<uint64_t> StreamIn::getFrameCount()  {
    return mStreamCommon->getFrameCount();
}

Return<uint64_t> StreamIn::getBufferSize()  {
    return mStreamCommon->getBufferSize();
}

Return<uint32_t> StreamIn::getSampleRate()  {
    return mStreamCommon->getSampleRate();
}

Return<void> StreamIn::getSupportedSampleRates(getSupportedSampleRates_cb _hidl_cb)  {
    return mStreamCommon->getSupportedSampleRates(_hidl_cb);
}

Return<Result> StreamIn::setSampleRate(uint32_t sampleRateHz)  {
    return mStreamCommon->setSampleRate(sampleRateHz);
}

Return<AudioChannelMask> StreamIn::getChannelMask()  {
    return mStreamCommon->getChannelMask();
}

Return<void> StreamIn::getSupportedChannelMasks(getSupportedChannelMasks_cb _hidl_cb)  {
    return mStreamCommon->getSupportedChannelMasks(_hidl_cb);
}

Return<Result> StreamIn::setChannelMask(AudioChannelMask mask)  {
    return mStreamCommon->setChannelMask(mask);
}

Return<AudioFormat> StreamIn::getFormat()  {
    return mStreamCommon->getFormat();
}

Return<void> StreamIn::getSupportedFormats(getSupportedFormats_cb _hidl_cb)  {
    return mStreamCommon->getSupportedFormats(_hidl_cb);
}

Return<Result> StreamIn::setFormat(AudioFormat format)  {
    return mStreamCommon->setFormat(format);
}

Return<void> StreamIn::getAudioProperties(getAudioProperties_cb _hidl_cb)  {
    return mStreamCommon->getAudioProperties(_hidl_cb);
}

Return<Result> StreamIn::addEffect(uint64_t effectId)  {
    return mStreamCommon->addEffect(effectId);
}

Return<Result> StreamIn::removeEffect(uint64_t effectId)  {
    return mStreamCommon->removeEffect(effectId);
}

Return<Result> StreamIn::standby()  {
    return mStreamCommon->standby();
}

Return<AudioDevice> StreamIn::getDevice()  {
    return mStreamCommon->getDevice();
}

Return<Result> StreamIn::setDevice(const DeviceAddress& address)  {
    return mStreamCommon->setDevice(address);
}

Return<Result> StreamIn::setConnectedState(const DeviceAddress& address, bool connected)  {
    return mStreamCommon->setConnectedState(address, connected);
}

Return<Result> StreamIn::setHwAvSync(uint32_t hwAvSync)  {
    return mStreamCommon->setHwAvSync(hwAvSync);
}

Return<void> StreamIn::getParameters(const hidl_vec<hidl_string>& keys, getParameters_cb _hidl_cb) {
    return mStreamCommon->getParameters(keys, _hidl_cb);
}

Return<Result> StreamIn::setParameters(const hidl_vec<ParameterValue>& parameters)  {
    return mStreamCommon->setParameters(parameters);
}

Return<void> StreamIn::debugDump(const hidl_handle& fd)  {
    return mStreamCommon->debugDump(fd);
}

Return<Result> StreamIn::start() {
    return mStreamMmap->start();
}

Return<Result> StreamIn::stop() {
    return mStreamMmap->stop();
}

Return<void> StreamIn::createMmapBuffer(int32_t minSizeFrames, createMmapBuffer_cb _hidl_cb) {
    return mStreamMmap->createMmapBuffer(
            minSizeFrames, audio_stream_in_frame_size(mStream), _hidl_cb);
}

Return<void> StreamIn::getMmapPosition(getMmapPosition_cb _hidl_cb) {
    return mStreamMmap->getMmapPosition(_hidl_cb);
}

// Methods from ::android::hardware::audio::V2_0::IStreamIn follow.
Return<void> StreamIn::getAudioSource(getAudioSource_cb _hidl_cb)  {
    int halSource;
    Result retval = mStreamCommon->getParam(AudioParameter::keyInputSource, &halSource);
    AudioSource source(AudioSource::DEFAULT);
    if (retval == Result::OK) {
        source = AudioSource(halSource);
    }
    _hidl_cb(retval, source);
    return Void();
}

Return<Result> StreamIn::setGain(float gain)  {
    return Stream::analyzeStatus("set_gain", mStream->set_gain(mStream, gain));
}

Return<void> StreamIn::read(uint64_t size, read_cb _hidl_cb)  {
    // TODO(mnaganov): Replace with FMQ version.
    hidl_vec<uint8_t> data;
    data.resize(size);
    Result retval(Result::OK);
    ssize_t readResult = mStream->read(mStream, &data[0], data.size());
    if (readResult >= 0 && static_cast<size_t>(readResult) != data.size()) {
        data.resize(readResult);
    } else if (readResult < 0) {
        data.resize(0);
        retval = Stream::analyzeStatus("read", readResult);
    }
    _hidl_cb(retval, data);
    return Void();
}

Return<uint32_t> StreamIn::getInputFramesLost()  {
    return mStream->get_input_frames_lost(mStream);
}

Return<void> StreamIn::getCapturePosition(getCapturePosition_cb _hidl_cb)  {
    Result retval(Result::NOT_SUPPORTED);
    uint64_t frames = 0, time = 0;
    if (mStream->get_capture_position != NULL) {
        int64_t halFrames, halTime;
        retval = Stream::analyzeStatus(
                "get_capture_position",
                mStream->get_capture_position(mStream, &halFrames, &halTime));
        if (retval == Result::OK) {
            frames = halFrames;
            time = halTime;
        }
    }
    _hidl_cb(retval, frames, time);
    return Void();
}

} // namespace implementation
}  // namespace V2_0
}  // namespace audio
}  // namespace hardware
}  // namespace android
