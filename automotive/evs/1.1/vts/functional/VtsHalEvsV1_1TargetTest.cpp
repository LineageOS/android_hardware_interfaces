/*
 * Copyright (C) 2019 The Android Open Source Project
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

#define LOG_TAG "VtsHalEvsTest"


// These values are called out in the EVS design doc (as of Mar 8, 2017)
static const int kMaxStreamStartMilliseconds = 500;
static const int kMinimumFramesPerSecond = 10;

static const int kSecondsToMilliseconds = 1000;
static const int kMillisecondsToMicroseconds = 1000;
static const float kNanoToMilliseconds = 0.000001f;
static const float kNanoToSeconds = 0.000000001f;


#include "FrameHandler.h"
#include "FrameHandlerUltrasonics.h"

#include <cstdio>
#include <cstring>
#include <cstdlib>
#include <thread>
#include <unordered_set>

#include <hidl/HidlTransportSupport.h>
#include <hwbinder/ProcessState.h>
#include <utils/Errors.h>
#include <utils/StrongPointer.h>

#include <android/hardware/automotive/evs/1.1/IEvsCamera.h>
#include <android/hardware/automotive/evs/1.1/IEvsCameraStream.h>
#include <android/hardware/automotive/evs/1.1/IEvsEnumerator.h>
#include <android/hardware/automotive/evs/1.1/IEvsDisplay.h>
#include <android/hardware/camera/device/3.2/ICameraDevice.h>
#include <android-base/logging.h>
#include <system/camera_metadata.h>
#include <ui/DisplayConfig.h>
#include <ui/DisplayState.h>

#include <gtest/gtest.h>
#include <hidl/GtestPrinter.h>
#include <hidl/ServiceManagement.h>

using namespace ::android::hardware::automotive::evs::V1_1;
using namespace std::chrono_literals;

using ::android::hardware::Return;
using ::android::hardware::Void;
using ::android::hardware::hidl_vec;
using ::android::hardware::hidl_handle;
using ::android::hardware::hidl_string;
using ::android::sp;
using ::android::wp;
using ::android::hardware::camera::device::V3_2::Stream;
using ::android::hardware::automotive::evs::V1_0::DisplayDesc;
using ::android::hardware::automotive::evs::V1_0::DisplayState;
using ::android::hardware::graphics::common::V1_0::PixelFormat;
using IEvsCamera_1_0 = ::android::hardware::automotive::evs::V1_0::IEvsCamera;
using IEvsCamera_1_1 = ::android::hardware::automotive::evs::V1_1::IEvsCamera;
using IEvsDisplay_1_0 = ::android::hardware::automotive::evs::V1_0::IEvsDisplay;
using IEvsDisplay_1_1 = ::android::hardware::automotive::evs::V1_1::IEvsDisplay;

/*
 * Plese note that this is different from what is defined in
 * libhardware/modules/camera/3_4/metadata/types.h; this has one additional
 * field to store a framerate.
 */
const size_t kStreamCfgSz = 5;
typedef struct {
    int32_t width;
    int32_t height;
    int32_t format;
    int32_t direction;
    int32_t framerate;
} RawStreamConfig;


// The main test class for EVS
class EvsHidlTest : public ::testing::TestWithParam<std::string> {
public:
    virtual void SetUp() override {
        // Make sure we can connect to the enumerator
        std::string service_name = GetParam();
        pEnumerator = IEvsEnumerator::getService(service_name);
        ASSERT_NE(pEnumerator.get(), nullptr);
        LOG(INFO) << "Test target service: " << service_name;

        mIsHwModule = pEnumerator->isHardware();
    }

    virtual void TearDown() override {
        // Attempt to close any active camera
        for (auto &&cam : activeCameras) {
            if (cam != nullptr) {
                pEnumerator->closeCamera(cam);
            }
        }
        activeCameras.clear();
    }

protected:
    void loadCameraList() {
        // SetUp() must run first!
        assert(pEnumerator != nullptr);

        // Get the camera list
        pEnumerator->getCameraList_1_1(
            [this](hidl_vec <CameraDesc> cameraList) {
                LOG(INFO) << "Camera list callback received "
                          << cameraList.size()
                          << " cameras";
                cameraInfo.reserve(cameraList.size());
                for (auto&& cam: cameraList) {
                    LOG(INFO) << "Found camera " << cam.v1.cameraId;
                    cameraInfo.push_back(cam);
                }
            }
        );
    }

    void loadUltrasonicsArrayList() {
        // SetUp() must run first!
        assert(pEnumerator != nullptr);

        // Get the ultrasonics array list
        pEnumerator->getUltrasonicsArrayList([this](hidl_vec<UltrasonicsArrayDesc> ultraList) {
            LOG(INFO) << "Ultrasonics array list callback received "
                      << ultraList.size()
                      << " arrays";
            ultrasonicsArraysInfo.reserve(ultraList.size());
            for (auto&& ultraArray : ultraList) {
                LOG(INFO) << "Found ultrasonics array " << ultraArray.ultrasonicsArrayId;
                ultrasonicsArraysInfo.push_back(ultraArray);
            }
        });
    }

    bool isLogicalCamera(const camera_metadata_t *metadata) {
        if (metadata == nullptr) {
            // A logical camera device must have a valid camera metadata.
            return false;
        }

        // Looking for LOGICAL_MULTI_CAMERA capability from metadata.
        camera_metadata_ro_entry_t entry;
        int rc = find_camera_metadata_ro_entry(metadata,
                                               ANDROID_REQUEST_AVAILABLE_CAPABILITIES,
                                               &entry);
        if (0 != rc) {
            // No capabilities are found.
            return false;
        }

        for (size_t i = 0; i < entry.count; ++i) {
            uint8_t cap = entry.data.u8[i];
            if (cap == ANDROID_REQUEST_AVAILABLE_CAPABILITIES_LOGICAL_MULTI_CAMERA) {
                return true;
            }
        }

        return false;
    }

    std::unordered_set<std::string> getPhysicalCameraIds(const std::string& id,
                                                         bool& flag) {
        std::unordered_set<std::string> physicalCameras;

        auto it = cameraInfo.begin();
        while (it != cameraInfo.end()) {
            if (it->v1.cameraId == id) {
                break;
            }
            ++it;
        }

        if (it == cameraInfo.end()) {
            // Unknown camera is requested.  Return an empty list.
            return physicalCameras;
        }

        const camera_metadata_t *metadata =
            reinterpret_cast<camera_metadata_t *>(&it->metadata[0]);
        flag = isLogicalCamera(metadata);
        if (!flag) {
            // EVS assumes that the device w/o a valid metadata is a physical
            // device.
            LOG(INFO) << id << " is not a logical camera device.";
            physicalCameras.emplace(id);
            return physicalCameras;
        }

        // Look for physical camera identifiers
        camera_metadata_ro_entry entry;
        int rc = find_camera_metadata_ro_entry(metadata,
                                               ANDROID_LOGICAL_MULTI_CAMERA_PHYSICAL_IDS,
                                               &entry);
        if (rc != 0) {
            LOG(ERROR) << "No physical camera ID is found for a logical camera device";
        }

        const uint8_t *ids = entry.data.u8;
        size_t start = 0;
        for (size_t i = 0; i < entry.count; ++i) {
            if (ids[i] == '\0') {
                if (start != i) {
                    std::string id(reinterpret_cast<const char *>(ids + start));
                    physicalCameras.emplace(id);
                }
                start = i + 1;
            }
        }

        LOG(INFO) << id
                  << " consists of "
                  << physicalCameras.size()
                  << " physical camera devices";
        return physicalCameras;
    }


    sp<IEvsEnumerator>              pEnumerator;   // Every test needs access to the service
    std::vector<CameraDesc>         cameraInfo;    // Empty unless/until loadCameraList() is called
    bool                            mIsHwModule;   // boolean to tell current module under testing
                                                   // is HW module implementation.
    std::deque<sp<IEvsCamera_1_1>>  activeCameras; // A list of active camera handles that are
                                                   // needed to be cleaned up.
    std::vector<UltrasonicsArrayDesc>
            ultrasonicsArraysInfo;                           // Empty unless/until
                                                             // loadUltrasonicsArrayList() is called
    std::deque<wp<IEvsCamera_1_1>> activeUltrasonicsArrays;  // A list of active ultrasonic array
                                                             // handles that are to be cleaned up.
};


// Test cases, their implementations, and corresponding requirements are
// documented at go/aae-evs-public-api-test.

/*
 * CameraOpenClean:
 * Opens each camera reported by the enumerator and then explicitly closes it via a
 * call to closeCamera.  Then repeats the test to ensure all cameras can be reopened.
 */
TEST_P(EvsHidlTest, CameraOpenClean) {
    LOG(INFO) << "Starting CameraOpenClean test";

    // Get the camera list
    loadCameraList();

    // Using null stream configuration makes EVS uses the default resolution and
    // output format.
    Stream nullCfg = {};

    // Open and close each camera twice
    for (auto&& cam: cameraInfo) {
        bool isLogicalCam = false;
        auto devices = getPhysicalCameraIds(cam.v1.cameraId, isLogicalCam);
        if (mIsHwModule && isLogicalCam) {
            LOG(INFO) << "Skip a logical device, " << cam.v1.cameraId << " for HW target.";
            continue;
        }

        for (int pass = 0; pass < 2; pass++) {
            sp<IEvsCamera_1_1> pCam = pEnumerator->openCamera_1_1(cam.v1.cameraId, nullCfg);
            ASSERT_NE(pCam, nullptr);

            for (auto&& devName : devices) {
                bool matched = false;
                pCam->getPhysicalCameraInfo(devName,
                                            [&devName, &matched](const CameraDesc& info) {
                                                matched = devName == info.v1.cameraId;
                                            });
                ASSERT_TRUE(matched);
            }

            // Store a camera handle for a clean-up
            activeCameras.push_back(pCam);

            // Verify that this camera self-identifies correctly
            pCam->getCameraInfo_1_1([&cam](CameraDesc desc) {
                                        LOG(DEBUG) << "Found camera " << desc.v1.cameraId;
                                        EXPECT_EQ(cam.v1.cameraId, desc.v1.cameraId);
                                    }
            );

            // Verify methods for extended info
            const auto id = 0xFFFFFFFF; // meaningless id
            hidl_vec<uint8_t> values;
            auto err = pCam->setExtendedInfo_1_1(id, values);
            ASSERT_NE(EvsResult::INVALID_ARG, err);

            pCam->getExtendedInfo_1_1(id, [](const auto& result, const auto& data) {
                ASSERT_NE(EvsResult::INVALID_ARG, result);
                ASSERT_EQ(0, data.size());
            });

            // Explicitly close the camera so resources are released right away
            pEnumerator->closeCamera(pCam);
            activeCameras.clear();
        }
    }
}


