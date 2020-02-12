//
// Copyright (C) 2020 The Android Open Source Project
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//      http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.
//

#define LOG_TAG "VtsHalSurroundViewTest"

#include <android/hardware/automotive/sv/1.0/types.h>
#include <android/hardware/automotive/sv/1.0/ISurroundViewService.h>
#include <android/hardware/automotive/sv/1.0/ISurroundViewStream.h>
#include <android/hardware/automotive/sv/1.0/ISurroundView2dSession.h>
#include <android/hardware/automotive/sv/1.0/ISurroundView3dSession.h>
#include <android/hardware_buffer.h>
#include <android/hidl/allocator/1.0/IAllocator.h>
#include <android/hidl/memory/1.0/IMemory.h>
#include <hidlmemory/mapping.h>
#include <math.h>
#include <utils/Log.h>

#include <gtest/gtest.h>
#include <hidl/GtestPrinter.h>
#include <hidl/ServiceManagement.h>

#include "SurroundViewStreamHandler.h"

using namespace ::android::hardware::automotive::sv::V1_0;
using ::android::sp;
using ::android::hardware::hidl_vec;
using ::android::hardware::hidl_string;
using ::android::hidl::allocator::V1_0::IAllocator;
using ::android::hidl::memory::V1_0::IMemory;
using ::android::hardware::hidl_memory;

const int kVertexByteSize = (3 * sizeof(float)) + 4;
const int kIdByteSize = 2;

// The main test class for Surround View Service
class SurroundViewHidlTest : public ::testing::TestWithParam<std::string> {
public:
    virtual void SetUp() override {
        mSurroundViewService = ISurroundViewService::getService(GetParam());
        ASSERT_NE(mSurroundViewService.get(), nullptr);
    }

    virtual void TearDown() override {}

    sp<ISurroundViewService> mSurroundViewService;    // Every test needs access to the service
};

TEST_P(SurroundViewHidlTest, startAndStop2dSession) {
    ALOGD("SurroundViewHidlTest::startAndStop2dSession");
    sp<ISurroundView2dSession> surroundView2dSession;
    mSurroundViewService->start2dSession(
        [&surroundView2dSession](
            const sp<ISurroundView2dSession>& session, SvResult result) {
        ASSERT_EQ(result, SvResult::OK);
        surroundView2dSession = session;
    });

    SvResult result = mSurroundViewService->stop2dSession(surroundView2dSession);
    ASSERT_EQ(result, SvResult::OK);
}

TEST_P(SurroundViewHidlTest, stopInvalid2dSession) {
    ALOGD("SurroundViewHidlTest::stopInvalid2dSession");
    sp<ISurroundView2dSession> surroundView2dSession;
    SvResult result = mSurroundViewService->stop2dSession(surroundView2dSession);
    ASSERT_NE(result, SvResult::OK);
}

TEST_P(SurroundViewHidlTest, startAndStop2dStream) {
    ALOGD("SurroundViewHidlTest::startAndStop2dStream");
    sp<ISurroundView2dSession> surroundView2dSession;
    mSurroundViewService->start2dSession(
        [&surroundView2dSession](
            const sp<ISurroundView2dSession>& session, SvResult result) {
        ASSERT_EQ(result, SvResult::OK);
        surroundView2dSession = session;
    });

    sp<SurroundViewServiceHandler> handler =
        new SurroundViewServiceHandler(surroundView2dSession);

    SvResult result = surroundView2dSession->startStream(handler);
    EXPECT_EQ(result, SvResult::OK);

    sleep(5);

    EXPECT_TRUE(handler->checkEventReceived(SvEvent::STREAM_STARTED));
    EXPECT_GT(handler->getReceiveFramesCount(), 0);

    surroundView2dSession->stopStream();

    sleep(1);
    EXPECT_TRUE(handler->checkEventReceived(SvEvent::STREAM_STOPPED));

    result = mSurroundViewService->stop2dSession(surroundView2dSession);
    EXPECT_EQ(result, SvResult::OK);
}

TEST_P(SurroundViewHidlTest, start2dStreamWithoutReturningFrames) {
    ALOGD("SurroundViewHidlTest::start2dStreamWithoutReturningFrames");
    sp<ISurroundView2dSession> surroundView2dSession;
    mSurroundViewService->start2dSession(
        [&surroundView2dSession](
            const sp<ISurroundView2dSession>& session, SvResult result) {
        ASSERT_EQ(result, SvResult::OK);
        surroundView2dSession = session;
    });

    sp<SurroundViewServiceHandler> handler =
        new SurroundViewServiceHandler(surroundView2dSession);
    handler->setDoNotReturnFrames(true);

    SvResult result = surroundView2dSession->startStream(handler);
    EXPECT_EQ(result, SvResult::OK);

    sleep(5);

    EXPECT_TRUE(handler->checkEventReceived(SvEvent::STREAM_STARTED));
    EXPECT_TRUE(handler->checkEventReceived(SvEvent::FRAME_DROPPED));
    EXPECT_GT(handler->getReceiveFramesCount(), 0);

    surroundView2dSession->stopStream();

    sleep(1);
    EXPECT_TRUE(handler->checkEventReceived(SvEvent::STREAM_STOPPED));

    result = mSurroundViewService->stop2dSession(surroundView2dSession);
    EXPECT_EQ(result, SvResult::OK);
}

TEST_P(SurroundViewHidlTest, duplicateStart2dStream) {
    ALOGD("SurroundViewHidlTest, duplicateStart2dStream");
    sp<ISurroundView2dSession> surroundView2dSession;
    mSurroundViewService->start2dSession(
        [&surroundView2dSession](
            const sp<ISurroundView2dSession>& session, SvResult result) {
        ASSERT_EQ(result, SvResult::OK);
        surroundView2dSession = session;
    });

    sp<SurroundViewServiceHandler> handler =
        new SurroundViewServiceHandler(surroundView2dSession);

    SvResult result = surroundView2dSession->startStream(handler);
    EXPECT_EQ(result, SvResult::OK);

    result = surroundView2dSession->startStream(handler);
    EXPECT_NE(result, SvResult::OK);

    surroundView2dSession->stopStream();
    mSurroundViewService->stop2dSession(surroundView2dSession);
}

