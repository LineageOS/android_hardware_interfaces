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

#include "SimpleThread.h"

namespace android {
namespace hardware {
namespace camera {
namespace common {
namespace helper {

SimpleThread::SimpleThread() : mDone(true), mThread() {}
SimpleThread::~SimpleThread() {
    // Safe to call requestExitAndWait() from the destructor because requestExitAndWait() ensures
    // that the thread is joinable before joining on it. This is different from how
    // android::Thread worked.
    requestExitAndWait();
}

void SimpleThread::run() {
    requestExitAndWait();  // Exit current execution, if any.

    // start thread
    mDone.store(false, std::memory_order_release);
    mThread = std::thread(&SimpleThread::runLoop, this);
}

void SimpleThread::requestExitAndWait() {
    // Signal thread to stop
    mDone.store(true, std::memory_order_release);

    // Wait for thread to exit if needed. This should happen in no more than one iteration of
    // threadLoop
    if (mThread.joinable()) {
        mThread.join();
    }
    mThread = std::thread();
}

void SimpleThread::runLoop() {
    while (!exitPending()) {
        if (!threadLoop()) {
            break;
        }
    }
}

}  // namespace helper
}  // namespace common
}  // namespace camera
}  // namespace hardware
}  // namespace android