/*
 * CameraOpenAggressive:
 * Opens each camera reported by the enumerator twice in a row without an intervening closeCamera
 * call.  This ensures that the intended "aggressive open" behavior works.  This is necessary for
 * the system to be tolerant of shutdown/restart race conditions.
 */
TEST_P(EvsHidlTest, CameraOpenAggressive) {
    LOG(INFO) << "Starting CameraOpenAggressive test";

    // Get the camera list
    loadCameraList();

    // Using null stream configuration makes EVS uses the default resolution and
    // output format.
    Stream nullCfg = {};

    // Open and close each camera twice
    for (auto&& cam: cameraInfo) {
        bool isLogicalCam = false;
        getPhysicalCameraIds(cam.v1.cameraId, isLogicalCam);
        if (mIsHwModule && isLogicalCam) {
            LOG(INFO) << "Skip a logical device, " << cam.v1.cameraId << " for HW target.";
            continue;
        }

        activeCameras.clear();
        sp<IEvsCamera_1_1> pCam =
            IEvsCamera_1_1::castFrom(pEnumerator->openCamera_1_1(cam.v1.cameraId, nullCfg))
            .withDefault(nullptr);
        ASSERT_NE(pCam, nullptr);

        // Store a camera handle for a clean-up
        activeCameras.push_back(pCam);

        // Verify that this camera self-identifies correctly
        pCam->getCameraInfo_1_1([&cam](CameraDesc desc) {
                                    LOG(DEBUG) << "Found camera " << desc.v1.cameraId;
                                    EXPECT_EQ(cam.v1.cameraId, desc.v1.cameraId);
                                }
        );

        sp<IEvsCamera_1_1> pCam2 =
            IEvsCamera_1_1::castFrom(pEnumerator->openCamera_1_1(cam.v1.cameraId, nullCfg))
            .withDefault(nullptr);
        ASSERT_NE(pCam2, nullptr);

        // Store a camera handle for a clean-up
        activeCameras.push_back(pCam2);

        ASSERT_NE(pCam, pCam2);

        Return<EvsResult> result = pCam->setMaxFramesInFlight(2);
        if (mIsHwModule) {
            // Verify that the old camera rejects calls via HW module.
            EXPECT_EQ(EvsResult::OWNERSHIP_LOST, EvsResult(result));
        } else {
            // default implementation supports multiple clients.
            EXPECT_EQ(EvsResult::OK, EvsResult(result));
        }

        // Close the superceded camera
        pEnumerator->closeCamera(pCam);
        activeCameras.pop_front();

        // Verify that the second camera instance self-identifies correctly
        pCam2->getCameraInfo_1_1([&cam](CameraDesc desc) {
                                     LOG(DEBUG) << "Found camera " << desc.v1.cameraId;
                                     EXPECT_EQ(cam.v1.cameraId, desc.v1.cameraId);
                                 }
        );

        // Close the second camera instance
        pEnumerator->closeCamera(pCam2);
        activeCameras.pop_front();
    }

    // Sleep here to ensure the destructor cleanup has time to run so we don't break follow on tests
    sleep(1);   // I hate that this is an arbitrary time to wait.  :(  b/36122635
}


/*
 * CameraStreamPerformance:
 * Measure and qualify the stream start up time and streaming frame rate of each reported camera
 */
TEST_P(EvsHidlTest, CameraStreamPerformance) {
    LOG(INFO) << "Starting CameraStreamPerformance test";

    // Get the camera list
    loadCameraList();

    // Using null stream configuration makes EVS uses the default resolution and
    // output format.
    Stream nullCfg = {};

    // Test each reported camera
    for (auto&& cam: cameraInfo) {
        bool isLogicalCam = false;
        auto devices = getPhysicalCameraIds(cam.v1.cameraId, isLogicalCam);
        if (mIsHwModule && isLogicalCam) {
            LOG(INFO) << "Skip a logical device " << cam.v1.cameraId;
            continue;
        }

        sp<IEvsCamera_1_1> pCam =
            IEvsCamera_1_1::castFrom(pEnumerator->openCamera_1_1(cam.v1.cameraId, nullCfg))
            .withDefault(nullptr);
        ASSERT_NE(pCam, nullptr);

        // Store a camera handle for a clean-up
        activeCameras.push_back(pCam);

        // Set up a frame receiver object which will fire up its own thread
        sp<FrameHandler> frameHandler = new FrameHandler(pCam, cam,
                                                         nullptr,
                                                         FrameHandler::eAutoReturn);

        // Start the camera's video stream
        nsecs_t start = systemTime(SYSTEM_TIME_MONOTONIC);

        bool startResult = frameHandler->startStream();
        ASSERT_TRUE(startResult);

        // Ensure the first frame arrived within the expected time
        frameHandler->waitForFrameCount(1);
        nsecs_t firstFrame = systemTime(SYSTEM_TIME_MONOTONIC);
        nsecs_t timeToFirstFrame = systemTime(SYSTEM_TIME_MONOTONIC) - start;

        // Extra delays are expected when we attempt to start a video stream on
        // the logical camera device.  The amount of delay is expected the
        // number of physical camera devices multiplied by
        // kMaxStreamStartMilliseconds at most.
        EXPECT_LE(nanoseconds_to_milliseconds(timeToFirstFrame),
                  kMaxStreamStartMilliseconds * devices.size());
        printf("%s: Measured time to first frame %0.2f ms\n",
               cam.v1.cameraId.c_str(), timeToFirstFrame * kNanoToMilliseconds);
        LOG(INFO) << cam.v1.cameraId
                  << ": Measured time to first frame "
                  << std::scientific << timeToFirstFrame * kNanoToMilliseconds
                  << " ms.";

        // Check aspect ratio
        unsigned width = 0, height = 0;
        frameHandler->getFrameDimension(&width, &height);
        EXPECT_GE(width, height);

        // Wait a bit, then ensure we get at least the required minimum number of frames
        sleep(5);
        nsecs_t end = systemTime(SYSTEM_TIME_MONOTONIC);

        // Even when the camera pointer goes out of scope, the FrameHandler object will
        // keep the stream alive unless we tell it to shutdown.
        // Also note that the FrameHandle and the Camera have a mutual circular reference, so
        // we have to break that cycle in order for either of them to get cleaned up.
        frameHandler->shutdown();

        unsigned framesReceived = 0;
        frameHandler->getFramesCounters(&framesReceived, nullptr);
        framesReceived = framesReceived - 1;    // Back out the first frame we already waited for
        nsecs_t runTime = end - firstFrame;
        float framesPerSecond = framesReceived / (runTime * kNanoToSeconds);
        printf("Measured camera rate %3.2f fps\n", framesPerSecond);
        LOG(INFO) << "Measured camera rate "
                  << std::scientific << framesPerSecond
                  << " fps.";
        EXPECT_GE(framesPerSecond, kMinimumFramesPerSecond);

        // Explicitly release the camera
        pEnumerator->closeCamera(pCam);
        activeCameras.clear();
    }
}


/*
 * CameraStreamBuffering:
 * Ensure the camera implementation behaves properly when the client holds onto buffers for more
 * than one frame time.  The camera must cleanly skip frames until the client is ready again.
 */
TEST_P(EvsHidlTest, CameraStreamBuffering) {
    LOG(INFO) << "Starting CameraStreamBuffering test";

    // Arbitrary constant (should be > 1 and less than crazy)
    static const unsigned int kBuffersToHold = 6;

    // Get the camera list
    loadCameraList();

    // Using null stream configuration makes EVS uses the default resolution and
    // output format.
    Stream nullCfg = {};

    // Test each reported camera
    for (auto&& cam: cameraInfo) {
        bool isLogicalCam = false;
        getPhysicalCameraIds(cam.v1.cameraId, isLogicalCam);
        if (mIsHwModule && isLogicalCam) {
            LOG(INFO) << "Skip a logical device " << cam.v1.cameraId << " for HW target.";
            continue;
        }

        sp<IEvsCamera_1_1> pCam =
            IEvsCamera_1_1::castFrom(pEnumerator->openCamera_1_1(cam.v1.cameraId, nullCfg))
            .withDefault(nullptr);
        ASSERT_NE(pCam, nullptr);

        // Store a camera handle for a clean-up
        activeCameras.push_back(pCam);

        // Ask for a crazy number of buffers in flight to ensure it errors correctly
        Return<EvsResult> badResult = pCam->setMaxFramesInFlight(0xFFFFFFFF);
        EXPECT_EQ(EvsResult::BUFFER_NOT_AVAILABLE, badResult);

        // Now ask for exactly two buffers in flight as we'll test behavior in that case
        Return<EvsResult> goodResult = pCam->setMaxFramesInFlight(kBuffersToHold);
        EXPECT_EQ(EvsResult::OK, goodResult);


        // Set up a frame receiver object which will fire up its own thread.
        sp<FrameHandler> frameHandler = new FrameHandler(pCam, cam,
                                                         nullptr,
                                                         FrameHandler::eNoAutoReturn);

        // Start the camera's video stream
        bool startResult = frameHandler->startStream();
        ASSERT_TRUE(startResult);

        // Check that the video stream stalls once we've gotten exactly the number of buffers
        // we requested since we told the frameHandler not to return them.
        sleep(1);   // 1 second should be enough for at least 5 frames to be delivered worst case
        unsigned framesReceived = 0;
        frameHandler->getFramesCounters(&framesReceived, nullptr);
        ASSERT_EQ(kBuffersToHold, framesReceived) << "Stream didn't stall at expected buffer limit";


        // Give back one buffer
        bool didReturnBuffer = frameHandler->returnHeldBuffer();
        EXPECT_TRUE(didReturnBuffer);

        // Once we return a buffer, it shouldn't take more than 1/10 second to get a new one
        // filled since we require 10fps minimum -- but give a 10% allowance just in case.
        usleep(110 * kMillisecondsToMicroseconds);
        frameHandler->getFramesCounters(&framesReceived, nullptr);
        EXPECT_EQ(kBuffersToHold+1, framesReceived) << "Stream should've resumed";

        // Even when the camera pointer goes out of scope, the FrameHandler object will
        // keep the stream alive unless we tell it to shutdown.
        // Also note that the FrameHandle and the Camera have a mutual circular reference, so
        // we have to break that cycle in order for either of them to get cleaned up.
        frameHandler->shutdown();

        // Explicitly release the camera
        pEnumerator->closeCamera(pCam);
        activeCameras.clear();
    }
}


