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

#ifdef __LP64__
#define OMX_ANDROID_COMPILE_AS_32BIT_ON_64BIT_PLATFORMS
#endif

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
using ::android::hardware::media::omx::V1_0::PortMode;
using ::android::hardware::media::omx::V1_0::Status;
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
#include <media/hardware/HardwareAPI.h>
#include <media_hidl_test_common.h>
#include <memory>

// set component role
Return<android::hardware::media::omx::V1_0::Status> setRole(
    sp<IOmxNode> omxNode, const char* role) {
    OMX_PARAM_COMPONENTROLETYPE params;
    strcpy((char*)params.cRole, role);
    return setParam(omxNode, OMX_IndexParamStandardComponentRole, &params);
}

// allocate buffers needed on a component port
void allocatePortBuffers(sp<IOmxNode> omxNode,
                         android::Vector<BufferInfo>* buffArray,
                         OMX_U32 portIndex, PortMode portMode) {
    android::hardware::media::omx::V1_0::Status status;
    OMX_PARAM_PORTDEFINITIONTYPE portDef;

    buffArray->clear();

    status = getPortParam(omxNode, OMX_IndexParamPortDefinition, portIndex,
                          &portDef);
    ASSERT_EQ(status, ::android::hardware::media::omx::V1_0::Status::OK);

    if (portMode == PortMode::PRESET_SECURE_BUFFER) {
        for (size_t i = 0; i < portDef.nBufferCountActual; i++) {
            BufferInfo buffer;
            buffer.owner = client;
            buffer.omxBuffer.type = CodecBuffer::Type::NATIVE_HANDLE;
            omxNode->allocateSecureBuffer(
                portIndex, portDef.nBufferSize,
                [&status, &buffer](
                    android::hardware::media::omx::V1_0::Status _s, uint32_t id,
                    ::android::hardware::hidl_handle const& nativeHandle) {
                    status = _s;
                    buffer.id = id;
                    buffer.omxBuffer.nativeHandle = nativeHandle;
                });
            buffArray->push(buffer);
            ASSERT_EQ(status,
                      ::android::hardware::media::omx::V1_0::Status::OK);
        }
    } else if (portMode == PortMode::PRESET_BYTE_BUFFER ||
               portMode == PortMode::DYNAMIC_ANW_BUFFER) {
        sp<IAllocator> allocator = IAllocator::getService("ashmem");
        EXPECT_NE(allocator.get(), nullptr);

        for (size_t i = 0; i < portDef.nBufferCountActual; i++) {
            BufferInfo buffer;
            buffer.owner = client;
            buffer.omxBuffer.type = CodecBuffer::Type::SHARED_MEM;
            buffer.omxBuffer.attr.preset.rangeOffset = 0;
            buffer.omxBuffer.attr.preset.rangeLength = 0;
            bool success = false;
            if (portMode != PortMode::PRESET_BYTE_BUFFER) {
                portDef.nBufferSize = sizeof(android::VideoNativeMetadata);
            }
            allocator->allocate(
                portDef.nBufferSize,
                [&success, &buffer](
                    bool _s, ::android::hardware::hidl_memory const& mem) {
                    success = _s;
                    buffer.omxBuffer.sharedMemory = mem;
                });
            ASSERT_EQ(success, true);
            ASSERT_EQ(buffer.omxBuffer.sharedMemory.size(),
                      portDef.nBufferSize);
            buffer.mMemory = mapMemory(buffer.omxBuffer.sharedMemory);
            ASSERT_NE(buffer.mMemory, nullptr);
            if (portMode == PortMode::DYNAMIC_ANW_BUFFER) {
                android::VideoNativeMetadata* metaData =
                    static_cast<android::VideoNativeMetadata*>(
                        static_cast<void*>(buffer.mMemory->getPointer()));
                metaData->nFenceFd = -1;
                buffer.slot = -1;
            }
            omxNode->useBuffer(
                portIndex, buffer.omxBuffer,
                [&status, &buffer](
                    android::hardware::media::omx::V1_0::Status _s,
                    uint32_t id) {
                    status = _s;
                    buffer.id = id;
                });
            buffArray->push(buffer);
            ASSERT_EQ(status,
                      ::android::hardware::media::omx::V1_0::Status::OK);
        }
    }
}

