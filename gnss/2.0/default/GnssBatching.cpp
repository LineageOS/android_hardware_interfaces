/*
 * Copyright (C) 2019 The Android Open Source Project
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

#define LOG_TAG "GnssBatching"

#include "GnssBatching.h"

namespace android {
namespace hardware {
namespace gnss {
namespace V2_0 {
namespace implementation {

sp<V2_0::IGnssBatchingCallback> GnssBatching::sCallback = nullptr;

// Methods from ::android::hardware::gnss::V1_0::IGnssBatching follow.
Return<bool> GnssBatching::init(const sp<V1_0::IGnssBatchingCallback>&) {
    // TODO implement
    return bool{};
}

Return<uint16_t> GnssBatching::getBatchSize() {
    // TODO implement
    return uint16_t{};
}

Return<bool> GnssBatching::start(const V1_0::IGnssBatching::Options&) {
    // TODO implement
    return bool{};
}

Return<void> GnssBatching::flush() {
    // TODO implement
    return Void();
}

Return<bool> GnssBatching::stop() {
    // TODO implement
    return bool{};
}

Return<void> GnssBatching::cleanup() {
    // TODO implement
    return Void();
}

// Methods from V2_0::IGnssBatching follow.
Return<bool> GnssBatching::init_2_0(const sp<V2_0::IGnssBatchingCallback>& callback) {
    sCallback = callback;
    return true;
}

}  // namespace implementation
}  // namespace V2_0
}  // namespace gnss
}  // namespace hardware
}  // namespace android
