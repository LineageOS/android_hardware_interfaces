/*
 * Copyright (C) 2020 The Android Open Source Project
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

#include <android/hardware/contexthub/1.0/IContexthubCallback.h>
#include <log/log.h>

#include <cinttypes>

namespace android {
namespace hardware {
namespace contexthub {
namespace vts_utils {

// Base callback implementation that just logs all callbacks by default, but
// records a failure if
class ContexthubCallbackBase : public V1_0::IContexthubCallback {
  public:
    virtual Return<void> handleClientMsg(const V1_0::ContextHubMsg& /*msg*/) override {
        ALOGD("Got client message callback");
        return Void();
    }

    virtual Return<void> handleTxnResult(uint32_t txnId, V1_0::TransactionResult result) override {
        ALOGD("Got transaction result callback for txnId %" PRIu32 " with result %" PRId32, txnId,
              result);
        return Void();
    }

    virtual Return<void> handleHubEvent(V1_0::AsyncEventType evt) override {
        ALOGD("Got hub event callback for event type %" PRIu32, evt);
        return Void();
    }

    virtual Return<void> handleAppAbort(uint64_t appId, uint32_t abortCode) override {
        ALOGD("Got app abort notification for appId 0x%" PRIx64 " with abort code 0x%" PRIx32,
              appId, abortCode);
        return Void();
    }

    virtual Return<void> handleAppsInfo(const hidl_vec<V1_0::HubAppInfo>& /*appInfo*/) override {
        ALOGD("Got app info callback");
        return Void();
    }
};

}  // namespace vts_utils
}  // namespace contexthub
}  // namespace hardware
}  // namespace android