// State Transition : Loaded -> Idle
// Note: This function does not make any background checks for this transition.
// The callee holds the reponsibility to ensure the legality of the transition.
void changeStateLoadedtoIdle(sp<IOmxNode> omxNode, sp<CodecObserver> observer,
                             android::Vector<BufferInfo>* iBuffer,
                             android::Vector<BufferInfo>* oBuffer,
                             OMX_U32 kPortIndexInput, OMX_U32 kPortIndexOutput,
                             PortMode* portMode) {
    android::hardware::media::omx::V1_0::Status status;
    Message msg;
    PortMode defaultPortMode[2], *pm;

    defaultPortMode[0] = PortMode::PRESET_BYTE_BUFFER;
    defaultPortMode[1] = PortMode::PRESET_BYTE_BUFFER;
    pm = portMode ? portMode : defaultPortMode;

    // set state to idle
    status = omxNode->sendCommand(toRawCommandType(OMX_CommandStateSet),
                                  OMX_StateIdle);
    ASSERT_EQ(status, android::hardware::media::omx::V1_0::Status::OK);

    // Dont switch states until the ports are populated
    status = observer->dequeueMessage(&msg, DEFAULT_TIMEOUT, iBuffer, oBuffer);
    ASSERT_EQ(status, android::hardware::media::omx::V1_0::Status::TIMED_OUT);

    // allocate buffers on input port
    allocatePortBuffers(omxNode, iBuffer, kPortIndexInput, pm[0]);

    // Dont switch states until the ports are populated
    status = observer->dequeueMessage(&msg, DEFAULT_TIMEOUT, iBuffer, oBuffer);
    ASSERT_EQ(status, android::hardware::media::omx::V1_0::Status::TIMED_OUT);

    // allocate buffers on output port
    allocatePortBuffers(omxNode, oBuffer, kPortIndexOutput, pm[1]);

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
    android::Vector<BufferInfo>::iterator it = buffArray->begin();
    while (it != buffArray->end()) {
        if (it->owner == client) {
            // This block of code ensures that all buffers allocated at init
            // time are utilized
            BufferInfo backup = *it;
            buffArray->erase(it);
            buffArray->push_back(backup);
            return buffArray->size() - 1;
        }
        it++;
    }
    return buffArray->size();
}

// dispatch buffer to output port
void dispatchOutputBuffer(sp<IOmxNode> omxNode,
                          android::Vector<BufferInfo>* buffArray,
                          size_t bufferIndex, PortMode portMode) {
    android::hardware::media::omx::V1_0::Status status;
    CodecBuffer t;
    native_handle_t* fenceNh = native_handle_create(0, 0);
    ASSERT_NE(fenceNh, nullptr);
    switch (portMode) {
        case PortMode::DYNAMIC_ANW_BUFFER:
            t = (*buffArray)[bufferIndex].omxBuffer;
            t.type = CodecBuffer::Type::ANW_BUFFER;
            status =
                omxNode->fillBuffer((*buffArray)[bufferIndex].id, t, fenceNh);
            break;
        case PortMode::PRESET_SECURE_BUFFER:
        case PortMode::PRESET_BYTE_BUFFER:
            t.sharedMemory = android::hardware::hidl_memory();
            t.nativeHandle = android::hardware::hidl_handle();
            t.type = CodecBuffer::Type::PRESET;
            t.attr.preset.rangeOffset = 0;
            t.attr.preset.rangeLength = 0;
            status =
                omxNode->fillBuffer((*buffArray)[bufferIndex].id, t, fenceNh);
            break;
        default:
            status = Status::NAME_NOT_FOUND;
    }
    native_handle_close(fenceNh);
    native_handle_delete(fenceNh);
    ASSERT_EQ(status, android::hardware::media::omx::V1_0::Status::OK);
    buffArray->editItemAt(bufferIndex).owner = component;
}

