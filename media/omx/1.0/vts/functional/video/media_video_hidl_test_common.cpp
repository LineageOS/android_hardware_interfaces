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

#define LOG_TAG "media_omx_hidl_video_test_common"
#include <android-base/logging.h>

#include <android/hardware/media/omx/1.0/IOmx.h>
#include <android/hardware/media/omx/1.0/IOmxNode.h>
#include <android/hardware/media/omx/1.0/IOmxObserver.h>
#include <android/hardware/media/omx/1.0/types.h>
#include <android/hidl/allocator/1.0/IAllocator.h>
#include <android/hidl/memory/1.0/IMapper.h>
#include <android/hidl/memory/1.0/IMemory.h>

using ::android::hardware::media::omx::V1_0::IOmx;
using ::android::hardware::media::omx::V1_0::IOmxObserver;
using ::android::hardware::media::omx::V1_0::IOmxNode;
using ::android::hardware::media::omx::V1_0::Message;
using ::android::hardware::media::omx::V1_0::CodecBuffer;
using ::android::hidl::allocator::V1_0::IAllocator;
using ::android::hidl::memory::V1_0::IMemory;
using ::android::hidl::memory::V1_0::IMapper;
using ::android::hardware::Return;
using ::android::hardware::Void;
using ::android::hardware::hidl_vec;
using ::android::hardware::hidl_string;
using ::android::sp;

#include <VtsHalHidlTargetTestBase.h>
#include <hidlmemory/mapping.h>
#include <media_hidl_test_common.h>
#include <media_video_hidl_test_common.h>
#include <memory>

// allocate buffers needed on a component port
void allocatePortBuffers(sp<IOmxNode> omxNode,
                         android::Vector<BufferInfo>* buffArray,
                         OMX_U32 portIndex) {
    android::hardware::media::omx::V1_0::Status status;
    OMX_PARAM_PORTDEFINITIONTYPE portDef;

    buffArray->clear();

    sp<IAllocator> allocator = IAllocator::getService("ashmem");
    EXPECT_NE(allocator.get(), nullptr);

    status = getPortParam(omxNode, OMX_IndexParamPortDefinition, portIndex,
                          &portDef);
    ASSERT_EQ(status, ::android::hardware::media::omx::V1_0::Status::OK);

    for (size_t i = 0; i < portDef.nBufferCountActual; i++) {
        BufferInfo buffer;
        buffer.owner = client;
        buffer.omxBuffer.type = CodecBuffer::Type::SHARED_MEM;
        buffer.omxBuffer.attr.preset.rangeOffset = 0;
        buffer.omxBuffer.attr.preset.rangeLength = 0;
        bool success = false;
        allocator->allocate(
            portDef.nBufferSize,
            [&success, &buffer](bool _s,
                                ::android::hardware::hidl_memory const& mem) {
                success = _s;
                buffer.omxBuffer.sharedMemory = mem;
            });
        ASSERT_EQ(success, true);
        ASSERT_EQ(buffer.omxBuffer.sharedMemory.size(), portDef.nBufferSize);
        buffer.mMemory = mapMemory(buffer.omxBuffer.sharedMemory);
        ASSERT_NE(buffer.mMemory, nullptr);
        omxNode->useBuffer(
            portIndex, buffer.omxBuffer,
            [&status, &buffer](android::hardware::media::omx::V1_0::Status _s,
                               uint32_t id) {
                status = _s;
                buffer.id = id;
            });
        buffArray->push(buffer);
        ASSERT_EQ(status, ::android::hardware::media::omx::V1_0::Status::OK);
    }
}