/*
 * CameraToDisplayRoundTrip:
 * End to end test of data flowing from the camera to the display.  Each delivered frame of camera
 * imagery is simply copied to the display buffer and presented on screen.  This is the one test
 * which a human could observe to see the operation of the system on the physical display.
 */
TEST_P(EvsHidlTest, CameraToDisplayRoundTrip) {
    LOG(INFO) << "Starting CameraToDisplayRoundTrip test";

    // Get the camera list
    loadCameraList();

    // Using null stream configuration makes EVS uses the default resolution and
    // output format.
    Stream nullCfg = {};

    // Request available display IDs
    uint8_t targetDisplayId = 0;
    pEnumerator->getDisplayIdList([&targetDisplayId](auto ids) {
        ASSERT_GT(ids.size(), 0);
        targetDisplayId = ids[0];
    });

    // Request exclusive access to the first EVS display
    sp<IEvsDisplay_1_1> pDisplay = pEnumerator->openDisplay_1_1(targetDisplayId);
    ASSERT_NE(pDisplay, nullptr);
    LOG(INFO) << "Display " << targetDisplayId << " is alreay in use.";

    // Get the display descriptor
    pDisplay->getDisplayInfo_1_1([](const auto& config, const auto& state) {
        android::DisplayConfig* pConfig = (android::DisplayConfig*)config.data();
        const auto width = pConfig->resolution.getWidth();
        const auto height = pConfig->resolution.getHeight();
        LOG(INFO) << "    Resolution: " << width << "x" << height;
        ASSERT_GT(width, 0);
        ASSERT_GT(height, 0);

        android::ui::DisplayState* pState = (android::ui::DisplayState*)state.data();
        ASSERT_NE(pState->layerStack, -1);
    });

    // Test each reported camera
    for (auto&& cam: cameraInfo) {
        bool isLogicalCam = false;
        getPhysicalCameraIds(cam.v1.cameraId, isLogicalCam);
        if (mIsHwModule && isLogicalCam) {
            LOG(INFO) << "Skip a logical device " << cam.v1.cameraId << " for HW target.";
            continue;
        }

        sp<IEvsCamera_1_1> pCam =
            IEvsCamera_1_1::castFrom(pEnumerator->openCamera_1_1(cam.v1.cameraId, nullCfg))
            .withDefault(nullptr);
        ASSERT_NE(pCam, nullptr);

        // Store a camera handle for a clean-up
        activeCameras.push_back(pCam);

        // Set up a frame receiver object which will fire up its own thread.
        sp<FrameHandler> frameHandler = new FrameHandler(pCam, cam,
                                                         pDisplay,
                                                         FrameHandler::eAutoReturn);


        // Activate the display
        pDisplay->setDisplayState(DisplayState::VISIBLE_ON_NEXT_FRAME);

        // Start the camera's video stream
        bool startResult = frameHandler->startStream();
        ASSERT_TRUE(startResult);

        // Wait a while to let the data flow
        static const int kSecondsToWait = 5;
        const int streamTimeMs = kSecondsToWait * kSecondsToMilliseconds -
                                 kMaxStreamStartMilliseconds;
        const unsigned minimumFramesExpected = streamTimeMs * kMinimumFramesPerSecond /
                                               kSecondsToMilliseconds;
        sleep(kSecondsToWait);
        unsigned framesReceived = 0;
        unsigned framesDisplayed = 0;
        frameHandler->getFramesCounters(&framesReceived, &framesDisplayed);
        EXPECT_EQ(framesReceived, framesDisplayed);
        EXPECT_GE(framesDisplayed, minimumFramesExpected);

        // Turn off the display (yes, before the stream stops -- it should be handled)
        pDisplay->setDisplayState(DisplayState::NOT_VISIBLE);

        // Shut down the streamer
        frameHandler->shutdown();

        // Explicitly release the camera
        pEnumerator->closeCamera(pCam);
        activeCameras.clear();
    }

    // Explicitly release the display
    pEnumerator->closeDisplay(pDisplay);
}


/*
 * MultiCameraStream:
 * Verify that each client can start and stop video streams on the same
 * underlying camera.
 */
TEST_P(EvsHidlTest, MultiCameraStream) {
    LOG(INFO) << "Starting MultiCameraStream test";

    if (mIsHwModule) {
        // This test is not for HW module implementation.
        return;
    }

    // Get the camera list
    loadCameraList();

    // Using null stream configuration makes EVS uses the default resolution and
    // output format.
    Stream nullCfg = {};

    // Test each reported camera
    for (auto&& cam: cameraInfo) {
        // Create two camera clients.
        sp<IEvsCamera_1_1> pCam0 =
            IEvsCamera_1_1::castFrom(pEnumerator->openCamera_1_1(cam.v1.cameraId, nullCfg))
            .withDefault(nullptr);
        ASSERT_NE(pCam0, nullptr);

        // Store a camera handle for a clean-up
        activeCameras.push_back(pCam0);

        sp<IEvsCamera_1_1> pCam1 =
            IEvsCamera_1_1::castFrom(pEnumerator->openCamera_1_1(cam.v1.cameraId, nullCfg))
            .withDefault(nullptr);
        ASSERT_NE(pCam1, nullptr);

        // Store a camera handle for a clean-up
        activeCameras.push_back(pCam1);

        // Set up per-client frame receiver objects which will fire up its own thread
        sp<FrameHandler> frameHandler0 = new FrameHandler(pCam0, cam,
                                                          nullptr,
                                                          FrameHandler::eAutoReturn);
        ASSERT_NE(frameHandler0, nullptr);

        sp<FrameHandler> frameHandler1 = new FrameHandler(pCam1, cam,
                                                          nullptr,
                                                          FrameHandler::eAutoReturn);
        ASSERT_NE(frameHandler1, nullptr);

        // Start the camera's video stream via client 0
        bool startResult = false;
        startResult = frameHandler0->startStream() &&
                      frameHandler1->startStream();
        ASSERT_TRUE(startResult);

        // Ensure the stream starts
        frameHandler0->waitForFrameCount(1);
        frameHandler1->waitForFrameCount(1);

        nsecs_t firstFrame = systemTime(SYSTEM_TIME_MONOTONIC);

        // Wait a bit, then ensure both clients get at least the required minimum number of frames
        sleep(5);
        nsecs_t end = systemTime(SYSTEM_TIME_MONOTONIC);
        unsigned framesReceived0 = 0, framesReceived1 = 0;
        frameHandler0->getFramesCounters(&framesReceived0, nullptr);
        frameHandler1->getFramesCounters(&framesReceived1, nullptr);
        framesReceived0 = framesReceived0 - 1;    // Back out the first frame we already waited for
        framesReceived1 = framesReceived1 - 1;    // Back out the first frame we already waited for
        nsecs_t runTime = end - firstFrame;
        float framesPerSecond0 = framesReceived0 / (runTime * kNanoToSeconds);
        float framesPerSecond1 = framesReceived1 / (runTime * kNanoToSeconds);
        LOG(INFO) << "Measured camera rate "
                  << std::scientific << framesPerSecond0 << " fps and "
                  << framesPerSecond1 << " fps";
        EXPECT_GE(framesPerSecond0, kMinimumFramesPerSecond);
        EXPECT_GE(framesPerSecond1, kMinimumFramesPerSecond);

        // Shutdown one client
        frameHandler0->shutdown();

        // Read frame counters again
        frameHandler0->getFramesCounters(&framesReceived0, nullptr);
        frameHandler1->getFramesCounters(&framesReceived1, nullptr);

        // Wait a bit again
        sleep(5);
        unsigned framesReceivedAfterStop0 = 0, framesReceivedAfterStop1 = 0;
        frameHandler0->getFramesCounters(&framesReceivedAfterStop0, nullptr);
        frameHandler1->getFramesCounters(&framesReceivedAfterStop1, nullptr);
        EXPECT_EQ(framesReceived0, framesReceivedAfterStop0);
        EXPECT_LT(framesReceived1, framesReceivedAfterStop1);

        // Shutdown another
        frameHandler1->shutdown();

        // Explicitly release the camera
        pEnumerator->closeCamera(pCam0);
        pEnumerator->closeCamera(pCam1);
        activeCameras.clear();

        // TODO(b/145459970, b/145457727): below sleep() is added to ensure the
        // destruction of active camera objects; this may be related with two
        // issues.
        sleep(1);
    }
}


/*
 * CameraParameter:
 * Verify that a client can adjust a camera parameter.
 */
TEST_P(EvsHidlTest, CameraParameter) {
    LOG(INFO) << "Starting CameraParameter test";

    // Get the camera list
    loadCameraList();

    // Using null stream configuration makes EVS uses the default resolution and
    // output format.
    Stream nullCfg = {};

    // Test each reported camera
    Return<EvsResult> result = EvsResult::OK;
    for (auto&& cam: cameraInfo) {
        bool isLogicalCam = false;
        getPhysicalCameraIds(cam.v1.cameraId, isLogicalCam);
        if (isLogicalCam) {
            // TODO(b/145465724): Support camera parameter programming on
            // logical devices.
            LOG(INFO) << "Skip a logical device " << cam.v1.cameraId;
            continue;
        }

        // Create a camera client
        sp<IEvsCamera_1_1> pCam =
            IEvsCamera_1_1::castFrom(pEnumerator->openCamera_1_1(cam.v1.cameraId, nullCfg))
            .withDefault(nullptr);
        ASSERT_NE(pCam, nullptr);

        // Store a camera
        activeCameras.push_back(pCam);

        // Get the parameter list
        std::vector<CameraParam> cmds;
        pCam->getParameterList([&cmds](hidl_vec<CameraParam> cmdList) {
                cmds.reserve(cmdList.size());
                for (auto &&cmd : cmdList) {
                    cmds.push_back(cmd);
                }
            }
        );

        if (cmds.size() < 1) {
            continue;
        }

        // Set up per-client frame receiver objects which will fire up its own thread
        sp<FrameHandler> frameHandler = new FrameHandler(pCam, cam,
                                                         nullptr,
                                                         FrameHandler::eAutoReturn);
        ASSERT_NE(frameHandler, nullptr);

        // Start the camera's video stream
        bool startResult = frameHandler->startStream();
        ASSERT_TRUE(startResult);

        // Ensure the stream starts
        frameHandler->waitForFrameCount(1);

        result = pCam->setMaster();
        ASSERT_EQ(EvsResult::OK, result);

        for (auto &cmd : cmds) {
            // Get a valid parameter value range
            int32_t minVal, maxVal, step;
            pCam->getIntParameterRange(
                cmd,
                [&minVal, &maxVal, &step](int32_t val0, int32_t val1, int32_t val2) {
                    minVal = val0;
                    maxVal = val1;
                    step   = val2;
                }
            );

            EvsResult result = EvsResult::OK;
            if (cmd == CameraParam::ABSOLUTE_FOCUS) {
                // Try to turn off auto-focus
                std::vector<int32_t> values;
                pCam->setIntParameter(CameraParam::AUTO_FOCUS, 0,
                                   [&result, &values](auto status, auto effectiveValues) {
                                       result = status;
                                       if (status == EvsResult::OK) {
                                          for (auto &&v : effectiveValues) {
                                              values.push_back(v);
                                          }
                                       }
                                   });
                ASSERT_EQ(EvsResult::OK, result);
                for (auto &&v : values) {
                    ASSERT_EQ(v, 0);
                }
            }

            // Try to program a parameter with a random value [minVal, maxVal]
            int32_t val0 = minVal + (std::rand() % (maxVal - minVal));
            std::vector<int32_t> values;

            // Rounding down
            val0 = val0 - (val0 % step);
            pCam->setIntParameter(cmd, val0,
                               [&result, &values](auto status, auto effectiveValues) {
                                   result = status;
                                   if (status == EvsResult::OK) {
                                      for (auto &&v : effectiveValues) {
                                          values.push_back(v);
                                      }
                                   }
                               });

            ASSERT_EQ(EvsResult::OK, result);

            values.clear();
            pCam->getIntParameter(cmd,
                               [&result, &values](auto status, auto readValues) {
                                   result = status;
                                   if (status == EvsResult::OK) {
                                      for (auto &&v : readValues) {
                                          values.push_back(v);
                                      }
                                   }
                               });
            ASSERT_EQ(EvsResult::OK, result);
            for (auto &&v : values) {
                ASSERT_EQ(val0, v) << "Values are not matched.";
            }
        }

        result = pCam->unsetMaster();
        ASSERT_EQ(EvsResult::OK, result);

        // Shutdown
        frameHandler->shutdown();

        // Explicitly release the camera
        pEnumerator->closeCamera(pCam);
        activeCameras.clear();
    }
}


