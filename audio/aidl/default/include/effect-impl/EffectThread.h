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
#include <atomic>
#include <string>
#include <thread>

#include <android-base/thread_annotations.h>
#include <system/thread_defs.h>

#include "effect-impl/EffectContext.h"
#include "effect-impl/EffectTypes.h"

namespace aidl::android::hardware::audio::effect {

class EffectThread {
  public:
    // default priority is same as HIDL: ANDROID_PRIORITY_URGENT_AUDIO
    EffectThread();
    virtual ~EffectThread();

    // called by effect implementation.
    RetCode createThread(std::shared_ptr<EffectContext> context, const std::string& name,
                         const int priority = ANDROID_PRIORITY_URGENT_AUDIO);
    RetCode destroyThread();
    RetCode startThread();
    RetCode stopThread();

    // Will call process() in a loop if the thread is running.
    void threadLoop();

    /**
     * @brief effectProcessImpl is running in worker thread which created in EffectThread.
     *
     * Effect implementation should think about concurrency in the implementation if necessary.
     * Parameter setting usually implemented in context (derived from EffectContext), and some
     * parameter maybe used in the processing, then effect implementation should consider using a
     * mutex to protect these parameter.
     *
     * EffectThread will make sure effectProcessImpl only be called after startThread() successful
     * and before stopThread() successful.
     *
     * effectProcessImpl implementation must not call any EffectThread interface, otherwise it will
     * cause deadlock.
     *
     * @param in address of input float buffer.
     * @param out address of output float buffer.
     * @param samples number of samples to process.
     * @return IEffect::Status
     */
    virtual IEffect::Status effectProcessImpl(float* in, float* out, int samples) = 0;

    /**
     * process() call effectProcessImpl() for effect data processing, it is necessary for the
     * processing to be called under Effect thread mutex mThreadMutex, to avoid the effect state
     * change before/during data processing, and keep the thread and effect state consistent.
     */
    virtual void process_l() REQUIRES(mThreadMutex);

  private:
    const int kMaxTaskNameLen = 15;
    std::mutex mThreadMutex;
    std::condition_variable mCv;
    bool mExit GUARDED_BY(mThreadMutex) = false;
    bool mStop GUARDED_BY(mThreadMutex) = true;
    std::shared_ptr<EffectContext> mThreadContext GUARDED_BY(mThreadMutex);
    std::thread mThread;
    int mPriority;
    std::string mName;

    RetCode handleStartStop(bool stop);
};
}  // namespace aidl::android::hardware::audio::effect