// State Transition : Loaded -> Idle
// Note: This function does not make any background checks for this transition.
// The callee holds the reponsibility to ensure the legality of the transition.
void changeStateLoadedtoIdle(sp<IOmxNode> omxNode, sp<CodecObserver> observer,
                             android::Vector<BufferInfo>* iBuffer,
                             android::Vector<BufferInfo>* oBuffer,
                             OMX_U32 kPortIndexInput,
                             OMX_U32 kPortIndexOutput) {
    android::hardware::media::omx::V1_0::Status status;
    Message msg;

    // set state to idle
    status = omxNode->sendCommand(toRawCommandType(OMX_CommandStateSet),
                                  OMX_StateIdle);
    ASSERT_EQ(status, android::hardware::media::omx::V1_0::Status::OK);

    // Dont switch states until the ports are populated
    status = observer->dequeueMessage(&msg, DEFAULT_TIMEOUT, iBuffer, oBuffer);
    ASSERT_EQ(status, android::hardware::media::omx::V1_0::Status::TIMED_OUT);

    // allocate buffers on input port
    allocatePortBuffers(omxNode, iBuffer, kPortIndexInput);

    // Dont switch states until the ports are populated
    status = observer->dequeueMessage(&msg, DEFAULT_TIMEOUT, iBuffer, oBuffer);
    ASSERT_EQ(status, android::hardware::media::omx::V1_0::Status::TIMED_OUT);

    // allocate buffers on output port
    allocatePortBuffers(omxNode, oBuffer, kPortIndexOutput);

    // As the ports are populated, check if the state transition is complete
    status = observer->dequeueMessage(&msg, DEFAULT_TIMEOUT, iBuffer, oBuffer);
    ASSERT_EQ(status, android::hardware::media::omx::V1_0::Status::OK);
    ASSERT_EQ(msg.type, Message::Type::EVENT);
    ASSERT_EQ(msg.data.eventData.event, OMX_EventCmdComplete);
    ASSERT_EQ(msg.data.eventData.data1, OMX_CommandStateSet);
    ASSERT_EQ(msg.data.eventData.data2, OMX_StateIdle);

    return;
}

// State Transition : Idle -> Loaded
// Note: This function does not make any background checks for this transition.
// The callee holds the reponsibility to ensure the legality of the transition.
void changeStateIdletoLoaded(sp<IOmxNode> omxNode, sp<CodecObserver> observer,
                             android::Vector<BufferInfo>* iBuffer,
                             android::Vector<BufferInfo>* oBuffer,
                             OMX_U32 kPortIndexInput,
                             OMX_U32 kPortIndexOutput) {
    android::hardware::media::omx::V1_0::Status status;
    Message msg;

    // set state to Loaded
    status = omxNode->sendCommand(toRawCommandType(OMX_CommandStateSet),
                                  OMX_StateLoaded);
    ASSERT_EQ(status, android::hardware::media::omx::V1_0::Status::OK);

    // dont change state until all buffers are freed
    status = observer->dequeueMessage(&msg, DEFAULT_TIMEOUT, iBuffer, oBuffer);
    ASSERT_EQ(status, android::hardware::media::omx::V1_0::Status::TIMED_OUT);

    for (size_t i = 0; i < iBuffer->size(); ++i) {
        status = omxNode->freeBuffer(kPortIndexInput, (*iBuffer)[i].id);
        ASSERT_EQ(status, android::hardware::media::omx::V1_0::Status::OK);
    }

    // dont change state until all buffers are freed
    status = observer->dequeueMessage(&msg, DEFAULT_TIMEOUT, iBuffer, oBuffer);
    ASSERT_EQ(status, android::hardware::media::omx::V1_0::Status::TIMED_OUT);

    for (size_t i = 0; i < oBuffer->size(); ++i) {
        status = omxNode->freeBuffer(kPortIndexOutput, (*oBuffer)[i].id);
        ASSERT_EQ(status, android::hardware::media::omx::V1_0::Status::OK);
    }

    status = observer->dequeueMessage(&msg, DEFAULT_TIMEOUT, iBuffer, oBuffer);
    ASSERT_EQ(status, android::hardware::media::omx::V1_0::Status::OK);
    ASSERT_EQ(msg.type, Message::Type::EVENT);
    ASSERT_EQ(msg.data.eventData.event, OMX_EventCmdComplete);
    ASSERT_EQ(msg.data.eventData.data1, OMX_CommandStateSet);
    ASSERT_EQ(msg.data.eventData.data2, OMX_StateLoaded);

    return;
}

// State Transition : Idle -> Execute
// Note: This function does not make any background checks for this transition.
// The callee holds the reponsibility to ensure the legality of the transition.
void changeStateIdletoExecute(sp<IOmxNode> omxNode,
                              sp<CodecObserver> observer) {
    android::hardware::media::omx::V1_0::Status status;
    Message msg;

    // set state to execute
    status = omxNode->sendCommand(toRawCommandType(OMX_CommandStateSet),
                                  OMX_StateExecuting);
    ASSERT_EQ(status, android::hardware::media::omx::V1_0::Status::OK);
    status = observer->dequeueMessage(&msg, DEFAULT_TIMEOUT);
    ASSERT_EQ(status, android::hardware::media::omx::V1_0::Status::OK);
    ASSERT_EQ(msg.type, Message::Type::EVENT);
    ASSERT_EQ(msg.data.eventData.event, OMX_EventCmdComplete);
    ASSERT_EQ(msg.data.eventData.data1, OMX_CommandStateSet);
    ASSERT_EQ(msg.data.eventData.data2, OMX_StateExecuting);

    return;
}