/*
 * CameraMasterRelease
 * Verify that non-master client gets notified when the master client either
 * terminates or releases a role.
 */
TEST_P(EvsHidlTest, CameraMasterRelease) {
    LOG(INFO) << "Starting CameraMasterRelease test";

    if (mIsHwModule) {
        // This test is not for HW module implementation.
        return;
    }

    // Get the camera list
    loadCameraList();

    // Using null stream configuration makes EVS uses the default resolution and
    // output format.
    Stream nullCfg = {};

    // Test each reported camera
    for (auto&& cam: cameraInfo) {
        bool isLogicalCam = false;
        getPhysicalCameraIds(cam.v1.cameraId, isLogicalCam);
        if (isLogicalCam) {
            // TODO(b/145465724): Support camera parameter programming on
            // logical devices.
            LOG(INFO) << "Skip a logical device " << cam.v1.cameraId;
            continue;
        }

        // Create two camera clients.
        sp<IEvsCamera_1_1> pCamMaster =
            IEvsCamera_1_1::castFrom(pEnumerator->openCamera_1_1(cam.v1.cameraId, nullCfg))
            .withDefault(nullptr);
        ASSERT_NE(pCamMaster, nullptr);

        // Store a camera handle for a clean-up
        activeCameras.push_back(pCamMaster);

        sp<IEvsCamera_1_1> pCamNonMaster =
            IEvsCamera_1_1::castFrom(pEnumerator->openCamera_1_1(cam.v1.cameraId, nullCfg))
            .withDefault(nullptr);
        ASSERT_NE(pCamNonMaster, nullptr);

        // Store a camera handle for a clean-up
        activeCameras.push_back(pCamNonMaster);

        // Set up per-client frame receiver objects which will fire up its own thread
        sp<FrameHandler> frameHandlerMaster =
            new FrameHandler(pCamMaster, cam,
                             nullptr,
                             FrameHandler::eAutoReturn);
        ASSERT_NE(frameHandlerMaster, nullptr);
        sp<FrameHandler> frameHandlerNonMaster =
            new FrameHandler(pCamNonMaster, cam,
                             nullptr,
                             FrameHandler::eAutoReturn);
        ASSERT_NE(frameHandlerNonMaster, nullptr);

        // Set one client as the master
        EvsResult result = pCamMaster->setMaster();
        ASSERT_TRUE(result == EvsResult::OK);

        // Try to set another client as the master.
        result = pCamNonMaster->setMaster();
        ASSERT_TRUE(result == EvsResult::OWNERSHIP_LOST);

        // Start the camera's video stream via a master client.
        bool startResult = frameHandlerMaster->startStream();
        ASSERT_TRUE(startResult);

        // Ensure the stream starts
        frameHandlerMaster->waitForFrameCount(1);

        // Start the camera's video stream via another client
        startResult = frameHandlerNonMaster->startStream();
        ASSERT_TRUE(startResult);

        // Ensure the stream starts
        frameHandlerNonMaster->waitForFrameCount(1);

        // Non-master client expects to receive a master role relesed
        // notification.
        EvsEventDesc aTargetEvent  = {};
        EvsEventDesc aNotification = {};

        bool listening = false;
        std::mutex eventLock;
        std::condition_variable eventCond;
        std::thread listener = std::thread(
            [&aNotification, &frameHandlerNonMaster, &listening, &eventCond]() {
                // Notify that a listening thread is running.
                listening = true;
                eventCond.notify_all();

                EvsEventDesc aTargetEvent;
                aTargetEvent.aType = EvsEventType::MASTER_RELEASED;
                if (!frameHandlerNonMaster->waitForEvent(aTargetEvent, aNotification, true)) {
                    LOG(WARNING) << "A timer is expired before a target event is fired.";
                }

            }
        );

        // Wait until a listening thread starts.
        std::unique_lock<std::mutex> lock(eventLock);
        auto timer = std::chrono::system_clock::now();
        while (!listening) {
            timer += 1s;
            eventCond.wait_until(lock, timer);
        }
        lock.unlock();

        // Release a master role.
        pCamMaster->unsetMaster();

        // Join a listening thread.
        if (listener.joinable()) {
            listener.join();
        }

        // Verify change notifications.
        ASSERT_EQ(EvsEventType::MASTER_RELEASED,
                  static_cast<EvsEventType>(aNotification.aType));

        // Non-master becomes a master.
        result = pCamNonMaster->setMaster();
        ASSERT_TRUE(result == EvsResult::OK);

        // Previous master client fails to become a master.
        result = pCamMaster->setMaster();
        ASSERT_TRUE(result == EvsResult::OWNERSHIP_LOST);

        listening = false;
        listener = std::thread(
            [&aNotification, &frameHandlerMaster, &listening, &eventCond]() {
                // Notify that a listening thread is running.
                listening = true;
                eventCond.notify_all();

                EvsEventDesc aTargetEvent;
                aTargetEvent.aType = EvsEventType::MASTER_RELEASED;
                if (!frameHandlerMaster->waitForEvent(aTargetEvent, aNotification, true)) {
                    LOG(WARNING) << "A timer is expired before a target event is fired.";
                }

            }
        );

        // Wait until a listening thread starts.
        timer = std::chrono::system_clock::now();
        lock.lock();
        while (!listening) {
            eventCond.wait_until(lock, timer + 1s);
        }
        lock.unlock();

        // Closing current master client.
        frameHandlerNonMaster->shutdown();

        // Join a listening thread.
        if (listener.joinable()) {
            listener.join();
        }

        // Verify change notifications.
        ASSERT_EQ(EvsEventType::MASTER_RELEASED,
                  static_cast<EvsEventType>(aNotification.aType));

        // Closing streams.
        frameHandlerMaster->shutdown();

        // Explicitly release the camera
        pEnumerator->closeCamera(pCamMaster);
        pEnumerator->closeCamera(pCamNonMaster);
        activeCameras.clear();
    }
}


/*
 * MultiCameraParameter:
 * Verify that master and non-master clients behave as expected when they try to adjust
 * camera parameters.
 */
