/*
 * Copyright (C) 2017 The Android Open Source Project
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
#define LOG_TAG "android.hardware.cas@1.0-DescramblerImpl"

#include <hidlmemory/mapping.h>
#include <media/hardware/CryptoAPI.h>
#include <media/cas/DescramblerAPI.h>
#include <utils/Log.h>

#include "DescramblerImpl.h"
#include "SharedLibrary.h"
#include "TypeConvert.h"

namespace android {
using hidl::memory::V1_0::IMemory;

namespace hardware {
namespace cas {
namespace V1_0 {
namespace implementation {

#define CHECK_SUBSAMPLE_DEF(type) \
static_assert(sizeof(SubSample) == sizeof(type::SubSample), \
        "SubSample: size doesn't match"); \
static_assert(offsetof(SubSample, numBytesOfClearData) \
        == offsetof(type::SubSample, mNumBytesOfClearData), \
        "SubSample: numBytesOfClearData offset doesn't match"); \
static_assert(offsetof(SubSample, numBytesOfEncryptedData) \
        == offsetof(type::SubSample, mNumBytesOfEncryptedData), \
        "SubSample: numBytesOfEncryptedData offset doesn't match")

CHECK_SUBSAMPLE_DEF(DescramblerPlugin);
CHECK_SUBSAMPLE_DEF(CryptoPlugin);

DescramblerImpl::DescramblerImpl(
        const sp<SharedLibrary>& library, DescramblerPlugin *plugin) :
        mLibrary(library), mPlugin(plugin) {
    ALOGV("CTOR: mPlugin=%p", mPlugin);
}

DescramblerImpl::~DescramblerImpl() {
    ALOGV("DTOR: mPlugin=%p", mPlugin);
    release();
}

Return<Status> DescramblerImpl::setMediaCasSession(const HidlCasSessionId& sessionId) {
    ALOGV("%s: sessionId=%s", __FUNCTION__,
            sessionIdToString(sessionId).string());

    return toStatus(mPlugin->setMediaCasSession(sessionId));
}

Return<bool> DescramblerImpl::requiresSecureDecoderComponent(
        const hidl_string& mime) {
    return mPlugin->requiresSecureDecoderComponent(String8(mime.c_str()));
}

Return<void> DescramblerImpl::descramble(
        ScramblingControl scramblingControl,
        const hidl_vec<SubSample>& subSamples,
        const SharedBuffer& srcBuffer,
        uint64_t srcOffset,
        const DestinationBuffer& dstBuffer,
        uint64_t dstOffset,
        descramble_cb _hidl_cb) {
    ALOGV("%s", __FUNCTION__);

    sp<IMemory> srcMem = mapMemory(srcBuffer.heapBase);
    void *srcPtr = (uint8_t *)(void *)srcMem->getPointer() + srcBuffer.offset;
    void *dstPtr = NULL;
    if (dstBuffer.type == BufferType::SHARED_MEMORY) {
        // When using shared memory, src buffer is also used as dst,
        // we don't map it again here.
        dstPtr = srcPtr;
    } else {
        native_handle_t *handle = const_cast<native_handle_t *>(
                dstBuffer.secureMemory.getNativeHandle());
        dstPtr = static_cast<void *>(handle);
    }
    // Casting hidl SubSample to DescramblerPlugin::SubSample, but need
    // to ensure structs are actually idential

    int32_t result = mPlugin->descramble(
            dstBuffer.type != BufferType::SHARED_MEMORY,
            (DescramblerPlugin::ScramblingControl)scramblingControl,
            subSamples.size(),
            (DescramblerPlugin::SubSample*)subSamples.data(),
            srcPtr,
            srcOffset,
            dstPtr,
            dstOffset,
            NULL);

    _hidl_cb(toStatus(result >= 0 ? OK : result), result, NULL);
    return Void();
}

Return<Status> DescramblerImpl::release() {
    ALOGV("%s: mPlugin=%p", __FUNCTION__, mPlugin);

    if (mPlugin != NULL) {
        delete mPlugin;
        mPlugin = NULL;
    }
    return Status::OK;
}

} // namespace implementation
} // namespace V1_0
} // namespace cas
} // namespace hardware
} // namespace android