TEST_P(SurroundViewHidlTest, stopInvalid2dStream) {
    ALOGD("SurroundViewHidlTest, stopInvalid2dStream");
    sp<ISurroundView2dSession> surroundView2dSession;
    mSurroundViewService->start2dSession(
        [&surroundView2dSession](
            const sp<ISurroundView2dSession>& session, SvResult result) {
        ASSERT_EQ(result, SvResult::OK);
        surroundView2dSession = session;
    });

    sp<SurroundViewServiceHandler> handler =
        new SurroundViewServiceHandler(surroundView2dSession);

    surroundView2dSession->stopStream();
    mSurroundViewService->stop2dSession(surroundView2dSession);
}

TEST_P(SurroundViewHidlTest, validate2dSvFramesDesc) {
    ALOGD("SurroundViewHidlTest, validate2dSvFramesDesc");
    sp<ISurroundView2dSession> surroundView2dSession;
    mSurroundViewService->start2dSession(
        [&surroundView2dSession](
            const sp<ISurroundView2dSession>& session, SvResult result) {
        ASSERT_EQ(result, SvResult::OK);
        surroundView2dSession = session;
    });

    sp<SurroundViewServiceHandler> handler =
        new SurroundViewServiceHandler(surroundView2dSession);

    SvResult result = surroundView2dSession->startStream(handler);
    EXPECT_EQ(result, SvResult::OK);

    sleep(5);

    // Validate timestampNs and sequenceId
    EXPECT_GT(handler->getReceiveFramesCount(), 0);
    EXPECT_TRUE(handler->areAllFramesValid());

    // Validate 2d SvFramesDesc. Do not compare nativeHandle since it is not
    // stored and already verified on the fly.
    SvFramesDesc frames = handler->getLastReceivedFrames();
    EXPECT_EQ(frames.svBuffers.size(), 1);

    SvBuffer svBuffer2d = frames.svBuffers[0];
    EXPECT_EQ(svBuffer2d.viewId, 0);

    const AHardwareBuffer_Desc* pDesc =
        reinterpret_cast<const AHardwareBuffer_Desc *>(&svBuffer2d.hardwareBuffer.description);
    float mapWidth, mapHeight;
    surroundView2dSession->get2dMappingInfo([&mapWidth, &mapHeight] (Sv2dMappingInfo info) {
        mapWidth = info.width;
        mapHeight = info.height;
    });
    EXPECT_EQ(pDesc->height, floor(pDesc->width * (mapHeight / mapWidth)));

    // Clean up
    surroundView2dSession->stopStream();
    result = mSurroundViewService->stop2dSession(surroundView2dSession);
}

TEST_P(SurroundViewHidlTest, get2dMappingInfo) {
    ALOGD("SurroundViewHidlTest, get2dMappingInfo");
    sp<ISurroundView2dSession> surroundView2dSession;
    mSurroundViewService->start2dSession(
        [&surroundView2dSession](
            const sp<ISurroundView2dSession>& session, SvResult result) {
        ASSERT_EQ(result, SvResult::OK);
        surroundView2dSession = session;
    });

    surroundView2dSession->get2dMappingInfo([] (Sv2dMappingInfo info) {
        EXPECT_GT(info.width, 0);
        EXPECT_GT(info.height, 0);
    });

    mSurroundViewService->stop2dSession(surroundView2dSession);
}

TEST_P(SurroundViewHidlTest, set2dConfigResolution) {
    ALOGD("SurroundViewHidlTest, set2dConfigResolution");
    sp<ISurroundView2dSession> surroundView2dSession;
    mSurroundViewService->start2dSession(
        [&surroundView2dSession](
            const sp<ISurroundView2dSession>& session, SvResult result) {
        ASSERT_EQ(result, SvResult::OK);
        surroundView2dSession = session;
    });

    sp<SurroundViewServiceHandler> handler =
        new SurroundViewServiceHandler(surroundView2dSession);

    SvResult result = surroundView2dSession->startStream(handler);
    EXPECT_EQ(result, SvResult::OK);

    sleep(1);

    // Change config
    Sv2dConfig config;
    config.width = 1920;
    config.blending = SvQuality::HIGH;
    surroundView2dSession->set2dConfig(config);

    sleep(1);

    EXPECT_TRUE(handler->checkEventReceived(SvEvent::CONFIG_UPDATED));

    // Check width has been changed but not the ratio
    SvFramesDesc frames = handler->getLastReceivedFrames();
    EXPECT_EQ(frames.svBuffers.size(), 1);
    SvBuffer svBuffer2d = frames.svBuffers[0];
    EXPECT_EQ(svBuffer2d.viewId, 0);
    const AHardwareBuffer_Desc* pDesc =
        reinterpret_cast<const AHardwareBuffer_Desc *>(&svBuffer2d.hardwareBuffer.description);
    EXPECT_EQ(pDesc->width, config.width);

    float mapWidth, mapHeight;
    surroundView2dSession->get2dMappingInfo([&mapWidth, &mapHeight] (Sv2dMappingInfo info) {
        mapWidth = info.width;
        mapHeight = info.height;
    });
    EXPECT_EQ(pDesc->height, floor (pDesc->width * (mapHeight / mapWidth)));

    // Clean up
    surroundView2dSession->stopStream();
    mSurroundViewService->stop2dSession(surroundView2dSession);
}

