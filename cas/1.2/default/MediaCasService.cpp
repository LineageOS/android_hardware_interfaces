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

//#define LOG_NDEBUG 0
#define LOG_TAG "android.hardware.cas@1.1-MediaCasService"

#include <android/hardware/cas/1.1/ICasListener.h>
#include <android/hardware/cas/1.2/ICasListener.h>
#include <media/cas/CasAPI.h>
#include <media/cas/DescramblerAPI.h>
#include <utils/Log.h>

#include "CasImpl.h"
#include "DescramblerImpl.h"
#include "MediaCasService.h"

namespace android {
namespace hardware {
namespace cas {
namespace V1_1 {
namespace implementation {

class Wrapper : public V1_1::ICasListener {
  public:
    static sp<V1_1::ICasListener> wrap(sp<V1_0::ICasListener> impl) {
        sp<V1_1::ICasListener> cast = V1_1::ICasListener::castFrom(impl);
        if (cast == NULL) {
            cast = new Wrapper(impl);
        }
        return cast;
    }

    virtual Return<void> onEvent(int32_t event, int32_t arg,
                                 const hidl_vec<uint8_t>& data) override {
        mImpl->onEvent(event, arg, data);
        return Void();
    }

    virtual Return<void> onSessionEvent(const hidl_vec<uint8_t>& /* sessionId */,
                                        int32_t /* event */, int32_t /* arg */,
                                        const hidl_vec<uint8_t>& /*data*/) override {
        ALOGV("Do nothing on Session Event for cas@1.0 client in cas@1.1");
        return Void();
    }

  private:
    Wrapper(sp<V1_0::ICasListener> impl) : mImpl(impl){};
    sp<V1_0::ICasListener> mImpl;
};

MediaCasService::MediaCasService()
    : mCasLoader("createCasFactory"), mDescramblerLoader("createDescramblerFactory") {}

MediaCasService::~MediaCasService() {}

Return<void> MediaCasService::enumeratePlugins(enumeratePlugins_cb _hidl_cb) {
    ALOGV("%s", __FUNCTION__);

    vector<HidlCasPluginDescriptor> results;
    mCasLoader.enumeratePlugins(&results);

    _hidl_cb(results);
    return Void();
}

Return<bool> MediaCasService::isSystemIdSupported(int32_t CA_system_id) {
    ALOGV("isSystemIdSupported: CA_system_id=%d", CA_system_id);

    return mCasLoader.findFactoryForScheme(CA_system_id);
}

Return<sp<V1_0::ICas>> MediaCasService::createPlugin(int32_t CA_system_id,
                                                     const sp<V1_0::ICasListener>& listener) {
    ALOGV("%s:Use createPluginExt to create plugin in cas@1.1", __FUNCTION__);

    sp<ICas> result;

    sp<V1_1::ICasListener> listenerV1_1 = Wrapper::wrap(listener);

    result = createPluginExt(CA_system_id, listenerV1_1);

    return result;
}

Return<sp<ICas>> MediaCasService::createPluginExt(int32_t CA_system_id,
                                                  const sp<ICasListener>& listener) {
    ALOGV("%s: CA_system_id=%d", __FUNCTION__, CA_system_id);
    if (listener == NULL) ALOGV("%s: Listener is NULL", __FUNCTION__);

    sp<V1_2::ICas> result;

    CasFactory* factory;
    sp<SharedLibrary> library;
    if (mCasLoader.findFactoryForScheme(CA_system_id, &library, &factory)) {
        CasPlugin* plugin = NULL;
        sp<CasImpl> casImpl = new CasImpl(listener);
        if (factory->createPlugin(CA_system_id, casImpl.get(), &CasImpl::CallBackExt, &plugin) ==
                    OK &&
            plugin != NULL) {
            casImpl->init(library, plugin);
            result = casImpl;

            sp<V1_2::ICasListener> listenerV1_2 = V1_2::ICasListener::castFrom(listener);
            if (listenerV1_2 != NULL) {
                casImpl->setPluginStatusUpdateCallback();
            }
        }
    }

    return result;
}

Return<bool> MediaCasService::isDescramblerSupported(int32_t CA_system_id) {
    ALOGV("%s: CA_system_id=%d", __FUNCTION__, CA_system_id);

    return mDescramblerLoader.findFactoryForScheme(CA_system_id);
}

Return<sp<IDescramblerBase>> MediaCasService::createDescrambler(int32_t CA_system_id) {
    ALOGV("%s: CA_system_id=%d", __FUNCTION__, CA_system_id);

    sp<IDescrambler> result;

    DescramblerFactory* factory;
    sp<SharedLibrary> library;
    if (mDescramblerLoader.findFactoryForScheme(CA_system_id, &library, &factory)) {
        DescramblerPlugin* plugin = NULL;
        if (factory->createPlugin(CA_system_id, &plugin) == OK && plugin != NULL) {
            result = new DescramblerImpl(library, plugin);
        }
    }

    return result;
}

}  // namespace implementation
}  // namespace V1_1
}  // namespace cas
}  // namespace hardware
}  // namespace android
