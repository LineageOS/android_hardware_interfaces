/*
 * Copyright (C) 2016 The Android Open Source Project
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

#ifndef ANDROID_HARDWARE_CONTEXTHUB_V1_0_CONTEXTHUB_H_
#define ANDROID_HARDWARE_CONTEXTHUB_V1_0_CONTEXTHUB_H_

#include <unordered_map>

#include <android/hardware/contexthub/1.0/IContexthub.h>
#include <hardware/context_hub.h>

namespace android {
namespace hardware {
namespace contexthub {
namespace V1_0 {
namespace implementation {

struct Contexthub : public ::android::hardware::contexthub::V1_0::IContexthub {
    Contexthub();

    Return<void> getHubs(getHubs_cb _hidl_cb) override;

    Return<Result> registerCallback(uint32_t hubId,
                                    const sp<IContexthubCallback> &cb) override;

    Return<Result> sendMessageToHub(uint32_t hubId,
                                    const ContextHubMsg &msg) override;

    Return<Result> loadNanoApp(uint32_t hubId,
                               const ::android::hardware::hidl_vec<uint8_t>& appBinary,
                               uint32_t transactionId) override;

    Return<Result> unloadNanoApp(uint32_t hubId,
                                 uint64_t appId,
                                 uint32_t transactionId) override;

    Return<Result> enableNanoApp(uint32_t hubId,
                                 uint64_t appId,
                                 uint32_t transactionId) override;

    Return<Result> disableNanoApp(uint32_t hubId,
                                  uint64_t appId,
                                  uint32_t transactionId) override;

    Return<Result> queryApps(uint32_t hubId) override;

    Return<Result> reboot(uint32_t hubId);

    bool isInitialized();

private:

    struct CachedHubInformation{
        struct hub_app_name_t osAppName;
        sp<IContexthubCallback> callBack;
    };

    status_t mInitCheck;
    const struct context_hub_module_t *mContextHubModule;
    std::unordered_map<uint32_t, CachedHubInformation> mCachedHubInfo;

    sp<IContexthubCallback> mCb;
    bool mIsTransactionPending;
    uint32_t mTransactionId;

    bool isValidHubId(uint32_t hubId);

    sp<IContexthubCallback> getCallBackForHubId(uint32_t hubId);

    int handleOsMessage(sp<IContexthubCallback> cb,
                        uint32_t msgType,
                        const uint8_t *msg,
                        int msgLen);

    static int contextHubCb(uint32_t hubId,
                            const struct hub_message_t *rxMsg,
                            void *cookie);

    bool setOsAppAsDestination(hub_message_t *msg, int hubId);

    DISALLOW_COPY_AND_ASSIGN(Contexthub);
};

extern "C" IContexthub *HIDL_FETCH_IContexthub(const char *name);

}  // namespace implementation
}  // namespace V1_0
}  // namespace contexthub
}  // namespace hardware
}  // namespace android

#endif  // ANDROID_HARDWARE_CONTEXTHUB_V1_0_CONTEXTHUB_H_