TEST_P(SurroundViewHidlTest, set2dConfigBlending) {
    ALOGD("SurroundViewHidlTest, set2dConfigBlending");
    sp<ISurroundView2dSession> surroundView2dSession;
    mSurroundViewService->start2dSession(
        [&surroundView2dSession](
            const sp<ISurroundView2dSession>& session, SvResult result) {
        ASSERT_EQ(result, SvResult::OK);
        surroundView2dSession = session;
    });

    sp<SurroundViewServiceHandler> handler =
        new SurroundViewServiceHandler(surroundView2dSession);

    SvResult result = surroundView2dSession->startStream(handler);
    EXPECT_EQ(result, SvResult::OK);

    sleep(1);

    // Get the width before config changed
    int oldWidth;
    SvFramesDesc frames = handler->getLastReceivedFrames();
    EXPECT_EQ(frames.svBuffers.size(), 1);
    SvBuffer svBuffer2d = frames.svBuffers[0];
    const AHardwareBuffer_Desc* pDesc =
        reinterpret_cast<const AHardwareBuffer_Desc *>(&svBuffer2d.hardwareBuffer.description);
    oldWidth = pDesc->width;

    // Change config
    Sv2dConfig config;
    config.width = oldWidth;
    config.blending = SvQuality::LOW;
    surroundView2dSession->set2dConfig(config);

    sleep(1);

    EXPECT_TRUE(handler->checkEventReceived(SvEvent::CONFIG_UPDATED));

    Sv2dConfig retConfig;
    surroundView2dSession->get2dConfig([&retConfig] (Sv2dConfig config) {
        retConfig.width = config.width;
        retConfig.blending = config.blending;
    });

    // Check config blending has been changed but not the width
    EXPECT_EQ(retConfig.blending, config.blending);
    EXPECT_EQ(retConfig.width, oldWidth);

    // Clean up
    surroundView2dSession->stopStream();
    mSurroundViewService->stop2dSession(surroundView2dSession);
}

TEST_P(SurroundViewHidlTest, projectCameraPointsWithValidCameraId) {
    ALOGD("SurroundViewHidlTest, projectCameraPointsWithValidCameraId");
    sp<ISurroundView2dSession> surroundView2dSession;
    mSurroundViewService->start2dSession(
        [&surroundView2dSession](
            const sp<ISurroundView2dSession>& session, SvResult result) {
        ASSERT_EQ(result, SvResult::OK);
        surroundView2dSession = session;
    });

    hidl_vec<hidl_string> cameraIds;
    mSurroundViewService->getCameraIds([&cameraIds](
            const hidl_vec<hidl_string>& camIds) {
        cameraIds = camIds;
    });

    sp<SurroundViewServiceHandler> handler =
        new SurroundViewServiceHandler(surroundView2dSession);

    SvResult result = surroundView2dSession->startStream(handler);
    EXPECT_EQ(result, SvResult::OK);

    sleep(1);

    // Get the width and height of the frame
    int width, height;
    SvFramesDesc frames = handler->getLastReceivedFrames();
    EXPECT_EQ(frames.svBuffers.size(), 1);
    SvBuffer svBuffer2d = frames.svBuffers[0];
    const AHardwareBuffer_Desc* pDesc =
        reinterpret_cast<const AHardwareBuffer_Desc *>(&svBuffer2d.hardwareBuffer.description);
    width = pDesc->width;
    height = pDesc->height;

    float mapWidth, mapHeight, mapCenter[2];
    surroundView2dSession->get2dMappingInfo(
        [&mapWidth, &mapHeight, &mapCenter] (Sv2dMappingInfo info) {
        mapWidth = info.width;
        mapHeight = info.height;
        mapCenter[0] = info.center.x;
        mapCenter[1] = info.center.y;
    });

    // Set one valid point and one invalid point
    hidl_vec<Point2dInt> points2dCamera;
    points2dCamera.resize(2);
    points2dCamera[0].x = 0;
    points2dCamera[0].y = 0;
    points2dCamera[1].x = width * 2;
    points2dCamera[1].y = height * 2;

    surroundView2dSession->projectCameraPoints(
        points2dCamera,
        cameraIds[0],
        [&mapWidth, &mapHeight, &mapCenter] (
            const hidl_vec<Point2dFloat>& outPoints) {
            // Make sure point[0] is valid.
            EXPECT_TRUE(outPoints[0].isValid);
            EXPECT_GE(outPoints[0].x, mapCenter[0] - mapWidth);
            EXPECT_LE(outPoints[0].x, mapCenter[0] + mapWidth);
            EXPECT_GE(outPoints[0].y, mapCenter[1] - mapHeight);
            EXPECT_LE(outPoints[0].y, mapCenter[1] + mapHeight);

            // Make sure point[1] is invalid.
            EXPECT_FALSE(outPoints[1].isValid);
        });

    // Clean up
    surroundView2dSession->stopStream();
    mSurroundViewService->stop2dSession(surroundView2dSession);
}

TEST_P(SurroundViewHidlTest, projectCameraPointsWithInvalidCameraId) {
    ALOGD("SurroundViewHidlTest, projectCameraPointsWithInvalidCameraId");
    sp<ISurroundView2dSession> surroundView2dSession;
    mSurroundViewService->start2dSession(
        [&surroundView2dSession](
            const sp<ISurroundView2dSession>& session, SvResult result) {
        ASSERT_EQ(result, SvResult::OK);
        surroundView2dSession = session;
    });

    hidl_vec<hidl_string> cameraIds;
    mSurroundViewService->getCameraIds([&cameraIds](
            const hidl_vec<hidl_string>& camIds) {
        cameraIds = camIds;
    });

    hidl_string invalidCameraId = "INVALID_CAMERA_ID";

    // In case one of the camera id happens to be identical to
    // the invalid camera id.
    for (auto cameraId : cameraIds) {
        ASSERT_NE(cameraId, invalidCameraId);
    }

    // Set one valid point
    hidl_vec<Point2dInt> points2dCamera;
    points2dCamera.resize(1);
    points2dCamera[0].x = 0;
    points2dCamera[0].y = 0;

    surroundView2dSession->projectCameraPoints(
        points2dCamera,
        invalidCameraId,
        [] (const hidl_vec<Point2dFloat>& outPoints) {
            // No points are return due to invalid camera id
            EXPECT_EQ(outPoints.size(), 0);
        });

    // Clean up
    surroundView2dSession->stopStream();
    mSurroundViewService->stop2dSession(surroundView2dSession);
}

