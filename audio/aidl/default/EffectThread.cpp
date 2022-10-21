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

RetCode EffectThread::createThread(const std::string& name, const int priority) {
    if (mThread.joinable()) {
        LOG(WARNING) << __func__ << " thread already created, no-op";
        return RetCode::SUCCESS;
    }
    mName = name;
    mPriority = priority;
    mThread = std::thread(&EffectThread::threadLoop, this);
    LOG(DEBUG) << __func__ << " " << name << " priority " << mPriority << " done";
    return RetCode::SUCCESS;
}

RetCode EffectThread::destroyThread() {
    {
        std::lock_guard lg(mMutex);
        mStop = mExit = true;
    }
    mCv.notify_one();

    if (mThread.joinable()) {
        mThread.join();
    }
    LOG(DEBUG) << __func__ << " done";
    return RetCode::SUCCESS;
}

RetCode EffectThread::startThread() {
    if (!mThread.joinable()) {
        LOG(ERROR) << __func__ << " thread already destroyed";
        return RetCode::ERROR_THREAD;
    }

    {
        std::lock_guard lg(mMutex);
        if (!mStop) {
            LOG(WARNING) << __func__ << " already start";
            return RetCode::SUCCESS;
        }
        mStop = false;
    }

    mCv.notify_one();
    LOG(DEBUG) << __func__ << " done";
    return RetCode::SUCCESS;
}

RetCode EffectThread::stopThread() {
    if (!mThread.joinable()) {
        LOG(ERROR) << __func__ << " thread already destroyed";
        return RetCode::ERROR_THREAD;
    }

    {
        std::lock_guard lg(mMutex);
        if (mStop) {
            LOG(WARNING) << __func__ << " already stop";
            return RetCode::SUCCESS;
        }
        mStop = true;
    }
    LOG(DEBUG) << __func__ << " done";
    return RetCode::SUCCESS;
}

void EffectThread::threadLoop() {
    pthread_setname_np(pthread_self(), mName.substr(0, MAX_TASK_COMM_LEN - 1).c_str());
    setpriority(PRIO_PROCESS, 0, mPriority);
    while (true) {
        bool needExit = false;
        {
            std::unique_lock l(mMutex);
            mCv.wait(l, [&]() REQUIRES(mMutex) {
                needExit = mExit;
                return mExit || !mStop;
            });
        }
        if (needExit) {
            LOG(WARNING) << __func__ << " EXIT!";
            return;
        }
        // process without lock
        process();
    }
}

}  // namespace aidl::android::hardware::audio::effect
