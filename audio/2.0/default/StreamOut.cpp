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
//#define LOG_NDEBUG 0

#include <android/log.h>
#include <hardware/audio.h>
#include <mediautils/SchedulingPolicyService.h>

#include "StreamOut.h"

namespace android {
namespace hardware {
namespace audio {
namespace V2_0 {
namespace implementation {

namespace {

class WriteThread : public Thread {
  public:
    // WriteThread's lifespan never exceeds StreamOut's lifespan.
    WriteThread(std::atomic<bool>* stop,
            audio_stream_out_t* stream,
            StreamOut::DataMQ* dataMQ,
            StreamOut::StatusMQ* statusMQ,
            EventFlag* efGroup,
            ThreadPriority threadPriority)
            : Thread(false /*canCallJava*/),
              mStop(stop),
              mStream(stream),
              mDataMQ(dataMQ),
              mStatusMQ(statusMQ),
              mEfGroup(efGroup),
              mThreadPriority(threadPriority),
              mBuffer(new uint8_t[dataMQ->getQuantumCount()]) {
    }
    virtual ~WriteThread() {}

    status_t readyToRun() override;

  private:
    std::atomic<bool>* mStop;
    audio_stream_out_t* mStream;
    StreamOut::DataMQ* mDataMQ;
    StreamOut::StatusMQ* mStatusMQ;
    EventFlag* mEfGroup;
    ThreadPriority mThreadPriority;
    std::unique_ptr<uint8_t[]> mBuffer;