TEST_P(SurroundViewHidlTest, startAndStop3dSession) {
    ALOGD("SurroundViewHidlTest, startAndStop3dSession");
    sp<ISurroundView3dSession> surroundView3dSession;
    mSurroundViewService->start3dSession(
        [&surroundView3dSession](
            const sp<ISurroundView3dSession>& session, SvResult result) {
        ASSERT_EQ(result, SvResult::OK);
        surroundView3dSession = session;
    });

    SvResult result = mSurroundViewService->stop3dSession(surroundView3dSession);
    EXPECT_EQ(result, SvResult::OK);
}

TEST_P(SurroundViewHidlTest, stopInvalid3dSession) {
    ALOGD("SurroundViewHidlTest, stopInvalid3dSession");
    sp<ISurroundView3dSession> surroundView3dSession;
    SvResult result = mSurroundViewService->stop3dSession(surroundView3dSession);
    EXPECT_NE(result, SvResult::OK);
}

TEST_P(SurroundViewHidlTest, startAndStop3dStream) {
    ALOGD("SurroundViewHidlTest::startAndStop3dStream");
    sp<ISurroundView3dSession> surroundView3dSession;
    mSurroundViewService->start3dSession(
        [&surroundView3dSession](
            const sp<ISurroundView3dSession>& session, SvResult result) {
        ASSERT_EQ(result, SvResult::OK);
        surroundView3dSession = session;
    });

    std::vector<View3d> views(1);
    SvResult setViewResult = surroundView3dSession->setViews(views);
    EXPECT_EQ(setViewResult, SvResult::OK);

    sp<SurroundViewServiceHandler> handler =
        new SurroundViewServiceHandler(surroundView3dSession);

    SvResult result = surroundView3dSession->startStream(handler);
    EXPECT_EQ(result, SvResult::OK);

    sleep(5);

    EXPECT_TRUE(handler->checkEventReceived(SvEvent::STREAM_STARTED));
    EXPECT_GT(handler->getReceiveFramesCount(), 0);

    surroundView3dSession->stopStream();

    sleep(1);
    EXPECT_TRUE(handler->checkEventReceived(SvEvent::STREAM_STOPPED));

    result = mSurroundViewService->stop3dSession(surroundView3dSession);
    EXPECT_EQ(result, SvResult::OK);
}

TEST_P(SurroundViewHidlTest, start3dStreamWithoutReturningFrames) {
    ALOGD("SurroundViewHidlTest::start3dStreamWithoutReturningFrames");
    sp<ISurroundView3dSession> surroundView3dSession;
    mSurroundViewService->start3dSession(
        [&surroundView3dSession](
            const sp<ISurroundView3dSession>& session, SvResult result) {
        ASSERT_EQ(result, SvResult::OK);
        surroundView3dSession = session;
    });

    std::vector<View3d> views(1);
    SvResult setViewResult = surroundView3dSession->setViews(views);
    EXPECT_EQ(setViewResult, SvResult::OK);

    sp<SurroundViewServiceHandler> handler =
        new SurroundViewServiceHandler(surroundView3dSession);
    handler->setDoNotReturnFrames(true);

    SvResult result = surroundView3dSession->startStream(handler);
    EXPECT_EQ(result, SvResult::OK);

    sleep(5);

    EXPECT_TRUE(handler->checkEventReceived(SvEvent::STREAM_STARTED));
    EXPECT_TRUE(handler->checkEventReceived(SvEvent::FRAME_DROPPED));
    EXPECT_GT(handler->getReceiveFramesCount(), 0);

    surroundView3dSession->stopStream();

    sleep(1);
    EXPECT_TRUE(handler->checkEventReceived(SvEvent::STREAM_STOPPED));

    result = mSurroundViewService->stop3dSession(surroundView3dSession);
    EXPECT_EQ(result, SvResult::OK);
}

TEST_P(SurroundViewHidlTest, duplicateStart3dStream) {
    ALOGD("SurroundViewHidlTest, duplicateStart3dStream");
    sp<ISurroundView3dSession> surroundView3dSession;
    mSurroundViewService->start3dSession(
        [&surroundView3dSession](
            const sp<ISurroundView3dSession>& session, SvResult result) {
        ASSERT_EQ(result, SvResult::OK);
        surroundView3dSession = session;
    });

    std::vector<View3d> views(1);
    SvResult setViewResult = surroundView3dSession->setViews(views);
    EXPECT_EQ(setViewResult, SvResult::OK);

    sp<SurroundViewServiceHandler> handler =
        new SurroundViewServiceHandler(surroundView3dSession);

    SvResult result = surroundView3dSession->startStream(handler);
    EXPECT_EQ(result, SvResult::OK);

    result = surroundView3dSession->startStream(handler);
    EXPECT_NE(result, SvResult::OK);

    surroundView3dSession->stopStream();
    mSurroundViewService->stop3dSession(surroundView3dSession);
}

