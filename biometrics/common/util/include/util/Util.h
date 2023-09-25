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

#include <android-base/parseint.h>
using ::android::base::ParseInt;

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

    // Returns a vector of integers for the string separated by comma,
    // Empty vector is returned if there is any parsing error
    static std::vector<int32_t> parseIntSequence(const std::string& str,
                                                 const std::string& sep = ",") {
        std::vector<std::string> seqs = Util::split(str, sep);
        std::vector<int32_t> res;

        for (const auto& seq : seqs) {
            int32_t val;
            if (ParseInt(seq, &val)) {
                res.push_back(val);
            } else {
                LOG(WARNING) << "Invalid int sequence:" + str + " seq:" + seq;
                res.clear();
                break;
            }
        }

        return res;
    }

    // Parses a single enrollment stage string in the format of
    //     enroll_stage_spec: <duration>[-acquiredInfos]
    //                                      duration: integerInMs
    //                                      acquiredInfos: [info1,info2,...]
    //
    // Returns false if there is parsing error
    //
    static bool parseEnrollmentCaptureSingle(const std::string& str,
                                             std::vector<std::vector<int32_t>>& res) {
        std::vector<int32_t> defaultAcquiredInfo = {1};
        bool aborted = true;

        do {
            std::smatch sms;
            // Parses strings like "1000-[5,1]" or "500"
            std::regex ex("((\\d+)(-\\[([\\d|,]+)\\])?)");
            if (!regex_match(str.cbegin(), str.cend(), sms, ex)) break;
            int32_t duration;
            if (!ParseInt(sms.str(2), &duration)) break;
            res.push_back({duration});
            if (!sms.str(4).empty()) {
                auto acqv = parseIntSequence(sms.str(4));
                if (acqv.empty()) break;
                res.push_back(acqv);
            } else
                res.push_back(defaultAcquiredInfo);
            aborted = false;
        } while (0);

        return !aborted;
    }

    // Parses enrollment string consisting of one or more stages in the formst of
    //  <enroll_stage_spec>[,enroll_stage_spec,...]
    // Empty vector is returned in case of parsing error
    static std::vector<std::vector<int32_t>> parseEnrollmentCapture(const std::string& str) {
        std::vector<std::vector<int32_t>> res;

        std::string s(str);
        s.erase(std::remove_if(s.begin(), s.end(), ::isspace), s.end());
        bool aborted = false;
        std::smatch sms;
        // Parses strings like "1000-[5,1],500,800-[6,5,1]"
        //                               -------------- ----- ---------------
        //  into parts:                       A       B       C
        while (regex_search(s, sms, std::regex("^(,)?(\\d+(-\\[[\\d|,]+\\])?)"))) {
            if (!parseEnrollmentCaptureSingle(sms.str(2), res)) {
                aborted = true;
                break;
            }
            s = sms.suffix();
        }
        if (aborted || s.length() != 0) {
            res.clear();
            LOG(ERROR) << "Failed to parse enrollment captures:" + str;
        }

        return res;
    }
};

}  // namespace aidl::android::hardware::biometrics
