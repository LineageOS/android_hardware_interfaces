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

#ifndef MEDIA_AUDIO_HIDL_TEST_COMMON_H
#define MEDIA_AUDIO_HIDL_TEST_COMMON_H

#include <media_hidl_test_common.h>
/*
 * Random Index used for monkey testing while get/set parameters
 */
#define RANDOM_INDEX 1729

/*
 * Common audio utils
 */
void allocatePortBuffers(sp<IOmxNode> omxNode,
                         android::Vector<BufferInfo>* buffArray,
                         OMX_U32 portIndex);

void changeStateLoadedtoIdle(sp<IOmxNode> omxNode, sp<CodecObserver> observer,
                             android::Vector<BufferInfo>* iBuffer,
                             android::Vector<BufferInfo>* oBuffer,
                             OMX_U32 kPortIndexInput, OMX_U32 kPortIndexOutput);

void changeStateIdletoLoaded(sp<IOmxNode> omxNode, sp<CodecObserver> observer,
                             android::Vector<BufferInfo>* iBuffer,
                             android::Vector<BufferInfo>* oBuffer,
                             OMX_U32 kPortIndexInput, OMX_U32 kPortIndexOutput);

void changeStateIdletoExecute(sp<IOmxNode> omxNode, sp<CodecObserver> observer);

void changeStateExecutetoIdle(sp<IOmxNode> omxNode, sp<CodecObserver> observer,
                              android::Vector<BufferInfo>* iBuffer,
                              android::Vector<BufferInfo>* oBuffer);

size_t getEmptyBufferID(android::Vector<BufferInfo>* buffArray);

void dispatchInputBuffer(sp<IOmxNode> omxNode,
                         android::Vector<BufferInfo>* buffArray,
                         size_t bufferIndex, int bytesCount, uint32_t flags,
                         uint64_t timestamp);

void dispatchOutputBuffer(sp<IOmxNode> omxNode,
                          android::Vector<BufferInfo>* buffArray,
                          size_t bufferIndex);

void flushPorts(sp<IOmxNode> omxNode, sp<CodecObserver> observer,
                android::Vector<BufferInfo>* iBuffer,
                android::Vector<BufferInfo>* oBuffer, OMX_U32 kPortIndexInput,
                OMX_U32 kPortIndexOutput);

Return<android::hardware::media::omx::V1_0::Status> setAudioPortFormat(
    sp<IOmxNode> omxNode, OMX_U32 portIndex, OMX_AUDIO_CODINGTYPE encoding);

Return<android::hardware::media::omx::V1_0::Status> setRole(
    sp<IOmxNode> omxNode, const char* role);

void setupPCMPort(sp<IOmxNode> omxNode, OMX_U32 portIndex, int32_t nChannels,
                  OMX_NUMERICALDATATYPE eNumData, int32_t nBitPerSample,
                  int32_t nSamplingRate);

void setupMP3Port(sp<IOmxNode> omxNode, OMX_U32 portIndex,
                  OMX_AUDIO_MP3STREAMFORMATTYPE eFormat, int32_t nChannels,
                  int32_t nBitRate, int32_t nSampleRate);

void setupFLACPort(sp<IOmxNode> omxNode, OMX_U32 portIndex, int32_t nChannels,
                   int32_t nSampleRate, int32_t nCompressionLevel);

void setupOPUSPort(sp<IOmxNode> omxNode, OMX_U32 portIndex, int32_t nChannels,
                   int32_t nBitRate, int32_t nSampleRate);

void setupAMRPort(sp<IOmxNode> omxNode, OMX_U32 portIndex, int32_t nBitRate,
                  bool isAMRWB);

void setupVORBISPort(sp<IOmxNode> omxNode, OMX_U32 portIndex, int32_t nChannels,
                     int32_t nBitRate, int32_t nSampleRate, int32_t nQuality);

void setupAACPort(sp<IOmxNode> omxNode, OMX_U32 portIndex,
                  OMX_AUDIO_AACPROFILETYPE eAACProfile,
                  OMX_AUDIO_AACSTREAMFORMATTYPE eAACStreamFormat,
                  int32_t nChannels, int32_t nBitRate, int32_t nSampleRate);

#endif  // MEDIA_AUDIO_HIDL_TEST_COMMON_H
