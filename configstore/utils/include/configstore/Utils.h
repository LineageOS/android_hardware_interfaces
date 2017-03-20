//
// Copyright (C) 2017 The Android Open Source Project
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//      http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.
//

#ifndef ANDROID_HARDWARE_CONFIGSTORE_UTILS_H
#define ANDROID_HARDWARE_CONFIGSTORE_UTILS_H

#include <hidl/Status.h>
#include <stdatomic.h>

#pragma push_macro("LOG_TAG")
#undef LOG_TAG
#define LOG_TAG "ConfigStoreUtil"

namespace android {
namespace hardware {
namespace configstore {
// arguments V: type for the value (i.e., OptionalXXX)
//           I: interface class name
//           func: member function pointer
using namespace V1_0;

template<typename V, typename I, android::hardware::Return<void> (I::* func)
        (std::function<void(const V&)>)>
decltype(V::value) get(const decltype(V::value) &defValue) {
    auto getHelper = []()->V {
        V ret;
        sp<I> configs = I::getService();

        if (!configs.get()) {
            // fallback to the default value
            ret.specified = false;
        } else {
            auto status = (*configs.*func)([&ret](V v) {
                ret = v;
            });
            if (!status.isOk()) {
                ALOGE("HIDL call failed. %s", status.description().c_str());
                ret.specified = false;
            }
        }

        return ret;
    };
    static V cachedValue = getHelper();

    return cachedValue.specified ? cachedValue.value : defValue;
}

template<typename I, android::hardware::Return<void> (I::* func)
        (std::function<void(const OptionalBool&)>)>
bool getBool(const bool defValue) {
    return get<OptionalBool, I, func>(defValue);
}

template<typename I, android::hardware::Return<void> (I::* func)
        (std::function<void(const OptionalInt32&)>)>
int32_t getInt32(const int32_t defValue) {
    return get<OptionalInt32, I, func>(defValue);
}

template<typename I, android::hardware::Return<void> (I::* func)
        (std::function<void(const OptionalUInt32&)>)>
uint32_t getUInt32(const uint32_t defValue) {
    return get<OptionalUInt32, I, func>(defValue);
}

template<typename I, android::hardware::Return<void> (I::* func)
        (std::function<void(const OptionalInt64&)>)>
int64_t getInt64(const int64_t defValue) {
    return get<OptionalInt64, I, func>(defValue);
}

template<typename I, android::hardware::Return<void> (I::* func)
        (std::function<void(const OptionalUInt64&)>)>
uint64_t getUInt64(const uint64_t defValue) {
    return get<OptionalUInt64, I, func>(defValue);
}

template<typename I, android::hardware::Return<void> (I::* func)
        (std::function<void(const OptionalString&)>)>
std::string getString(const std::string &defValue) {
    return get<OptionalString, I, func>(defValue);
}

}  // namespace configstore
}  // namespace hardware
}  // namespace android

#pragma pop_macro("LOG_TAG")

#endif  // ANDROID_HARDWARE_CONFIGSTORE_UTILS_H
