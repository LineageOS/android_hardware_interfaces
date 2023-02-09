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

#include <memory>
#define LOG_TAG "AHAL_EffectThread"
#include <android-base/logging.h>
#include <pthread.h>
#include <sys/resource.h>

#include "effect-impl/EffectThread.h"

namespace aidl::android::hardware::audio::effect {

EffectThread::EffectThread() {
    LOG(DEBUG) << __func__;
}

EffectThread::~EffectThread() {
    destroyThread();
    LOG(DEBUG) << __func__ << " done";
};

RetCode EffectThread::createThread(std::shared_ptr<EffectContext> context, const std::string& name,
                                   const int priority) {
    if (mThread.joinable()) {
        LOG(WARNING) << __func__ << " thread already created, no-op";
        return RetCode::SUCCESS;
    }
    mName = name;
    mPriority = priority;
    {
        std::lock_guard lg(mThreadMutex);
        mThreadContext = std::move(context);
    }
    mThread = std::thread(&EffectThread::threadLoop, this);
    LOG(DEBUG) << __func__ << " " << name << " priority " << mPriority << " done";
    return RetCode::SUCCESS;
}

RetCode EffectThread::destroyThread() {
    {
        std::lock_guard lg(mThreadMutex);
        mStop = mExit = true;
    }
    mCv.notify_one();

    if (mThread.joinable()) {
        mThread.join();
    }

    {
        std::lock_guard lg(mThreadMutex);
        mThreadContext.reset();
    }
    LOG(DEBUG) << __func__ << " done";
    return RetCode::SUCCESS;
}

RetCode EffectThread::startThread() {
    return handleStartStop(false /* stop */);
}

RetCode EffectThread::stopThread() {
    return handleStartStop(true /* stop */);
}

RetCode EffectThread::handleStartStop(bool stop) {
    if (!mThread.joinable()) {
        LOG(ERROR) << __func__ << " thread already destroyed";
        return RetCode::ERROR_THREAD;
    }

    {
        std::lock_guard lg(mThreadMutex);
        if (stop == mStop) {
            LOG(WARNING) << __func__ << " already " << (stop ? "stop" : "start");
            return RetCode::SUCCESS;
        }
        mStop = stop;
    }

    mCv.notify_one();
    LOG(DEBUG) << (stop ? "stop done" : "start done");
    return RetCode::SUCCESS;
}

void EffectThread::threadLoop() {
    pthread_setname_np(pthread_self(), mName.substr(0, kMaxTaskNameLen - 1).c_str());
    setpriority(PRIO_PROCESS, 0, mPriority);
    while (true) {
        std::unique_lock l(mThreadMutex);
        ::android::base::ScopedLockAssertion lock_assertion(mThreadMutex);
        mCv.wait(l, [&]() REQUIRES(mThreadMutex) { return mExit || !mStop; });
        if (mExit) {
            LOG(WARNING) << __func__ << " EXIT!";
            return;
        }
        process_l();
    }
}

void EffectThread::process_l() {
    RETURN_VALUE_IF(!mThreadContext, void(), "nullContext");
    std::shared_ptr<EffectContext::StatusMQ> statusMQ = mThreadContext->getStatusFmq();
    std::shared_ptr<EffectContext::DataMQ> inputMQ = mThreadContext->getInputDataFmq();
    std::shared_ptr<EffectContext::DataMQ> outputMQ = mThreadContext->getOutputDataFmq();
    auto buffer = mThreadContext->getWorkBuffer();

    // Only this worker will read from input data MQ and write to output data MQ.
    auto readSamples = inputMQ->availableToRead(), writeSamples = outputMQ->availableToWrite();
    if (readSamples && writeSamples) {
        auto processSamples = std::min(readSamples, writeSamples);
        LOG(DEBUG) << __func__ << " available to read " << readSamples << " available to write "
                   << writeSamples << " process " << processSamples;

        inputMQ->read(buffer, processSamples);

        IEffect::Status status = effectProcessImpl(buffer, buffer, processSamples);
        outputMQ->write(buffer, status.fmqProduced);
        statusMQ->writeBlocking(&status, 1);
        LOG(DEBUG) << __func__ << " done processing, effect consumed " << status.fmqConsumed
                   << " produced " << status.fmqProduced;
    } else {
        // TODO: maybe add some sleep here to avoid busy waiting
    }
}

}  // namespace aidl::android::hardware::audio::effect
