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

#define LOG_TAG "media_omx_hidl_audio_test_common"
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
#include <media_audio_hidl_test_common.h>
#include <media_hidl_test_common.h>
#include <memory>

void allocatePortBuffers(sp<IOmxNode> omxNode,
                         android::Vector<BufferInfo>* buffArray,
                         OMX_U32 portIndex) {
    android::hardware::media::omx::V1_0::Status status;
    OMX_PARAM_PORTDEFINITIONTYPE portDef;

    buffArray->clear();

    sp<IAllocator> allocator = IAllocator::getService("ashmem");
    EXPECT_NE(allocator, nullptr);

    status = getPortParam(omxNode, OMX_IndexParamPortDefinition, portIndex,
                          &portDef);
    ASSERT_EQ(status, ::android::hardware::media::omx::V1_0::Status::OK);
    for (size_t i = 0; i < portDef.nBufferCountActual; i++) {
        BufferInfo buffer;
        buffer.owner = client;
        buffer.omxBuffer.type = CodecBuffer::Type::SHARED_MEM;
        buffer.omxBuffer.attr.preset.rangeOffset = 0;
        buffer.omxBuffer.attr.preset.rangeLength = 0;
        bool success;
        allocator->allocate(
            portDef.nBufferSize,
            [&success, &buffer](bool _s,
                                ::android::hardware::hidl_memory const& mem) {
                success = _s;
                buffer.omxBuffer.sharedMemory = mem;
            });
        ASSERT_EQ(success, true);
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

size_t getEmptyBufferID(android::Vector<BufferInfo>* buffArray) {
    for (size_t i = 0; i < buffArray->size(); i++) {
        if ((*buffArray)[i].owner == client) return i;
    }
    return buffArray->size();
}

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

Return<android::hardware::media::omx::V1_0::Status> setAudioPortFormat(
    sp<IOmxNode> omxNode, OMX_U32 portIndex, OMX_AUDIO_CODINGTYPE encoding) {
    OMX_U32 index = 0;
    OMX_AUDIO_PARAM_PORTFORMATTYPE portFormat;
    std::vector<OMX_AUDIO_CODINGTYPE> eEncoding;
    android::hardware::media::omx::V1_0::Status status;

    while (1) {
        portFormat.nIndex = index;
        status = getPortParam(omxNode, OMX_IndexParamAudioPortFormat, portIndex,
                              &portFormat);
        if (status != ::android::hardware::media::omx::V1_0::Status::OK) break;
        eEncoding.push_back(portFormat.eEncoding);
        index++;
        if (index == 512) {
            EXPECT_LE(index, 512U)
                << "Expecting OMX_ErrorNoMore but not received";
            break;
        }
    }
    if (!index) return status;
    for (index = 0; index < eEncoding.size(); index++) {
        if (eEncoding[index] == encoding) {
            portFormat.eEncoding = eEncoding[index];
            break;
        }
    }
    if (index == eEncoding.size()) {
        ALOGI("setting default Port format");
        portFormat.eEncoding = eEncoding[0];
    }
    // In setParam call nIndex shall be ignored as per omx-il specification.
    // see how this holds up by corrupting nIndex
    portFormat.nIndex = RANDOM_INDEX;
    status = setPortParam(omxNode, OMX_IndexParamAudioPortFormat, portIndex,
                          &portFormat);
    return status;
}

Return<android::hardware::media::omx::V1_0::Status> setRole(
    sp<IOmxNode> omxNode, const char* role) {
    OMX_PARAM_COMPONENTROLETYPE params;
    strcpy((char*)params.cRole, role);
    return setParam(omxNode, OMX_IndexParamStandardComponentRole, &params);
}

void setupPCMPort(sp<IOmxNode> omxNode, OMX_U32 portIndex, int32_t nChannels,
                  OMX_NUMERICALDATATYPE eNumData, int32_t nBitPerSample,
                  int32_t nSamplingRate) {
    OMX_AUDIO_PARAM_PCMMODETYPE param;
    android::hardware::media::omx::V1_0::Status status;
    status = getPortParam(omxNode, OMX_IndexParamAudioPcm, portIndex, &param);
    EXPECT_EQ(status, ::android::hardware::media::omx::V1_0::Status::OK);
    param.nChannels = nChannels;
    param.eNumData = eNumData;
    param.eEndian = OMX_EndianBig;
    param.bInterleaved = OMX_TRUE;
    param.nBitPerSample = nBitPerSample;
    param.nSamplingRate = nSamplingRate;
    param.ePCMMode = OMX_AUDIO_PCMModeLinear;
    switch (nChannels) {
        case 1:
            param.eChannelMapping[0] = OMX_AUDIO_ChannelCF;
            break;
        case 2:
            param.eChannelMapping[0] = OMX_AUDIO_ChannelLF;
            param.eChannelMapping[1] = OMX_AUDIO_ChannelRF;
            break;
        default:
            EXPECT_TRUE(false);
    }
    status = setPortParam(omxNode, OMX_IndexParamAudioPcm, portIndex, &param);
    ASSERT_EQ(status, ::android::hardware::media::omx::V1_0::Status::OK);
}

void setupMP3Port(sp<IOmxNode> omxNode, OMX_U32 portIndex,
                  OMX_AUDIO_MP3STREAMFORMATTYPE eFormat, int32_t nChannels,
                  int32_t nBitRate, int32_t nSampleRate, bool isEncoder) {
    if (isEncoder == false) return;
    OMX_AUDIO_PARAM_MP3TYPE param;
    android::hardware::media::omx::V1_0::Status status;
    status = getPortParam(omxNode, OMX_IndexParamAudioMp3, portIndex, &param);
    EXPECT_EQ(status, ::android::hardware::media::omx::V1_0::Status::OK);
    param.nChannels = nChannels;
    param.nBitRate = nBitRate;
    param.nSampleRate = nSampleRate;
    param.nAudioBandWidth = 0;
    param.eChannelMode = (nChannels == 1) ? OMX_AUDIO_ChannelModeMono
                                          : OMX_AUDIO_ChannelModeStereo;
    param.eFormat = eFormat;
    status = setPortParam(omxNode, OMX_IndexParamAudioMp3, portIndex, &param);
    ASSERT_EQ(status, ::android::hardware::media::omx::V1_0::Status::OK);
}

void setupFLACPort(sp<IOmxNode> omxNode, OMX_U32 portIndex, int32_t nChannels,
                   int32_t nSampleRate, int32_t nCompressionLevel,
                   bool isEncoder) {
    if (isEncoder == false) return;
    android::hardware::media::omx::V1_0::Status status;
    OMX_AUDIO_PARAM_FLACTYPE param;
    status = getPortParam(omxNode, OMX_IndexParamAudioFlac, portIndex, &param);
    EXPECT_EQ(status, ::android::hardware::media::omx::V1_0::Status::OK);
    param.nChannels = nChannels;
    param.nSampleRate = nSampleRate;
    param.nCompressionLevel = nCompressionLevel;
    status = setPortParam(omxNode, OMX_IndexParamAudioFlac, portIndex, &param);
    ASSERT_EQ(status, ::android::hardware::media::omx::V1_0::Status::OK);
}

void setupOPUSPort(sp<IOmxNode> omxNode, OMX_U32 portIndex, int32_t nChannels,
                   int32_t nBitRate, int32_t nSampleRate, bool isEncoder) {
    if (isEncoder == false) return;
    android::hardware::media::omx::V1_0::Status status;
    OMX_AUDIO_PARAM_ANDROID_OPUSTYPE param;
    status =
        getPortParam(omxNode, (OMX_INDEXTYPE)OMX_IndexParamAudioAndroidOpus,
                     portIndex, &param);
    EXPECT_EQ(status, ::android::hardware::media::omx::V1_0::Status::OK);
    param.nChannels = nChannels;
    param.nBitRate = nBitRate;
    param.nSampleRate = nSampleRate;
    status =
        setPortParam(omxNode, (OMX_INDEXTYPE)OMX_IndexParamAudioAndroidOpus,
                     portIndex, &param);
    ASSERT_EQ(status, ::android::hardware::media::omx::V1_0::Status::OK);
}

void setupAMRPort(sp<IOmxNode> omxNode, OMX_U32 portIndex, int32_t nBitRate,
                  OMX_AUDIO_AMRBANDMODETYPE eAMRBandMode, bool isEncoder) {
    if (isEncoder == false) return;
    android::hardware::media::omx::V1_0::Status status;
    OMX_AUDIO_PARAM_AMRTYPE param;
    status = getPortParam(omxNode, OMX_IndexParamAudioAmr, portIndex, &param);
    EXPECT_EQ(status, ::android::hardware::media::omx::V1_0::Status::OK);
    param.nChannels = 1;
    param.nBitRate = nBitRate;
    param.eAMRBandMode = eAMRBandMode;
    status = setPortParam(omxNode, OMX_IndexParamAudioAmr, portIndex, &param);
    ASSERT_EQ(status, ::android::hardware::media::omx::V1_0::Status::OK);
}

void setupVORBISPort(sp<IOmxNode> omxNode, OMX_U32 portIndex, int32_t nChannels,
                     int32_t nBitRate, int32_t nSampleRate, int32_t nQuality,
                     bool isEncoder) {
    if (isEncoder == false) return;
    android::hardware::media::omx::V1_0::Status status;
    OMX_AUDIO_PARAM_VORBISTYPE param;
    status =
        getPortParam(omxNode, OMX_IndexParamAudioVorbis, portIndex, &param);
    EXPECT_EQ(status, ::android::hardware::media::omx::V1_0::Status::OK);
    param.nChannels = nChannels;
    param.nBitRate = nBitRate;
    param.nSampleRate = nSampleRate;
    param.nQuality = nQuality;
    status =
        setPortParam(omxNode, OMX_IndexParamAudioVorbis, portIndex, &param);
    ASSERT_EQ(status, ::android::hardware::media::omx::V1_0::Status::OK);
}

void setupAACPort(sp<IOmxNode> omxNode, OMX_U32 portIndex,
                  OMX_AUDIO_AACPROFILETYPE eAACProfile,
                  OMX_AUDIO_AACSTREAMFORMATTYPE eAACStreamFormat,
                  int32_t nChannels, int32_t nBitRate, int32_t nSampleRate,
                  bool isEncoder) {
    if (isEncoder == false) return;
    android::hardware::media::omx::V1_0::Status status;
    OMX_AUDIO_PARAM_AACPROFILETYPE param;
    status = getPortParam(omxNode, OMX_IndexParamAudioAac, portIndex, &param);
    EXPECT_EQ(status, ::android::hardware::media::omx::V1_0::Status::OK);
    param.nChannels = nChannels;
    param.nSampleRate = nSampleRate;
    param.nBitRate = nBitRate;
    param.eAACProfile = eAACProfile;
    param.eAACStreamFormat = eAACStreamFormat;
    param.eChannelMode = (nChannels == 1) ? OMX_AUDIO_ChannelModeMono
                                          : OMX_AUDIO_ChannelModeStereo;
    status = setPortParam(omxNode, OMX_IndexParamAudioAac, portIndex, &param);
    ASSERT_EQ(status, ::android::hardware::media::omx::V1_0::Status::OK);
}