TEST_P(EvsHidlTest, MultiCameraParameter) {
    LOG(INFO) << "Starting MultiCameraParameter test";

    if (mIsHwModule) {
        // This test is not for HW module implementation.
        return;
    }

    // Get the camera list
    loadCameraList();

    // Using null stream configuration makes EVS uses the default resolution and
    // output format.
    Stream nullCfg = {};

    // Test each reported camera
    for (auto&& cam: cameraInfo) {
        bool isLogicalCam = false;
        getPhysicalCameraIds(cam.v1.cameraId, isLogicalCam);
        if (isLogicalCam) {
            // TODO(b/145465724): Support camera parameter programming on
            // logical devices.
            LOG(INFO) << "Skip a logical device " << cam.v1.cameraId;
            continue;
        }

        // Create two camera clients.
        sp<IEvsCamera_1_1> pCamMaster =
            IEvsCamera_1_1::castFrom(pEnumerator->openCamera_1_1(cam.v1.cameraId, nullCfg))
            .withDefault(nullptr);
        ASSERT_NE(pCamMaster, nullptr);

        // Store a camera handle for a clean-up
        activeCameras.push_back(pCamMaster);

        sp<IEvsCamera_1_1> pCamNonMaster =
            IEvsCamera_1_1::castFrom(pEnumerator->openCamera_1_1(cam.v1.cameraId, nullCfg))
            .withDefault(nullptr);
        ASSERT_NE(pCamNonMaster, nullptr);

        // Store a camera handle for a clean-up
        activeCameras.push_back(pCamNonMaster);

        // Get the parameter list
        std::vector<CameraParam> camMasterCmds, camNonMasterCmds;
        pCamMaster->getParameterList([&camMasterCmds](hidl_vec<CameraParam> cmdList) {
                camMasterCmds.reserve(cmdList.size());
                for (auto &&cmd : cmdList) {
                    camMasterCmds.push_back(cmd);
                }
            }
        );

        pCamNonMaster->getParameterList([&camNonMasterCmds](hidl_vec<CameraParam> cmdList) {
                camNonMasterCmds.reserve(cmdList.size());
                for (auto &&cmd : cmdList) {
                    camNonMasterCmds.push_back(cmd);
                }
            }
        );

        if (camMasterCmds.size() < 1 ||
            camNonMasterCmds.size() < 1) {
            // Skip a camera device if it does not support any parameter.
            continue;
        }

        // Set up per-client frame receiver objects which will fire up its own thread
        sp<FrameHandler> frameHandlerMaster =
            new FrameHandler(pCamMaster, cam,
                             nullptr,
                             FrameHandler::eAutoReturn);
        ASSERT_NE(frameHandlerMaster, nullptr);
        sp<FrameHandler> frameHandlerNonMaster =
            new FrameHandler(pCamNonMaster, cam,
                             nullptr,
                             FrameHandler::eAutoReturn);
        ASSERT_NE(frameHandlerNonMaster, nullptr);

        // Set one client as the master
        EvsResult result = pCamMaster->setMaster();
        ASSERT_EQ(EvsResult::OK, result);

        // Try to set another client as the master.
        result = pCamNonMaster->setMaster();
        ASSERT_EQ(EvsResult::OWNERSHIP_LOST, result);

        // Start the camera's video stream via a master client.
        bool startResult = frameHandlerMaster->startStream();
        ASSERT_TRUE(startResult);

        // Ensure the stream starts
        frameHandlerMaster->waitForFrameCount(1);

        // Start the camera's video stream via another client
        startResult = frameHandlerNonMaster->startStream();
        ASSERT_TRUE(startResult);

        // Ensure the stream starts
        frameHandlerNonMaster->waitForFrameCount(1);

        int32_t val0 = 0;
        std::vector<int32_t> values;
        EvsEventDesc aNotification0 = {};
        EvsEventDesc aNotification1 = {};
        for (auto &cmd : camMasterCmds) {
            // Get a valid parameter value range
            int32_t minVal, maxVal, step;
            pCamMaster->getIntParameterRange(
                cmd,
                [&minVal, &maxVal, &step](int32_t val0, int32_t val1, int32_t val2) {
                    minVal = val0;
                    maxVal = val1;
                    step   = val2;
                }
            );

            EvsResult result = EvsResult::OK;
            if (cmd == CameraParam::ABSOLUTE_FOCUS) {
                // Try to turn off auto-focus
                values.clear();
                pCamMaster->setIntParameter(CameraParam::AUTO_FOCUS, 0,
                                   [&result, &values](auto status, auto effectiveValues) {
                                       result = status;
                                       if (status == EvsResult::OK) {
                                          for (auto &&v : effectiveValues) {
                                              values.push_back(v);
                                          }
                                       }
                                   });
                ASSERT_EQ(EvsResult::OK, result);
                for (auto &&v : values) {
                    ASSERT_EQ(v, 0);
                }
            }

            // Calculate a parameter value to program.
            val0 = minVal + (std::rand() % (maxVal - minVal));
            val0 = val0 - (val0 % step);

            // Prepare and start event listeners.
            bool listening0 = false;
            bool listening1 = false;
            std::condition_variable eventCond;
            std::thread listener0 = std::thread(
                [cmd, val0,
                 &aNotification0, &frameHandlerMaster, &listening0, &listening1, &eventCond]() {
                    listening0 = true;
                    if (listening1) {
                        eventCond.notify_all();
                    }

                    EvsEventDesc aTargetEvent;
                    aTargetEvent.aType = EvsEventType::PARAMETER_CHANGED;
                    aTargetEvent.payload[0] = static_cast<uint32_t>(cmd);
                    aTargetEvent.payload[1] = val0;
                    if (!frameHandlerMaster->waitForEvent(aTargetEvent, aNotification0)) {
                        LOG(WARNING) << "A timer is expired before a target event is fired.";
                    }
                }
            );
            std::thread listener1 = std::thread(
                [cmd, val0,
                 &aNotification1, &frameHandlerNonMaster, &listening0, &listening1, &eventCond]() {
                    listening1 = true;
                    if (listening0) {
                        eventCond.notify_all();
                    }

                    EvsEventDesc aTargetEvent;
                    aTargetEvent.aType = EvsEventType::PARAMETER_CHANGED;
                    aTargetEvent.payload[0] = static_cast<uint32_t>(cmd);
                    aTargetEvent.payload[1] = val0;
                    if (!frameHandlerNonMaster->waitForEvent(aTargetEvent, aNotification1)) {
                        LOG(WARNING) << "A timer is expired before a target event is fired.";
                    }
                }
            );

            // Wait until a listening thread starts.
            std::mutex eventLock;
            std::unique_lock<std::mutex> lock(eventLock);
            auto timer = std::chrono::system_clock::now();
            while (!listening0 || !listening1) {
                eventCond.wait_until(lock, timer + 1s);
            }
            lock.unlock();

            // Try to program a parameter
            values.clear();
            pCamMaster->setIntParameter(cmd, val0,
                                     [&result, &values](auto status, auto effectiveValues) {
                                         result = status;
                                         if (status == EvsResult::OK) {
                                            for (auto &&v : effectiveValues) {
                                                values.push_back(v);
                                            }
                                         }
                                     });

            ASSERT_EQ(EvsResult::OK, result);
            for (auto &&v : values) {
                ASSERT_EQ(val0, v) << "Values are not matched.";
            }

            // Join a listening thread.
            if (listener0.joinable()) {
                listener0.join();
            }
            if (listener1.joinable()) {
                listener1.join();
            }

            // Verify a change notification
            ASSERT_EQ(EvsEventType::PARAMETER_CHANGED,
                      static_cast<EvsEventType>(aNotification0.aType));
            ASSERT_EQ(EvsEventType::PARAMETER_CHANGED,
                      static_cast<EvsEventType>(aNotification1.aType));
            ASSERT_EQ(cmd,
                      static_cast<CameraParam>(aNotification0.payload[0]));
            ASSERT_EQ(cmd,
                      static_cast<CameraParam>(aNotification1.payload[0]));
            for (auto &&v : values) {
                ASSERT_EQ(v,
                          static_cast<int32_t>(aNotification0.payload[1]));
                ASSERT_EQ(v,
                          static_cast<int32_t>(aNotification1.payload[1]));
            }

            // Clients expects to receive a parameter change notification
            // whenever a master client adjusts it.
            values.clear();
            pCamMaster->getIntParameter(cmd,
                                     [&result, &values](auto status, auto readValues) {
                                         result = status;
                                         if (status == EvsResult::OK) {
                                            for (auto &&v : readValues) {
                                                values.push_back(v);
                                            }
                                         }
                                     });
            ASSERT_EQ(EvsResult::OK, result);
            for (auto &&v : values) {
                ASSERT_EQ(val0, v) << "Values are not matched.";
            }
        }

        // Try to adjust a parameter via non-master client
        values.clear();
        pCamNonMaster->setIntParameter(camNonMasterCmds[0], val0,
                                    [&result, &values](auto status, auto effectiveValues) {
                                        result = status;
                                        if (status == EvsResult::OK) {
                                            for (auto &&v : effectiveValues) {
                                                values.push_back(v);
                                            }
                                        }
                                    });
        ASSERT_EQ(EvsResult::INVALID_ARG, result);

        // Non-master client attemps to be a master
        result = pCamNonMaster->setMaster();
        ASSERT_EQ(EvsResult::OWNERSHIP_LOST, result);

        // Master client retires from a master role
        bool listening = false;
        std::condition_variable eventCond;
        std::thread listener = std::thread(
            [&aNotification0, &frameHandlerNonMaster, &listening, &eventCond]() {
                listening = true;
                eventCond.notify_all();

                EvsEventDesc aTargetEvent;
                aTargetEvent.aType = EvsEventType::MASTER_RELEASED;
                if (!frameHandlerNonMaster->waitForEvent(aTargetEvent, aNotification0, true)) {
                    LOG(WARNING) << "A timer is expired before a target event is fired.";
                }
            }
        );

        std::mutex eventLock;
        auto timer = std::chrono::system_clock::now();
        std::unique_lock<std::mutex> lock(eventLock);
        while (!listening) {
            eventCond.wait_until(lock, timer + 1s);
        }
        lock.unlock();

        result = pCamMaster->unsetMaster();
        ASSERT_EQ(EvsResult::OK, result);

        if (listener.joinable()) {
            listener.join();
        }
        ASSERT_EQ(EvsEventType::MASTER_RELEASED,
                  static_cast<EvsEventType>(aNotification0.aType));

        // Try to adjust a parameter after being retired
        values.clear();
        pCamMaster->setIntParameter(camMasterCmds[0], val0,
                                 [&result, &values](auto status, auto effectiveValues) {
                                     result = status;
                                     if (status == EvsResult::OK) {
                                        for (auto &&v : effectiveValues) {
                                            values.push_back(v);
                                        }
                                     }
                                 });
        ASSERT_EQ(EvsResult::INVALID_ARG, result);

        // Non-master client becomes a master
        result = pCamNonMaster->setMaster();
        ASSERT_EQ(EvsResult::OK, result);

        // Try to adjust a parameter via new master client
        for (auto &cmd : camNonMasterCmds) {
            // Get a valid parameter value range
            int32_t minVal, maxVal, step;
            pCamNonMaster->getIntParameterRange(
                cmd,
                [&minVal, &maxVal, &step](int32_t val0, int32_t val1, int32_t val2) {
                    minVal = val0;
                    maxVal = val1;
                    step   = val2;
                }
            );

            EvsResult result = EvsResult::OK;
            values.clear();
            if (cmd == CameraParam::ABSOLUTE_FOCUS) {
                // Try to turn off auto-focus
                values.clear();
                pCamNonMaster->setIntParameter(CameraParam::AUTO_FOCUS, 0,
                                   [&result, &values](auto status, auto effectiveValues) {
                                       result = status;
                                       if (status == EvsResult::OK) {
                                          for (auto &&v : effectiveValues) {
                                              values.push_back(v);
                                          }
                                       }
                                   });
                ASSERT_EQ(EvsResult::OK, result);
                for (auto &&v : values) {
                    ASSERT_EQ(v, 0);
                }
            }

            // Calculate a parameter value to program.  This is being rounding down.
            val0 = minVal + (std::rand() % (maxVal - minVal));
            val0 = val0 - (val0 % step);

            // Prepare and start event listeners.
            bool listening0 = false;
            bool listening1 = false;
            std::condition_variable eventCond;
            std::thread listener0 = std::thread(
                [&cmd, &val0, &aNotification0, &frameHandlerMaster, &listening0, &listening1, &eventCond]() {
                    listening0 = true;
                    if (listening1) {
                        eventCond.notify_all();
                    }

                    EvsEventDesc aTargetEvent;
                    aTargetEvent.aType = EvsEventType::PARAMETER_CHANGED;
                    aTargetEvent.payload[0] = static_cast<uint32_t>(cmd);
                    aTargetEvent.payload[1] = val0;
                    if (!frameHandlerMaster->waitForEvent(aTargetEvent, aNotification0)) {
                        LOG(WARNING) << "A timer is expired before a target event is fired.";
                    }
                }
            );
            std::thread listener1 = std::thread(
                [&cmd, &val0, &aNotification1, &frameHandlerNonMaster, &listening0, &listening1, &eventCond]() {
                    listening1 = true;
                    if (listening0) {
                        eventCond.notify_all();
                    }

                    EvsEventDesc aTargetEvent;
                    aTargetEvent.aType = EvsEventType::PARAMETER_CHANGED;
                    aTargetEvent.payload[0] = static_cast<uint32_t>(cmd);
                    aTargetEvent.payload[1] = val0;
                    if (!frameHandlerNonMaster->waitForEvent(aTargetEvent, aNotification1)) {
                        LOG(WARNING) << "A timer is expired before a target event is fired.";
                    }
                }
            );

            // Wait until a listening thread starts.
            std::mutex eventLock;
            std::unique_lock<std::mutex> lock(eventLock);
            auto timer = std::chrono::system_clock::now();
            while (!listening0 || !listening1) {
                eventCond.wait_until(lock, timer + 1s);
            }
            lock.unlock();

            // Try to program a parameter
            values.clear();
            pCamNonMaster->setIntParameter(cmd, val0,
                                        [&result, &values](auto status, auto effectiveValues) {
                                            result = status;
                                            if (status == EvsResult::OK) {
                                                for (auto &&v : effectiveValues) {
                                                    values.push_back(v);
                                                }
                                            }
                                        });
            ASSERT_EQ(EvsResult::OK, result);

            // Clients expects to receive a parameter change notification
            // whenever a master client adjusts it.
            values.clear();
            pCamNonMaster->getIntParameter(cmd,
                                        [&result, &values](auto status, auto readValues) {
                                            result = status;
                                            if (status == EvsResult::OK) {
                                                for (auto &&v : readValues) {
                                                    values.push_back(v);
                                                }
                                            }
                                        });
            ASSERT_EQ(EvsResult::OK, result);
            for (auto &&v : values) {
                ASSERT_EQ(val0, v) << "Values are not matched.";
            }

            // Join a listening thread.
            if (listener0.joinable()) {
                listener0.join();
            }
            if (listener1.joinable()) {
                listener1.join();
            }

            // Verify a change notification
            ASSERT_EQ(EvsEventType::PARAMETER_CHANGED,
                      static_cast<EvsEventType>(aNotification0.aType));
            ASSERT_EQ(EvsEventType::PARAMETER_CHANGED,
                      static_cast<EvsEventType>(aNotification1.aType));
            ASSERT_EQ(cmd,
                      static_cast<CameraParam>(aNotification0.payload[0]));
            ASSERT_EQ(cmd,
                      static_cast<CameraParam>(aNotification1.payload[0]));
            for (auto &&v : values) {
                ASSERT_EQ(v,
                          static_cast<int32_t>(aNotification0.payload[1]));
                ASSERT_EQ(v,
                          static_cast<int32_t>(aNotification1.payload[1]));
            }
        }

        // New master retires from a master role
        result = pCamNonMaster->unsetMaster();
        ASSERT_EQ(EvsResult::OK, result);

        // Shutdown
        frameHandlerMaster->shutdown();
        frameHandlerNonMaster->shutdown();

        // Explicitly release the camera
        pEnumerator->closeCamera(pCamMaster);
        pEnumerator->closeCamera(pCamNonMaster);
        activeCameras.clear();
    }
}