TEST_P(SurroundViewHidlTest, start3dStreamNoViewSetFail) {
    ALOGD("SurroundViewHidlTest, start3dStreamNoViewSetFail");
    sp<ISurroundView3dSession> surroundView3dSession;
    mSurroundViewService->start3dSession(
        [&surroundView3dSession](
            const sp<ISurroundView3dSession>& session, SvResult result) {
        ASSERT_EQ(result, SvResult::OK);
        surroundView3dSession = session;
    });

    sp<SurroundViewServiceHandler> handler =
        new SurroundViewServiceHandler(surroundView3dSession);

    SvResult result = surroundView3dSession->startStream(handler);
    EXPECT_EQ(result, SvResult::VIEW_NOT_SET);

    mSurroundViewService->stop3dSession(surroundView3dSession);
}

TEST_P(SurroundViewHidlTest, validate3dSvFramesDesc) {
    ALOGD("SurroundViewHidlTest, validate3dSvFramesDesc");
    sp<ISurroundView3dSession> surroundView3dSession;
    mSurroundViewService->start3dSession(
        [&surroundView3dSession](
            const sp<ISurroundView3dSession>& session, SvResult result) {
        ASSERT_EQ(result, SvResult::OK);
        surroundView3dSession = session;
    });

    sp<SurroundViewServiceHandler> handler =
        new SurroundViewServiceHandler(surroundView3dSession);

    std::vector<View3d> views(1);
    views[0].viewId = 0;
    SvResult setViewResult = surroundView3dSession->setViews(views);
    EXPECT_EQ(setViewResult, SvResult::OK);

    SvResult result = surroundView3dSession->startStream(handler);
    EXPECT_EQ(result, SvResult::OK);

    sleep(5);

    EXPECT_GT(handler->getReceiveFramesCount(), 0);
    EXPECT_TRUE(handler->areAllFramesValid());

    // Validate 3d SvFramesDesc when only one view is set.
    SvFramesDesc frames = handler->getLastReceivedFrames();
    EXPECT_EQ(frames.svBuffers.size(), 1);
    EXPECT_EQ(frames.svBuffers[0].viewId, 0);

    views.resize(3);
    views[0].viewId = 0;
    views[1].viewId = 1;
    views[2].viewId = 2;
    setViewResult = surroundView3dSession->setViews(views);
    EXPECT_EQ(setViewResult, SvResult::OK);

    sleep(1);

    // Validate 3d SvFramesDesc when multiple views are set.
    EXPECT_TRUE(handler->areAllFramesValid());
    frames = handler->getLastReceivedFrames();
    EXPECT_EQ(frames.svBuffers.size(), 3);
    EXPECT_EQ(frames.svBuffers[0].viewId, 0);
    EXPECT_EQ(frames.svBuffers[1].viewId, 1);
    EXPECT_EQ(frames.svBuffers[2].viewId, 2);
    EXPECT_EQ(frames.svBuffers[0].hardwareBuffer.description[0],
              frames.svBuffers[1].hardwareBuffer.description[0]);
    EXPECT_EQ(frames.svBuffers[0].hardwareBuffer.description[1],
              frames.svBuffers[1].hardwareBuffer.description[1]);
    EXPECT_EQ(frames.svBuffers[1].hardwareBuffer.description[0],
              frames.svBuffers[2].hardwareBuffer.description[0]);
    EXPECT_EQ(frames.svBuffers[1].hardwareBuffer.description[1],
              frames.svBuffers[2].hardwareBuffer.description[1]);

    // Clean up
    surroundView3dSession->stopStream();
    result = mSurroundViewService->stop3dSession(surroundView3dSession);
}

TEST_P(SurroundViewHidlTest, set3dConfigResolution) {
    ALOGD("SurroundViewHidlTest, set3dConfigResolution");
    sp<ISurroundView3dSession> surroundView3dSession;
    mSurroundViewService->start3dSession(
        [&surroundView3dSession](
            const sp<ISurroundView3dSession>& session, SvResult result) {
        ASSERT_EQ(result, SvResult::OK);
        surroundView3dSession = session;
    });

    sp<SurroundViewServiceHandler> handler =
        new SurroundViewServiceHandler(surroundView3dSession);

    std::vector<View3d> views(1);
    views[0].viewId = 0;
    SvResult setViewResult = surroundView3dSession->setViews(views);
    EXPECT_EQ(setViewResult, SvResult::OK);
    SvResult result = surroundView3dSession->startStream(handler);
    EXPECT_EQ(result, SvResult::OK);

    sleep(1);

    // Change config
    Sv3dConfig config;
    config.width = 1920;
    config.height = 1080;
    config.carDetails = SvQuality::HIGH;
    surroundView3dSession->set3dConfig(config);

    sleep(1);

    EXPECT_TRUE(handler->checkEventReceived(SvEvent::CONFIG_UPDATED));

    // Check width has been changed but not the ratio
    SvFramesDesc frames = handler->getLastReceivedFrames();
    EXPECT_EQ(frames.svBuffers.size(), 1);
    SvBuffer svBuffer3d = frames.svBuffers[0];
    EXPECT_EQ(svBuffer3d.viewId, 0);
    const AHardwareBuffer_Desc* pDesc =
        reinterpret_cast<const AHardwareBuffer_Desc *>(&svBuffer3d.hardwareBuffer.description);
    EXPECT_EQ(pDesc->width, config.width);
    EXPECT_EQ(pDesc->height, config.height);

    // Clean up
    surroundView3dSession->stopStream();
    mSurroundViewService->stop3dSession(surroundView3dSession);
}

