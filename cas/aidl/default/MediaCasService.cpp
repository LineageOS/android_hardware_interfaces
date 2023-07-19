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

#define LOG_TAG "android.hardware.cas-MediaCasService"

#include <media/cas/CasAPI.h>
#include <media/cas/DescramblerAPI.h>
#include <utils/Log.h>

#include "CasImpl.h"
#include "DescramblerImpl.h"
#include "MediaCasService.h"

namespace aidl {
namespace android {
namespace hardware {
namespace cas {

MediaCasService::MediaCasService()
    : mCasLoader("createCasFactory"), mDescramblerLoader("createDescramblerFactory") {}

MediaCasService::~MediaCasService() {}

ScopedAStatus MediaCasService::enumeratePlugins(
        vector<AidlCasPluginDescriptor>* aidlCasPluginDescriptors) {
    ALOGV("%s", __FUNCTION__);

    mCasLoader.enumeratePlugins(aidlCasPluginDescriptors);
    return ScopedAStatus::ok();
}

ScopedAStatus MediaCasService::isSystemIdSupported(int32_t CA_system_id, bool* _aidl_return) {
    ALOGV("isSystemIdSupported: CA_system_id=%d", CA_system_id);

    *_aidl_return = mCasLoader.findFactoryForScheme(CA_system_id);
    return ScopedAStatus::ok();
}

ScopedAStatus MediaCasService::createPlugin(int32_t CA_system_id,
                                            const shared_ptr<ICasListener>& listener,
                                            shared_ptr<ICas>* _aidl_return) {
    ALOGV("%s: CA_system_id=%d", __FUNCTION__, CA_system_id);
    if (listener == NULL) ALOGV("%s: Listener is NULL", __FUNCTION__);

    CasFactory* factory;
    shared_ptr<SharedLibrary> library;
    if (mCasLoader.findFactoryForScheme(CA_system_id, &library, &factory)) {
        CasPlugin* plugin = NULL;
        shared_ptr<CasImpl> casImpl = ::ndk::SharedRefBase::make<CasImpl>(listener);
        if (factory->createPlugin(CA_system_id, casImpl.get(), &CasImpl::CallBackExt, &plugin) ==
                    OK &&
            plugin != NULL) {
            casImpl->init(plugin);
            *_aidl_return = casImpl;
            casImpl->setPluginStatusUpdateCallback();
        }
    }

    return ScopedAStatus::ok();
}

ScopedAStatus MediaCasService::isDescramblerSupported(int32_t CA_system_id, bool* _aidl_return) {
    ALOGV("%s: CA_system_id=%d", __FUNCTION__, CA_system_id);

    *_aidl_return = mDescramblerLoader.findFactoryForScheme(CA_system_id);
    return ScopedAStatus::ok();
}

ScopedAStatus MediaCasService::createDescrambler(int32_t CA_system_id,
                                                 shared_ptr<IDescrambler>* _aidl_return) {
    ALOGV("%s: CA_system_id=%d", __FUNCTION__, CA_system_id);

    DescramblerFactory* factory;
    shared_ptr<SharedLibrary> library;
    if (mDescramblerLoader.findFactoryForScheme(CA_system_id, &library, &factory)) {
        DescramblerPlugin* plugin = NULL;
        if (factory->createPlugin(CA_system_id, &plugin) == OK && plugin != NULL) {
            *_aidl_return = ::ndk::SharedRefBase::make<DescramblerImpl>(plugin);
        }
    }

    return ScopedAStatus::ok();
}

}  // namespace cas
}  // namespace hardware
}  // namespace android
}  // namespace aidl