/*
 * HighPriorityCameraClient:
 * EVS client, which owns the display, is priortized and therefore can take over
 * a master role from other EVS clients without the display.
 */
TEST_P(EvsHidlTest, HighPriorityCameraClient) {
    LOG(INFO) << "Starting HighPriorityCameraClient test";

    if (mIsHwModule) {
        // This test is not for HW module implementation.
        return;
    }

    // Get the camera list
    loadCameraList();

    // Using null stream configuration makes EVS uses the default resolution and
    // output format.
    Stream nullCfg = {};

    // Request exclusive access to the EVS display
    sp<IEvsDisplay_1_0> pDisplay = pEnumerator->openDisplay();
    ASSERT_NE(pDisplay, nullptr);

    // Test each reported camera
    for (auto&& cam: cameraInfo) {
        // Create two clients
        sp<IEvsCamera_1_1> pCam0 =
            IEvsCamera_1_1::castFrom(pEnumerator->openCamera_1_1(cam.v1.cameraId, nullCfg))
            .withDefault(nullptr);
        ASSERT_NE(pCam0, nullptr);

        // Store a camera handle for a clean-up
        activeCameras.push_back(pCam0);

        sp<IEvsCamera_1_1> pCam1 =
            IEvsCamera_1_1::castFrom(pEnumerator->openCamera_1_1(cam.v1.cameraId, nullCfg))
            .withDefault(nullptr);
        ASSERT_NE(pCam1, nullptr);

        // Store a camera handle for a clean-up
        activeCameras.push_back(pCam1);

        // Get the parameter list; this test will use the first command in both
        // lists.
        std::vector<CameraParam> cam0Cmds, cam1Cmds;
        pCam0->getParameterList([&cam0Cmds](hidl_vec<CameraParam> cmdList) {
                cam0Cmds.reserve(cmdList.size());
                for (auto &&cmd : cmdList) {
                    cam0Cmds.push_back(cmd);
                }
            }
        );

        pCam1->getParameterList([&cam1Cmds](hidl_vec<CameraParam> cmdList) {
                cam1Cmds.reserve(cmdList.size());
                for (auto &&cmd : cmdList) {
                    cam1Cmds.push_back(cmd);
                }
            }
        );
        if (cam0Cmds.size() < 1 || cam1Cmds.size() < 1) {
            // Cannot execute this test.
            return;
        }

        // Set up a frame receiver object which will fire up its own thread.
        sp<FrameHandler> frameHandler0 = new FrameHandler(pCam0, cam,
                                                          pDisplay,
                                                          FrameHandler::eAutoReturn);
        sp<FrameHandler> frameHandler1 = new FrameHandler(pCam1, cam,
                                                          nullptr,
                                                          FrameHandler::eAutoReturn);

        // Activate the display
        pDisplay->setDisplayState(DisplayState::VISIBLE_ON_NEXT_FRAME);

        // Start the camera's video stream
        ASSERT_TRUE(frameHandler0->startStream());
        ASSERT_TRUE(frameHandler1->startStream());

        // Ensure the stream starts
        frameHandler0->waitForFrameCount(1);
        frameHandler1->waitForFrameCount(1);

        // Client 1 becomes a master and programs a parameter.
        EvsResult result = EvsResult::OK;
        // Get a valid parameter value range
        int32_t minVal, maxVal, step;
        pCam1->getIntParameterRange(
            cam1Cmds[0],
            [&minVal, &maxVal, &step](int32_t val0, int32_t val1, int32_t val2) {
                minVal = val0;
                maxVal = val1;
                step   = val2;
            }
        );

        // Client1 becomes a master
        result = pCam1->setMaster();
        ASSERT_EQ(EvsResult::OK, result);

        std::vector<int32_t> values;
        EvsEventDesc aTargetEvent  = {};
        EvsEventDesc aNotification = {};
        bool listening = false;
        std::mutex eventLock;
        std::condition_variable eventCond;
        if (cam1Cmds[0] == CameraParam::ABSOLUTE_FOCUS) {
            std::thread listener = std::thread(
                [&frameHandler0, &aNotification, &listening, &eventCond] {
                    listening = true;
                    eventCond.notify_all();

                    EvsEventDesc aTargetEvent;
                    aTargetEvent.aType = EvsEventType::PARAMETER_CHANGED;
                    aTargetEvent.payload[0] = static_cast<uint32_t>(CameraParam::AUTO_FOCUS);
                    aTargetEvent.payload[1] = 0;
                    if (!frameHandler0->waitForEvent(aTargetEvent, aNotification)) {
                        LOG(WARNING) << "A timer is expired before a target event is fired.";
                    }
                }
            );

            // Wait until a lister starts.
            std::unique_lock<std::mutex> lock(eventLock);
            auto timer = std::chrono::system_clock::now();
            while (!listening) {
                eventCond.wait_until(lock, timer + 1s);
            }
            lock.unlock();

            // Try to turn off auto-focus
            pCam1->setIntParameter(CameraParam::AUTO_FOCUS, 0,
                               [&result, &values](auto status, auto effectiveValues) {
                                   result = status;
                                   if (status == EvsResult::OK) {
                                      for (auto &&v : effectiveValues) {
                                          values.push_back(v);
                                      }
                                   }
                               });
            ASSERT_EQ(EvsResult::OK, result);
            for (auto &&v : values) {
                ASSERT_EQ(v, 0);
            }

            // Join a listener
            if (listener.joinable()) {
                listener.join();
            }

            // Make sure AUTO_FOCUS is off.
            ASSERT_EQ(static_cast<EvsEventType>(aNotification.aType),
                      EvsEventType::PARAMETER_CHANGED);
        }

        // Try to program a parameter with a random value [minVal, maxVal] after
        // rounding it down.
        int32_t val0 = minVal + (std::rand() % (maxVal - minVal));
        val0 = val0 - (val0 % step);

        std::thread listener = std::thread(
            [&frameHandler1, &aNotification, &listening, &eventCond, &cam1Cmds, val0] {
                listening = true;
                eventCond.notify_all();

                EvsEventDesc aTargetEvent;
                aTargetEvent.aType = EvsEventType::PARAMETER_CHANGED;
                aTargetEvent.payload[0] = static_cast<uint32_t>(cam1Cmds[0]);
                aTargetEvent.payload[1] = val0;
                if (!frameHandler1->waitForEvent(aTargetEvent, aNotification)) {
                    LOG(WARNING) << "A timer is expired before a target event is fired.";
                }
            }
        );

        // Wait until a lister starts.
        listening = false;
        std::unique_lock<std::mutex> lock(eventLock);
        auto timer = std::chrono::system_clock::now();
        while (!listening) {
            eventCond.wait_until(lock, timer + 1s);
        }
        lock.unlock();

        values.clear();
        pCam1->setIntParameter(cam1Cmds[0], val0,
                            [&result, &values](auto status, auto effectiveValues) {
                                result = status;
                                if (status == EvsResult::OK) {
                                    for (auto &&v : effectiveValues) {
                                        values.push_back(v);
                                    }
                                }
                            });
        ASSERT_EQ(EvsResult::OK, result);
        for (auto &&v : values) {
            ASSERT_EQ(val0, v);
        }

        // Join a listener
        if (listener.joinable()) {
            listener.join();
        }

        // Verify a change notification
        ASSERT_EQ(static_cast<EvsEventType>(aNotification.aType),
                  EvsEventType::PARAMETER_CHANGED);
        ASSERT_EQ(static_cast<CameraParam>(aNotification.payload[0]),
                  cam1Cmds[0]);
        for (auto &&v : values) {
            ASSERT_EQ(v, static_cast<int32_t>(aNotification.payload[1]));
        }

        listener = std::thread(
            [&frameHandler1, &aNotification, &listening, &eventCond] {
                listening = true;
                eventCond.notify_all();

                EvsEventDesc aTargetEvent;
                aTargetEvent.aType = EvsEventType::MASTER_RELEASED;
                if (!frameHandler1->waitForEvent(aTargetEvent, aNotification, true)) {
                    LOG(WARNING) << "A timer is expired before a target event is fired.";
                }
            }
        );

        // Wait until a lister starts.
        listening = false;
        lock.lock();
        timer = std::chrono::system_clock::now();
        while (!listening) {
            eventCond.wait_until(lock, timer + 1s);
        }
        lock.unlock();

        // Client 0 steals a master role
        ASSERT_EQ(EvsResult::OK, pCam0->forceMaster(pDisplay));

        // Join a listener
        if (listener.joinable()) {
            listener.join();
        }

        ASSERT_EQ(static_cast<EvsEventType>(aNotification.aType),
                  EvsEventType::MASTER_RELEASED);

        // Client 0 programs a parameter
        val0 = minVal + (std::rand() % (maxVal - minVal));

        // Rounding down
        val0 = val0 - (val0 % step);

        if (cam0Cmds[0] == CameraParam::ABSOLUTE_FOCUS) {
            std::thread listener = std::thread(
                [&frameHandler1, &aNotification, &listening, &eventCond] {
                    listening = true;
                    eventCond.notify_all();

                    EvsEventDesc aTargetEvent;
                    aTargetEvent.aType = EvsEventType::PARAMETER_CHANGED;
                    aTargetEvent.payload[0] = static_cast<uint32_t>(CameraParam::AUTO_FOCUS);
                    aTargetEvent.payload[1] = 0;
                    if (!frameHandler1->waitForEvent(aTargetEvent, aNotification)) {
                        LOG(WARNING) << "A timer is expired before a target event is fired.";
                    }
                }
            );

            // Wait until a lister starts.
            std::unique_lock<std::mutex> lock(eventLock);
            auto timer = std::chrono::system_clock::now();
            while (!listening) {
                eventCond.wait_until(lock, timer + 1s);
            }
            lock.unlock();

            // Try to turn off auto-focus
            values.clear();
            pCam0->setIntParameter(CameraParam::AUTO_FOCUS, 0,
                               [&result, &values](auto status, auto effectiveValues) {
                                   result = status;
                                   if (status == EvsResult::OK) {
                                      for (auto &&v : effectiveValues) {
                                          values.push_back(v);
                                      }
                                   }
                               });
            ASSERT_EQ(EvsResult::OK, result);
            for (auto &&v : values) {
                ASSERT_EQ(v, 0);
            }

            // Join a listener
            if (listener.joinable()) {
                listener.join();
            }

            // Make sure AUTO_FOCUS is off.
            ASSERT_EQ(static_cast<EvsEventType>(aNotification.aType),
                      EvsEventType::PARAMETER_CHANGED);
        }

        listener = std::thread(
            [&frameHandler0, &aNotification, &listening, &eventCond, &cam0Cmds, val0] {
                listening = true;
                eventCond.notify_all();

                EvsEventDesc aTargetEvent;
                aTargetEvent.aType = EvsEventType::PARAMETER_CHANGED;
                aTargetEvent.payload[0] = static_cast<uint32_t>(cam0Cmds[0]);
                aTargetEvent.payload[1] = val0;
                if (!frameHandler0->waitForEvent(aTargetEvent, aNotification)) {
                    LOG(WARNING) << "A timer is expired before a target event is fired.";
                }
            }
        );

        // Wait until a lister starts.
        listening = false;
        timer = std::chrono::system_clock::now();
        lock.lock();
        while (!listening) {
            eventCond.wait_until(lock, timer + 1s);
        }
        lock.unlock();

        values.clear();
        pCam0->setIntParameter(cam0Cmds[0], val0,
                            [&result, &values](auto status, auto effectiveValues) {
                                result = status;
                                if (status == EvsResult::OK) {
                                    for (auto &&v : effectiveValues) {
                                        values.push_back(v);
                                    }
                                }
                            });
        ASSERT_EQ(EvsResult::OK, result);

        // Join a listener
        if (listener.joinable()) {
            listener.join();
        }
        // Verify a change notification
        ASSERT_EQ(static_cast<EvsEventType>(aNotification.aType),
                  EvsEventType::PARAMETER_CHANGED);
        ASSERT_EQ(static_cast<CameraParam>(aNotification.payload[0]),
                  cam0Cmds[0]);
        for (auto &&v : values) {
            ASSERT_EQ(v, static_cast<int32_t>(aNotification.payload[1]));
        }

        // Turn off the display (yes, before the stream stops -- it should be handled)
        pDisplay->setDisplayState(DisplayState::NOT_VISIBLE);

        // Shut down the streamer
        frameHandler0->shutdown();
        frameHandler1->shutdown();

        // Explicitly release the camera
        pEnumerator->closeCamera(pCam0);
        pEnumerator->closeCamera(pCam1);
        activeCameras.clear();

    }

    // Explicitly release the display
    pEnumerator->closeDisplay(pDisplay);
}


