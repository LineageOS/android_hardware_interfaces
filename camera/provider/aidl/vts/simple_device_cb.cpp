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

#include "simple_device_cb.h"

ScopedAStatus SimpleDeviceCb::notify(const std::vector<NotifyMsg>& msgs) {
    std::unique_lock<std::mutex> l(mParent->mLock);
    mParent->mNotifyMessages = msgs;
    mParent->mResultCondition.notify_one();

    return ndk::ScopedAStatus::ok();
}
ScopedAStatus SimpleDeviceCb::processCaptureResult(const std::vector<CaptureResult>&) {
    return ndk::ScopedAStatus::fromStatus(STATUS_UNKNOWN_TRANSACTION);
}
ScopedAStatus SimpleDeviceCb::requestStreamBuffers(const std::vector<BufferRequest>&,
                                                   std::vector<StreamBufferRet>*,
                                                   BufferRequestStatus*) {
    return ndk::ScopedAStatus::fromStatus(STATUS_UNKNOWN_TRANSACTION);
}
ScopedAStatus SimpleDeviceCb::returnStreamBuffers(const std::vector<StreamBuffer>&) {
    return ndk::ScopedAStatus::fromStatus(STATUS_UNKNOWN_TRANSACTION);
}