TEST_P(SurroundViewHidlTest, set3dConfigCarDetails) {
    ALOGD("SurroundViewHidlTest, set3dConfigCarDetails");
    sp<ISurroundView3dSession> surroundView3dSession;
    mSurroundViewService->start3dSession(
        [&surroundView3dSession](
            const sp<ISurroundView3dSession>& session, SvResult result) {
        ASSERT_EQ(result, SvResult::OK);
        surroundView3dSession = session;
    });

    sp<SurroundViewServiceHandler> handler =
        new SurroundViewServiceHandler(surroundView3dSession);

    std::vector<View3d> views(1);
    views[0].viewId = 0;
    SvResult setViewResult = surroundView3dSession->setViews(views);
    EXPECT_EQ(setViewResult, SvResult::OK);
    SvResult result = surroundView3dSession->startStream(handler);
    EXPECT_EQ(result, SvResult::OK);

    sleep(1);

    // Get the width before config changed
    int oldWidth, oldHeight;
    SvFramesDesc frames = handler->getLastReceivedFrames();
    EXPECT_EQ(frames.svBuffers.size(), 1);
    SvBuffer svBuffer3d = frames.svBuffers[0];
    const AHardwareBuffer_Desc* pDesc =
        reinterpret_cast<const AHardwareBuffer_Desc *>(&svBuffer3d.hardwareBuffer.description);
    oldWidth = pDesc->width;
    oldHeight = pDesc->height;

    // Change config
    Sv3dConfig config;
    config.width = oldWidth;
    config.height = oldHeight;
    config.carDetails = SvQuality::LOW;
    surroundView3dSession->set3dConfig(config);

    sleep(1);

    EXPECT_TRUE(handler->checkEventReceived(SvEvent::CONFIG_UPDATED));

    Sv3dConfig retConfig;
    surroundView3dSession->get3dConfig([&retConfig] (Sv3dConfig config) {
        retConfig.width = config.width;
        retConfig.height = config.height;
        retConfig.carDetails = config.carDetails;
    });

    // Check config blending has been changed but not the width
    EXPECT_EQ(retConfig.carDetails, config.carDetails);
    EXPECT_EQ(retConfig.width, oldWidth);
    EXPECT_EQ(retConfig.height, oldHeight);

    // Clean up
    surroundView3dSession->stopStream();
    mSurroundViewService->stop3dSession(surroundView3dSession);
}

std::pair<hidl_memory, sp<IMemory>> GetMappedSharedMemory(int bytesSize) {

    const auto nullResult = std::make_pair(hidl_memory(), nullptr);

    sp<IAllocator> ashmemAllocator = IAllocator::getService("ashmem");
    if (ashmemAllocator.get() == nullptr) {
        ALOGE("SurroundViewHidlTest getService ashmem failed");
        return nullResult;
    }

    // Allocate shared memory.
    hidl_memory hidlMemory;
    bool allocateSuccess = false;
    Return<void> result = ashmemAllocator->allocate(bytesSize,
            [&](bool success, const hidl_memory& hidlMem) {
                if (!success) {
                    return;
                }
                allocateSuccess = success;
                hidlMemory = hidlMem;
            });

    // Check result of allocated memory.
    if (!result.isOk() || !allocateSuccess) {
        ALOGE("SurroundViewHidlTest allocate shared memory failed");
        return nullResult;
    }

    // Map shared memory.
    sp<IMemory> pIMemory = mapMemory(hidlMemory);
    if (pIMemory.get() == nullptr) {
        ALOGE("SurroundViewHidlTest map shared memory failed");
        return nullResult;
    }

    return std::make_pair(hidlMemory, pIMemory);
}

void SetIndexOfOverlaysMemory(
        const std::vector<OverlayMemoryDesc>& overlaysMemDesc,
        sp<IMemory> pIMemory, int indexPosition, uint16_t indexValue) {

    // Count the number of vertices until the index.
    int totalVerticesCount = 0;
    for (int i = 0; i < indexPosition; i++) {
        totalVerticesCount += overlaysMemDesc[i].verticesCount;
    }

    const int indexBytePosition = (indexPosition * kIdByteSize) +
            (kVertexByteSize * totalVerticesCount);

    uint8_t* pSharedMemoryData = (uint8_t*)((void*)pIMemory->getPointer());
    pSharedMemoryData += indexBytePosition;
    uint16_t* pIndex16bit = (uint16_t*)pSharedMemoryData;

    ALOGD("Setting index at pos %d", indexBytePosition);

    // Modify shared memory.
    pIMemory->update();
    *pIndex16bit = indexValue;
    pIMemory->commit();
}

std::pair<OverlaysData, sp<IMemory>> GetSampleOverlaysData() {
    OverlaysData overlaysData;
    overlaysData.overlaysMemoryDesc.resize(2);

    int sharedMemBytesSize = 0;
    OverlayMemoryDesc overlayMemDesc1, overlayMemDesc2;
    overlayMemDesc1.id = 0;
    overlayMemDesc1.verticesCount = 6;
    overlayMemDesc1.overlayPrimitive = OverlayPrimitive::TRIANGLES;
    overlaysData.overlaysMemoryDesc[0] = overlayMemDesc1;
    sharedMemBytesSize += kIdByteSize +
            kVertexByteSize * overlayMemDesc1.verticesCount;

    overlayMemDesc2.id = 1;
    overlayMemDesc2.verticesCount = 4;
    overlayMemDesc2.overlayPrimitive = OverlayPrimitive::TRIANGLES_STRIP;
    overlaysData.overlaysMemoryDesc[1] = overlayMemDesc2;
    sharedMemBytesSize += kIdByteSize +
            kVertexByteSize * overlayMemDesc2.verticesCount;

    std::pair<hidl_memory, sp<IMemory>> sharedMem =
            GetMappedSharedMemory(sharedMemBytesSize);
    sp<IMemory> pIMemory = sharedMem.second;
    if (pIMemory.get() == nullptr) {
        return std::make_pair(OverlaysData(), nullptr);
    }

    // Get pointer to shared memory data and set all bytes to 0.
    uint8_t* pSharedMemoryData = (uint8_t*)((void*)pIMemory->getPointer());
    pIMemory->update();
    memset(pSharedMemoryData, 0, sharedMemBytesSize);
    pIMemory->commit();

    std::vector<OverlayMemoryDesc> overlaysDesc = {overlayMemDesc1,
            overlayMemDesc2};

    // Set indexes in shared memory.
    SetIndexOfOverlaysMemory(overlaysDesc, pIMemory, 0, overlayMemDesc1.id);
    SetIndexOfOverlaysMemory(overlaysDesc, pIMemory, 1, overlayMemDesc2.id);

    overlaysData.overlaysMemoryDesc = overlaysDesc;
    overlaysData.overlaysMemory = sharedMem.first;

    return std::make_pair(overlaysData, pIMemory);
}

