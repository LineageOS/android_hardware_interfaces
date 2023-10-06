/*
 * Copyright (C) 2021 The Android Open Source Project
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

#include <android-base/logging.h>

#include <chrono>
#include <regex>
#include <thread>
#include <vector>

namespace aidl::android::hardware::biometrics {

#define SLEEP_MS(x) \
    if (x > 0) std::this_thread::sleep_for(std::chrono::milliseconds(x))
#define BEGIN_OP(x)            \
    do {                       \
        LOG(INFO) << __func__; \
        SLEEP_MS(x);           \
    } while (0)
#define IS_TRUE(x) ((x == "1") || (x == "true"))

// This is for non-test situations, such as casual cuttlefish users, that don't
// set an explicit value.
// Some operations (i.e. enroll, authenticate) will be executed in tight loops
// by parts of the UI or fail if there is no latency. For example, the
// Face settings page constantly runs auth and the enrollment UI uses a
// cancel/restart cycle that requires some latency while the activities change.
#define DEFAULT_LATENCY 400

class Util {
  public:
    static int64_t getSystemNanoTime() {
        timespec now;
        clock_gettime(CLOCK_MONOTONIC, &now);
        return now.tv_sec * 1000000000LL + now.tv_nsec;
    }

    static bool hasElapsed(int64_t start, int64_t durationMillis) {
        auto now = getSystemNanoTime();
        if (now < start) return true;
        if (durationMillis <= 0) return true;
        return ((now - start) / 1000000LL) > durationMillis;
    }

    static std::vector<std::string> split(const std::string& str, const std::string& sep) {
        std::regex regex(sep);
        std::vector<std::string> parts(
                std::sregex_token_iterator(str.begin(), str.end(), regex, -1),
                std::sregex_token_iterator());
        return parts;
    }
};

}  // namespace aidl::android::hardware::biometrics
