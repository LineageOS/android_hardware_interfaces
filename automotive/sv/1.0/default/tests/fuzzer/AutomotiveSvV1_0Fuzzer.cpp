/*
 * Copyright (C) 2022 The Android Open Source Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at:
 *
 * http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 *
 */

#include "AutomotiveSvV1_0Fuzzer.h"
#include <SurroundViewStream.h>
#include <android/hidl/allocator/1.0/IAllocator.h>
#include <hidlmemory/mapping.h>

namespace android::hardware::automotive::sv::V1_0::implementation::fuzzer {

using ::android::hardware::hidl_memory;
using ::android::hardware::hidl_string;
using ::android::hardware::hidl_vec;
using ::android::hidl::allocator::V1_0::IAllocator;

constexpr uint32_t kMinConfigDimension = 0;
constexpr uint32_t kMaxConfigDimension = 4096;
constexpr uint32_t kVertexByteSize = (3 * sizeof(float)) + 4;
constexpr uint32_t kIdByteSize = 2;
constexpr size_t kMaxCharacters = 30;
constexpr size_t kMaxVertices = 10;
constexpr size_t kMaxCameraPoints = 10;
constexpr size_t kMaxViews = 10;
constexpr size_t kMaxOverlays = 10;
constexpr size_t kMinSvBuffers = 0;
constexpr size_t kMaxSvBuffers = 10;

void SurroundViewFuzzer::invoke2dSessionAPI() {
    sp<ISurroundView2dSession> surroundView2dSession;
    sp<SurroundViewStream> handler;
    mSurroundViewService->start2dSession(
            [&surroundView2dSession](const sp<ISurroundView2dSession>& session, SvResult result) {
                if (result == SvResult::OK) {
                    surroundView2dSession = session;
                }
            });

    if (surroundView2dSession && !mIs2dStreamStarted) {
        handler = sp<SurroundViewStream>::make(surroundView2dSession);
        if (surroundView2dSession->startStream(handler) == SvResult::OK) {
            mIs2dStreamStarted = true;
        }
    }

    while (mFuzzedDataProvider.remaining_bytes() > 0) {
        auto surroundView2dFunc = mFuzzedDataProvider.PickValueInArray<
                const std::function<void()>>({
                [&]() {
                    if (surroundView2dSession) {
                        surroundView2dSession->get2dMappingInfo(
                                []([[maybe_unused]] Sv2dMappingInfo info) {});
                    }
                },
                [&]() {
                    if (surroundView2dSession && mIs2dStreamStarted) {
                        Sv2dConfig config;
                        config.width = mFuzzedDataProvider.ConsumeIntegralInRange<uint32_t>(
                                kMinConfigDimension, kMaxConfigDimension);
                        if (mFuzzedDataProvider.ConsumeBool()) {
                            config.blending = static_cast<SvQuality>(
                                    mFuzzedDataProvider.ConsumeIntegral<uint32_t>());
                        } else {
                            config.blending = mFuzzedDataProvider.ConsumeBool() ? (SvQuality::HIGH)
                                                                                : (SvQuality::LOW);
                        }
                        surroundView2dSession->set2dConfig(config);
                    }
                },
                [&]() {
                    if (surroundView2dSession) {
                        surroundView2dSession->get2dConfig([&](Sv2dConfig) {});
                    }
                },
                [&]() {
                    if (surroundView2dSession) {
                        hidl_vec<Point2dInt> points2dCamera;
                        const size_t camPoints = mFuzzedDataProvider.ConsumeIntegralInRange<size_t>(
                                1, kMaxCameraPoints);
                        points2dCamera.resize(camPoints);
                        for (size_t i = 0; i < camPoints; ++i) {
                            points2dCamera[i].x = mFuzzedDataProvider.ConsumeFloatingPoint<float>();
                            points2dCamera[i].y = mFuzzedDataProvider.ConsumeFloatingPoint<float>();
                        }

                        hidl_vec<hidl_string> cameraIds;
                        mSurroundViewService->getCameraIds(
                                [&cameraIds](const hidl_vec<hidl_string>& camIds) {
                                    cameraIds = camIds;
                                });
                        hidl_string cameraId;
                        if (cameraIds.size() > 0 && mFuzzedDataProvider.ConsumeBool()) {
                            const size_t cameraIndex =
                                    mFuzzedDataProvider.ConsumeIntegralInRange<size_t>(
                                            0, cameraIds.size() - 1);
                            cameraId = cameraIds[cameraIndex];
                        } else {
                            cameraId =
                                    mFuzzedDataProvider.ConsumeRandomLengthString(kMaxCharacters);
                        }
                        surroundView2dSession->projectCameraPoints(
                                points2dCamera, cameraId,
                                []([[maybe_unused]] const hidl_vec<Point2dFloat>& outPoints) {});
                    }
                },
                [&]() {
                    if (surroundView2dSession) {
                        SvFramesDesc frames;
                        frames.timestampNs = mFuzzedDataProvider.ConsumeIntegral<uint64_t>();
                        frames.sequenceId = mFuzzedDataProvider.ConsumeIntegral<uint32_t>();
                        size_t numSvBuffers = mFuzzedDataProvider.ConsumeIntegralInRange<size_t>(
                                kMinSvBuffers, kMaxSvBuffers);
                        frames.svBuffers.resize(numSvBuffers);
                        for (int i = 0; i < numSvBuffers; ++i) {
                            frames.svBuffers[i].viewId =
                                    mFuzzedDataProvider.ConsumeIntegral<uint32_t>();
                            frames.svBuffers[i].hardwareBuffer.nativeHandle = new native_handle_t();
                            frames.svBuffers[i].hardwareBuffer.description[0] =
                                    mFuzzedDataProvider.ConsumeIntegral<uint32_t>();
                            frames.svBuffers[i].hardwareBuffer.description[1] =
                                    mFuzzedDataProvider.ConsumeIntegral<uint32_t>();
                        }
                        surroundView2dSession->doneWithFrames(frames);
                        for (int i = 0; i < numSvBuffers; ++i) {
                            delete frames.svBuffers[i].hardwareBuffer.nativeHandle;
                        }
                    }
                },
                [&]() {
                    if (surroundView2dSession) {
                        surroundView2dSession->stopStream();
                        mIs2dStreamStarted = false;
                    }
                },
                [&]() {
                    SvResult result = mSurroundViewService->stop2dSession(
                            mFuzzedDataProvider.ConsumeBool() ? surroundView2dSession : nullptr);
                    if (result == SvResult::OK) {
                        mIs2dStreamStarted = false;
                    }
                },
        });
        surroundView2dFunc();
    }

    if (surroundView2dSession && mIs2dStreamStarted) {
        surroundView2dSession->stopStream();
    }

    if (surroundView2dSession) {
        mSurroundViewService->stop2dSession(surroundView2dSession);
    }
}

void SurroundViewFuzzer::invoke3dSessionAPI() {
    sp<ISurroundView3dSession> surroundView3dSession;
    sp<SurroundViewStream> handler;
    mSurroundViewService->start3dSession(
            [&surroundView3dSession](const sp<ISurroundView3dSession>& session, SvResult result) {
                if (result == SvResult::OK) {
                    surroundView3dSession = session;
                }
            });

    const size_t numViews = mFuzzedDataProvider.ConsumeIntegralInRange<size_t>(1, kMaxViews);
    std::vector<View3d> views(numViews);
    for (size_t i = 0; i < numViews; ++i) {
        views[i].viewId = mFuzzedDataProvider.ConsumeIntegral<uint32_t>();
    }
    surroundView3dSession->setViews(views);

    if (surroundView3dSession) {
        handler = sp<SurroundViewStream>::make(surroundView3dSession);

        if (surroundView3dSession->startStream(handler) == SvResult::OK) {
            mIs3dStreamStarted = true;
        }
    }
    while (mFuzzedDataProvider.remaining_bytes() > 0) {
        auto surroundView3dFunc = mFuzzedDataProvider.PickValueInArray<
                const std::function<void()>>({
                [&]() {
                    if (surroundView3dSession) {
                        const size_t numViews =
                                mFuzzedDataProvider.ConsumeIntegralInRange<size_t>(1, kMaxViews);
                        std::vector<View3d> views(numViews);
                        for (size_t i = 0; i < numViews; ++i) {
                            views[i].viewId = mFuzzedDataProvider.ConsumeIntegral<uint32_t>();
                        }
                        surroundView3dSession->setViews(views);
                    }
                },
                [&]() {
                    if (surroundView3dSession && mIs3dStreamStarted) {
                        Sv3dConfig config;
                        config.width = mFuzzedDataProvider.ConsumeIntegralInRange<uint32_t>(
                                kMinConfigDimension, kMaxConfigDimension);
                        config.height = mFuzzedDataProvider.ConsumeIntegralInRange<uint32_t>(
                                kMinConfigDimension, kMaxConfigDimension);
                        if (mFuzzedDataProvider.ConsumeBool()) {
                            config.carDetails = static_cast<SvQuality>(
                                    mFuzzedDataProvider.ConsumeIntegral<uint32_t>());
                        } else {
                            config.carDetails = mFuzzedDataProvider.ConsumeBool()
                                                        ? (SvQuality::HIGH)
                                                        : (SvQuality::LOW);
                        }
                        surroundView3dSession->set3dConfig(config);
                    }
                },
                [&]() {
                    if (surroundView3dSession) {
                        surroundView3dSession->get3dConfig([&](Sv3dConfig) {});
                    }
                },
                [&]() {
                    if (surroundView3dSession) {
                        Point2dInt cameraPoint;
                        cameraPoint.x = mFuzzedDataProvider.ConsumeFloatingPoint<float>();
                        cameraPoint.y = mFuzzedDataProvider.ConsumeFloatingPoint<float>();
                        std::vector<Point2dInt> cameraPoints = {cameraPoint};
                        hidl_vec<hidl_string> cameraIds;
                        mSurroundViewService->getCameraIds(
                                [&cameraIds](const hidl_vec<hidl_string>& camIds) {
                                    cameraIds = camIds;
                                });
                        hidl_string cameraId;
                        if (cameraIds.size() > 0 && mFuzzedDataProvider.ConsumeBool()) {
                            const size_t cameraIndex =
                                    mFuzzedDataProvider.ConsumeIntegralInRange<size_t>(
                                            0, cameraIds.size() - 1);
                            cameraId = cameraIds[cameraIndex];
                        } else {
                            cameraId =
                                    mFuzzedDataProvider.ConsumeRandomLengthString(kMaxCharacters);
                        }
                        std::vector<Point3dFloat> points3d;
                        surroundView3dSession->projectCameraPointsTo3dSurface(
                                cameraPoints, cameraId,
                                [&points3d]([[maybe_unused]] const hidl_vec<Point3dFloat>&
                                                    points3dproj) { points3d = points3dproj; });
                    }
                },
                [&]() {
                    if (surroundView3dSession) {
                        // success case
                        surroundView3dSession->updateOverlays(mOverlaysdata);
                    }
                },
                [&]() {
                    if (surroundView3dSession) {
                        initSampleOverlaysData();
                        // Fail with ID mismatch
                        // Set id of second overlay in shared memory to 2 (expected is 1).
                        auto& overlaysDescVector = mOverlaysdata.overlaysMemoryDesc;
                        auto& pIMemory = mMemory;
                        int32_t indexPosition = mFuzzedDataProvider.ConsumeIntegralInRange<int32_t>(
                                0, mNumOverlays - 1);
                        int32_t mismatchedValueIndex =
                                mFuzzedDataProvider.ConsumeIntegralInRange<int32_t>(
                                        0, mNumOverlays - 1);
                        setIndexOfOverlaysMemory(overlaysDescVector, pIMemory, indexPosition,
                                                 overlaysDescVector[mismatchedValueIndex].id);
                        surroundView3dSession->updateOverlays(mOverlaysdata);
                    }
                },
                [&]() {
                    if (surroundView3dSession) {
                        // Fail with NULL memory
                        // Set shared memory to null.
                        mOverlaysdata.overlaysMemory = hidl_memory();
                        surroundView3dSession->updateOverlays(mOverlaysdata);
                    }
                },
                [&]() {
                    if (surroundView3dSession) {
                        SvFramesDesc frames;
                        frames.timestampNs = mFuzzedDataProvider.ConsumeIntegral<uint64_t>();
                        frames.sequenceId = mFuzzedDataProvider.ConsumeIntegral<uint32_t>();
                        size_t numSvBuffers = mFuzzedDataProvider.ConsumeIntegralInRange<size_t>(
                                kMinSvBuffers, kMaxSvBuffers);
                        frames.svBuffers.resize(numSvBuffers);
                        for (int i = 0; i < numSvBuffers; ++i) {
                            frames.svBuffers[i].viewId =
                                    mFuzzedDataProvider.ConsumeIntegral<uint32_t>();
                            frames.svBuffers[i].hardwareBuffer.nativeHandle = new native_handle_t();
                            frames.svBuffers[i].hardwareBuffer.description[0] =
                                    mFuzzedDataProvider.ConsumeIntegral<uint32_t>();
                            frames.svBuffers[i].hardwareBuffer.description[1] =
                                    mFuzzedDataProvider.ConsumeIntegral<uint32_t>();
                        }
                        surroundView3dSession->doneWithFrames(frames);
                        for (int i = 0; i < numSvBuffers; ++i) {
                            delete frames.svBuffers[i].hardwareBuffer.nativeHandle;
                        }
                    }
                },
                [&]() {
                    if (surroundView3dSession) {
                        surroundView3dSession->stopStream();
                        mIs3dStreamStarted = false;
                    }
                },
                [&]() {
                    SvResult result = mSurroundViewService->stop3dSession(
                            mFuzzedDataProvider.ConsumeBool() ? surroundView3dSession : nullptr);
                    if (result == SvResult::OK) {
                        mIs3dStreamStarted = false;
                    }
                },
        });
        surroundView3dFunc();
    }
    if (surroundView3dSession && mIs3dStreamStarted) {
        surroundView3dSession->stopStream();
    }

    if (surroundView3dSession) {
        mSurroundViewService->stop3dSession(surroundView3dSession);
    }
}

void SurroundViewFuzzer::process() {
    mFuzzedDataProvider.ConsumeBool() ? invoke2dSessionAPI() : invoke3dSessionAPI();
}

std::pair<hidl_memory, sp<IMemory>> SurroundViewFuzzer::getMappedSharedMemory(int32_t bytesSize) {
    const auto nullResult = std::make_pair(hidl_memory(), nullptr);

    sp<IAllocator> ashmemAllocator = IAllocator::getService("ashmem");
    if (ashmemAllocator.get() == nullptr) {
        return nullResult;
    }

    // Allocate shared memory.
    hidl_memory hidlMemory;
    bool allocateSuccess = false;
    Return<void> result =
            ashmemAllocator->allocate(bytesSize, [&](bool success, const hidl_memory& hidlMem) {
                if (!success) {
                    return;
                }
                allocateSuccess = success;
                hidlMemory = hidlMem;
            });

    // Check result of allocated memory.
    if (!result.isOk() || !allocateSuccess) {
        return nullResult;
    }

    // Map shared memory.
    sp<IMemory> pIMemory = mapMemory(hidlMemory);
    if (pIMemory.get() == nullptr) {
        return nullResult;
    }

    return std::make_pair(hidlMemory, pIMemory);
}

void SurroundViewFuzzer::setIndexOfOverlaysMemory(
        const std::vector<OverlayMemoryDesc>& overlaysMemDesc, sp<IMemory> pIMemory,
        int32_t indexPosition, uint16_t indexValue) {
    // Count the number of vertices until the index.
    int32_t totalVerticesCount = 0;
    for (int32_t i = 0; i < indexPosition; ++i) {
        totalVerticesCount += overlaysMemDesc[i].verticesCount;
    }

    const int32_t indexBytePosition =
            (indexPosition * kIdByteSize) + (kVertexByteSize * totalVerticesCount);

    uint8_t* pSharedMemoryData = (uint8_t*)((void*)pIMemory->getPointer());
    pSharedMemoryData += indexBytePosition;
    uint16_t* pIndex16bit = (uint16_t*)pSharedMemoryData;

    // Modify shared memory.
    pIMemory->update();
    *pIndex16bit = indexValue;
    pIMemory->commit();
}

void SurroundViewFuzzer::initSampleOverlaysData() {
    const size_t mNumOverlays =
            mFuzzedDataProvider.ConsumeIntegralInRange<size_t>(kMinOverlays, kMaxOverlays);
    mOverlaysdata.overlaysMemoryDesc.resize(mNumOverlays);

    int32_t sharedMemBytesSize = 0;
    std::vector<OverlayMemoryDesc> overlaysDescVector = {};
    OverlayMemoryDesc overlayMemDesc[mNumOverlays];
    for (size_t i = 0; i < mNumOverlays; ++i) {
        overlayMemDesc[i].id = i;
        overlayMemDesc[i].verticesCount =
                mFuzzedDataProvider.ConsumeIntegralInRange<size_t>(1, kMaxVertices);
        overlayMemDesc[i].overlayPrimitive = mFuzzedDataProvider.ConsumeBool()
                                                     ? (OverlayPrimitive::TRIANGLES)
                                                     : (OverlayPrimitive::TRIANGLES_STRIP);
        mOverlaysdata.overlaysMemoryDesc[i] = overlayMemDesc[i];

        sharedMemBytesSize += kIdByteSize + kVertexByteSize * overlayMemDesc[i].verticesCount;
        overlaysDescVector.push_back(overlayMemDesc[i]);
    }

    std::pair<hidl_memory, sp<IMemory>> sharedMem = getMappedSharedMemory(sharedMemBytesSize);
    sp<IMemory> pIMemory = std::get<1>(sharedMem);
    if (pIMemory.get() == nullptr) {
        mOverlaysdata = OverlaysData();
        mMemory = nullptr;
        return;
    }

    // Get pointer to shared memory data and set all bytes to 0.
    uint8_t* pSharedMemoryData = (uint8_t*)((void*)pIMemory->getPointer());
    pIMemory->update();
    memset(pSharedMemoryData, 0, sharedMemBytesSize);
    pIMemory->commit();

    // Set indexes in shared memory.
    for (size_t i = 0; i < mNumOverlays; ++i) {
        setIndexOfOverlaysMemory(overlaysDescVector, pIMemory, i, overlayMemDesc[i].id);
    }

    mOverlaysdata.overlaysMemoryDesc = overlaysDescVector;
    mOverlaysdata.overlaysMemory = std::get<0>(sharedMem);
    mMemory = pIMemory;
}

extern "C" int LLVMFuzzerTestOneInput(const uint8_t* data, size_t size) {
    if (size < 1) {
        return 0;
    }
    SurroundViewFuzzer surroundViewFuzzer(data, size);
    surroundViewFuzzer.process();
    return 0;
}
}  // namespace android::hardware::automotive::sv::V1_0::implementation::fuzzer