/*
 * CameraUseStreamConfigToDisplay:
 * End to end test of data flowing from the camera to the display.  Similar to
 * CameraToDisplayRoundTrip test case but this case retrieves available stream
 * configurations from EVS and uses one of them to start a video stream.
 */
TEST_P(EvsHidlTest, CameraUseStreamConfigToDisplay) {
    LOG(INFO) << "Starting CameraUseStreamConfigToDisplay test";

    // Get the camera list
    loadCameraList();

    // Request exclusive access to the EVS display
    sp<IEvsDisplay_1_0> pDisplay = pEnumerator->openDisplay();
    ASSERT_NE(pDisplay, nullptr);

    // Test each reported camera
    for (auto&& cam: cameraInfo) {
        // choose a configuration that has a frame rate faster than minReqFps.
        Stream targetCfg = {};
        const int32_t minReqFps = 15;
        int32_t maxArea = 0;
        camera_metadata_entry_t streamCfgs;
        bool foundCfg = false;
        if (!find_camera_metadata_entry(
                 reinterpret_cast<camera_metadata_t *>(cam.metadata.data()),
                 ANDROID_SCALER_AVAILABLE_STREAM_CONFIGURATIONS,
                 &streamCfgs)) {
            // Stream configurations are found in metadata
            RawStreamConfig *ptr = reinterpret_cast<RawStreamConfig *>(streamCfgs.data.i32);
            for (unsigned idx = 0; idx < streamCfgs.count; idx += kStreamCfgSz) {
                if (ptr->direction == ANDROID_SCALER_AVAILABLE_STREAM_CONFIGURATIONS_OUTPUT &&
                    ptr->format == HAL_PIXEL_FORMAT_RGBA_8888) {

                    if (ptr->width * ptr->height > maxArea &&
                        ptr->framerate >= minReqFps) {
                        targetCfg.width = ptr->width;
                        targetCfg.height = ptr->height;

                        maxArea = ptr->width * ptr->height;
                        foundCfg = true;
                    }
                }
                ++ptr;
            }
        }
        targetCfg.format =
            static_cast<PixelFormat>(HAL_PIXEL_FORMAT_RGBA_8888);

        if (!foundCfg) {
            // Current EVS camera does not provide stream configurations in the
            // metadata.
            continue;
        }

        sp<IEvsCamera_1_1> pCam =
            IEvsCamera_1_1::castFrom(pEnumerator->openCamera_1_1(cam.v1.cameraId, targetCfg))
            .withDefault(nullptr);
        ASSERT_NE(pCam, nullptr);

        // Store a camera handle for a clean-up
        activeCameras.push_back(pCam);

        // Set up a frame receiver object which will fire up its own thread.
        sp<FrameHandler> frameHandler = new FrameHandler(pCam, cam,
                                                         pDisplay,
                                                         FrameHandler::eAutoReturn);


        // Activate the display
        pDisplay->setDisplayState(DisplayState::VISIBLE_ON_NEXT_FRAME);

        // Start the camera's video stream
        bool startResult = frameHandler->startStream();
        ASSERT_TRUE(startResult);

        // Wait a while to let the data flow
        static const int kSecondsToWait = 5;
        const int streamTimeMs = kSecondsToWait * kSecondsToMilliseconds -
                                 kMaxStreamStartMilliseconds;
        const unsigned minimumFramesExpected = streamTimeMs * kMinimumFramesPerSecond /
                                               kSecondsToMilliseconds;
        sleep(kSecondsToWait);
        unsigned framesReceived = 0;
        unsigned framesDisplayed = 0;
        frameHandler->getFramesCounters(&framesReceived, &framesDisplayed);
        EXPECT_EQ(framesReceived, framesDisplayed);
        EXPECT_GE(framesDisplayed, minimumFramesExpected);

        // Turn off the display (yes, before the stream stops -- it should be handled)
        pDisplay->setDisplayState(DisplayState::NOT_VISIBLE);

        // Shut down the streamer
        frameHandler->shutdown();

        // Explicitly release the camera
        pEnumerator->closeCamera(pCam);
        activeCameras.clear();
    }

    // Explicitly release the display
    pEnumerator->closeDisplay(pDisplay);
}


/*
 * MultiCameraStreamUseConfig:
 * Verify that each client can start and stop video streams on the same
 * underlying camera with same configuration.
 */