TEST_P(SurroundViewHidlTest, updateOverlaysSuccess) {
    ALOGD("SurroundViewHidlTest, updateOverlaysSuccess");

    sp<ISurroundView3dSession> surroundView3dSession;
    mSurroundViewService->start3dSession(
        [&surroundView3dSession](
            const sp<ISurroundView3dSession>& session, SvResult result) {
        ASSERT_EQ(result, SvResult::OK);
        surroundView3dSession = session;
    });

    std::pair<OverlaysData, sp<IMemory>> overlaysData = GetSampleOverlaysData();
    ASSERT_NE(overlaysData.second, nullptr);

    SvResult result = surroundView3dSession->updateOverlays(overlaysData.first);
    EXPECT_EQ(result, SvResult::OK);

    mSurroundViewService->stop3dSession(surroundView3dSession);
}

TEST_P(SurroundViewHidlTest, overlaysDataMismatchIdFail) {
    ALOGD("SurroundViewHidlTest, overlaysDataMismatchIdFail");

    sp<ISurroundView3dSession> surroundView3dSession;
    mSurroundViewService->start3dSession(
        [&surroundView3dSession](
            const sp<ISurroundView3dSession>& session, SvResult result) {
        ASSERT_EQ(result, SvResult::OK);
        surroundView3dSession = session;
    });

    std::pair<OverlaysData, sp<IMemory>> overlaysDataMismatchId
            = GetSampleOverlaysData();
    ASSERT_NE(overlaysDataMismatchId.second, nullptr);

    // Set id of second overlay in shared memory to 2 (expected is 1).
    auto& overlaysDesc = overlaysDataMismatchId.first.overlaysMemoryDesc;
    auto& pIMemory = overlaysDataMismatchId.second;
    SetIndexOfOverlaysMemory(overlaysDesc, pIMemory, 1, 2);

    SvResult result = surroundView3dSession->updateOverlays(
            overlaysDataMismatchId.first);
    EXPECT_EQ(result, SvResult::INVALID_ARG);

    mSurroundViewService->stop3dSession(surroundView3dSession);
}

TEST_P(SurroundViewHidlTest, overlaysDataNullMemoryFail) {
    ALOGD("SurroundViewHidlTest, overlaysDataNullMemoryFail");

    sp<ISurroundView3dSession> surroundView3dSession;
    mSurroundViewService->start3dSession(
        [&surroundView3dSession](
            const sp<ISurroundView3dSession>& session, SvResult result) {
        ASSERT_EQ(result, SvResult::OK);
        surroundView3dSession = session;
    });

    std::pair<OverlaysData, sp<IMemory>> overlaysDataNullMemory
            = GetSampleOverlaysData();

    // Set shared memory to null.
    overlaysDataNullMemory.first.overlaysMemory = hidl_memory();

    SvResult result = surroundView3dSession->updateOverlays(
            overlaysDataNullMemory.first);
    EXPECT_EQ(result, SvResult::INVALID_ARG);

    mSurroundViewService->stop3dSession(surroundView3dSession);
}

TEST_P(SurroundViewHidlTest, overlaysDataLessThan3VerticesFail) {
    ALOGD("SurroundViewHidlTest, overlaysDataLessThan3VerticesFail");

    sp<ISurroundView3dSession> surroundView3dSession;
    mSurroundViewService->start3dSession(
        [&surroundView3dSession](
            const sp<ISurroundView3dSession>& session, SvResult result) {
        ASSERT_EQ(result, SvResult::OK);
        surroundView3dSession = session;
    });

    std::pair<OverlaysData, sp<IMemory>> overlaysData2Vertices
            = GetSampleOverlaysData();

    // Set vertices count of second overlay to 2.
    overlaysData2Vertices.first.overlaysMemoryDesc[1].verticesCount = 2;

    SvResult result = surroundView3dSession->updateOverlays(
            overlaysData2Vertices.first);
    EXPECT_EQ(result, SvResult::INVALID_ARG);

    mSurroundViewService->stop3dSession(surroundView3dSession);
}

TEST_P(SurroundViewHidlTest, overlaysDataVerticesCountFail) {
    ALOGD("SurroundViewHidlTest, overlaysDataVerticesNotMultipleOf3Fail");

    sp<ISurroundView3dSession> surroundView3dSession;
    mSurroundViewService->start3dSession(
        [&surroundView3dSession](
            const sp<ISurroundView3dSession>& session, SvResult result) {
        ASSERT_EQ(result, SvResult::OK);
        surroundView3dSession = session;
    });

    OverlaysData overlaysData;
    overlaysData.overlaysMemoryDesc.resize(1);

    OverlayMemoryDesc overlayMemDesc1;
    overlayMemDesc1.id = 0;
    overlayMemDesc1.verticesCount = 4; // Invalid count for TRIANGLES primitive.
    overlayMemDesc1.overlayPrimitive = OverlayPrimitive::TRIANGLES;
    overlaysData.overlaysMemoryDesc[0] = overlayMemDesc1;

    const int sharedMemBytesSize =
            2 + sizeof(float) * overlayMemDesc1.verticesCount;
    auto sharedMem = GetMappedSharedMemory(sharedMemBytesSize);

    // Set index in shared memory.
    auto& overlaysDesc = overlaysData.overlaysMemoryDesc;
    auto& pIMemory = sharedMem.second;
    SetIndexOfOverlaysMemory(overlaysDesc, pIMemory, 0, overlayMemDesc1.id);

    SvResult result = surroundView3dSession->updateOverlays(overlaysData);
    EXPECT_EQ(result, SvResult::INVALID_ARG);

    mSurroundViewService->stop3dSession(surroundView3dSession);
}

