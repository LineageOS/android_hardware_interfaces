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

#include <cstddef>
#include <memory>

#define LOG_TAG "AHAL_EffectThread"
#include <android-base/logging.h>
#include <pthread.h>
#include <sys/resource.h>

#include "effect-impl/EffectThread.h"
#include "effect-impl/EffectTypes.h"

using ::android::hardware::EventFlag;

namespace aidl::android::hardware::audio::effect {

EffectThread::EffectThread() {
    LOG(DEBUG) << __func__;
}

EffectThread::~EffectThread() {
    destroyThread();
    LOG(DEBUG) << __func__ << " done";
}

RetCode EffectThread::createThread(std::shared_ptr<EffectContext> context, const std::string& name,
                                   int priority) {
    if (mThread.joinable()) {
        LOG(WARNING) << mName << __func__ << " thread already created, no-op";
        return RetCode::SUCCESS;
    }
    mName = name;
    mPriority = priority;
    {
        std::lock_guard lg(mThreadMutex);
        mStop = true;
        mExit = false;
        mThreadContext = std::move(context);
        auto statusMQ = mThreadContext->getStatusFmq();
        EventFlag* efGroup = nullptr;
        ::android::status_t status =
                EventFlag::createEventFlag(statusMQ->getEventFlagWord(), &efGroup);
        if (status != ::android::OK || !efGroup) {
            LOG(ERROR) << mName << __func__ << " create EventFlagGroup failed " << status
                       << " efGroup " << efGroup;
            return RetCode::ERROR_THREAD;
        }
        mEfGroup.reset(efGroup);
        // kickoff and wait for commands (CommandId::START/STOP) or IEffect.close from client
        mEfGroup->wake(kEventFlagNotEmpty);
    }

    mThread = std::thread(&EffectThread::threadLoop, this);
    LOG(DEBUG) << mName << __func__ << " priority " << mPriority << " done";
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
    LOG(DEBUG) << mName << __func__;
    return RetCode::SUCCESS;
}

RetCode EffectThread::startThread() {
    {
        std::lock_guard lg(mThreadMutex);
        mStop = false;
        mCv.notify_one();
    }

    mEfGroup->wake(kEventFlagNotEmpty);
    LOG(DEBUG) << mName << __func__;
    return RetCode::SUCCESS;
}

RetCode EffectThread::stopThread() {
    {
        std::lock_guard lg(mThreadMutex);
        mStop = true;
        mCv.notify_one();
    }

    mEfGroup->wake(kEventFlagNotEmpty);
    LOG(DEBUG) << mName << __func__;
    return RetCode::SUCCESS;
}

void EffectThread::threadLoop() {
    pthread_setname_np(pthread_self(), mName.substr(0, kMaxTaskNameLen - 1).c_str());
    setpriority(PRIO_PROCESS, 0, mPriority);
    while (true) {
        /**
         * wait for the EventFlag without lock, it's ok because the mEfGroup pointer will not change
         * in the life cycle of workerThread (threadLoop).
         */
        uint32_t efState = 0;
        mEfGroup->wait(kEventFlagNotEmpty, &efState);

        {
            std::unique_lock l(mThreadMutex);
            ::android::base::ScopedLockAssertion lock_assertion(mThreadMutex);
            mCv.wait(l, [&]() REQUIRES(mThreadMutex) { return mExit || !mStop; });
            if (mExit) {
                LOG(INFO) << __func__ << " EXIT!";
                return;
            }
            process_l();
        }
    }
}

void EffectThread::process_l() {
    RETURN_VALUE_IF(!mThreadContext, void(), "nullContext");

    auto statusMQ = mThreadContext->getStatusFmq();
    auto inputMQ = mThreadContext->getInputDataFmq();
    auto outputMQ = mThreadContext->getOutputDataFmq();
    auto buffer = mThreadContext->getWorkBuffer();

    auto processSamples = inputMQ->availableToRead();
    if (processSamples) {
        inputMQ->read(buffer, processSamples);
        IEffect::Status status = effectProcessImpl(buffer, buffer, processSamples);
        outputMQ->write(buffer, status.fmqProduced);
        statusMQ->writeBlocking(&status, 1);
        LOG(VERBOSE) << mName << __func__ << ": done processing, effect consumed "
                     << status.fmqConsumed << " produced " << status.fmqProduced;
    }
}

}  // namespace aidl::android::hardware::audio::effect
