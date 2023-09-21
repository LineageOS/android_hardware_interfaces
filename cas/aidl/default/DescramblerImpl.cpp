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

#define LOG_TAG "android.hardware.cas-DescramblerImpl"

#include <aidlcommonsupport/NativeHandle.h>
#include <inttypes.h>
#include <media/cas/DescramblerAPI.h>
#include <media/hardware/CryptoAPI.h>
#include <media/stagefright/foundation/AUtils.h>
#include <sys/mman.h>
#include <utils/Log.h>

#include "DescramblerImpl.h"
#include "TypeConvert.h"

namespace aidl {
namespace android {
namespace hardware {
namespace cas {

#define CHECK_SUBSAMPLE_DEF(type)                                                                 \
    static_assert(sizeof(SubSample) == sizeof(type::SubSample), "SubSample: size doesn't match"); \
    static_assert(offsetof(SubSample, numBytesOfClearData) ==                                     \
                          offsetof(type::SubSample, mNumBytesOfClearData),                        \
                  "SubSample: numBytesOfClearData offset doesn't match");                         \
    static_assert(offsetof(SubSample, numBytesOfEncryptedData) ==                                 \
                          offsetof(type::SubSample, mNumBytesOfEncryptedData),                    \
                  "SubSample: numBytesOfEncryptedData offset doesn't match")

CHECK_SUBSAMPLE_DEF(DescramblerPlugin);
CHECK_SUBSAMPLE_DEF(CryptoPlugin);

DescramblerImpl::DescramblerImpl(DescramblerPlugin* plugin) : mPluginHolder(plugin) {
    ALOGV("CTOR: plugin=%p", mPluginHolder.get());
}

DescramblerImpl::~DescramblerImpl() {
    ALOGV("DTOR: plugin=%p", mPluginHolder.get());
    release();
}

ScopedAStatus DescramblerImpl::setMediaCasSession(const vector<uint8_t>& in_sessionId) {
    ALOGV("%s: sessionId=%s", __FUNCTION__, sessionIdToString(in_sessionId).c_str());

    shared_ptr<DescramblerPlugin> holder = atomic_load(&mPluginHolder);
    if (holder.get() == nullptr) {
        return toStatus(INVALID_OPERATION);
    }

    return toStatus(holder->setMediaCasSession(in_sessionId));
}

ScopedAStatus DescramblerImpl::requiresSecureDecoderComponent(const string& in_mime,
                                                              bool* _aidl_return) {
    shared_ptr<DescramblerPlugin> holder = atomic_load(&mPluginHolder);
    if (holder.get() == nullptr) {
        *_aidl_return = false;
    }

    *_aidl_return = holder->requiresSecureDecoderComponent(in_mime.c_str());
    return ScopedAStatus::ok();
}

static inline bool validateRangeForSize(int64_t offset, int64_t length, int64_t size) {
    return isInRange<int64_t, uint64_t>(0, (uint64_t)size, offset, (uint64_t)length);
}

ScopedAStatus DescramblerImpl::descramble(ScramblingControl scramblingControl,
                                          const vector<SubSample>& subSamples,
                                          const SharedBuffer& srcBuffer, int64_t srcOffset,
                                          const DestinationBuffer& dstBuffer, int64_t dstOffset,
                                          int32_t* _aidl_return) {
    ALOGV("%s", __FUNCTION__);

    // heapbase's size is stored in int64_t, but mapMemory's mmap will map size in
    // size_t. If size is over SIZE_MAX, mapMemory mapMemory could succeed but the
    // mapped memory's actual size will be smaller than the reported size.
    if (srcBuffer.heapBase.size > SIZE_MAX) {
        ALOGE("Invalid memory size: %" PRIu64 "", srcBuffer.heapBase.size);
        android_errorWriteLog(0x534e4554, "79376389");
        return toStatus(BAD_VALUE);
    }

    void* srcPtr = mmap(NULL, srcBuffer.heapBase.size, PROT_READ | PROT_WRITE, MAP_SHARED,
                        srcBuffer.heapBase.fd.get(), 0);

    // Validate if the offset and size in the SharedBuffer is consistent with the
    // mapped heapbase, since the offset and size is controlled by client.
    if (srcPtr == NULL) {
        ALOGE("Failed to map src buffer.");
        return toStatus(BAD_VALUE);
    }
    if (!validateRangeForSize(srcBuffer.offset, srcBuffer.size, srcBuffer.heapBase.size)) {
        ALOGE("Invalid src buffer range: offset %" PRIu64 ", size %" PRIu64
              ", srcMem"
              "size %" PRIu64 "",
              srcBuffer.offset, srcBuffer.size, srcBuffer.heapBase.size);
        android_errorWriteLog(0x534e4554, "67962232");
        return toStatus(BAD_VALUE);
    }

    // use 64-bit here to catch bad subsample size that might be overflowing.
    uint64_t totalBytesInSubSamples = 0;
    for (size_t i = 0; i < subSamples.size(); i++) {
        uint32_t numBytesOfClearData = subSamples[i].numBytesOfClearData;
        uint32_t numBytesOfEncryptedData = subSamples[i].numBytesOfEncryptedData;
        totalBytesInSubSamples += (uint64_t)numBytesOfClearData + numBytesOfEncryptedData;
    }
    // Further validate if the specified srcOffset and requested total subsample size
    // is consistent with the source shared buffer size.
    if (!validateRangeForSize(srcOffset, totalBytesInSubSamples, srcBuffer.size)) {
        ALOGE("Invalid srcOffset and subsample size: "
              "srcOffset %" PRIu64 ", totalBytesInSubSamples %" PRIu64
              ", srcBuffer"
              "size %" PRIu64 "",
              srcOffset, totalBytesInSubSamples, srcBuffer.size);
        android_errorWriteLog(0x534e4554, "67962232");
        return toStatus(BAD_VALUE);
    }
    srcPtr = (uint8_t*)srcPtr + srcBuffer.offset;

    void* dstPtr = NULL;
    if (dstBuffer.getTag() == DestinationBuffer::Tag::nonsecureMemory) {
        // When using shared memory, src buffer is also used as dst,
        // we don't map it again here.
        dstPtr = srcPtr;

        // In this case the dst and src would be the same buffer, need to validate
        // dstOffset against the buffer size too.
        if (!validateRangeForSize(dstOffset, totalBytesInSubSamples, srcBuffer.size)) {
            ALOGE("Invalid dstOffset and subsample size: "
                  "dstOffset %" PRIu64 ", totalBytesInSubSamples %" PRIu64
                  ", srcBuffer"
                  "size %" PRIu64 "",
                  dstOffset, totalBytesInSubSamples, srcBuffer.size);
            android_errorWriteLog(0x534e4554, "67962232");
            return toStatus(BAD_VALUE);
        }
    } else {
        native_handle_t* handle = makeFromAidl(dstBuffer.get<DestinationBuffer::secureMemory>());
        dstPtr = static_cast<void*>(handle);
    }

    // Get a local copy of the shared_ptr for the plugin. Note that before
    // calling the callback, this shared_ptr must be manually reset, since
    // the client side could proceed as soon as the callback is called
    // without waiting for this method to go out of scope.
    shared_ptr<DescramblerPlugin> holder = atomic_load(&mPluginHolder);
    if (holder.get() == nullptr) {
        return toStatus(INVALID_OPERATION);
    }

    // Casting SubSample to DescramblerPlugin::SubSample, but need to ensure
    // structs are actually identical

    auto returnStatus =
            holder->descramble(dstBuffer.getTag() != DestinationBuffer::Tag::nonsecureMemory,
                               (DescramblerPlugin::ScramblingControl)scramblingControl,
                               subSamples.size(), (DescramblerPlugin::SubSample*)subSamples.data(),
                               srcPtr, srcOffset, dstPtr, dstOffset, NULL);

    holder.reset();
    *_aidl_return = returnStatus;
    return toStatus(returnStatus >= 0 ? OK : returnStatus);
}

ScopedAStatus DescramblerImpl::release() {
    ALOGV("%s: plugin=%p", __FUNCTION__, mPluginHolder.get());

    shared_ptr<DescramblerPlugin> holder(nullptr);
    atomic_store(&mPluginHolder, holder);

    return ScopedAStatus::ok();
}

}  // namespace cas
}  // namespace hardware
}  // namespace android
}  // namespace aidl