// State Transition : Execute -> Idle
// Note: This function does not make any background checks for this transition.
// The callee holds the reponsibility to ensure the legality of the transition.
void changeStateExecutetoIdle(sp<IOmxNode> omxNode, sp<CodecObserver> observer,
                              android::Vector<BufferInfo>* iBuffer,
                              android::Vector<BufferInfo>* oBuffer) {
    android::hardware::media::omx::V1_0::Status status;
    Message msg;

    // set state to Idle
    status = omxNode->sendCommand(toRawCommandType(OMX_CommandStateSet),
                                  OMX_StateIdle);
    ASSERT_EQ(status, android::hardware::media::omx::V1_0::Status::OK);
    status = observer->dequeueMessage(&msg, DEFAULT_TIMEOUT, iBuffer, oBuffer);
    ASSERT_EQ(status, android::hardware::media::omx::V1_0::Status::OK);
    ASSERT_EQ(msg.type, Message::Type::EVENT);
    ASSERT_EQ(msg.data.eventData.event, OMX_EventCmdComplete);
    ASSERT_EQ(msg.data.eventData.data1, OMX_CommandStateSet);
    ASSERT_EQ(msg.data.eventData.data2, OMX_StateIdle);

    // test if client got all its buffers back
    for (size_t i = 0; i < oBuffer->size(); ++i) {
        EXPECT_EQ((*oBuffer)[i].owner, client);
    }
    for (size_t i = 0; i < iBuffer->size(); ++i) {
        EXPECT_EQ((*iBuffer)[i].owner, client);
    }
}

// get empty buffer index
size_t getEmptyBufferID(android::Vector<BufferInfo>* buffArray) {
    for (size_t i = 0; i < buffArray->size(); i++) {
        if ((*buffArray)[i].owner == client) return i;
    }
    return buffArray->size();
}

// dispatch buffer to output port
void dispatchOutputBuffer(sp<IOmxNode> omxNode,
                          android::Vector<BufferInfo>* buffArray,
                          size_t bufferIndex) {
    android::hardware::media::omx::V1_0::Status status;
    CodecBuffer t;
    t.sharedMemory = android::hardware::hidl_memory();
    t.nativeHandle = android::hardware::hidl_handle();
    t.type = CodecBuffer::Type::PRESET;
    t.attr.preset.rangeOffset = 0;
    t.attr.preset.rangeLength = 0;
    native_handle_t* fenceNh = native_handle_create(0, 0);
    ASSERT_NE(fenceNh, nullptr);
    status = omxNode->fillBuffer((*buffArray)[bufferIndex].id, t, fenceNh);
    native_handle_close(fenceNh);
    native_handle_delete(fenceNh);
    ASSERT_EQ(status, android::hardware::media::omx::V1_0::Status::OK);
    buffArray->editItemAt(bufferIndex).owner = component;
}

// dispatch buffer to input port
void dispatchInputBuffer(sp<IOmxNode> omxNode,
                         android::Vector<BufferInfo>* buffArray,
                         size_t bufferIndex, int bytesCount, uint32_t flags,
                         uint64_t timestamp) {
    android::hardware::media::omx::V1_0::Status status;
    CodecBuffer t;
    t.sharedMemory = android::hardware::hidl_memory();
    t.nativeHandle = android::hardware::hidl_handle();
    t.type = CodecBuffer::Type::PRESET;
    t.attr.preset.rangeOffset = 0;
    t.attr.preset.rangeLength = bytesCount;
    native_handle_t* fenceNh = native_handle_create(0, 0);
    ASSERT_NE(fenceNh, nullptr);
    status = omxNode->emptyBuffer((*buffArray)[bufferIndex].id, t, flags,
                                  timestamp, fenceNh);
    native_handle_close(fenceNh);
    native_handle_delete(fenceNh);
    ASSERT_EQ(status, android::hardware::media::omx::V1_0::Status::OK);
    buffArray->editItemAt(bufferIndex).owner = component;
}

