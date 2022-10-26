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

#include <aidl/android/hardware/cas/BnDescrambler.h>
#include <media/stagefright/foundation/ABase.h>

namespace aidl {
namespace android {
namespace hardware {
namespace cas {

using namespace ::android;
using namespace std;
using ndk::ScopedAStatus;

class DescramblerImpl : public BnDescrambler {
  public:
    DescramblerImpl(DescramblerPlugin* plugin);
    virtual ~DescramblerImpl();

    virtual ScopedAStatus setMediaCasSession(const vector<uint8_t>& in_sessionId) override;

    virtual ScopedAStatus requiresSecureDecoderComponent(const string& in_mime,
                                                         bool* _aidl_return) override;

    virtual ScopedAStatus descramble(ScramblingControl in_scramblingControl,
                                     const vector<SubSample>& in_subSamples,
                                     const SharedBuffer& in_srcBuffer, int64_t in_srcOffset,
                                     const DestinationBuffer& in_dstBuffer, int64_t in_dstOffset,
                                     int32_t* _aidl_return) override;

    virtual ScopedAStatus release() override;

  private:
    shared_ptr<DescramblerPlugin> mPluginHolder;

    DISALLOW_EVIL_CONSTRUCTORS(DescramblerImpl);
};

}  // namespace cas
}  // namespace hardware
}  // namespace android
}  // namespace aidl
