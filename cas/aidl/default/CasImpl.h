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

#include <aidl/android/hardware/cas/BnCas.h>
#include <aidl/android/hardware/cas/ICasListener.h>
#include <media/stagefright/foundation/ABase.h>

namespace aidl {
namespace android {
namespace hardware {
namespace cas {

using namespace ::android;
using namespace std;
using ndk::ScopedAStatus;

class CasImpl : public BnCas {
  public:
    CasImpl(const shared_ptr<ICasListener>& listener);
    virtual ~CasImpl();

    static void OnEvent(void* appData, int32_t event, int32_t arg, uint8_t* data, size_t size);

    static void CallBackExt(void* appData, int32_t event, int32_t arg, uint8_t* data, size_t size,
                            const CasSessionId* sessionId);

    static void StatusUpdate(void* appData, int32_t event, int32_t arg);

    void init(CasPlugin* plugin);
    void onEvent(int32_t event, int32_t arg, uint8_t* data, size_t size);

    void onEvent(const CasSessionId* sessionId, int32_t event, int32_t arg, uint8_t* data,
                 size_t size);

    void onStatusUpdate(int32_t event, int32_t arg);

    // ICas inherits

    ScopedAStatus setPluginStatusUpdateCallback();

    virtual ScopedAStatus setPrivateData(const vector<uint8_t>& pvtData) override;

    virtual ScopedAStatus openSessionDefault(vector<uint8_t>* sessionId) override;

    virtual ScopedAStatus openSession(SessionIntent intent, ScramblingMode mode,
                                      vector<uint8_t>* sessionId) override;

    virtual ScopedAStatus closeSession(const vector<uint8_t>& sessionId) override;

    virtual ScopedAStatus setSessionPrivateData(const vector<uint8_t>& sessionId,
                                                const vector<uint8_t>& pvtData) override;

    virtual ScopedAStatus processEcm(const vector<uint8_t>& sessionId,
                                     const vector<uint8_t>& ecm) override;

    virtual ScopedAStatus processEmm(const vector<uint8_t>& emm) override;

    virtual ScopedAStatus sendEvent(int32_t event, int32_t arg,
                                    const vector<uint8_t>& eventData) override;

    virtual ScopedAStatus sendSessionEvent(const vector<uint8_t>& sessionId, int32_t event,
                                           int32_t arg, const vector<uint8_t>& eventData) override;

    virtual ScopedAStatus provision(const string& provisionString) override;

    virtual ScopedAStatus refreshEntitlements(int32_t refreshType,
                                              const vector<uint8_t>& refreshData) override;

    virtual ScopedAStatus release() override;

  private:
    struct PluginHolder;
    shared_ptr<CasPlugin> mPluginHolder;
    shared_ptr<ICasListener> mListener;

    DISALLOW_EVIL_CONSTRUCTORS(CasImpl);
};

}  // namespace cas
}  // namespace hardware
}  // namespace android
}  // namespace aidl