// Flush input and output ports
void flushPorts(sp<IOmxNode> omxNode, sp<CodecObserver> observer,
                android::Vector<BufferInfo>* iBuffer,
                android::Vector<BufferInfo>* oBuffer, OMX_U32 kPortIndexInput,
                OMX_U32 kPortIndexOutput) {
    android::hardware::media::omx::V1_0::Status status;
    Message msg;

    // Flush input port
    status = omxNode->sendCommand(toRawCommandType(OMX_CommandFlush),
                                  kPortIndexInput);
    ASSERT_EQ(status, android::hardware::media::omx::V1_0::Status::OK);
    status = observer->dequeueMessage(&msg, DEFAULT_TIMEOUT, iBuffer, oBuffer);
    ASSERT_EQ(status, android::hardware::media::omx::V1_0::Status::OK);
    ASSERT_EQ(msg.type, Message::Type::EVENT);
    ASSERT_EQ(msg.data.eventData.event, OMX_EventCmdComplete);
    ASSERT_EQ(msg.data.eventData.data1, OMX_CommandFlush);
    ASSERT_EQ(msg.data.eventData.data2, kPortIndexInput);
    // test if client got all its buffers back
    for (size_t i = 0; i < iBuffer->size(); ++i) {
        EXPECT_EQ((*iBuffer)[i].owner, client);
    }

    // Flush output port
    status = omxNode->sendCommand(toRawCommandType(OMX_CommandFlush),
                                  kPortIndexOutput);
    ASSERT_EQ(status, android::hardware::media::omx::V1_0::Status::OK);
    status = observer->dequeueMessage(&msg, DEFAULT_TIMEOUT, iBuffer, oBuffer);
    ASSERT_EQ(status, android::hardware::media::omx::V1_0::Status::OK);
    ASSERT_EQ(msg.type, Message::Type::EVENT);
    ASSERT_EQ(msg.data.eventData.event, OMX_EventCmdComplete);
    ASSERT_EQ(msg.data.eventData.data1, OMX_CommandFlush);
    ASSERT_EQ(msg.data.eventData.data2, kPortIndexOutput);
    // test if client got all its buffers back
    for (size_t i = 0; i < oBuffer->size(); ++i) {
        EXPECT_EQ((*oBuffer)[i].owner, client);
    }
}

Return<android::hardware::media::omx::V1_0::Status> setVideoPortFormat(
    sp<IOmxNode> omxNode, OMX_U32 portIndex,
    OMX_VIDEO_CODINGTYPE eCompressionFormat, OMX_COLOR_FORMATTYPE eColorFormat,
    OMX_U32 xFramerate) {
    OMX_U32 index = 0;
    OMX_VIDEO_PARAM_PORTFORMATTYPE portFormat;
    std::vector<OMX_COLOR_FORMATTYPE> arrColorFormat;
    std::vector<OMX_VIDEO_CODINGTYPE> arrCompressionFormat;
    android::hardware::media::omx::V1_0::Status status;

    while (1) {
        portFormat.nIndex = index;
        status = getPortParam(omxNode, OMX_IndexParamVideoPortFormat, portIndex,
                              &portFormat);
        if (status != ::android::hardware::media::omx::V1_0::Status::OK) break;
        if (eCompressionFormat == OMX_VIDEO_CodingUnused)
            arrColorFormat.push_back(portFormat.eColorFormat);
        else
            arrCompressionFormat.push_back(portFormat.eCompressionFormat);
        index++;
        if (index == 512) {
            // enumerated way too many formats, highly unusual for this to
            // happen.
            EXPECT_LE(index, 512U)
                << "Expecting OMX_ErrorNoMore but not received";
            break;
        }
    }
    if (!index) return status;
    if (eCompressionFormat == OMX_VIDEO_CodingUnused) {
        for (index = 0; index < arrColorFormat.size(); index++) {
            if (arrColorFormat[index] == eColorFormat) {
                portFormat.eColorFormat = arrColorFormat[index];
                break;
            }
        }
        if (index == arrColorFormat.size()) {
            ALOGI("setting default color format");
            portFormat.eColorFormat = arrColorFormat[0];
        }
        portFormat.eCompressionFormat = OMX_VIDEO_CodingUnused;
    } else {
        for (index = 0; index < arrCompressionFormat.size(); index++) {
            if (arrCompressionFormat[index] == eCompressionFormat) {
                portFormat.eCompressionFormat = arrCompressionFormat[index];
                break;
            }
        }
        if (index == arrCompressionFormat.size()) {
            ALOGI("setting default compression format");
            portFormat.eCompressionFormat = arrCompressionFormat[0];
        }
        portFormat.eColorFormat = OMX_COLOR_FormatUnused;
    }
    portFormat.nIndex = 0;
    portFormat.xFramerate = xFramerate;
    status = setPortParam(omxNode, OMX_IndexParamVideoPortFormat, portIndex,
                          &portFormat);
    return status;
}

