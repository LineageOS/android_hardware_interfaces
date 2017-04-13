/*
 * Copyright 2016, The Android Open Source Project
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

#ifndef MEDIA_HIDL_TEST_COMMON_H
#define MEDIA_HIDL_TEST_COMMON_H

#include <media/stagefright/foundation/ALooper.h>
#include <utils/Condition.h>
#include <utils/List.h>
#include <utils/Mutex.h>

#include <media/openmax/OMX_Index.h>
#include <media/openmax/OMX_Core.h>
#include <media/openmax/OMX_Component.h>
#include <media/openmax/OMX_IndexExt.h>
#include <media/openmax/OMX_AudioExt.h>

/*
 * TODO: Borrowed from Conversion.h. This is not the ideal way to do it.
 * Loose these definitions once you include Conversion.h
 */
inline uint32_t toRawIndexType(OMX_INDEXTYPE l) {
    return static_cast<uint32_t>(l);
}

inline android::hardware::media::omx::V1_0::Status toStatus(
    android::status_t l) {
    return static_cast<android::hardware::media::omx::V1_0::Status>(l);
}

inline hidl_vec<uint8_t> inHidlBytes(void const* l, size_t size) {
    hidl_vec<uint8_t> t;
    t.setToExternal(static_cast<uint8_t*>(const_cast<void*>(l)), size, false);
    return t;
}

inline uint32_t toRawCommandType(OMX_COMMANDTYPE l) {
    return static_cast<uint32_t>(l);
}

/*
 * Handle Callback functions EmptythisBuffer(), FillthisBuffer(),
 * EventHandler()
 */
#define DEFAULT_TIMEOUT 40000

enum bufferOwner {
    client,
    component,
    unknown,
};

struct BufferInfo {
    uint32_t id;
    bufferOwner owner;
    android::hardware::media::omx::V1_0::CodecBuffer omxBuffer;
    ::android::sp<IMemory> mMemory;
};

struct CodecObserver : public IOmxObserver {
   public:
    Return<void> onMessages(const hidl_vec<Message>& messages) override {
        android::Mutex::Autolock autoLock(msgLock);
        for (hidl_vec<Message>::const_iterator it = messages.begin();
             it != messages.end(); ++it) {
            msgQueue.push_back(*it);
        }
        msgCondition.signal();
        return Void();
    }
    android::hardware::media::omx::V1_0::Status dequeueMessage(
        Message* msg, int64_t timeoutUs,
        android::Vector<BufferInfo>* iBuffers = nullptr,
        android::Vector<BufferInfo>* oBuffers = nullptr) {
        int64_t finishBy = android::ALooper::GetNowUs() + timeoutUs;
        for (;;) {
            android::Mutex::Autolock autoLock(msgLock);
            android::List<Message>::iterator it = msgQueue.begin();
            while (it != msgQueue.end()) {
                if (it->type ==
                    android::hardware::media::omx::V1_0::Message::Type::EVENT) {
                    *msg = *it;
                    msgQueue.erase(it);
                    return ::android::hardware::media::omx::V1_0::Status::OK;
                } else if (it->type == android::hardware::media::omx::V1_0::
                                           Message::Type::FILL_BUFFER_DONE) {
                    if (oBuffers) {
                        size_t i;
                        for (i = 0; i < oBuffers->size(); ++i) {
                            if ((*oBuffers)[i].id ==
                                it->data.bufferData.buffer) {
                                oBuffers->editItemAt(i).owner = client;
                                msgQueue.erase(it);
                                break;
                            }
                        }
                        EXPECT_LE(i, oBuffers->size());
                    }
                } else if (it->type == android::hardware::media::omx::V1_0::
                                           Message::Type::EMPTY_BUFFER_DONE) {
                    if (iBuffers) {
                        size_t i;
                        for (i = 0; i < iBuffers->size(); ++i) {
                            if ((*iBuffers)[i].id ==
                                it->data.bufferData.buffer) {
                                iBuffers->editItemAt(i).owner = client;
                                msgQueue.erase(it);
                                break;
                            }
                        }
                        EXPECT_LE(i, iBuffers->size());
                    }
                }
                ++it;
            }
            android::status_t err =
                (timeoutUs < 0)
                    ? msgCondition.wait(msgLock)
                    : msgCondition.waitRelative(
                          msgLock,
                          (finishBy - android::ALooper::GetNowUs()) * 1000);
            if (err == android::TIMED_OUT) return toStatus(err);
        }
    }

    android::List<Message> msgQueue;
    android::Mutex msgLock;
    android::Condition msgCondition;
};

/*
 * Useful Wrapper utilities
 */
template <class T>
void InitOMXParams(T* params) {
    params->nSize = sizeof(T);
    params->nVersion.s.nVersionMajor = 1;
    params->nVersion.s.nVersionMinor = 0;
    params->nVersion.s.nRevision = 0;
    params->nVersion.s.nStep = 0;
}

template <class T>
Return<android::hardware::media::omx::V1_0::Status> getParam(
    sp<IOmxNode> omxNode, OMX_INDEXTYPE omxIdx, T* params) {
    android::hardware::media::omx::V1_0::Status status;
    InitOMXParams(params);
    omxNode->getParameter(
        toRawIndexType(omxIdx), inHidlBytes(params, sizeof(*params)),
        [&status, &params](android::hardware::media::omx::V1_0::Status _s,
                           hidl_vec<uint8_t> const& outParams) {
            status = _s;
            std::copy(outParams.data(), outParams.data() + outParams.size(),
                      static_cast<uint8_t*>(static_cast<void*>(params)));
        });
    return status;
}

template <class T>
Return<android::hardware::media::omx::V1_0::Status> setParam(
    sp<IOmxNode> omxNode, OMX_INDEXTYPE omxIdx, T* params) {
    InitOMXParams(params);
    return omxNode->setParameter(toRawIndexType(omxIdx),
                                 inHidlBytes(params, sizeof(*params)));
}

template <class T>
Return<android::hardware::media::omx::V1_0::Status> getPortParam(
    sp<IOmxNode> omxNode, OMX_INDEXTYPE omxIdx, OMX_U32 nPortIndex, T* params) {
    android::hardware::media::omx::V1_0::Status status;
    InitOMXParams(params);
    params->nPortIndex = nPortIndex;
    omxNode->getParameter(
        toRawIndexType(omxIdx), inHidlBytes(params, sizeof(*params)),
        [&status, &params](android::hardware::media::omx::V1_0::Status _s,
                           hidl_vec<uint8_t> const& outParams) {
            status = _s;
            std::copy(outParams.data(), outParams.data() + outParams.size(),
                      static_cast<uint8_t*>(static_cast<void*>(params)));
        });
    return status;
}

template <class T>
Return<android::hardware::media::omx::V1_0::Status> setPortParam(
    sp<IOmxNode> omxNode, OMX_INDEXTYPE omxIdx, OMX_U32 nPortIndex, T* params) {
    InitOMXParams(params);
    params->nPortIndex = nPortIndex;
    return omxNode->setParameter(toRawIndexType(omxIdx),
                                 inHidlBytes(params, sizeof(*params)));
}

#endif  // MEDIA_HIDL_TEST_COMMON_H
