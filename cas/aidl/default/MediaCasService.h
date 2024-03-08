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
#include <aidl/android/hardware/cas/BnMediaCasService.h>
#include <media/cas/DescramblerAPI.h>

#include "FactoryLoader.h"

namespace aidl {
namespace android {
namespace hardware {
namespace cas {

using namespace ::android;
using namespace std;
using ndk::ScopedAStatus;

class MediaCasService : public BnMediaCasService {
  public:
    MediaCasService();

    virtual ScopedAStatus enumeratePlugins(
            vector<AidlCasPluginDescriptor>* aidlCasPluginDescriptors) override;

    virtual ScopedAStatus isSystemIdSupported(int32_t in_CA_system_id, bool* _aidl_return) override;

    virtual ScopedAStatus createPlugin(int32_t in_CA_system_id,
                                       const shared_ptr<ICasListener>& in_listener,
                                       shared_ptr<ICas>* _aidl_return) override;

    virtual ScopedAStatus isDescramblerSupported(int32_t in_CA_system_id,
                                                 bool* _aidl_return) override;

    virtual ScopedAStatus createDescrambler(int32_t in_CA_system_id,
                                            shared_ptr<IDescrambler>* _aidl_return) override;

  private:
    FactoryLoader<CasFactory> mCasLoader;
    FactoryLoader<DescramblerFactory> mDescramblerLoader;

    virtual ~MediaCasService();
};

}  // namespace cas
}  // namespace hardware
}  // namespace android
}  // namespace aidl