// dispatch buffer to input port
void dispatchInputBuffer(sp<IOmxNode> omxNode,
                         android::Vector<BufferInfo>* buffArray,
                         size_t bufferIndex, int bytesCount, uint32_t flags,
                         uint64_t timestamp, PortMode portMode) {
    android::hardware::media::omx::V1_0::Status status;
    CodecBuffer t;
    native_handle_t* fenceNh = native_handle_create(0, 0);
    ASSERT_NE(fenceNh, nullptr);
    switch (portMode) {
        case PortMode::PRESET_SECURE_BUFFER:
        case PortMode::PRESET_BYTE_BUFFER:
            t.sharedMemory = android::hardware::hidl_memory();
            t.nativeHandle = android::hardware::hidl_handle();
            t.type = CodecBuffer::Type::PRESET;
            t.attr.preset.rangeOffset = 0;
            t.attr.preset.rangeLength = bytesCount;
            status = omxNode->emptyBuffer((*buffArray)[bufferIndex].id, t,
                                          flags, timestamp, fenceNh);
            break;
        default:
            status = Status::NAME_NOT_FOUND;
    }
    native_handle_close(fenceNh);
    native_handle_delete(fenceNh);
    ASSERT_EQ(status, android::hardware::media::omx::V1_0::Status::OK);
    buffArray->editItemAt(bufferIndex).owner = component;
}

// Flush input and output ports
void flushPorts(sp<IOmxNode> omxNode, sp<CodecObserver> observer,
                android::Vector<BufferInfo>* iBuffer,
                android::Vector<BufferInfo>* oBuffer, OMX_U32 kPortIndexInput,
                OMX_U32 kPortIndexOutput, int64_t timeoutUs) {
    android::hardware::media::omx::V1_0::Status status;
    Message msg;

    // Flush input port
    status = omxNode->sendCommand(toRawCommandType(OMX_CommandFlush),
                                  kPortIndexInput);
    ASSERT_EQ(status, android::hardware::media::omx::V1_0::Status::OK);
    status = observer->dequeueMessage(&msg, timeoutUs, iBuffer, oBuffer);
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
    status = observer->dequeueMessage(&msg, timeoutUs, iBuffer, oBuffer);
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

// dispatch an empty input buffer with eos flag set if requested.
// This call assumes that all input buffers are processed completely.
// feed output buffers till we receive a buffer with eos flag set
void testEOS(sp<IOmxNode> omxNode, sp<CodecObserver> observer,
             android::Vector<BufferInfo>* iBuffer,
             android::Vector<BufferInfo>* oBuffer, bool signalEOS,
             bool& eosFlag, PortMode* portMode) {
    android::hardware::media::omx::V1_0::Status status;
    PortMode defaultPortMode[2], *pm;

    defaultPortMode[0] = PortMode::PRESET_BYTE_BUFFER;
    defaultPortMode[1] = PortMode::PRESET_BYTE_BUFFER;
    pm = portMode ? portMode : defaultPortMode;

    size_t i = 0;
    if (signalEOS) {
        if ((i = getEmptyBufferID(iBuffer)) < iBuffer->size()) {
            // signal an empty buffer with flag set to EOS
            dispatchInputBuffer(omxNode, iBuffer, i, 0, OMX_BUFFERFLAG_EOS, 0);
        } else {
            ASSERT_TRUE(false);
        }
    }

    int timeOut = TIMEOUT_COUNTER;
    while (timeOut--) {
        // Dispatch all client owned output buffers to recover remaining frames
        while (1) {
            if ((i = getEmptyBufferID(oBuffer)) < oBuffer->size()) {
                dispatchOutputBuffer(omxNode, oBuffer, i, pm[1]);
                // if dispatch is successful, perhaps there is a latency
                // in the component. Dont be in a haste to leave. reset timeout
                // counter
                timeOut = TIMEOUT_COUNTER;
            } else {
                break;
            }
        }

        Message msg;
        status =
            observer->dequeueMessage(&msg, DEFAULT_TIMEOUT, iBuffer, oBuffer);
        if (status == android::hardware::media::omx::V1_0::Status::OK) {
            if (msg.data.eventData.event == OMX_EventBufferFlag) {
                // soft omx components donot send this, we will just ignore it
                // for now
            } else {
                // something unexpected happened
                EXPECT_TRUE(false);
            }
        }
        if (eosFlag == true) break;
    }
    // test for flag
    EXPECT_EQ(eosFlag, true);
    eosFlag = false;
}
