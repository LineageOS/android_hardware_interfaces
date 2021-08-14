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

#ifndef ANDROID_HARDWARE_CONTEXTHUB_V1_X_ICONTEXTHUBCALLBACKWRAPPER_H
#define ANDROID_HARDWARE_CONTEXTHUB_V1_X_ICONTEXTHUBCALLBACKWRAPPER_H

#include "android/hardware/contexthub/1.0/IContexthub.h"
#include "android/hardware/contexthub/1.0/IContexthubCallback.h"
#include "android/hardware/contexthub/1.0/types.h"
#include "android/hardware/contexthub/1.2/IContexthubCallback.h"
#include "android/hardware/contexthub/1.2/types.h"

#include <utils/LightRefBase.h>

#include <cassert>

namespace android {
namespace hardware {
namespace contexthub {
namespace V1_X {
namespace implementation {

inline V1_0::ContextHubMsg convertToOldMsg(V1_2::ContextHubMsg msg) {
    return msg.msg_1_0;
}

inline hidl_vec<V1_0::HubAppInfo> convertToOldAppInfo(hidl_vec<V1_2::HubAppInfo> appInfos) {
    hidl_vec<V1_0::HubAppInfo> convertedInfo(appInfos.size());
    for (int i = 0; i < appInfos.size(); ++i) {
        convertedInfo[i] = appInfos[i].info_1_0;
    }

    return convertedInfo;
}

/**
 * The IContexthubCallback classes below abstract away the common logic between both the V1.0, and
 * V1.2 versions of the Contexthub HAL callback interface. This allows users of these classes to
 * only care about the HAL version at init time and then interact with either version of the
 * callback without worrying about the class type by utilizing the base class.
 */
class IContextHubCallbackWrapperBase : public VirtualLightRefBase {
  public:
    virtual Return<void> handleClientMsg(V1_2::ContextHubMsg msg,
                                         hidl_vec<hidl_string> msgContentPerms) = 0;

    virtual Return<void> handleTxnResult(uint32_t txnId, V1_0::TransactionResult result) = 0;

    virtual Return<void> handleHubEvent(V1_0::AsyncEventType evt) = 0;

    virtual Return<void> handleAppAbort(uint64_t appId, uint32_t abortCode) = 0;

    virtual Return<void> handleAppsInfo(hidl_vec<V1_2::HubAppInfo> appInfo) = 0;

    virtual Return<bool> linkToDeath(const sp<hidl_death_recipient>& recipient,
                                     uint64_t cookie) = 0;

    virtual Return<bool> unlinkToDeath(const sp<hidl_death_recipient>& recipient) = 0;
};

template <typename T>
class ContextHubCallbackWrapper : public IContextHubCallbackWrapperBase {
  public:
    ContextHubCallbackWrapper(sp<T> callback) : mCallback(callback){};

    virtual Return<void> handleClientMsg(V1_2::ContextHubMsg msg,
                                         hidl_vec<hidl_string> /* msgContentPerms */) override {
        return mCallback->handleClientMsg(convertToOldMsg(msg));
    }

    virtual Return<void> handleTxnResult(uint32_t txnId, V1_0::TransactionResult result) override {
        return mCallback->handleTxnResult(txnId, result);
    }

    virtual Return<void> handleHubEvent(V1_0::AsyncEventType evt) override {
        return mCallback->handleHubEvent(evt);
    }

    virtual Return<void> handleAppAbort(uint64_t appId, uint32_t abortCode) override {
        return mCallback->handleAppAbort(appId, abortCode);
    }

    virtual Return<void> handleAppsInfo(hidl_vec<V1_2::HubAppInfo> appInfo) override {
        return mCallback->handleAppsInfo(convertToOldAppInfo(appInfo));
    }

    Return<bool> linkToDeath(const sp<hidl_death_recipient>& recipient, uint64_t cookie) override {
        return mCallback->linkToDeath(recipient, cookie);
    }

    Return<bool> unlinkToDeath(const sp<hidl_death_recipient>& recipient) override {
        return mCallback->unlinkToDeath(recipient);
    }

  protected:
    sp<T> mCallback;
};

class IContextHubCallbackWrapperV1_0 : public ContextHubCallbackWrapper<V1_0::IContexthubCallback> {
  public:
    IContextHubCallbackWrapperV1_0(sp<V1_0::IContexthubCallback> callback)
        : ContextHubCallbackWrapper(callback){};
};

class IContextHubCallbackWrapperV1_2 : public ContextHubCallbackWrapper<V1_2::IContexthubCallback> {
  public:
    IContextHubCallbackWrapperV1_2(sp<V1_2::IContexthubCallback> callback)
        : ContextHubCallbackWrapper(callback){};

    Return<void> handleClientMsg(V1_2::ContextHubMsg msg,
                                 hidl_vec<hidl_string> msgContentPerms) override {
        return mCallback->handleClientMsg_1_2(msg, msgContentPerms);
    }

    Return<void> handleAppsInfo(hidl_vec<V1_2::HubAppInfo> appInfo) override {
        return mCallback->handleAppsInfo_1_2(appInfo);
    }
};

}  // namespace implementation
}  // namespace V1_X
}  // namespace contexthub
}  // namespace hardware
}  // namespace android

#endif  // ANDROID_HARDWARE_CONTEXTHUB_V1_X_ICONTEXTHUBCALLBACKWRAPPER_H