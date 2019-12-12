/*
 * Copyright (C) 2017 The Android Open Source Project
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
#ifndef ANDROID_HARDWARE_BROADCASTRADIO_VTS_POINTER_UTILS
#define ANDROID_HARDWARE_BROADCASTRADIO_VTS_POINTER_UTILS

#include <chrono>
#include <thread>

using namespace std::chrono_literals;

namespace android {
namespace hardware {
namespace broadcastradio {
namespace vts {

/**
 * Clears strong pointer and waits until the object gets destroyed.
 *
 * @param ptr The pointer to get cleared.
 * @param timeout Time to wait for other references.
 */
template <typename T>
static void clearAndWait(sp<T>& ptr, std::chrono::milliseconds timeout) {
    using std::chrono::steady_clock;

    constexpr auto step = 10ms;

    wp<T> wptr = ptr;
    ptr.clear();

    auto limit = steady_clock::now() + timeout;
    while (wptr.promote() != nullptr) {
        if (steady_clock::now() + step > limit) {
            FAIL() << "Pointer was not released within timeout";
            break;
        }
        std::this_thread::sleep_for(step);
    }
}

}  // namespace vts
}  // namespace broadcastradio
}  // namespace hardware
}  // namespace android

#endif  // ANDROID_HARDWARE_BROADCASTRADIO_VTS_POINTER_UTILS
