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

#include "effect-impl/EffectTypes.h"

namespace aidl::android::hardware::audio::effect {

class EffectThread {
  public:
    // default priority is same as HIDL: ANDROID_PRIORITY_URGENT_AUDIO
    EffectThread();
    virtual ~EffectThread();

    // called by effect implementation.
    RetCode createThread(const std::string& name,
                         const int priority = ANDROID_PRIORITY_URGENT_AUDIO);
    RetCode destroyThread();
    RetCode startThread();
    RetCode stopThread();

    // Will call process() in a loop if the thread is running.
    void threadLoop();

    // User of EffectThread must implement the effect processing logic in this method.
    virtual void process() = 0;
    const int MAX_TASK_COMM_LEN = 15;

  private:
    std::mutex mMutex;
    std::condition_variable mCv;
    bool mExit GUARDED_BY(mMutex) = false;
    bool mStop GUARDED_BY(mMutex) = true;
    std::thread mThread;
    int mPriority;
    std::string mName;
};
}  // namespace aidl::android::hardware::audio::effect
