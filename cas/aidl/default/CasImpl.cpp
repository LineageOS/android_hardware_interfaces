/*
 * Copyright (C) 2022 The Android Open Source Project
 *
 icensed under the Apache License, Version 2.0 (the "License");
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

#define LOG_TAG "android.hardware.cas-CasImpl"

#include <media/cas/CasAPI.h>
#include <utils/Log.h>

#include "CasImpl.h"
#include "TypeConvert.h"

namespace aidl {
namespace android {
namespace hardware {
namespace cas {

CasImpl::CasImpl(const shared_ptr<ICasListener>& listener) : mListener(listener) {
    ALOGV("CTOR");
}

CasImpl::~CasImpl() {
    ALOGV("DTOR");
    release();
}

// static
void CasImpl::OnEvent(void* appData, int32_t event, int32_t arg, uint8_t* data, size_t size) {
    if (appData == NULL) {
        ALOGE("Invalid appData!");
        return;
    }
    CasImpl* casImpl = static_cast<CasImpl*>(appData);
    casImpl->onEvent(event, arg, data, size);
}

// static
void CasImpl::CallBackExt(void* appData, int32_t event, int32_t arg, uint8_t* data, size_t size,
                          const CasSessionId* sessionId) {
    if (appData == NULL) {
        ALOGE("Invalid appData!");
        return;
    }
    CasImpl* casImpl = static_cast<CasImpl*>(appData);
    casImpl->onEvent(sessionId, event, arg, data, size);
}

// static
void CasImpl::StatusUpdate(void* appData, int32_t event, int32_t arg) {
    if (appData == NULL) {
        ALOGE("Invalid appData!");
        return;
    }
    CasImpl* casImpl = static_cast<CasImpl*>(appData);
    casImpl->onStatusUpdate(event, arg);
}

void CasImpl::init(CasPlugin* plugin) {
    shared_ptr<CasPlugin> holder(plugin);
    atomic_store(&mPluginHolder, holder);
}

void CasImpl::onEvent(int32_t event, int32_t arg, uint8_t* data, size_t size) {
    if (mListener == NULL) {
        return;
    }

    vector<uint8_t> eventData;
    if (data != NULL) {
        eventData.assign(data, data + size);
    }

    mListener->onEvent(event, arg, eventData);
}

void CasImpl::onEvent(const CasSessionId* sessionId, int32_t event, int32_t arg, uint8_t* data,
                      size_t size) {
    if (mListener == NULL) {
        return;
    }

    vector<uint8_t> eventData;
    if (data != NULL) {
        eventData.assign(data, data + size);
    }

    if (sessionId != NULL) {
        mListener->onSessionEvent(*sessionId, event, arg, eventData);
    } else {
        mListener->onEvent(event, arg, eventData);
    }
}

void CasImpl::onStatusUpdate(int32_t event, int32_t arg) {
    if (mListener == NULL) {
        return;
    }
    mListener->onStatusUpdate(static_cast<StatusEvent>(event), arg);
}

ScopedAStatus CasImpl::setPluginStatusUpdateCallback() {
    ALOGV("%s", __FUNCTION__);
    shared_ptr<CasPlugin> holder = atomic_load(&mPluginHolder);
    if (holder.get() == nullptr) {
        return toStatus(INVALID_OPERATION);
    }
    return toStatus(holder->setStatusCallback(&CasImpl::StatusUpdate));
}

ScopedAStatus CasImpl::setPrivateData(const vector<uint8_t>& pvtData) {
    ALOGV("%s", __FUNCTION__);
    shared_ptr<CasPlugin> holder = atomic_load(&mPluginHolder);
    if (holder.get() == nullptr) {
        return toStatus(INVALID_OPERATION);
    }
    return toStatus(holder->setPrivateData(pvtData));
}

ScopedAStatus CasImpl::openSessionDefault(vector<uint8_t>* sessionId) {
    ALOGV("%s", __FUNCTION__);

    shared_ptr<CasPlugin> holder = atomic_load(&mPluginHolder);
    status_t err = INVALID_OPERATION;
    if (holder.get() != nullptr) {
        err = holder->openSession(sessionId);
        holder.reset();
    }

    return toStatus(err);
}

ScopedAStatus CasImpl::openSession(SessionIntent intent, ScramblingMode mode,
                                   vector<uint8_t>* sessionId) {
    ALOGV("%s", __FUNCTION__);

    shared_ptr<CasPlugin> holder = atomic_load(&mPluginHolder);
    status_t err = INVALID_OPERATION;
    if (holder.get() != nullptr) {
        err = holder->openSession(static_cast<uint32_t>(intent), static_cast<uint32_t>(mode),
                                  sessionId);
        holder.reset();
    }

    return toStatus(err);
}

ScopedAStatus CasImpl::setSessionPrivateData(const vector<uint8_t>& sessionId,
                                             const vector<uint8_t>& pvtData) {
    ALOGV("%s: sessionId=%s", __FUNCTION__, sessionIdToString(sessionId).c_str());
    shared_ptr<CasPlugin> holder = atomic_load(&mPluginHolder);
    if (holder.get() == nullptr) {
        return toStatus(INVALID_OPERATION);
    }
    return toStatus(holder->setSessionPrivateData(sessionId, pvtData));
}

ScopedAStatus CasImpl::closeSession(const vector<uint8_t>& sessionId) {
    ALOGV("%s: sessionId=%s", __FUNCTION__, sessionIdToString(sessionId).c_str());
    shared_ptr<CasPlugin> holder = atomic_load(&mPluginHolder);
    if (holder.get() == nullptr) {
        return toStatus(INVALID_OPERATION);
    }
    return toStatus(holder->closeSession(sessionId));
}

ScopedAStatus CasImpl::processEcm(const vector<uint8_t>& sessionId, const vector<uint8_t>& ecm) {
    ALOGV("%s: sessionId=%s", __FUNCTION__, sessionIdToString(sessionId).c_str());
    shared_ptr<CasPlugin> holder = atomic_load(&mPluginHolder);
    if (holder.get() == nullptr) {
        return toStatus(INVALID_OPERATION);
    }

    return toStatus(holder->processEcm(sessionId, ecm));
}

ScopedAStatus CasImpl::processEmm(const vector<uint8_t>& emm) {
    ALOGV("%s", __FUNCTION__);
    shared_ptr<CasPlugin> holder = atomic_load(&mPluginHolder);
    if (holder.get() == nullptr) {
        return toStatus(INVALID_OPERATION);
    }

    return toStatus(holder->processEmm(emm));
}

ScopedAStatus CasImpl::sendEvent(int32_t event, int32_t arg, const vector<uint8_t>& eventData) {
    ALOGV("%s", __FUNCTION__);
    shared_ptr<CasPlugin> holder = atomic_load(&mPluginHolder);
    if (holder.get() == nullptr) {
        return toStatus(INVALID_OPERATION);
    }

    status_t err = holder->sendEvent(event, arg, eventData);
    return toStatus(err);
}

ScopedAStatus CasImpl::sendSessionEvent(const vector<uint8_t>& sessionId, int32_t event,
                                        int32_t arg, const vector<uint8_t>& eventData) {
    ALOGV("%s", __FUNCTION__);
    shared_ptr<CasPlugin> holder = atomic_load(&mPluginHolder);
    if (holder.get() == nullptr) {
        return toStatus(INVALID_OPERATION);
    }

    status_t err = holder->sendSessionEvent(sessionId, event, arg, eventData);
    return toStatus(err);
}

ScopedAStatus CasImpl::provision(const string& provisionString) {
    ALOGV("%s: provisionString=%s", __FUNCTION__, provisionString.c_str());
    shared_ptr<CasPlugin> holder = atomic_load(&mPluginHolder);
    if (holder.get() == nullptr) {
        return toStatus(INVALID_OPERATION);
    }

    return toStatus(holder->provision(String8(provisionString.c_str())));
}

ScopedAStatus CasImpl::refreshEntitlements(int32_t refreshType,
                                           const vector<uint8_t>& refreshData) {
    ALOGV("%s", __FUNCTION__);
    shared_ptr<CasPlugin> holder = atomic_load(&mPluginHolder);
    if (holder.get() == nullptr) {
        return toStatus(INVALID_OPERATION);
    }

    status_t err = holder->refreshEntitlements(refreshType, refreshData);
    return toStatus(err);
}

ScopedAStatus CasImpl::release() {
    ALOGV("%s: plugin=%p", __FUNCTION__, mPluginHolder.get());

    shared_ptr<CasPlugin> holder(nullptr);
    atomic_store(&mPluginHolder, holder);

    return ScopedAStatus::ok();
}

}  // namespace cas
}  // namespace hardware
}  // namespace android
}  // namespace aidl