TEST_P(EvsHidlTest, MultiCameraStreamUseConfig) {
    LOG(INFO) << "Starting MultiCameraStream test";

    if (mIsHwModule) {
        // This test is not for HW module implementation.
        return;
    }

    // Get the camera list
    loadCameraList();

    // Test each reported camera
    for (auto&& cam: cameraInfo) {
        // choose a configuration that has a frame rate faster than minReqFps.
        Stream targetCfg = {};
        const int32_t minReqFps = 15;
        int32_t maxArea = 0;
        camera_metadata_entry_t streamCfgs;
        bool foundCfg = false;
        if (!find_camera_metadata_entry(
                 reinterpret_cast<camera_metadata_t *>(cam.metadata.data()),
                 ANDROID_SCALER_AVAILABLE_STREAM_CONFIGURATIONS,
                 &streamCfgs)) {
            // Stream configurations are found in metadata
            RawStreamConfig *ptr = reinterpret_cast<RawStreamConfig *>(streamCfgs.data.i32);
            for (unsigned idx = 0; idx < streamCfgs.count; idx += kStreamCfgSz) {
                if (ptr->direction == ANDROID_SCALER_AVAILABLE_STREAM_CONFIGURATIONS_OUTPUT &&
                    ptr->format == HAL_PIXEL_FORMAT_RGBA_8888) {

                    if (ptr->width * ptr->height > maxArea &&
                        ptr->framerate >= minReqFps) {
                        targetCfg.width = ptr->width;
                        targetCfg.height = ptr->height;

                        maxArea = ptr->width * ptr->height;
                        foundCfg = true;
                    }
                }
                ++ptr;
            }
        }
        targetCfg.format =
            static_cast<PixelFormat>(HAL_PIXEL_FORMAT_RGBA_8888);

        if (!foundCfg) {
            LOG(INFO) << "Device " << cam.v1.cameraId
                      << " does not provide a list of supported stream configurations, skipped";
            continue;
        }

        // Create the first camera client with a selected stream configuration.
        sp<IEvsCamera_1_1> pCam0 =
            IEvsCamera_1_1::castFrom(pEnumerator->openCamera_1_1(cam.v1.cameraId, targetCfg))
            .withDefault(nullptr);
        ASSERT_NE(pCam0, nullptr);

        // Store a camera handle for a clean-up
        activeCameras.push_back(pCam0);

        // Try to create the second camera client with different stream
        // configuration.
        int32_t id = targetCfg.id;
        targetCfg.id += 1;  // EVS manager sees only the stream id.
        sp<IEvsCamera_1_1> pCam1 =
            IEvsCamera_1_1::castFrom(pEnumerator->openCamera_1_1(cam.v1.cameraId, targetCfg))
            .withDefault(nullptr);
        ASSERT_EQ(pCam1, nullptr);

        // Store a camera handle for a clean-up
        activeCameras.push_back(pCam0);

        // Try again with same stream configuration.
        targetCfg.id = id;
        pCam1 =
            IEvsCamera_1_1::castFrom(pEnumerator->openCamera_1_1(cam.v1.cameraId, targetCfg))
            .withDefault(nullptr);
        ASSERT_NE(pCam1, nullptr);

        // Set up per-client frame receiver objects which will fire up its own thread
        sp<FrameHandler> frameHandler0 = new FrameHandler(pCam0, cam,
                                                          nullptr,
                                                          FrameHandler::eAutoReturn);
        ASSERT_NE(frameHandler0, nullptr);

        sp<FrameHandler> frameHandler1 = new FrameHandler(pCam1, cam,
                                                          nullptr,
                                                          FrameHandler::eAutoReturn);
        ASSERT_NE(frameHandler1, nullptr);

        // Start the camera's video stream via client 0
        bool startResult = false;
        startResult = frameHandler0->startStream() &&
                      frameHandler1->startStream();
        ASSERT_TRUE(startResult);

        // Ensure the stream starts
        frameHandler0->waitForFrameCount(1);
        frameHandler1->waitForFrameCount(1);

        nsecs_t firstFrame = systemTime(SYSTEM_TIME_MONOTONIC);

        // Wait a bit, then ensure both clients get at least the required minimum number of frames
        sleep(5);
        nsecs_t end = systemTime(SYSTEM_TIME_MONOTONIC);
        unsigned framesReceived0 = 0, framesReceived1 = 0;
        frameHandler0->getFramesCounters(&framesReceived0, nullptr);
        frameHandler1->getFramesCounters(&framesReceived1, nullptr);
        framesReceived0 = framesReceived0 - 1;    // Back out the first frame we already waited for
        framesReceived1 = framesReceived1 - 1;    // Back out the first frame we already waited for
        nsecs_t runTime = end - firstFrame;
        float framesPerSecond0 = framesReceived0 / (runTime * kNanoToSeconds);
        float framesPerSecond1 = framesReceived1 / (runTime * kNanoToSeconds);
        LOG(INFO) << "Measured camera rate "
                  << std::scientific << framesPerSecond0 << " fps and "
                  << framesPerSecond1 << " fps";
        EXPECT_GE(framesPerSecond0, kMinimumFramesPerSecond);
        EXPECT_GE(framesPerSecond1, kMinimumFramesPerSecond);

        // Shutdown one client
        frameHandler0->shutdown();

        // Read frame counters again
        frameHandler0->getFramesCounters(&framesReceived0, nullptr);
        frameHandler1->getFramesCounters(&framesReceived1, nullptr);

        // Wait a bit again
        sleep(5);
        unsigned framesReceivedAfterStop0 = 0, framesReceivedAfterStop1 = 0;
        frameHandler0->getFramesCounters(&framesReceivedAfterStop0, nullptr);
        frameHandler1->getFramesCounters(&framesReceivedAfterStop1, nullptr);
        EXPECT_EQ(framesReceived0, framesReceivedAfterStop0);
        EXPECT_LT(framesReceived1, framesReceivedAfterStop1);

        // Shutdown another
        frameHandler1->shutdown();

        // Explicitly release the camera
        pEnumerator->closeCamera(pCam0);
        pEnumerator->closeCamera(pCam1);
        activeCameras.clear();
    }
}


/*
 * LogicalCameraMetadata:
 * Opens logical camera reported by the enumerator and validate its metadata by
 * checking its capability and locating supporting physical camera device
 * identifiers.
 */
TEST_P(EvsHidlTest, LogicalCameraMetadata) {
    LOG(INFO) << "Starting LogicalCameraMetadata test";

    // Get the camera list
    loadCameraList();

    // Open and close each camera twice
    for (auto&& cam: cameraInfo) {
        bool isLogicalCam = false;
        auto devices = getPhysicalCameraIds(cam.v1.cameraId, isLogicalCam);
        if (isLogicalCam) {
            ASSERT_GE(devices.size(), 1) <<
                "Logical camera device must have at least one physical camera device ID in its metadata.";
        }
    }
}


/*
 * UltrasonicsArrayOpenClean:
 * Opens each ultrasonics arrays reported by the enumerator and then explicitly closes it via a
 * call to closeUltrasonicsArray. Then repeats the test to ensure all ultrasonics arrays
 * can be reopened.
 */
TEST_P(EvsHidlTest, UltrasonicsArrayOpenClean) {
    LOG(INFO) << "Starting UltrasonicsArrayOpenClean test";

    // Get the ultrasonics array list
    loadUltrasonicsArrayList();

    // Open and close each ultrasonics array twice
    for (auto&& ultraInfo : ultrasonicsArraysInfo) {
        for (int pass = 0; pass < 2; pass++) {
            sp<IEvsUltrasonicsArray> pUltrasonicsArray =
                    pEnumerator->openUltrasonicsArray(ultraInfo.ultrasonicsArrayId);
            ASSERT_NE(pUltrasonicsArray, nullptr);

            // Verify that this ultrasonics array self-identifies correctly
            pUltrasonicsArray->getUltrasonicArrayInfo([&ultraInfo](UltrasonicsArrayDesc desc) {
                LOG(DEBUG) << "Found ultrasonics array " << ultraInfo.ultrasonicsArrayId;
                EXPECT_EQ(ultraInfo.ultrasonicsArrayId, desc.ultrasonicsArrayId);
            });

            // Explicitly close the ultrasonics array so resources are released right away
            pEnumerator->closeUltrasonicsArray(pUltrasonicsArray);
        }
    }
}


// Starts a stream and verifies all data received is valid.
TEST_P(EvsHidlTest, UltrasonicsVerifyStreamData) {
    LOG(INFO) << "Starting UltrasonicsVerifyStreamData";

    // Get the ultrasonics array list
    loadUltrasonicsArrayList();

    // For each ultrasonics array.
    for (auto&& ultraInfo : ultrasonicsArraysInfo) {
        LOG(DEBUG) << "Testing ultrasonics array: " << ultraInfo.ultrasonicsArrayId;

        sp<IEvsUltrasonicsArray> pUltrasonicsArray =
                pEnumerator->openUltrasonicsArray(ultraInfo.ultrasonicsArrayId);
        ASSERT_NE(pUltrasonicsArray, nullptr);

        sp<FrameHandlerUltrasonics> frameHandler = new FrameHandlerUltrasonics(pUltrasonicsArray);

        // Start stream.
        EvsResult result = pUltrasonicsArray->startStream(frameHandler);
        ASSERT_EQ(result, EvsResult::OK);

        // Wait 5 seconds to receive frames.
        sleep(5);

        // Stop stream.
        pUltrasonicsArray->stopStream();

        EXPECT_GT(frameHandler->getReceiveFramesCount(), 0);
        EXPECT_TRUE(frameHandler->areAllFramesValid());

        // Explicitly close the ultrasonics array so resources are released right away
        pEnumerator->closeUltrasonicsArray(pUltrasonicsArray);
    }
}


// Sets frames in flight before and after start of stream and verfies success.
TEST_P(EvsHidlTest, UltrasonicsSetFramesInFlight) {
    LOG(INFO) << "Starting UltrasonicsSetFramesInFlight";

    // Get the ultrasonics array list
    loadUltrasonicsArrayList();

    // For each ultrasonics array.
    for (auto&& ultraInfo : ultrasonicsArraysInfo) {
        LOG(DEBUG) << "Testing ultrasonics array: " << ultraInfo.ultrasonicsArrayId;

        sp<IEvsUltrasonicsArray> pUltrasonicsArray =
                pEnumerator->openUltrasonicsArray(ultraInfo.ultrasonicsArrayId);
        ASSERT_NE(pUltrasonicsArray, nullptr);

        EvsResult result = pUltrasonicsArray->setMaxFramesInFlight(10);
        EXPECT_EQ(result, EvsResult::OK);

        sp<FrameHandlerUltrasonics> frameHandler = new FrameHandlerUltrasonics(pUltrasonicsArray);

        // Start stream.
        result = pUltrasonicsArray->startStream(frameHandler);
        ASSERT_EQ(result, EvsResult::OK);

        result = pUltrasonicsArray->setMaxFramesInFlight(5);
        EXPECT_EQ(result, EvsResult::OK);

        // Stop stream.
        pUltrasonicsArray->stopStream();

        // Explicitly close the ultrasonics array so resources are released right away
        pEnumerator->closeUltrasonicsArray(pUltrasonicsArray);
    }
}


INSTANTIATE_TEST_SUITE_P(
    PerInstance,
    EvsHidlTest,
    testing::ValuesIn(android::hardware::getAllHalInstanceNames(IEvsEnumerator::descriptor)),
    android::hardware::PrintInstanceNameToString);

