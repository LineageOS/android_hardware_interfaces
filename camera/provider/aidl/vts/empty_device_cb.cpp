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

#include "empty_device_cb.h"
#include <log/log.h>

ScopedAStatus EmptyDeviceCb::notify(const std::vector<NotifyMsg>&) {
    ALOGI("notify callback");
    ADD_FAILURE();  // Empty callback should not reach here
    return ndk::ScopedAStatus::ok();
}
ScopedAStatus EmptyDeviceCb::processCaptureResult(const std::vector<CaptureResult>&) {
    ALOGI("processCaptureResult callback");
    ADD_FAILURE();  // Empty callback should not reach here
    return ndk::ScopedAStatus::ok();
}
ScopedAStatus EmptyDeviceCb::requestStreamBuffers(const std::vector<BufferRequest>&,
                                                  std::vector<StreamBufferRet>*,
                                                  BufferRequestStatus* _aidl_return) {
    ALOGI("requestStreamBuffers callback");
    // HAL might want to request buffer after configureStreams, but tests with EmptyDeviceCb
    // doesn't actually need to send capture requests, so just return an error.
    *_aidl_return = BufferRequestStatus::FAILED_UNKNOWN;
    return ndk::ScopedAStatus::ok();
}
ScopedAStatus EmptyDeviceCb::returnStreamBuffers(const std::vector<StreamBuffer>&) {
    ALOGI("returnStreamBuffers");
    ADD_FAILURE();  // Empty callback should not reach here
    return ndk::ScopedAStatus::ok();
}
