/*
 * Copyright (C) 2018 The Android Open Source Project
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

#ifndef ANDROID_SENSOR_EVENTS_CHECKER_H
#define ANDROID_SENSOR_EVENTS_CHECKER_H

#include <android/hardware/sensors/1.0/types.h>

#include <cmath>

class SensorEventsChecker {
   public:
    using Event = ::android::hardware::sensors::V1_0::Event;
    virtual bool check(const std::vector<Event>& events, std::string* out) const = 0;
    virtual ~SensorEventsChecker() {}
};

class NullChecker : public SensorEventsChecker {
   public:
    virtual bool check(const std::vector<Event>&, std::string*) const { return true; }
};

class SensorEventPerEventChecker : public SensorEventsChecker {
   public:
    virtual bool checkEvent(const Event& event, std::string* out) const = 0;
    virtual bool check(const std::vector<Event>& events, std::string* out) const {
        for (const auto& e : events) {
            if (!checkEvent(e, out)) {
                return false;
            }
        }
        return true;
    }
};

class Vec3NormChecker : public SensorEventPerEventChecker {
   public:
    Vec3NormChecker(float min, float max) : mLowerLimit(min), mUpperLimit(max) {}
    static Vec3NormChecker byNominal(float nominal, float allowedError) {
        return Vec3NormChecker(nominal - allowedError, nominal + allowedError);
    }

    virtual bool checkEvent(const Event& event, std::string* out) const {
        android::hardware::sensors::V1_0::Vec3 v = event.u.vec3;
        float norm = std::sqrt(v.x * v.x + v.y * v.y + v.z * v.z);
        if (norm < mLowerLimit || norm > mUpperLimit) {
            if (out != nullptr) {
                std::ostringstream ss;
                ss << "Event @ " << event.timestamp << " (" << v.x << ", " << v.y << ", " << v.z
                   << ")"
                   << " has norm " << norm << ", which is beyond range"
                   << " [" << mLowerLimit << ", " << mUpperLimit << "]";
                *out = ss.str();
            }
            return false;
        }
        return true;
    }

   protected:
    float mLowerLimit;
    float mUpperLimit;
};

#endif  // ANDROID_SENSOR_EVENTS_CHECKER_H