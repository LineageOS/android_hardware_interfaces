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

#ifndef MEDIA_VIDEO_HIDL_TEST_COMMON_H
#define MEDIA_VIDEO_HIDL_TEST_COMMON_H

/*
 * Random Index used for monkey testing while get/set parameters
 */
#define RANDOM_INDEX 1729

/*
 * Common video utils
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

void dispatchOutputBuffer(sp<IOmxNode> omxNode,
                          android::Vector<BufferInfo>* buffArray,
                          size_t bufferIndex);

void dispatchInputBuffer(sp<IOmxNode> omxNode,
                         android::Vector<BufferInfo>* buffArray,
                         size_t bufferIndex, int bytesCount, uint32_t flags,
                         uint64_t timestamp);

void flushPorts(sp<IOmxNode> omxNode, sp<CodecObserver> observer,
                android::Vector<BufferInfo>* iBuffer,
                android::Vector<BufferInfo>* oBuffer, OMX_U32 kPortIndexInput,
                OMX_U32 kPortIndexOutput, int64_t timeoutUs = DEFAULT_TIMEOUT);

Return<android::hardware::media::omx::V1_0::Status> setVideoPortFormat(
    sp<IOmxNode> omxNode, OMX_U32 portIndex,
    OMX_VIDEO_CODINGTYPE eCompressionFormat, OMX_COLOR_FORMATTYPE eColorFormat,
    OMX_U32 xFramerate);

Return<android::hardware::media::omx::V1_0::Status> setRole(
    sp<IOmxNode> omxNode, const char* role);

void setupRAWPort(sp<IOmxNode> omxNode, OMX_U32 portIndex, OMX_U32 nFrameWidth,
                  OMX_U32 nFrameHeight, OMX_U32 nBitrate, OMX_U32 xFramerate,
                  OMX_COLOR_FORMATTYPE eColorFormat);

void setupAVCPort(sp<IOmxNode> omxNode, OMX_U32 portIndex,
                  OMX_VIDEO_AVCPROFILETYPE eProfile,
                  OMX_VIDEO_AVCLEVELTYPE eLevel, OMX_U32 xFramerate);

void setupHEVCPort(sp<IOmxNode> omxNode, OMX_U32 portIndex,
                   OMX_VIDEO_HEVCPROFILETYPE eProfile,
                   OMX_VIDEO_HEVCLEVELTYPE eLevel);

#endif  // MEDIA_VIDEO_HIDL_TEST_COMMON_H
