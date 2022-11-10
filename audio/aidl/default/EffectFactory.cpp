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

#define LOG_TAG "AHAL_EffectFactory"
#include <android-base/logging.h>
#include <dlfcn.h>
#include <unordered_set>

#include "effect-impl/EffectTypes.h"
#include "effect-impl/EffectUUID.h"
#include "effectFactory-impl/EffectFactory.h"

using aidl::android::media::audio::common::AudioUuid;

namespace aidl::android::hardware::audio::effect {

Factory::Factory(const std::string& file) : mConfig(EffectConfig(file)) {
    LOG(DEBUG) << __func__ << " with config file: " << file;
    loadEffectLibs();
}

Factory::~Factory() {
    if (auto count = mEffectUuidMap.size()) {
        LOG(ERROR) << __func__ << " remaining " << count
                   << " effect instances not destroyed indicating resource leak!";
        for (const auto& it : mEffectUuidMap) {
            if (auto spEffect = it.first.lock()) {
                LOG(ERROR) << __func__ << " erase remaining instance UUID " << it.second.toString();
                destroyEffectImpl(spEffect);
            }
        }
    }
}

ndk::ScopedAStatus Factory::queryEffects(const std::optional<AudioUuid>& in_type_uuid,
                                         const std::optional<AudioUuid>& in_impl_uuid,
                                         const std::optional<AudioUuid>& in_proxy_uuid,
                                         std::vector<Descriptor::Identity>* _aidl_return) {
    std::copy_if(
            mIdentitySet.begin(), mIdentitySet.end(), std::back_inserter(*_aidl_return),
            [&](auto& desc) {
                return (!in_type_uuid.has_value() || in_type_uuid.value() == desc.type) &&
                       (!in_impl_uuid.has_value() || in_impl_uuid.value() == desc.uuid) &&
                       (!in_proxy_uuid.has_value() ||
                        (desc.proxy.has_value() && in_proxy_uuid.value() == desc.proxy.value()));
            });
    return ndk::ScopedAStatus::ok();
}

ndk::ScopedAStatus Factory::queryProcessing(const std::optional<Processing::Type>& in_type,
                                            std::vector<Processing>* _aidl_return) {
    // TODO: implement this with audio_effect.xml.
    if (in_type.has_value()) {
        // return all matching process filter
        LOG(DEBUG) << __func__ << " process type: " << in_type.value().toString();
    }
    LOG(DEBUG) << __func__ << " return " << _aidl_return->size();
    return ndk::ScopedAStatus::ok();
}

#define RETURN_IF_BINDER_EXCEPTION(functor)                                 \
    {                                                                       \
        binder_exception_t exception = functor;                             \
        if (EX_NONE != exception) {                                         \
            LOG(ERROR) << #functor << ":  failed with error " << exception; \
            return ndk::ScopedAStatus::fromExceptionCode(exception);        \
        }                                                                   \
    }

ndk::ScopedAStatus Factory::createEffect(const AudioUuid& in_impl_uuid,
                                         std::shared_ptr<IEffect>* _aidl_return) {
    LOG(DEBUG) << __func__ << ": UUID " << in_impl_uuid.toString();
    if (mEffectLibMap.count(in_impl_uuid)) {
        auto& lib = mEffectLibMap[in_impl_uuid];
        // didn't do dlsym yet
        if (nullptr == lib.second) {
            void* libHandle = lib.first.get();
            auto dlInterface = std::make_unique<struct effect_dl_interface_s>();
            dlInterface->createEffectFunc = (EffectCreateFunctor)dlsym(libHandle, "createEffect");
            dlInterface->destroyEffectFunc =
                    (EffectDestroyFunctor)dlsym(libHandle, "destroyEffect");
            if (!dlInterface->createEffectFunc || !dlInterface->destroyEffectFunc) {
                LOG(ERROR) << __func__
                           << ": create or destroy symbol not exist in library: " << libHandle
                           << " with dlerror: " << dlerror();
                return ndk::ScopedAStatus::fromExceptionCode(EX_TRANSACTION_FAILED);
            }
            lib.second = std::move(dlInterface);
        }

        auto& libInterface = lib.second;
        std::shared_ptr<IEffect> effectSp;
        RETURN_IF_BINDER_EXCEPTION(libInterface->createEffectFunc(&in_impl_uuid, &effectSp));
        if (!effectSp) {
            LOG(ERROR) << __func__ << ": library created null instance without return error!";
            return ndk::ScopedAStatus::fromExceptionCode(EX_TRANSACTION_FAILED);
        }
        *_aidl_return = effectSp;
        mEffectUuidMap[std::weak_ptr<IEffect>(effectSp)] = in_impl_uuid;
        LOG(DEBUG) << __func__ << ": instance " << effectSp.get() << " created successfully";
        return ndk::ScopedAStatus::ok();
    } else {
        LOG(ERROR) << __func__ << ": library doesn't exist";
        return ndk::ScopedAStatus::fromExceptionCode(EX_ILLEGAL_ARGUMENT);
    }
    return ndk::ScopedAStatus::ok();
}

ndk::ScopedAStatus Factory::destroyEffectImpl(const std::shared_ptr<IEffect>& in_handle) {
    std::weak_ptr<IEffect> wpHandle(in_handle);
    // find UUID with key (std::weak_ptr<IEffect>)
    if (auto uuidIt = mEffectUuidMap.find(wpHandle); uuidIt != mEffectUuidMap.end()) {
        auto& uuid = uuidIt->second;
        // find implementation library with UUID
        if (auto libIt = mEffectLibMap.find(uuid); libIt != mEffectLibMap.end()) {
            if (libIt->second.second->destroyEffectFunc) {
                RETURN_IF_BINDER_EXCEPTION(libIt->second.second->destroyEffectFunc(in_handle));
            }
        } else {
            LOG(ERROR) << __func__ << ": UUID " << uuid.toString() << " does not exist in libMap!";
            return ndk::ScopedAStatus::fromExceptionCode(EX_ILLEGAL_ARGUMENT);
        }
        mEffectUuidMap.erase(uuidIt);
        return ndk::ScopedAStatus::ok();
    } else {
        LOG(ERROR) << __func__ << ": instance " << in_handle << " does not exist!";
        return ndk::ScopedAStatus::fromExceptionCode(EX_ILLEGAL_ARGUMENT);
    }
}

// go over the map and cleanup all expired weak_ptrs.
void Factory::cleanupEffectMap() {
    for (auto it = mEffectUuidMap.begin(); it != mEffectUuidMap.end();) {
        if (nullptr == it->first.lock()) {
            it = mEffectUuidMap.erase(it);
        } else {
            ++it;
        }
    }
}

ndk::ScopedAStatus Factory::destroyEffect(const std::shared_ptr<IEffect>& in_handle) {
    LOG(DEBUG) << __func__ << ": instance " << in_handle.get();
    ndk::ScopedAStatus status = destroyEffectImpl(in_handle);
    // always do the cleanup
    cleanupEffectMap();
    return status;
}

void Factory::openEffectLibrary(const AudioUuid& impl, const std::string& libName) {
    std::function<void(void*)> dlClose = [](void* handle) -> void {
        if (handle && dlclose(handle)) {
            LOG(ERROR) << "dlclose failed " << dlerror();
        }
    };

    auto libHandle =
            std::unique_ptr<void, decltype(dlClose)>{dlopen(libName.c_str(), RTLD_LAZY), dlClose};
    if (!libHandle) {
        LOG(ERROR) << __func__ << ": dlopen failed, err: " << dlerror();
        return;
    }

    LOG(DEBUG) << __func__ << " dlopen lib:" << libName << "\nimpl:" << impl.toString()
               << "\nhandle:" << libHandle;
    mEffectLibMap.insert({impl, std::make_pair(std::move(libHandle), nullptr)});
}

void Factory::createIdentityWithConfig(const EffectConfig::LibraryUuid& configLib,
                                       const AudioUuid& typeUuid,
                                       const std::optional<AudioUuid> proxyUuid) {
    static const auto& libMap = mConfig.getLibraryMap();
    const std::string& libName = configLib.name;
    if (auto path = libMap.find(libName); path != libMap.end()) {
        Descriptor::Identity id;
        id.type = typeUuid;
        id.uuid = configLib.uuid;
        id.proxy = proxyUuid;
        LOG(DEBUG) << __func__ << ": typeUuid " << id.type.toString() << "\nimplUuid "
                   << id.uuid.toString() << " proxyUuid "
                   << (proxyUuid.has_value() ? proxyUuid->toString() : "null");
        openEffectLibrary(id.uuid, path->second);
        mIdentitySet.insert(std::move(id));
    } else {
        LOG(ERROR) << __func__ << ": library " << libName << " not exist!";
        return;
    }
}

void Factory::loadEffectLibs() {
    const auto& configEffectsMap = mConfig.getEffectsMap();
    for (const auto& configEffects : configEffectsMap) {
        if (auto typeUuid = kUuidNameTypeMap.find(configEffects.first /* effect name */);
            typeUuid != kUuidNameTypeMap.end()) {
            const auto& configLibs = configEffects.second;
            std::optional<AudioUuid> proxyUuid;
            if (configLibs.proxyLibrary.has_value()) {
                const auto& proxyLib = configLibs.proxyLibrary.value();
                proxyUuid = proxyLib.uuid;
            }
            for (const auto& configLib : configLibs.libraries) {
                createIdentityWithConfig(configLib, typeUuid->second, proxyUuid);
            }
        } else {
            LOG(ERROR) << __func__ << ": can not find type UUID for effect " << configEffects.first
                       << " skipping!";
        }
    }
}

}  // namespace aidl::android::hardware::audio::effect