    bool threadLoop() override;
};

status_t WriteThread::readyToRun() {
    if (mThreadPriority != ThreadPriority::NORMAL) {
        int err = requestPriority(
                getpid(), getTid(), static_cast<int>(mThreadPriority), true /*asynchronous*/);
        ALOGW_IF(err, "failed to set priority %d for pid %d tid %d; error %d",
                static_cast<int>(mThreadPriority), getpid(), getTid(), err);
    }
    return OK;
}

bool WriteThread::threadLoop() {
    // This implementation doesn't return control back to the Thread until it decides to stop,
    // as the Thread uses mutexes, and this can lead to priority inversion.
    while(!std::atomic_load_explicit(mStop, std::memory_order_acquire)) {
        // TODO: Remove manual event flag handling once blocking MQ is implemented. b/33815422
        uint32_t efState = 0;
        mEfGroup->wait(
                static_cast<uint32_t>(MessageQueueFlagBits::NOT_EMPTY), &efState, NS_PER_SEC);
        if (!(efState & static_cast<uint32_t>(MessageQueueFlagBits::NOT_EMPTY))) {
            continue;  // Nothing to do.
        }

        const size_t availToRead = mDataMQ->availableToRead();
        Result retval = Result::OK;
        uint64_t written = 0;
        if (mDataMQ->read(&mBuffer[0], availToRead)) {
            ssize_t writeResult = mStream->write(mStream, &mBuffer[0], availToRead);
            if (writeResult >= 0) {
                written = writeResult;
            } else {
                retval = Stream::analyzeStatus("write", writeResult);
            }
        }
        uint64_t frames = 0;
        struct timespec halTimeStamp = { 0, 0 };
        if (retval == Result::OK && mStream->get_presentation_position != NULL) {
            mStream->get_presentation_position(mStream, &frames, &halTimeStamp);
        }
        IStreamOut::WriteStatus status = { retval, written, frames,
                                           { static_cast<uint64_t>(halTimeStamp.tv_sec),
                                             static_cast<uint64_t>(halTimeStamp.tv_nsec) } };
        if (!mStatusMQ->write(&status)) {
            ALOGW("status message queue write failed");
        }
        mEfGroup->wake(static_cast<uint32_t>(MessageQueueFlagBits::NOT_FULL));
    }

    return false;
}

}  // namespace

StreamOut::StreamOut(audio_hw_device_t* device, audio_stream_out_t* stream)
        : mIsClosed(false), mDevice(device), mStream(stream),
          mStreamCommon(new Stream(&stream->common)),
          mStreamMmap(new StreamMmap<audio_stream_out_t>(stream)),
          mEfGroup(nullptr), mStopWriteThread(false) {
}

StreamOut::~StreamOut() {
    close();
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

Return<Result> StreamOut::close()  {
    if (mIsClosed) return Result::INVALID_STATE;
    mIsClosed = true;
    if (mWriteThread.get()) {
        mStopWriteThread.store(true, std::memory_order_release);
        status_t status = mWriteThread->requestExitAndWait();
        ALOGE_IF(status, "write thread exit error: %s", strerror(-status));
    }
    if (mEfGroup) {
        status_t status = EventFlag::deleteEventFlag(&mEfGroup);
        ALOGE_IF(status, "write MQ event flag deletion error: %s", strerror(-status));
    }
    mCallback.clear();
    mDevice->close_output_stream(mDevice, mStream);
    return Result::OK;
}

// Methods from ::android::hardware::audio::V2_0::IStreamOut follow.
Return<uint32_t> StreamOut::getLatency()  {
    return mStream->get_latency(mStream);
}

Return<Result> StreamOut::setVolume(float left, float right)  {
    Result retval(Result::NOT_SUPPORTED);
    if (mStream->set_volume != NULL) {
        retval = Stream::analyzeStatus(
                "set_volume", mStream->set_volume(mStream, left, right));
    }
    return retval;
}

Return<void> StreamOut::prepareForWriting(
        uint32_t frameSize, uint32_t framesCount, ThreadPriority threadPriority,
        prepareForWriting_cb _hidl_cb)  {
    status_t status;
    // Create message queues.
    if (mDataMQ) {
        ALOGE("the client attempts to call prepareForWriting twice");
        _hidl_cb(Result::INVALID_STATE,
                DataMQ::Descriptor(), StatusMQ::Descriptor());
        return Void();
    }
    std::unique_ptr<DataMQ> tempDataMQ(
            new DataMQ(frameSize * framesCount, true /* EventFlag */));
    std::unique_ptr<StatusMQ> tempStatusMQ(new StatusMQ(1));
    if (!tempDataMQ->isValid() || !tempStatusMQ->isValid()) {
        ALOGE_IF(!tempDataMQ->isValid(), "data MQ is invalid");
        ALOGE_IF(!tempStatusMQ->isValid(), "status MQ is invalid");
        _hidl_cb(Result::INVALID_ARGUMENTS,
                DataMQ::Descriptor(), StatusMQ::Descriptor());
        return Void();
    }
    // TODO: Remove event flag management once blocking MQ is implemented. b/33815422
    status = EventFlag::createEventFlag(tempDataMQ->getEventFlagWord(), &mEfGroup);
    if (status != OK || !mEfGroup) {
        ALOGE("failed creating event flag for data MQ: %s", strerror(-status));
        _hidl_cb(Result::INVALID_ARGUMENTS,
                DataMQ::Descriptor(), StatusMQ::Descriptor());
        return Void();
    }

    // Create and launch the thread.
    mWriteThread = new WriteThread(
            &mStopWriteThread,
            mStream,
            tempDataMQ.get(),
            tempStatusMQ.get(),
            mEfGroup,
            threadPriority);
    status = mWriteThread->run("writer", PRIORITY_URGENT_AUDIO);
    if (status != OK) {
        ALOGW("failed to start writer thread: %s", strerror(-status));
        _hidl_cb(Result::INVALID_ARGUMENTS,
                DataMQ::Descriptor(), StatusMQ::Descriptor());
        return Void();
    }

    mDataMQ = std::move(tempDataMQ);
    mStatusMQ = std::move(tempStatusMQ);
    _hidl_cb(Result::OK, *mDataMQ->getDesc(), *mStatusMQ->getDesc());
    return Void();
}

Return<void> StreamOut::getRenderPosition(getRenderPosition_cb _hidl_cb)  {
    uint32_t halDspFrames;
    Result retval = Stream::analyzeStatus(
            "get_render_position", mStream->get_render_position(mStream, &halDspFrames));
    _hidl_cb(retval, halDspFrames);
    return Void();
}

Return<void> StreamOut::getNextWriteTimestamp(getNextWriteTimestamp_cb _hidl_cb)  {
    Result retval(Result::NOT_SUPPORTED);
    int64_t timestampUs = 0;
    if (mStream->get_next_write_timestamp != NULL) {
        retval = Stream::analyzeStatus(
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
    return Stream::analyzeStatus("set_callback", result);
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
            Stream::analyzeStatus("pause", mStream->pause(mStream)) :
            Result::NOT_SUPPORTED;
}

Return<Result> StreamOut::resume()  {
    return mStream->resume != NULL ?
            Stream::analyzeStatus("resume", mStream->resume(mStream)) :
            Result::NOT_SUPPORTED;
}

Return<bool> StreamOut::supportsDrain()  {
    return mStream->drain != NULL;
}

Return<Result> StreamOut::drain(AudioDrain type)  {
    return mStream->drain != NULL ?
            Stream::analyzeStatus(
                    "drain", mStream->drain(mStream, static_cast<audio_drain_type_t>(type))) :
            Result::NOT_SUPPORTED;
}

Return<Result> StreamOut::flush()  {
    return mStream->flush != NULL ?
            Stream::analyzeStatus("flush", mStream->flush(mStream)) :
            Result::NOT_SUPPORTED;
}

Return<void> StreamOut::getPresentationPosition(getPresentationPosition_cb _hidl_cb)  {
    Result retval(Result::NOT_SUPPORTED);
    uint64_t frames = 0;
    TimeSpec timeStamp = { 0, 0 };
    if (mStream->get_presentation_position != NULL) {
        struct timespec halTimeStamp;
        retval = Stream::analyzeStatus(
                "get_presentation_position",
                mStream->get_presentation_position(mStream, &frames, &halTimeStamp),
                // Don't logspam on EINVAL--it's normal for get_presentation_position
                // to return it sometimes.
                EINVAL);
        if (retval == Result::OK) {
            timeStamp.tvSec = halTimeStamp.tv_sec;
            timeStamp.tvNSec = halTimeStamp.tv_nsec;
        }
    }
    _hidl_cb(retval, frames, timeStamp);
    return Void();
}

Return<Result> StreamOut::start() {
    return mStreamMmap->start();
}

Return<Result> StreamOut::stop() {
    return mStreamMmap->stop();
}

Return<void> StreamOut::createMmapBuffer(int32_t minSizeFrames, createMmapBuffer_cb _hidl_cb) {
    return mStreamMmap->createMmapBuffer(
            minSizeFrames, audio_stream_out_frame_size(mStream), _hidl_cb);
}

Return<void> StreamOut::getMmapPosition(getMmapPosition_cb _hidl_cb) {
    return mStreamMmap->getMmapPosition(_hidl_cb);
}

}  // namespace implementation
}  // namespace V2_0
}  // namespace audio
}  // namespace hardware
}  // namespace android