TEST_P(SurroundViewHidlTest, overlaysDataSameIdFail) {
    ALOGD("SurroundViewHidlTest, overlaysDataSameIdFail");

    sp<ISurroundView3dSession> surroundView3dSession;
    mSurroundViewService->start3dSession(
        [&surroundView3dSession](
            const sp<ISurroundView3dSession>& session, SvResult result) {
        ASSERT_EQ(result, SvResult::OK);
        surroundView3dSession = session;
    });

    std::pair<OverlaysData, sp<IMemory>> overlaysDataSameId
            = GetSampleOverlaysData();
    ASSERT_NE(overlaysDataSameId.second, nullptr);

    // Set id of second overlay as id of first.
    auto& overlaysDesc = overlaysDataSameId.first.overlaysMemoryDesc;
        auto& pIMemory = overlaysDataSameId.second;
    SetIndexOfOverlaysMemory(overlaysDesc, pIMemory, 1, overlaysDesc[0].id);

    SvResult result = surroundView3dSession->updateOverlays(
            overlaysDataSameId.first);
    EXPECT_EQ(result, SvResult::INVALID_ARG);

    mSurroundViewService->stop3dSession(surroundView3dSession);
}

TEST_P(SurroundViewHidlTest, projectPointsIncorrectCameraIdFail) {
    ALOGD("SurroundViewHidlTest, projectPointsIncorrectCameraIdFail");

    hidl_vec<hidl_string> cameraIds;
    mSurroundViewService->getCameraIds([&cameraIds](
            const hidl_vec<hidl_string>& camIds) {
        cameraIds = camIds;
    });
    EXPECT_GT(cameraIds.size(), 0);

    sp<ISurroundView3dSession> surroundView3dSession;
    mSurroundViewService->start3dSession(
        [&surroundView3dSession](
            const sp<ISurroundView3dSession>& session, SvResult result) {
        ASSERT_EQ(result, SvResult::OK);
        surroundView3dSession = session;
    });

    Point2dInt cameraPoint;
    cameraPoint.x = 0;
    cameraPoint.y = 0;
    std::vector<Point2dInt> cameraPoints = {cameraPoint};

    hidl_string invalidCameraId = "INVALID_CAMERA_ID";

    std::vector<Point3dFloat> points3d;
    surroundView3dSession->projectCameraPointsTo3dSurface(
        cameraPoints, invalidCameraId,
        [&points3d](const hidl_vec<Point3dFloat>& points3dproj) {
                points3d = points3dproj;
            });

    EXPECT_TRUE(points3d.empty());

    mSurroundViewService->stop3dSession(surroundView3dSession);
}

TEST_P(SurroundViewHidlTest, projectPointsInvalidPointsFail) {
    ALOGD("SurroundViewHidlTest, projectPointsInvalidPointsFail");

    hidl_vec<hidl_string> cameraIds;
    mSurroundViewService->getCameraIds([&cameraIds](
            const hidl_vec<hidl_string>& camIds) {
        cameraIds = camIds;
    });

    EXPECT_GT(cameraIds.size(), 0);

    sp<ISurroundView3dSession> surroundView3dSession;
    mSurroundViewService->start3dSession(
        [&surroundView3dSession](
            const sp<ISurroundView3dSession>& session, SvResult result) {
        ASSERT_EQ(result, SvResult::OK);
        surroundView3dSession = session;
    });

    sp<SurroundViewServiceHandler> handler =
        new SurroundViewServiceHandler(surroundView3dSession);

    std::vector<View3d> views(1);
    views[0].viewId = 0;
    SvResult setViewResult = surroundView3dSession->setViews(views);
    EXPECT_EQ(setViewResult, SvResult::OK);
    SvResult result = surroundView3dSession->startStream(handler);
    EXPECT_EQ(result, SvResult::OK);

    sleep(1);

    // Get the width and height of the frame
    int width, height;
    SvFramesDesc frames = handler->getLastReceivedFrames();
    EXPECT_EQ(frames.svBuffers.size(), 1);
    SvBuffer svBuffer2d = frames.svBuffers[0];
    const AHardwareBuffer_Desc* pDesc =
        reinterpret_cast<const AHardwareBuffer_Desc *>(&svBuffer2d.hardwareBuffer.description);
    width = pDesc->width;
    height = pDesc->height;

    Point2dInt cameraPoint;
    cameraPoint.x = width * 2;
    cameraPoint.y = height * 2;
    std::vector<Point2dInt> cameraPoints = {cameraPoint};

    std::vector<Point3dFloat> points3d;
    surroundView3dSession->projectCameraPointsTo3dSurface(
              cameraPoints, cameraIds[0],
        [&points3d](const hidl_vec<Point3dFloat>& points3dproj) {
                points3d = points3dproj;
            });

    EXPECT_FALSE(points3d[0].isValid);

    surroundView3dSession->stopStream();
    mSurroundViewService->stop3dSession(surroundView3dSession);
}

INSTANTIATE_TEST_SUITE_P(
    PerInstance,
    SurroundViewHidlTest,
    testing::ValuesIn(
        android::hardware::getAllHalInstanceNames(ISurroundViewService::descriptor)
    ),
    android::hardware::PrintInstanceNameToString
);
