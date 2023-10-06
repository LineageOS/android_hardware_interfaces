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

#ifndef HARDWARE_INTERFACES_CAMERA_COMMON_SIMPLETHREAD_H_
#define HARDWARE_INTERFACES_CAMERA_COMMON_SIMPLETHREAD_H_

#include <thread>

namespace android {
namespace hardware {
namespace camera {
namespace common {
namespace helper {

// A simple looper based on std::thread.
class SimpleThread {
  public:
    SimpleThread();
    virtual ~SimpleThread();

    // Explicit call to start execution of the thread. No thread is created before this function
    // is called.
    virtual void run() final;
    virtual void requestExitAndWait() final;

  protected:
    // Main logic of the thread. This function is called repeatedly until it returns false.
    // Thread execution stops if this function returns false.
    virtual bool threadLoop() = 0;

    // Returns true if the thread execution should stop. Should be used by threadLoop to check if
    // the thread has been requested to exit.
    virtual inline bool exitPending() final { return mDone.load(std::memory_order_acquire); }

  private:
    // Wraps threadLoop in a simple while loop that allows safe exit
    virtual void runLoop() final;

    // Flag to signal end of thread execution. This flag is checked before every iteration
    // of threadLoop.
    std::atomic_bool mDone;
    std::thread mThread;
};

}  // namespace helper
}  // namespace common
}  // namespace camera
}  // namespace hardware
}  // namespace android

#endif  // HARDWARE_INTERFACES_CAMERA_COMMON_SIMPLETHREAD_H_
