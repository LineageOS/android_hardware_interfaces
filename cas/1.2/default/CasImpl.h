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

#ifndef ANDROID_HARDWARE_CAS_V1_1_CAS_IMPL_H_
#define ANDROID_HARDWARE_CAS_V1_1_CAS_IMPL_H_

#include <android/hardware/cas/1.1/ICas.h>
#include <android/hardware/cas/1.2/ICas.h>
#include <media/stagefright/foundation/ABase.h>

namespace android {
struct CasPlugin;

namespace hardware {
namespace cas {
namespace V1_1 {
struct ICasListener;
namespace implementation {

using ::android::hardware::cas::V1_0::HidlCasData;
using ::android::hardware::cas::V1_0::HidlCasSessionId;
using ::android::hardware::cas::V1_0::Status;
using ::android::hardware::cas::V1_2::ScramblingMode;
using ::android::hardware::cas::V1_2::SessionIntent;
using ::android::hardware::cas::V1_2::StatusEvent;

class SharedLibrary;

class CasImpl : public V1_2::ICas {
  public:
    CasImpl(const sp<ICasListener>& listener);
    virtual ~CasImpl();

    static void OnEvent(void* appData, int32_t event, int32_t arg, uint8_t* data, size_t size);

    static void CallBackExt(void* appData, int32_t event, int32_t arg, uint8_t* data, size_t size,
                            const CasSessionId* sessionId);

    static void StatusUpdate(void* appData, int32_t event, int32_t arg);

    void init(const sp<SharedLibrary>& library, CasPlugin* plugin);
    void onEvent(int32_t event, int32_t arg, uint8_t* data, size_t size);

    void onEvent(const CasSessionId* sessionId, int32_t event, int32_t arg, uint8_t* data,
                 size_t size);

    void onStatusUpdate(int32_t event, int32_t arg);

    // ICas inherits

    Return<Status> setPluginStatusUpdateCallback();

    virtual Return<Status> setPrivateData(const HidlCasData& pvtData) override;

    virtual Return<void> openSession(openSession_cb _hidl_cb) override;

    virtual Return<void> openSession_1_2(const SessionIntent intent, const ScramblingMode mode,
                                         openSession_1_2_cb _hidl_cb) override;

    virtual Return<Status> closeSession(const HidlCasSessionId& sessionId) override;

    virtual Return<Status> setSessionPrivateData(const HidlCasSessionId& sessionId,
                                                 const HidlCasData& pvtData) override;

    virtual Return<Status> processEcm(const HidlCasSessionId& sessionId,
                                      const HidlCasData& ecm) override;

    virtual Return<Status> processEmm(const HidlCasData& emm) override;

    virtual Return<Status> sendEvent(int32_t event, int32_t arg,
                                     const HidlCasData& eventData) override;

    virtual Return<Status> sendSessionEvent(const HidlCasSessionId& sessionId, int32_t event,
                                            int32_t arg, const HidlCasData& eventData) override;

    virtual Return<Status> provision(const hidl_string& provisionString) override;

    virtual Return<Status> refreshEntitlements(int32_t refreshType,
                                               const HidlCasData& refreshData) override;

    virtual Return<Status> release() override;

  private:
    struct PluginHolder;
    sp<SharedLibrary> mLibrary;
    std::shared_ptr<CasPlugin> mPluginHolder;
    sp<ICasListener> mListener;

    DISALLOW_EVIL_CONSTRUCTORS(CasImpl);
};

}  // namespace implementation
}  // namespace V1_1
}  // namespace cas
}  // namespace hardware
}  // namespace android

#endif  // ANDROID_HARDWARE_CAS_V1_1_CAS_IMPL_H_