Return<android::hardware::media::omx::V1_0::Status> setRole(
    sp<IOmxNode> omxNode, const char* role) {
    OMX_PARAM_COMPONENTROLETYPE params;
    strcpy((char*)params.cRole, role);
    return setParam(omxNode, OMX_IndexParamStandardComponentRole, &params);
}

void setupRAWPort(sp<IOmxNode> omxNode, OMX_U32 portIndex, OMX_U32 nFrameWidth,
                  OMX_U32 nFrameHeight, OMX_U32 nBitrate, OMX_U32 xFramerate,
                  OMX_COLOR_FORMATTYPE eColorFormat) {
    android::hardware::media::omx::V1_0::Status status;

    OMX_PARAM_PORTDEFINITIONTYPE portDef;
    status = getPortParam(omxNode, OMX_IndexParamPortDefinition, portIndex,
                          &portDef);
    EXPECT_EQ(status, ::android::hardware::media::omx::V1_0::Status::OK);
    portDef.format.video.nFrameWidth = nFrameWidth;
    portDef.format.video.nFrameHeight = nFrameHeight;
    portDef.format.video.nStride = (((nFrameWidth + 15) >> 4) << 4);
    portDef.format.video.nSliceHeight = (((nFrameHeight + 15) >> 4) << 4);
    portDef.format.video.nBitrate = nBitrate;
    portDef.format.video.xFramerate = xFramerate;
    portDef.format.video.bFlagErrorConcealment = OMX_TRUE;
    portDef.format.video.eCompressionFormat = OMX_VIDEO_CodingUnused;
    portDef.format.video.eColorFormat = eColorFormat;
    status = setPortParam(omxNode, OMX_IndexParamPortDefinition, portIndex,
                          &portDef);
    EXPECT_EQ(status, ::android::hardware::media::omx::V1_0::Status::OK);
}

void setupAVCPort(sp<IOmxNode> omxNode, OMX_U32 portIndex,
                  OMX_VIDEO_AVCPROFILETYPE eProfile,
                  OMX_VIDEO_AVCLEVELTYPE eLevel, OMX_U32 xFramerate) {
    android::hardware::media::omx::V1_0::Status status;
    OMX_VIDEO_PARAM_AVCTYPE param;
    (void)xFramerate;  // necessary for intra frame spacing

    status = getPortParam(omxNode, OMX_IndexParamVideoAvc, portIndex, &param);
    EXPECT_EQ(status, ::android::hardware::media::omx::V1_0::Status::OK);
    param.nSliceHeaderSpacing = 0;
    param.nPFrames = 0xFFFFFFFE;
    param.nBFrames = 0;
    param.bUseHadamard = OMX_TRUE;
    param.nRefFrames = 1;
    param.eProfile = eProfile;
    param.eLevel = eLevel;
    param.nAllowedPictureTypes =
        OMX_VIDEO_PictureTypeI | OMX_VIDEO_PictureTypeP;
    param.bFrameMBsOnly = OMX_TRUE;
    param.bEntropyCodingCABAC = OMX_FALSE;
    param.bWeightedPPrediction = OMX_FALSE;
    param.eLoopFilterMode = OMX_VIDEO_AVCLoopFilterEnable;
    status = setPortParam(omxNode, OMX_IndexParamVideoAvc, portIndex, &param);
    EXPECT_EQ(status, ::android::hardware::media::omx::V1_0::Status::OK);
}

void setupHEVCPort(sp<IOmxNode> omxNode, OMX_U32 portIndex,
                   OMX_VIDEO_HEVCPROFILETYPE eProfile,
                   OMX_VIDEO_HEVCLEVELTYPE eLevel) {
    android::hardware::media::omx::V1_0::Status status;
    OMX_VIDEO_PARAM_HEVCTYPE param;

    status = getPortParam(omxNode, (OMX_INDEXTYPE)OMX_IndexParamVideoHevc,
                          portIndex, &param);
    EXPECT_EQ(status, ::android::hardware::media::omx::V1_0::Status::OK);
    param.eProfile = eProfile;
    param.eLevel = eLevel;
    param.nKeyFrameInterval = 0xFFFFFFFE;
    status = setPortParam(omxNode, (OMX_INDEXTYPE)OMX_IndexParamVideoHevc,
                          portIndex, &param);
    EXPECT_EQ(status, ::android::hardware::media::omx::V1_0::Status::OK);
}
