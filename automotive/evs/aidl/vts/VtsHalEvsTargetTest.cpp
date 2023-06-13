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

#include "FrameHandler.h"
#include "FrameHandlerUltrasonics.h"

#include <aidl/Gtest.h>
#include <aidl/Vintf.h>
#include <aidl/android/hardware/automotive/evs/BnEvsEnumeratorStatusCallback.h>
#include <aidl/android/hardware/automotive/evs/BufferDesc.h>
#include <aidl/android/hardware/automotive/evs/CameraDesc.h>
#include <aidl/android/hardware/automotive/evs/CameraParam.h>
#include <aidl/android/hardware/automotive/evs/DeviceStatus.h>
#include <aidl/android/hardware/automotive/evs/DisplayDesc.h>
#include <aidl/android/hardware/automotive/evs/DisplayState.h>
#include <aidl/android/hardware/automotive/evs/EvsEventDesc.h>
#include <aidl/android/hardware/automotive/evs/EvsEventType.h>
#include <aidl/android/hardware/automotive/evs/EvsResult.h>
#include <aidl/android/hardware/automotive/evs/IEvsCamera.h>
#include <aidl/android/hardware/automotive/evs/IEvsDisplay.h>
#include <aidl/android/hardware/automotive/evs/IEvsEnumerator.h>
#include <aidl/android/hardware/automotive/evs/IEvsEnumeratorStatusCallback.h>
#include <aidl/android/hardware/automotive/evs/IEvsUltrasonicsArray.h>
#include <aidl/android/hardware/automotive/evs/ParameterRange.h>
#include <aidl/android/hardware/automotive/evs/Stream.h>
#include <aidl/android/hardware/automotive/evs/UltrasonicsArrayDesc.h>
#include <aidl/android/hardware/common/NativeHandle.h>
#include <aidl/android/hardware/graphics/common/HardwareBufferDescription.h>
#include <aidl/android/hardware/graphics/common/PixelFormat.h>
#include <aidlcommonsupport/NativeHandle.h>
#include <android-base/logging.h>
#include <android/binder_ibinder.h>
#include <android/binder_manager.h>
#include <android/binder_process.h>
#include <android/binder_status.h>
#include <system/camera_metadata.h>
#include <ui/GraphicBuffer.h>
#include <ui/GraphicBufferAllocator.h>
#include <utils/Timers.h>

#include <deque>
#include <thread>
#include <unordered_set>

namespace {

// These values are called out in the EVS design doc (as of Mar 8, 2017)
constexpr int kMaxStreamStartMilliseconds = 500;
constexpr int kMinimumFramesPerSecond = 10;
constexpr int kSecondsToMilliseconds = 1000;
constexpr int kMillisecondsToMicroseconds = 1000;
constexpr float kNanoToMilliseconds = 0.000001f;
constexpr float kNanoToSeconds = 0.000000001f;

/*
 * Please note that this is different from what is defined in
 * libhardware/modules/camera/3_4/metadata/types.h; this has one additional
 * field to store a framerate.
 */
typedef struct {
    int32_t id;
    int32_t width;
    int32_t height;
    int32_t format;
    int32_t direction;
    int32_t framerate;
} RawStreamConfig;
constexpr size_t kStreamCfgSz = sizeof(RawStreamConfig) / sizeof(int32_t);

using ::aidl::android::hardware::automotive::evs::BnEvsEnumeratorStatusCallback;
using ::aidl::android::hardware::automotive::evs::BufferDesc;
using ::aidl::android::hardware::automotive::evs::CameraDesc;
using ::aidl::android::hardware::automotive::evs::CameraParam;
using ::aidl::android::hardware::automotive::evs::DeviceStatus;
using ::aidl::android::hardware::automotive::evs::DisplayDesc;
using ::aidl::android::hardware::automotive::evs::DisplayState;
using ::aidl::android::hardware::automotive::evs::EvsEventDesc;
using ::aidl::android::hardware::automotive::evs::EvsEventType;
using ::aidl::android::hardware::automotive::evs::EvsResult;
using ::aidl::android::hardware::automotive::evs::IEvsCamera;
using ::aidl::android::hardware::automotive::evs::IEvsDisplay;
using ::aidl::android::hardware::automotive::evs::IEvsEnumerator;
using ::aidl::android::hardware::automotive::evs::IEvsEnumeratorStatusCallback;
using ::aidl::android::hardware::automotive::evs::IEvsUltrasonicsArray;
using ::aidl::android::hardware::automotive::evs::ParameterRange;
using ::aidl::android::hardware::automotive::evs::Stream;
using ::aidl::android::hardware::automotive::evs::UltrasonicsArrayDesc;
using ::aidl::android::hardware::graphics::common::BufferUsage;
using ::aidl::android::hardware::graphics::common::HardwareBufferDescription;
using ::aidl::android::hardware::graphics::common::PixelFormat;
using std::chrono_literals::operator""s;

}  // namespace

// The main test class for EVS
class EvsAidlTest : public ::testing::TestWithParam<std::string> {
  public:
    virtual void SetUp() override {
        // Make sure we can connect to the enumerator
        std::string service_name = GetParam();
        AIBinder* binder = AServiceManager_waitForService(service_name.data());
        ASSERT_NE(binder, nullptr);
        mEnumerator = IEvsEnumerator::fromBinder(::ndk::SpAIBinder(binder));
        LOG(INFO) << "Test target service: " << service_name;

        ASSERT_TRUE(mEnumerator->isHardware(&mIsHwModule).isOk());
    }

    virtual void TearDown() override {
        // Attempt to close any active camera
        for (auto&& cam : mActiveCameras) {
            if (cam != nullptr) {
                mEnumerator->closeCamera(cam);
            }
        }
        mActiveCameras.clear();
    }

  protected:
    void loadCameraList() {
        // SetUp() must run first!
        ASSERT_NE(mEnumerator, nullptr);

        // Get the camera list
        ASSERT_TRUE(mEnumerator->getCameraList(&mCameraInfo).isOk())
                << "Failed to get a list of available cameras";
        LOG(INFO) << "We have " << mCameraInfo.size() << " cameras.";
    }

    void loadUltrasonicsArrayList() {
        // SetUp() must run first!
        ASSERT_NE(mEnumerator, nullptr);

        // Get the ultrasonics array list
        auto result = mEnumerator->getUltrasonicsArrayList(&mUltrasonicsArraysInfo);
        ASSERT_TRUE(result.isOk() ||
                // TODO(b/149874793): Remove below conditions when
                // getUltrasonicsArrayList() is implemented.
                (!result.isOk() && result.getServiceSpecificError() ==
                        static_cast<int32_t>(EvsResult::NOT_IMPLEMENTED)))
                << "Failed to get a list of available ultrasonics arrays";
        LOG(INFO) << "We have " << mCameraInfo.size() << " ultrasonics arrays.";
    }

    bool isLogicalCamera(const camera_metadata_t* metadata) {
        if (metadata == nullptr) {
            // A logical camera device must have a valid camera metadata.
            return false;
        }

        // Looking for LOGICAL_MULTI_CAMERA capability from metadata.
        camera_metadata_ro_entry_t entry;
        int rc = find_camera_metadata_ro_entry(metadata, ANDROID_REQUEST_AVAILABLE_CAPABILITIES,
                                               &entry);
        if (rc != 0) {
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

    std::unordered_set<std::string> getPhysicalCameraIds(const std::string& id, bool& flag) {
        std::unordered_set<std::string> physicalCameras;
        const auto it = std::find_if(mCameraInfo.begin(), mCameraInfo.end(),
                                     [&id](const CameraDesc& desc) { return id == desc.id; });
        if (it == mCameraInfo.end()) {
            // Unknown camera is requested.  Return an empty list.
            return physicalCameras;
        }

        const camera_metadata_t* metadata = reinterpret_cast<camera_metadata_t*>(&it->metadata[0]);
        flag = isLogicalCamera(metadata);
        if (!flag) {
            // EVS assumes that the device w/o a valid metadata is a physical
            // device.
            LOG(INFO) << id << " is not a logical camera device.";
            physicalCameras.insert(id);
            return physicalCameras;
        }

        // Look for physical camera identifiers
        camera_metadata_ro_entry entry;
        int rc = find_camera_metadata_ro_entry(metadata, ANDROID_LOGICAL_MULTI_CAMERA_PHYSICAL_IDS,
                                               &entry);
        if (rc != 0) {
            LOG(ERROR) << "No physical camera ID is found for a logical camera device";
        }

        const uint8_t* ids = entry.data.u8;
        size_t start = 0;
        for (size_t i = 0; i < entry.count; ++i) {
            if (ids[i] == '\0') {
                if (start != i) {
                    std::string id(reinterpret_cast<const char*>(ids + start));
                    physicalCameras.insert(id);
                }
                start = i + 1;
            }
        }

        LOG(INFO) << id << " consists of " << physicalCameras.size() << " physical camera devices";
        return physicalCameras;
    }

    Stream getFirstStreamConfiguration(camera_metadata_t* metadata) {
        Stream targetCfg = {};
        camera_metadata_entry_t streamCfgs;
        if (!find_camera_metadata_entry(metadata, ANDROID_SCALER_AVAILABLE_STREAM_CONFIGURATIONS,
                                        &streamCfgs)) {
            // Stream configurations are found in metadata
            RawStreamConfig* ptr = reinterpret_cast<RawStreamConfig*>(streamCfgs.data.i32);
            for (unsigned offset = 0; offset < streamCfgs.count; offset += kStreamCfgSz) {
                if (ptr->direction == ANDROID_SCALER_AVAILABLE_STREAM_CONFIGURATIONS_OUTPUT) {
                    targetCfg.width = ptr->width;
                    targetCfg.height = ptr->height;
                    targetCfg.format = static_cast<PixelFormat>(ptr->format);
                    break;
                }
                ++ptr;
            }
        }

        return targetCfg;
    }

    class DeviceStatusCallback : public BnEvsEnumeratorStatusCallback {
        ndk::ScopedAStatus deviceStatusChanged(const std::vector<DeviceStatus>&) override {
            // This empty implementation returns always ok().
            return ndk::ScopedAStatus::ok();
        }
    };

    // Every test needs access to the service
    std::shared_ptr<IEvsEnumerator> mEnumerator;
    // Empty unless/util loadCameraList() is called
    std::vector<CameraDesc> mCameraInfo;
    // boolean to tell current module under testing is HW module implementation
    // or not
    bool mIsHwModule;
    // A list of active camera handles that are need to be cleaned up
    std::deque<std::shared_ptr<IEvsCamera>> mActiveCameras;
    // Empty unless/util loadUltrasonicsArrayList() is called
    std::vector<UltrasonicsArrayDesc> mUltrasonicsArraysInfo;
    // A list of active ultrasonics array handles that are to be cleaned up
    std::deque<std::weak_ptr<IEvsUltrasonicsArray>> mActiveUltrasonicsArrays;
};

// Test cases, their implementations, and corresponding requirements are
// documented at go/aae-evs-public-api-test.

/*
 * CameraOpenClean:
 * Opens each camera reported by the enumerator and then explicitly closes it via a
 * call to closeCamera.  Then repeats the test to ensure all cameras can be reopened.
 */
TEST_P(EvsAidlTest, CameraOpenClean) {
    LOG(INFO) << "Starting CameraOpenClean test";

    // Get the camera list
    loadCameraList();

    // Open and close each camera twice
    for (auto&& cam : mCameraInfo) {
        bool isLogicalCam = false;
        auto devices = getPhysicalCameraIds(cam.id, isLogicalCam);
        if (mIsHwModule && isLogicalCam) {
            LOG(INFO) << "Skip a logical device, " << cam.id << " for HW target.";
            continue;
        }

        // Read a target resolution from the metadata
        Stream targetCfg = getFirstStreamConfiguration(
                reinterpret_cast<camera_metadata_t*>(cam.metadata.data()));
        ASSERT_GT(targetCfg.width, 0);
        ASSERT_GT(targetCfg.height, 0);

        for (int pass = 0; pass < 2; pass++) {
            std::shared_ptr<IEvsCamera> pCam;
            ASSERT_TRUE(mEnumerator->openCamera(cam.id, targetCfg, &pCam).isOk());
            ASSERT_NE(pCam, nullptr);

            CameraDesc cameraInfo;
            for (auto&& devName : devices) {
                ASSERT_TRUE(pCam->getPhysicalCameraInfo(devName, &cameraInfo).isOk());
                EXPECT_EQ(devName, cameraInfo.id);
            }

            // Store a camera handle for a clean-up
            mActiveCameras.push_back(pCam);

            // Verify that this camera self-identifies correctly
            ASSERT_TRUE(pCam->getCameraInfo(&cameraInfo).isOk());
            EXPECT_EQ(cam.id, cameraInfo.id);

            // Verify methods for extended info
            const auto id = 0xFFFFFFFF;  // meaningless id
            std::vector<uint8_t> values;
            auto status = pCam->setExtendedInfo(id, values);
            if (isLogicalCam) {
                EXPECT_TRUE(!status.isOk() && status.getServiceSpecificError() ==
                                                      static_cast<int>(EvsResult::NOT_SUPPORTED));
            } else {
                EXPECT_TRUE(status.isOk());
            }

            status = pCam->getExtendedInfo(id, &values);
            if (isLogicalCam) {
                EXPECT_TRUE(!status.isOk() && status.getServiceSpecificError() ==
                                                      static_cast<int>(EvsResult::NOT_SUPPORTED));
            } else {
                EXPECT_TRUE(status.isOk());
            }

            // Explicitly close the camera so resources are released right away
            ASSERT_TRUE(mEnumerator->closeCamera(pCam).isOk());
            mActiveCameras.clear();
        }
    }
}

/*
 * CameraOpenAggressive:
 * Opens each camera reported by the enumerator twice in a row without an intervening closeCamera
 * call.  This ensures that the intended "aggressive open" behavior works.  This is necessary for
 * the system to be tolerant of shutdown/restart race conditions.
 */
TEST_P(EvsAidlTest, CameraOpenAggressive) {
    LOG(INFO) << "Starting CameraOpenAggressive test";

    // Get the camera list
    loadCameraList();

    // Open and close each camera twice
    for (auto&& cam : mCameraInfo) {
        bool isLogicalCam = false;
        getPhysicalCameraIds(cam.id, isLogicalCam);
        if (mIsHwModule && isLogicalCam) {
            LOG(INFO) << "Skip a logical device, " << cam.id << " for HW target.";
            continue;
        }

        // Read a target resolution from the metadata
        Stream targetCfg = getFirstStreamConfiguration(
                reinterpret_cast<camera_metadata_t*>(cam.metadata.data()));
        ASSERT_GT(targetCfg.width, 0);
        ASSERT_GT(targetCfg.height, 0);

        mActiveCameras.clear();
        std::shared_ptr<IEvsCamera> pCam;
        ASSERT_TRUE(mEnumerator->openCamera(cam.id, targetCfg, &pCam).isOk());
        EXPECT_NE(pCam, nullptr);

        // Store a camera handle for a clean-up
        mActiveCameras.push_back(pCam);

        // Verify that this camera self-identifies correctly
        CameraDesc cameraInfo;
        ASSERT_TRUE(pCam->getCameraInfo(&cameraInfo).isOk());
        EXPECT_EQ(cam.id, cameraInfo.id);

        std::shared_ptr<IEvsCamera> pCam2;
        ASSERT_TRUE(mEnumerator->openCamera(cam.id, targetCfg, &pCam2).isOk());
        EXPECT_NE(pCam2, nullptr);
        EXPECT_NE(pCam, pCam2);

        // Store a camera handle for a clean-up
        mActiveCameras.push_back(pCam2);

        auto status = pCam->setMaxFramesInFlight(2);
        if (mIsHwModule) {
            // Verify that the old camera rejects calls via HW module.
            EXPECT_TRUE(!status.isOk() && status.getServiceSpecificError() ==
                                                  static_cast<int>(EvsResult::OWNERSHIP_LOST));
        } else {
            // default implementation supports multiple clients.
            EXPECT_TRUE(status.isOk());
        }

        // Close the superseded camera
        ASSERT_TRUE(mEnumerator->closeCamera(pCam).isOk());
        mActiveCameras.pop_front();

        // Verify that the second camera instance self-identifies correctly
        ASSERT_TRUE(pCam2->getCameraInfo(&cameraInfo).isOk());
        EXPECT_EQ(cam.id, cameraInfo.id);

        // Close the second camera instance
        ASSERT_TRUE(mEnumerator->closeCamera(pCam2).isOk());
        mActiveCameras.pop_front();
    }

    // Sleep here to ensure the destructor cleanup has time to run so we don't break follow on tests
    sleep(1);  // I hate that this is an arbitrary time to wait.  :(  b/36122635
}

/*
 * CameraStreamPerformance:
 * Measure and qualify the stream start up time and streaming frame rate of each reported camera
 */
TEST_P(EvsAidlTest, CameraStreamPerformance) {
    LOG(INFO) << "Starting CameraStreamPerformance test";

    // Get the camera list
    loadCameraList();

    // Test each reported camera
    for (auto&& cam : mCameraInfo) {
        bool isLogicalCam = false;
        auto devices = getPhysicalCameraIds(cam.id, isLogicalCam);
        if (mIsHwModule && isLogicalCam) {
            LOG(INFO) << "Skip a logical device " << cam.id;
            continue;
        }

        // Read a target resolution from the metadata
        Stream targetCfg = getFirstStreamConfiguration(
                reinterpret_cast<camera_metadata_t*>(cam.metadata.data()));
        ASSERT_GT(targetCfg.width, 0);
        ASSERT_GT(targetCfg.height, 0);

        std::shared_ptr<IEvsCamera> pCam;
        ASSERT_TRUE(mEnumerator->openCamera(cam.id, targetCfg, &pCam).isOk());
        EXPECT_NE(pCam, nullptr);

        // Store a camera handle for a clean-up
        mActiveCameras.push_back(pCam);

        // Set up a frame receiver object which will fire up its own thread
        std::shared_ptr<FrameHandler> frameHandler = ndk::SharedRefBase::make<FrameHandler>(
                pCam, cam, nullptr, FrameHandler::eAutoReturn);
        EXPECT_NE(frameHandler, nullptr);

        // Start the camera's video stream
        nsecs_t start = systemTime(SYSTEM_TIME_MONOTONIC);
        ASSERT_TRUE(frameHandler->startStream());

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
        printf("%s: Measured time to first frame %0.2f ms\n", cam.id.data(),
               timeToFirstFrame * kNanoToMilliseconds);
        LOG(INFO) << cam.id << ": Measured time to first frame " << std::scientific
                  << timeToFirstFrame * kNanoToMilliseconds << " ms.";

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
        framesReceived = framesReceived - 1;  // Back out the first frame we already waited for
        nsecs_t runTime = end - firstFrame;
        float framesPerSecond = framesReceived / (runTime * kNanoToSeconds);
        printf("Measured camera rate %3.2f fps\n", framesPerSecond);
        LOG(INFO) << "Measured camera rate " << std::scientific << framesPerSecond << " fps.";
        EXPECT_GE(framesPerSecond, kMinimumFramesPerSecond);

        // Explicitly release the camera
        ASSERT_TRUE(mEnumerator->closeCamera(pCam).isOk());
        mActiveCameras.clear();
    }
}

/*
 * CameraStreamBuffering:
 * Ensure the camera implementation behaves properly when the client holds onto buffers for more
 * than one frame time.  The camera must cleanly skip frames until the client is ready again.
 */
TEST_P(EvsAidlTest, CameraStreamBuffering) {
    LOG(INFO) << "Starting CameraStreamBuffering test";

    // Arbitrary constant (should be > 1 and not too big)
    static const unsigned int kBuffersToHold = 6;

    // Get the camera list
    loadCameraList();

    // Test each reported camera
    for (auto&& cam : mCameraInfo) {
        bool isLogicalCam = false;
        getPhysicalCameraIds(cam.id, isLogicalCam);
        if (mIsHwModule && isLogicalCam) {
            LOG(INFO) << "Skip a logical device " << cam.id << " for HW target.";
            continue;
        }

        // Read a target resolution from the metadata
        Stream targetCfg = getFirstStreamConfiguration(
                reinterpret_cast<camera_metadata_t*>(cam.metadata.data()));
        ASSERT_GT(targetCfg.width, 0);
        ASSERT_GT(targetCfg.height, 0);

        std::shared_ptr<IEvsCamera> pCam;
        ASSERT_TRUE(mEnumerator->openCamera(cam.id, targetCfg, &pCam).isOk());
        EXPECT_NE(pCam, nullptr);

        // Store a camera handle for a clean-up
        mActiveCameras.push_back(pCam);

        // Ask for a very large number of buffers in flight to ensure it errors correctly
        auto badResult = pCam->setMaxFramesInFlight(std::numeric_limits<int32_t>::max());
        EXPECT_TRUE(!badResult.isOk() && badResult.getServiceSpecificError() ==
                                                 static_cast<int>(EvsResult::BUFFER_NOT_AVAILABLE));

        // Now ask for exactly two buffers in flight as we'll test behavior in that case
        ASSERT_TRUE(pCam->setMaxFramesInFlight(kBuffersToHold).isOk());

        // Set up a frame receiver object which will fire up its own thread.
        std::shared_ptr<FrameHandler> frameHandler = ndk::SharedRefBase::make<FrameHandler>(
                pCam, cam, nullptr, FrameHandler::eNoAutoReturn);
        EXPECT_NE(frameHandler, nullptr);

        // Start the camera's video stream
        ASSERT_TRUE(frameHandler->startStream());

        // Check that the video stream stalls once we've gotten exactly the number of buffers
        // we requested since we told the frameHandler not to return them.
        sleep(1);  // 1 second should be enough for at least 5 frames to be delivered worst case
        unsigned framesReceived = 0;
        frameHandler->getFramesCounters(&framesReceived, nullptr);
        ASSERT_EQ(kBuffersToHold, framesReceived) << "Stream didn't stall at expected buffer limit";

        // Give back one buffer
        ASSERT_TRUE(frameHandler->returnHeldBuffer());

        // Once we return a buffer, it shouldn't take more than 1/10 second to get a new one
        // filled since we require 10fps minimum -- but give a 10% allowance just in case.
        usleep(110 * kMillisecondsToMicroseconds);
        frameHandler->getFramesCounters(&framesReceived, nullptr);
        EXPECT_EQ(kBuffersToHold + 1, framesReceived) << "Stream should've resumed";

        // Even when the camera pointer goes out of scope, the FrameHandler object will
        // keep the stream alive unless we tell it to shutdown.
        // Also note that the FrameHandle and the Camera have a mutual circular reference, so
        // we have to break that cycle in order for either of them to get cleaned up.
        frameHandler->shutdown();

        // Explicitly release the camera
        ASSERT_TRUE(mEnumerator->closeCamera(pCam).isOk());
        mActiveCameras.clear();
    }
}

/*
 * CameraToDisplayRoundTrip:
 * End to end test of data flowing from the camera to the display.  Each delivered frame of camera
 * imagery is simply copied to the display buffer and presented on screen.  This is the one test
 * which a human could observe to see the operation of the system on the physical display.
 */
TEST_P(EvsAidlTest, CameraToDisplayRoundTrip) {
    LOG(INFO) << "Starting CameraToDisplayRoundTrip test";

    // Get the camera list
    loadCameraList();

    // Request available display IDs
    uint8_t targetDisplayId = 0;
    std::vector<uint8_t> displayIds;
    ASSERT_TRUE(mEnumerator->getDisplayIdList(&displayIds).isOk());
    EXPECT_GT(displayIds.size(), 0);
    targetDisplayId = displayIds[0];

    // Test each reported camera
    for (auto&& cam : mCameraInfo) {
        // Request exclusive access to the first EVS display
        std::shared_ptr<IEvsDisplay> pDisplay;
        ASSERT_TRUE(mEnumerator->openDisplay(targetDisplayId, &pDisplay).isOk());
        EXPECT_NE(pDisplay, nullptr);
        LOG(INFO) << "Display " << static_cast<int>(targetDisplayId) << " is in use.";

        // Get the display descriptor
        DisplayDesc displayDesc;
        ASSERT_TRUE(pDisplay->getDisplayInfo(&displayDesc).isOk());
        LOG(INFO) << "    Resolution: " << displayDesc.width << "x" << displayDesc.height;
        ASSERT_GT(displayDesc.width, 0);
        ASSERT_GT(displayDesc.height, 0);

        bool isLogicalCam = false;
        getPhysicalCameraIds(cam.id, isLogicalCam);
        if (mIsHwModule && isLogicalCam) {
            LOG(INFO) << "Skip a logical device " << cam.id << " for HW target.";
            continue;
        }

        // Read a target resolution from the metadata
        Stream targetCfg = getFirstStreamConfiguration(
                reinterpret_cast<camera_metadata_t*>(cam.metadata.data()));
        ASSERT_GT(targetCfg.width, 0);
        ASSERT_GT(targetCfg.height, 0);

        std::shared_ptr<IEvsCamera> pCam;
        ASSERT_TRUE(mEnumerator->openCamera(cam.id, targetCfg, &pCam).isOk());
        EXPECT_NE(pCam, nullptr);

        // Store a camera handle for a clean-up
        mActiveCameras.push_back(pCam);

        // Set up a frame receiver object which will fire up its own thread.
        std::shared_ptr<FrameHandler> frameHandler = ndk::SharedRefBase::make<FrameHandler>(
                pCam, cam, pDisplay, FrameHandler::eAutoReturn);
        EXPECT_NE(frameHandler, nullptr);

        // Activate the display
        ASSERT_TRUE(pDisplay->setDisplayState(DisplayState::VISIBLE_ON_NEXT_FRAME).isOk());

        // Start the camera's video stream
        ASSERT_TRUE(frameHandler->startStream());

        // Wait a while to let the data flow
        static const int kSecondsToWait = 5;
        const int streamTimeMs =
                kSecondsToWait * kSecondsToMilliseconds - kMaxStreamStartMilliseconds;
        const unsigned minimumFramesExpected =
                streamTimeMs * kMinimumFramesPerSecond / kSecondsToMilliseconds;
        sleep(kSecondsToWait);
        unsigned framesReceived = 0;
        unsigned framesDisplayed = 0;
        frameHandler->getFramesCounters(&framesReceived, &framesDisplayed);
        EXPECT_EQ(framesReceived, framesDisplayed);
        EXPECT_GE(framesDisplayed, minimumFramesExpected);

        // Turn off the display (yes, before the stream stops -- it should be handled)
        ASSERT_TRUE(pDisplay->setDisplayState(DisplayState::NOT_VISIBLE).isOk());

        // Shut down the streamer
        frameHandler->shutdown();

        // Explicitly release the camera
        ASSERT_TRUE(mEnumerator->closeCamera(pCam).isOk());
        mActiveCameras.clear();

        // Explicitly release the display
        ASSERT_TRUE(mEnumerator->closeDisplay(pDisplay).isOk());
    }
}

/*
 * MultiCameraStream:
 * Verify that each client can start and stop video streams on the same
 * underlying camera.
 */
TEST_P(EvsAidlTest, MultiCameraStream) {
    LOG(INFO) << "Starting MultiCameraStream test";

    if (mIsHwModule) {
        // This test is not for HW module implementation.
        return;
    }

    // Get the camera list
    loadCameraList();

    // Test each reported camera
    for (auto&& cam : mCameraInfo) {
        // Read a target resolution from the metadata
        Stream targetCfg = getFirstStreamConfiguration(
                reinterpret_cast<camera_metadata_t*>(cam.metadata.data()));
        ASSERT_GT(targetCfg.width, 0);
        ASSERT_GT(targetCfg.height, 0);

        // Create two camera clients.
        std::shared_ptr<IEvsCamera> pCam0;
        ASSERT_TRUE(mEnumerator->openCamera(cam.id, targetCfg, &pCam0).isOk());
        EXPECT_NE(pCam0, nullptr);

        // Store a camera handle for a clean-up
        mActiveCameras.push_back(pCam0);

        std::shared_ptr<IEvsCamera> pCam1;
        ASSERT_TRUE(mEnumerator->openCamera(cam.id, targetCfg, &pCam1).isOk());
        EXPECT_NE(pCam1, nullptr);

        // Store a camera handle for a clean-up
        mActiveCameras.push_back(pCam1);

        // Set up per-client frame receiver objects which will fire up its own thread
        std::shared_ptr<FrameHandler> frameHandler0 = ndk::SharedRefBase::make<FrameHandler>(
                pCam0, cam, nullptr, FrameHandler::eAutoReturn);
        std::shared_ptr<FrameHandler> frameHandler1 = ndk::SharedRefBase::make<FrameHandler>(
                pCam1, cam, nullptr, FrameHandler::eAutoReturn);
        EXPECT_NE(frameHandler0, nullptr);
        EXPECT_NE(frameHandler1, nullptr);

        // Start the camera's video stream via client 0
        ASSERT_TRUE(frameHandler0->startStream());
        ASSERT_TRUE(frameHandler1->startStream());

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
        framesReceived0 = framesReceived0 - 1;  // Back out the first frame we already waited for
        framesReceived1 = framesReceived1 - 1;  // Back out the first frame we already waited for
        nsecs_t runTime = end - firstFrame;
        float framesPerSecond0 = framesReceived0 / (runTime * kNanoToSeconds);
        float framesPerSecond1 = framesReceived1 / (runTime * kNanoToSeconds);
        LOG(INFO) << "Measured camera rate " << std::scientific << framesPerSecond0 << " fps and "
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
        ASSERT_TRUE(mEnumerator->closeCamera(pCam0).isOk());
        ASSERT_TRUE(mEnumerator->closeCamera(pCam1).isOk());
        mActiveCameras.clear();

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
TEST_P(EvsAidlTest, CameraParameter) {
    LOG(INFO) << "Starting CameraParameter test";

    // Get the camera list
    loadCameraList();

    // Test each reported camera
    for (auto&& cam : mCameraInfo) {
        bool isLogicalCam = false;
        getPhysicalCameraIds(cam.id, isLogicalCam);
        if (isLogicalCam) {
            // TODO(b/145465724): Support camera parameter programming on
            // logical devices.
            LOG(INFO) << "Skip a logical device " << cam.id;
            continue;
        }

        // Read a target resolution from the metadata
        Stream targetCfg = getFirstStreamConfiguration(
                reinterpret_cast<camera_metadata_t*>(cam.metadata.data()));
        ASSERT_GT(targetCfg.width, 0);
        ASSERT_GT(targetCfg.height, 0);

        // Create a camera client
        std::shared_ptr<IEvsCamera> pCam;
        ASSERT_TRUE(mEnumerator->openCamera(cam.id, targetCfg, &pCam).isOk());
        EXPECT_NE(pCam, nullptr);

        // Store a camera
        mActiveCameras.push_back(pCam);

        // Get the parameter list
        std::vector<CameraParam> cmds;
        ASSERT_TRUE(pCam->getParameterList(&cmds).isOk());
        if (cmds.size() < 1) {
            continue;
        }

        // Set up per-client frame receiver objects which will fire up its own thread
        std::shared_ptr<FrameHandler> frameHandler = ndk::SharedRefBase::make<FrameHandler>(
                pCam, cam, nullptr, FrameHandler::eAutoReturn);
        EXPECT_NE(frameHandler, nullptr);

        // Start the camera's video stream
        ASSERT_TRUE(frameHandler->startStream());

        // Ensure the stream starts
        frameHandler->waitForFrameCount(1);

        // Set current client is the primary client
        ASSERT_TRUE(pCam->setPrimaryClient().isOk());
        for (auto& cmd : cmds) {
            // Get a valid parameter value range
            ParameterRange range;
            ASSERT_TRUE(pCam->getIntParameterRange(cmd, &range).isOk());

            std::vector<int32_t> values;
            if (cmd == CameraParam::ABSOLUTE_FOCUS) {
                // Try to turn off auto-focus
                ASSERT_TRUE(pCam->setIntParameter(CameraParam::AUTO_FOCUS, 0, &values).isOk());
                for (auto&& v : values) {
                    EXPECT_EQ(v, 0);
                }
            }

            // Try to program a parameter with a random value [minVal, maxVal]
            int32_t val0 = range.min + (std::rand() % (range.max - range.min));

            // Rounding down
            val0 = val0 - (val0 % range.step);
            values.clear();
            ASSERT_TRUE(pCam->setIntParameter(cmd, val0, &values).isOk());

            values.clear();
            ASSERT_TRUE(pCam->getIntParameter(cmd, &values).isOk());
            for (auto&& v : values) {
                EXPECT_EQ(val0, v) << "Values are not matched.";
            }
        }
        ASSERT_TRUE(pCam->unsetPrimaryClient().isOk());

        // Shutdown
        frameHandler->shutdown();

        // Explicitly release the camera
        ASSERT_TRUE(mEnumerator->closeCamera(pCam).isOk());
        mActiveCameras.clear();
    }
}

/*
 * CameraPrimaryClientRelease
 * Verify that non-primary client gets notified when the primary client either
 * terminates or releases a role.
 */
TEST_P(EvsAidlTest, CameraPrimaryClientRelease) {
    LOG(INFO) << "Starting CameraPrimaryClientRelease test";

    if (mIsHwModule) {
        // This test is not for HW module implementation.
        return;
    }

    // Get the camera list
    loadCameraList();

    // Test each reported camera
    for (auto&& cam : mCameraInfo) {
        bool isLogicalCam = false;
        getPhysicalCameraIds(cam.id, isLogicalCam);
        if (isLogicalCam) {
            // TODO(b/145465724): Support camera parameter programming on
            // logical devices.
            LOG(INFO) << "Skip a logical device " << cam.id;
            continue;
        }

        // Read a target resolution from the metadata
        Stream targetCfg = getFirstStreamConfiguration(
                reinterpret_cast<camera_metadata_t*>(cam.metadata.data()));
        ASSERT_GT(targetCfg.width, 0);
        ASSERT_GT(targetCfg.height, 0);

        // Create two camera clients.
        std::shared_ptr<IEvsCamera> pPrimaryCam;
        ASSERT_TRUE(mEnumerator->openCamera(cam.id, targetCfg, &pPrimaryCam).isOk());
        EXPECT_NE(pPrimaryCam, nullptr);

        // Store a camera handle for a clean-up
        mActiveCameras.push_back(pPrimaryCam);

        std::shared_ptr<IEvsCamera> pSecondaryCam;
        ASSERT_TRUE(mEnumerator->openCamera(cam.id, targetCfg, &pSecondaryCam).isOk());
        EXPECT_NE(pSecondaryCam, nullptr);

        // Store a camera handle for a clean-up
        mActiveCameras.push_back(pSecondaryCam);

        // Set up per-client frame receiver objects which will fire up its own thread
        std::shared_ptr<FrameHandler> frameHandlerPrimary = ndk::SharedRefBase::make<FrameHandler>(
                pPrimaryCam, cam, nullptr, FrameHandler::eAutoReturn);
        std::shared_ptr<FrameHandler> frameHandlerSecondary =
                ndk::SharedRefBase::make<FrameHandler>(pSecondaryCam, cam, nullptr,
                                                       FrameHandler::eAutoReturn);
        EXPECT_NE(frameHandlerPrimary, nullptr);
        EXPECT_NE(frameHandlerSecondary, nullptr);

        // Set one client as the primary client
        ASSERT_TRUE(pPrimaryCam->setPrimaryClient().isOk());

        // Try to set another client as the primary client.
        ASSERT_FALSE(pSecondaryCam->setPrimaryClient().isOk());

        // Start the camera's video stream via a primary client client.
        ASSERT_TRUE(frameHandlerPrimary->startStream());

        // Ensure the stream starts
        frameHandlerPrimary->waitForFrameCount(1);

        // Start the camera's video stream via another client
        ASSERT_TRUE(frameHandlerSecondary->startStream());

        // Ensure the stream starts
        frameHandlerSecondary->waitForFrameCount(1);

        // Non-primary client expects to receive a primary client role relesed
        // notification.
        EvsEventDesc aTargetEvent = {};
        EvsEventDesc aNotification = {};

        bool listening = false;
        std::mutex eventLock;
        std::condition_variable eventCond;
        std::thread listener =
                std::thread([&aNotification, &frameHandlerSecondary, &listening, &eventCond]() {
                    // Notify that a listening thread is running.
                    listening = true;
                    eventCond.notify_all();

                    EvsEventDesc aTargetEvent;
                    aTargetEvent.aType = EvsEventType::MASTER_RELEASED;
                    if (!frameHandlerSecondary->waitForEvent(aTargetEvent, aNotification, true)) {
                        LOG(WARNING) << "A timer is expired before a target event is fired.";
                    }
                });

        // Wait until a listening thread starts.
        std::unique_lock<std::mutex> lock(eventLock);
        auto timer = std::chrono::system_clock::now();
        while (!listening) {
            timer += 1s;
            eventCond.wait_until(lock, timer);
        }
        lock.unlock();

        // Release a primary client role.
        ASSERT_TRUE(pPrimaryCam->unsetPrimaryClient().isOk());

        // Join a listening thread.
        if (listener.joinable()) {
            listener.join();
        }

        // Verify change notifications.
        ASSERT_EQ(EvsEventType::MASTER_RELEASED, static_cast<EvsEventType>(aNotification.aType));

        // Non-primary becomes a primary client.
        ASSERT_TRUE(pSecondaryCam->setPrimaryClient().isOk());

        // Previous primary client fails to become a primary client.
        ASSERT_FALSE(pPrimaryCam->setPrimaryClient().isOk());

        listening = false;
        listener = std::thread([&aNotification, &frameHandlerPrimary, &listening, &eventCond]() {
            // Notify that a listening thread is running.
            listening = true;
            eventCond.notify_all();

            EvsEventDesc aTargetEvent;
            aTargetEvent.aType = EvsEventType::MASTER_RELEASED;
            if (!frameHandlerPrimary->waitForEvent(aTargetEvent, aNotification, true)) {
                LOG(WARNING) << "A timer is expired before a target event is fired.";
            }
        });

        // Wait until a listening thread starts.
        timer = std::chrono::system_clock::now();
        lock.lock();
        while (!listening) {
            eventCond.wait_until(lock, timer + 1s);
        }
        lock.unlock();

        // Closing current primary client.
        frameHandlerSecondary->shutdown();

        // Join a listening thread.
        if (listener.joinable()) {
            listener.join();
        }

        // Verify change notifications.
        ASSERT_EQ(EvsEventType::MASTER_RELEASED, static_cast<EvsEventType>(aNotification.aType));

        // Closing streams.
        frameHandlerPrimary->shutdown();

        // Explicitly release the camera
        ASSERT_TRUE(mEnumerator->closeCamera(pPrimaryCam).isOk());
        ASSERT_TRUE(mEnumerator->closeCamera(pSecondaryCam).isOk());
        mActiveCameras.clear();
    }
}

/*
 * MultiCameraParameter:
 * Verify that primary and non-primary clients behave as expected when they try to adjust
 * camera parameters.
 */
TEST_P(EvsAidlTest, MultiCameraParameter) {
    LOG(INFO) << "Starting MultiCameraParameter test";

    if (mIsHwModule) {
        // This test is not for HW module implementation.
        return;
    }

    // Get the camera list
    loadCameraList();

    // Test each reported camera
    for (auto&& cam : mCameraInfo) {
        bool isLogicalCam = false;
        getPhysicalCameraIds(cam.id, isLogicalCam);
        if (isLogicalCam) {
            // TODO(b/145465724): Support camera parameter programming on
            // logical devices.
            LOG(INFO) << "Skip a logical device " << cam.id;
            continue;
        }

        // Read a target resolution from the metadata
        Stream targetCfg = getFirstStreamConfiguration(
                reinterpret_cast<camera_metadata_t*>(cam.metadata.data()));
        ASSERT_GT(targetCfg.width, 0);
        ASSERT_GT(targetCfg.height, 0);

        // Create two camera clients.
        std::shared_ptr<IEvsCamera> pPrimaryCam;
        ASSERT_TRUE(mEnumerator->openCamera(cam.id, targetCfg, &pPrimaryCam).isOk());
        EXPECT_NE(pPrimaryCam, nullptr);

        // Store a camera handle for a clean-up
        mActiveCameras.push_back(pPrimaryCam);

        std::shared_ptr<IEvsCamera> pSecondaryCam;
        ASSERT_TRUE(mEnumerator->openCamera(cam.id, targetCfg, &pSecondaryCam).isOk());
        EXPECT_NE(pSecondaryCam, nullptr);

        // Store a camera handle for a clean-up
        mActiveCameras.push_back(pSecondaryCam);

        // Get the parameter list
        std::vector<CameraParam> camPrimaryCmds, camSecondaryCmds;
        ASSERT_TRUE(pPrimaryCam->getParameterList(&camPrimaryCmds).isOk());
        ASSERT_TRUE(pSecondaryCam->getParameterList(&camSecondaryCmds).isOk());
        if (camPrimaryCmds.size() < 1 || camSecondaryCmds.size() < 1) {
            // Skip a camera device if it does not support any parameter.
            continue;
        }

        // Set up per-client frame receiver objects which will fire up its own thread
        std::shared_ptr<FrameHandler> frameHandlerPrimary = ndk::SharedRefBase::make<FrameHandler>(
                pPrimaryCam, cam, nullptr, FrameHandler::eAutoReturn);
        std::shared_ptr<FrameHandler> frameHandlerSecondary =
                ndk::SharedRefBase::make<FrameHandler>(pSecondaryCam, cam, nullptr,
                                                       FrameHandler::eAutoReturn);
        EXPECT_NE(frameHandlerPrimary, nullptr);
        EXPECT_NE(frameHandlerSecondary, nullptr);

        // Set one client as the primary client.
        ASSERT_TRUE(pPrimaryCam->setPrimaryClient().isOk());

        // Try to set another client as the primary client.
        ASSERT_FALSE(pSecondaryCam->setPrimaryClient().isOk());

        // Start the camera's video stream via a primary client client.
        ASSERT_TRUE(frameHandlerPrimary->startStream());

        // Ensure the stream starts
        frameHandlerPrimary->waitForFrameCount(1);

        // Start the camera's video stream via another client
        ASSERT_TRUE(frameHandlerSecondary->startStream());

        // Ensure the stream starts
        frameHandlerSecondary->waitForFrameCount(1);

        int32_t val0 = 0;
        std::vector<int32_t> values;
        EvsEventDesc aNotification0 = {};
        EvsEventDesc aNotification1 = {};
        for (auto& cmd : camPrimaryCmds) {
            // Get a valid parameter value range
            ParameterRange range;
            ASSERT_TRUE(pPrimaryCam->getIntParameterRange(cmd, &range).isOk());
            if (cmd == CameraParam::ABSOLUTE_FOCUS) {
                // Try to turn off auto-focus
                values.clear();
                ASSERT_TRUE(
                        pPrimaryCam->setIntParameter(CameraParam::AUTO_FOCUS, 0, &values).isOk());
                for (auto&& v : values) {
                    EXPECT_EQ(v, 0);
                }
            }

            // Calculate a parameter value to program.
            val0 = range.min + (std::rand() % (range.max - range.min));
            val0 = val0 - (val0 % range.step);

            // Prepare and start event listeners.
            bool listening0 = false;
            bool listening1 = false;
            std::condition_variable eventCond;
            std::thread listener0 = std::thread([cmd, val0, &aNotification0, &frameHandlerPrimary,
                                                 &listening0, &listening1, &eventCond]() {
                listening0 = true;
                if (listening1) {
                    eventCond.notify_all();
                }

                EvsEventDesc aTargetEvent;
                aTargetEvent.aType = EvsEventType::PARAMETER_CHANGED;
                aTargetEvent.payload.push_back(static_cast<int32_t>(cmd));
                aTargetEvent.payload.push_back(val0);
                if (!frameHandlerPrimary->waitForEvent(aTargetEvent, aNotification0)) {
                    LOG(WARNING) << "A timer is expired before a target event is fired.";
                }
            });
            std::thread listener1 = std::thread([cmd, val0, &aNotification1, &frameHandlerSecondary,
                                                 &listening0, &listening1, &eventCond]() {
                listening1 = true;
                if (listening0) {
                    eventCond.notify_all();
                }

                EvsEventDesc aTargetEvent;
                aTargetEvent.aType = EvsEventType::PARAMETER_CHANGED;
                aTargetEvent.payload.push_back(static_cast<int32_t>(cmd));
                aTargetEvent.payload.push_back(val0);
                if (!frameHandlerSecondary->waitForEvent(aTargetEvent, aNotification1)) {
                    LOG(WARNING) << "A timer is expired before a target event is fired.";
                }
            });

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
            ASSERT_TRUE(pPrimaryCam->setIntParameter(cmd, val0, &values).isOk());
            for (auto&& v : values) {
                EXPECT_EQ(val0, v) << "Values are not matched.";
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
            ASSERT_GE(aNotification0.payload.size(), 2);
            ASSERT_GE(aNotification1.payload.size(), 2);
            ASSERT_EQ(cmd, static_cast<CameraParam>(aNotification0.payload[0]));
            ASSERT_EQ(cmd, static_cast<CameraParam>(aNotification1.payload[0]));
            for (auto&& v : values) {
                ASSERT_EQ(v, aNotification0.payload[1]);
                ASSERT_EQ(v, aNotification1.payload[1]);
            }

            // Clients expects to receive a parameter change notification
            // whenever a primary client client adjusts it.
            values.clear();
            ASSERT_TRUE(pPrimaryCam->getIntParameter(cmd, &values).isOk());
            for (auto&& v : values) {
                EXPECT_EQ(val0, v) << "Values are not matched.";
            }
        }

        // Try to adjust a parameter via non-primary client
        values.clear();
        ASSERT_FALSE(pSecondaryCam->setIntParameter(camSecondaryCmds[0], val0, &values).isOk());

        // Non-primary client attempts to be a primary client
        ASSERT_FALSE(pSecondaryCam->setPrimaryClient().isOk());

        // Primary client retires from a primary client role
        bool listening = false;
        std::condition_variable eventCond;
        std::thread listener =
                std::thread([&aNotification0, &frameHandlerSecondary, &listening, &eventCond]() {
                    listening = true;
                    eventCond.notify_all();

                    EvsEventDesc aTargetEvent;
                    aTargetEvent.aType = EvsEventType::MASTER_RELEASED;
                    if (!frameHandlerSecondary->waitForEvent(aTargetEvent, aNotification0, true)) {
                        LOG(WARNING) << "A timer is expired before a target event is fired.";
                    }
                });

        std::mutex eventLock;
        auto timer = std::chrono::system_clock::now();
        std::unique_lock<std::mutex> lock(eventLock);
        while (!listening) {
            eventCond.wait_until(lock, timer + 1s);
        }
        lock.unlock();

        ASSERT_TRUE(pPrimaryCam->unsetPrimaryClient().isOk());

        if (listener.joinable()) {
            listener.join();
        }
        ASSERT_EQ(EvsEventType::MASTER_RELEASED, static_cast<EvsEventType>(aNotification0.aType));

        // Try to adjust a parameter after being retired
        values.clear();
        ASSERT_FALSE(pPrimaryCam->setIntParameter(camPrimaryCmds[0], val0, &values).isOk());

        // Non-primary client becomes a primary client
        ASSERT_TRUE(pSecondaryCam->setPrimaryClient().isOk());

        // Try to adjust a parameter via new primary client
        for (auto& cmd : camSecondaryCmds) {
            // Get a valid parameter value range
            ParameterRange range;
            ASSERT_TRUE(pSecondaryCam->getIntParameterRange(cmd, &range).isOk());

            values.clear();
            if (cmd == CameraParam::ABSOLUTE_FOCUS) {
                // Try to turn off auto-focus
                values.clear();
                ASSERT_TRUE(
                        pSecondaryCam->setIntParameter(CameraParam::AUTO_FOCUS, 0, &values).isOk());
                for (auto&& v : values) {
                    EXPECT_EQ(v, 0);
                }
            }

            // Calculate a parameter value to program.  This is being rounding down.
            val0 = range.min + (std::rand() % (range.max - range.min));
            val0 = val0 - (val0 % range.step);

            // Prepare and start event listeners.
            bool listening0 = false;
            bool listening1 = false;
            std::condition_variable eventCond;
            std::thread listener0 = std::thread([&]() {
                listening0 = true;
                if (listening1) {
                    eventCond.notify_all();
                }

                EvsEventDesc aTargetEvent;
                aTargetEvent.aType = EvsEventType::PARAMETER_CHANGED;
                aTargetEvent.payload.push_back(static_cast<int32_t>(cmd));
                aTargetEvent.payload.push_back(val0);
                if (!frameHandlerPrimary->waitForEvent(aTargetEvent, aNotification0)) {
                    LOG(WARNING) << "A timer is expired before a target event is fired.";
                }
            });
            std::thread listener1 = std::thread([&]() {
                listening1 = true;
                if (listening0) {
                    eventCond.notify_all();
                }

                EvsEventDesc aTargetEvent;
                aTargetEvent.aType = EvsEventType::PARAMETER_CHANGED;
                aTargetEvent.payload.push_back(static_cast<int32_t>(cmd));
                aTargetEvent.payload.push_back(val0);
                if (!frameHandlerSecondary->waitForEvent(aTargetEvent, aNotification1)) {
                    LOG(WARNING) << "A timer is expired before a target event is fired.";
                }
            });

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
            ASSERT_TRUE(pSecondaryCam->setIntParameter(cmd, val0, &values).isOk());

            // Clients expects to receive a parameter change notification
            // whenever a primary client client adjusts it.
            values.clear();
            ASSERT_TRUE(pSecondaryCam->getIntParameter(cmd, &values).isOk());
            for (auto&& v : values) {
                EXPECT_EQ(val0, v) << "Values are not matched.";
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
            ASSERT_GE(aNotification0.payload.size(), 2);
            ASSERT_GE(aNotification1.payload.size(), 2);
            ASSERT_EQ(cmd, static_cast<CameraParam>(aNotification0.payload[0]));
            ASSERT_EQ(cmd, static_cast<CameraParam>(aNotification1.payload[0]));
            for (auto&& v : values) {
                ASSERT_EQ(v, aNotification0.payload[1]);
                ASSERT_EQ(v, aNotification1.payload[1]);
            }
        }

        // New primary client retires from the role
        ASSERT_TRUE(pSecondaryCam->unsetPrimaryClient().isOk());

        // Shutdown
        frameHandlerPrimary->shutdown();
        frameHandlerSecondary->shutdown();

        // Explicitly release the camera
        ASSERT_TRUE(mEnumerator->closeCamera(pPrimaryCam).isOk());
        ASSERT_TRUE(mEnumerator->closeCamera(pSecondaryCam).isOk());
        mActiveCameras.clear();
    }
}

/*
 * HighPriorityCameraClient:
 * EVS client, which owns the display, is priortized and therefore can take over
 * a primary client role from other EVS clients without the display.
 */
TEST_P(EvsAidlTest, HighPriorityCameraClient) {
    LOG(INFO) << "Starting HighPriorityCameraClient test";

    if (mIsHwModule) {
        // This test is not for HW module implementation.
        return;
    }

    // Get the camera list
    loadCameraList();

    // Test each reported camera
    for (auto&& cam : mCameraInfo) {
        // Request available display IDs
        uint8_t targetDisplayId = 0;
        std::vector<uint8_t> displayIds;
        ASSERT_TRUE(mEnumerator->getDisplayIdList(&displayIds).isOk());
        EXPECT_GT(displayIds.size(), 0);
        targetDisplayId = displayIds[0];

        // Request exclusive access to the EVS display
        std::shared_ptr<IEvsDisplay> pDisplay;
        ASSERT_TRUE(mEnumerator->openDisplay(targetDisplayId, &pDisplay).isOk());
        EXPECT_NE(pDisplay, nullptr);

        // Read a target resolution from the metadata
        Stream targetCfg = getFirstStreamConfiguration(
                reinterpret_cast<camera_metadata_t*>(cam.metadata.data()));
        ASSERT_GT(targetCfg.width, 0);
        ASSERT_GT(targetCfg.height, 0);

        // Create two clients
        std::shared_ptr<IEvsCamera> pCam0;
        ASSERT_TRUE(mEnumerator->openCamera(cam.id, targetCfg, &pCam0).isOk());
        EXPECT_NE(pCam0, nullptr);

        // Store a camera handle for a clean-up
        mActiveCameras.push_back(pCam0);

        std::shared_ptr<IEvsCamera> pCam1;
        ASSERT_TRUE(mEnumerator->openCamera(cam.id, targetCfg, &pCam1).isOk());
        EXPECT_NE(pCam1, nullptr);

        // Store a camera handle for a clean-up
        mActiveCameras.push_back(pCam1);

        // Get the parameter list; this test will use the first command in both
        // lists.
        std::vector<CameraParam> cam0Cmds, cam1Cmds;
        ASSERT_TRUE(pCam0->getParameterList(&cam0Cmds).isOk());
        ASSERT_TRUE(pCam1->getParameterList(&cam1Cmds).isOk());
        if (cam0Cmds.size() < 1 || cam1Cmds.size() < 1) {
            // Cannot execute this test.
            return;
        }

        // Set up a frame receiver object which will fire up its own thread.
        std::shared_ptr<FrameHandler> frameHandler0 = ndk::SharedRefBase::make<FrameHandler>(
                pCam0, cam, nullptr, FrameHandler::eAutoReturn);
        std::shared_ptr<FrameHandler> frameHandler1 = ndk::SharedRefBase::make<FrameHandler>(
                pCam1, cam, nullptr, FrameHandler::eAutoReturn);
        EXPECT_NE(frameHandler0, nullptr);
        EXPECT_NE(frameHandler1, nullptr);

        // Activate the display
        ASSERT_TRUE(pDisplay->setDisplayState(DisplayState::VISIBLE_ON_NEXT_FRAME).isOk());

        // Start the camera's video stream
        ASSERT_TRUE(frameHandler0->startStream());
        ASSERT_TRUE(frameHandler1->startStream());

        // Ensure the stream starts
        frameHandler0->waitForFrameCount(1);
        frameHandler1->waitForFrameCount(1);

        // Client 1 becomes a primary client and programs a parameter.

        // Get a valid parameter value range
        ParameterRange range;
        ASSERT_TRUE(pCam1->getIntParameterRange(cam1Cmds[0], &range).isOk());

        // Client1 becomes a primary client
        ASSERT_TRUE(pCam1->setPrimaryClient().isOk());

        std::vector<int32_t> values;
        EvsEventDesc aTargetEvent = {};
        EvsEventDesc aNotification = {};
        bool listening = false;
        std::mutex eventLock;
        std::condition_variable eventCond;
        if (cam1Cmds[0] == CameraParam::ABSOLUTE_FOCUS) {
            std::thread listener =
                    std::thread([&frameHandler0, &aNotification, &listening, &eventCond] {
                        listening = true;
                        eventCond.notify_all();

                        EvsEventDesc aTargetEvent;
                        aTargetEvent.aType = EvsEventType::PARAMETER_CHANGED;
                        aTargetEvent.payload.push_back(
                                static_cast<int32_t>(CameraParam::AUTO_FOCUS));
                        aTargetEvent.payload.push_back(0);
                        if (!frameHandler0->waitForEvent(aTargetEvent, aNotification)) {
                            LOG(WARNING) << "A timer is expired before a target event is fired.";
                        }
                    });

            // Wait until a lister starts.
            std::unique_lock<std::mutex> lock(eventLock);
            auto timer = std::chrono::system_clock::now();
            while (!listening) {
                eventCond.wait_until(lock, timer + 1s);
            }
            lock.unlock();

            // Try to turn off auto-focus
            ASSERT_TRUE(pCam1->setIntParameter(CameraParam::AUTO_FOCUS, 0, &values).isOk());
            for (auto&& v : values) {
                EXPECT_EQ(v, 0);
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
        int32_t val0 = range.min + (std::rand() % (range.max - range.min));
        val0 = val0 - (val0 % range.step);

        std::thread listener = std::thread(
                [&frameHandler1, &aNotification, &listening, &eventCond, &cam1Cmds, val0] {
                    listening = true;
                    eventCond.notify_all();

                    EvsEventDesc aTargetEvent;
                    aTargetEvent.aType = EvsEventType::PARAMETER_CHANGED;
                    aTargetEvent.payload.push_back(static_cast<int32_t>(cam1Cmds[0]));
                    aTargetEvent.payload.push_back(val0);
                    if (!frameHandler1->waitForEvent(aTargetEvent, aNotification)) {
                        LOG(WARNING) << "A timer is expired before a target event is fired.";
                    }
                });

        // Wait until a lister starts.
        listening = false;
        std::unique_lock<std::mutex> lock(eventLock);
        auto timer = std::chrono::system_clock::now();
        while (!listening) {
            eventCond.wait_until(lock, timer + 1s);
        }
        lock.unlock();

        values.clear();
        ASSERT_TRUE(pCam1->setIntParameter(cam1Cmds[0], val0, &values).isOk());
        for (auto&& v : values) {
            EXPECT_EQ(val0, v);
        }

        // Join a listener
        if (listener.joinable()) {
            listener.join();
        }

        // Verify a change notification
        ASSERT_EQ(static_cast<EvsEventType>(aNotification.aType), EvsEventType::PARAMETER_CHANGED);
        ASSERT_GE(aNotification.payload.size(), 2);
        ASSERT_EQ(static_cast<CameraParam>(aNotification.payload[0]), cam1Cmds[0]);
        for (auto&& v : values) {
            ASSERT_EQ(v, aNotification.payload[1]);
        }

        listener = std::thread([&frameHandler1, &aNotification, &listening, &eventCond] {
            listening = true;
            eventCond.notify_all();

            EvsEventDesc aTargetEvent;
            aTargetEvent.aType = EvsEventType::MASTER_RELEASED;
            if (!frameHandler1->waitForEvent(aTargetEvent, aNotification, true)) {
                LOG(WARNING) << "A timer is expired before a target event is fired.";
            }
        });

        // Wait until a lister starts.
        listening = false;
        lock.lock();
        timer = std::chrono::system_clock::now();
        while (!listening) {
            eventCond.wait_until(lock, timer + 1s);
        }
        lock.unlock();

        // Client 0 steals a primary client role
        ASSERT_TRUE(pCam0->forcePrimaryClient(pDisplay).isOk());

        // Join a listener
        if (listener.joinable()) {
            listener.join();
        }

        ASSERT_EQ(static_cast<EvsEventType>(aNotification.aType), EvsEventType::MASTER_RELEASED);

        // Client 0 programs a parameter
        val0 = range.min + (std::rand() % (range.max - range.min));

        // Rounding down
        val0 = val0 - (val0 % range.step);

        if (cam0Cmds[0] == CameraParam::ABSOLUTE_FOCUS) {
            std::thread listener =
                    std::thread([&frameHandler1, &aNotification, &listening, &eventCond] {
                        listening = true;
                        eventCond.notify_all();

                        EvsEventDesc aTargetEvent;
                        aTargetEvent.aType = EvsEventType::PARAMETER_CHANGED;
                        aTargetEvent.payload.push_back(
                                static_cast<int32_t>(CameraParam::AUTO_FOCUS));
                        aTargetEvent.payload.push_back(0);
                        if (!frameHandler1->waitForEvent(aTargetEvent, aNotification)) {
                            LOG(WARNING) << "A timer is expired before a target event is fired.";
                        }
                    });

            // Wait until a lister starts.
            std::unique_lock<std::mutex> lock(eventLock);
            auto timer = std::chrono::system_clock::now();
            while (!listening) {
                eventCond.wait_until(lock, timer + 1s);
            }
            lock.unlock();

            // Try to turn off auto-focus
            values.clear();
            ASSERT_TRUE(pCam0->setIntParameter(CameraParam::AUTO_FOCUS, 0, &values).isOk());
            for (auto&& v : values) {
                EXPECT_EQ(v, 0);
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
                    aTargetEvent.payload.push_back(static_cast<int32_t>(cam0Cmds[0]));
                    aTargetEvent.payload.push_back(val0);
                    if (!frameHandler0->waitForEvent(aTargetEvent, aNotification)) {
                        LOG(WARNING) << "A timer is expired before a target event is fired.";
                    }
                });

        // Wait until a lister starts.
        listening = false;
        timer = std::chrono::system_clock::now();
        lock.lock();
        while (!listening) {
            eventCond.wait_until(lock, timer + 1s);
        }
        lock.unlock();

        values.clear();
        ASSERT_TRUE(pCam0->setIntParameter(cam0Cmds[0], val0, &values).isOk());

        // Join a listener
        if (listener.joinable()) {
            listener.join();
        }
        // Verify a change notification
        ASSERT_EQ(static_cast<EvsEventType>(aNotification.aType), EvsEventType::PARAMETER_CHANGED);
        ASSERT_GE(aNotification.payload.size(), 2);
        ASSERT_EQ(static_cast<CameraParam>(aNotification.payload[0]), cam0Cmds[0]);
        for (auto&& v : values) {
            ASSERT_EQ(v, aNotification.payload[1]);
        }

        // Turn off the display (yes, before the stream stops -- it should be handled)
        ASSERT_TRUE(pDisplay->setDisplayState(DisplayState::NOT_VISIBLE).isOk());

        // Shut down the streamer
        frameHandler0->shutdown();
        frameHandler1->shutdown();

        // Explicitly release the camera
        ASSERT_TRUE(mEnumerator->closeCamera(pCam0).isOk());
        ASSERT_TRUE(mEnumerator->closeCamera(pCam1).isOk());
        mActiveCameras.clear();

        // Explicitly release the display
        ASSERT_TRUE(mEnumerator->closeDisplay(pDisplay).isOk());
    }
}

/*
 * CameraUseStreamConfigToDisplay:
 * End to end test of data flowing from the camera to the display.  Similar to
 * CameraToDisplayRoundTrip test case but this case retrieves available stream
 * configurations from EVS and uses one of them to start a video stream.
 */
TEST_P(EvsAidlTest, CameraUseStreamConfigToDisplay) {
    LOG(INFO) << "Starting CameraUseStreamConfigToDisplay test";

    // Get the camera list
    loadCameraList();

    // Request available display IDs
    uint8_t targetDisplayId = 0;
    std::vector<uint8_t> displayIds;
    ASSERT_TRUE(mEnumerator->getDisplayIdList(&displayIds).isOk());
    EXPECT_GT(displayIds.size(), 0);
    targetDisplayId = displayIds[0];

    // Test each reported camera
    for (auto&& cam : mCameraInfo) {
        // Request exclusive access to the EVS display
        std::shared_ptr<IEvsDisplay> pDisplay;
        ASSERT_TRUE(mEnumerator->openDisplay(targetDisplayId, &pDisplay).isOk());
        EXPECT_NE(pDisplay, nullptr);

        // choose a configuration that has a frame rate faster than minReqFps.
        Stream targetCfg = {};
        const int32_t minReqFps = 15;
        int32_t maxArea = 0;
        camera_metadata_entry_t streamCfgs;
        bool foundCfg = false;
        if (!find_camera_metadata_entry(reinterpret_cast<camera_metadata_t*>(cam.metadata.data()),
                                        ANDROID_SCALER_AVAILABLE_STREAM_CONFIGURATIONS,
                                        &streamCfgs)) {
            // Stream configurations are found in metadata
            RawStreamConfig* ptr = reinterpret_cast<RawStreamConfig*>(streamCfgs.data.i32);
            for (unsigned offset = 0; offset < streamCfgs.count; offset += kStreamCfgSz) {
                if (ptr->direction == ANDROID_SCALER_AVAILABLE_STREAM_CONFIGURATIONS_OUTPUT) {
                    if (ptr->width * ptr->height > maxArea && ptr->framerate >= minReqFps) {
                        targetCfg.width = ptr->width;
                        targetCfg.height = ptr->height;
                        targetCfg.format = static_cast<PixelFormat>(ptr->format);

                        maxArea = ptr->width * ptr->height;
                        foundCfg = true;
                    }
                }
                ++ptr;
            }
        }

        if (!foundCfg) {
            // Current EVS camera does not provide stream configurations in the
            // metadata.
            continue;
        }

        std::shared_ptr<IEvsCamera> pCam;
        ASSERT_TRUE(mEnumerator->openCamera(cam.id, targetCfg, &pCam).isOk());
        EXPECT_NE(pCam, nullptr);

        // Store a camera handle for a clean-up
        mActiveCameras.push_back(pCam);

        // Set up a frame receiver object which will fire up its own thread.
        std::shared_ptr<FrameHandler> frameHandler = ndk::SharedRefBase::make<FrameHandler>(
                pCam, cam, pDisplay, FrameHandler::eAutoReturn);
        EXPECT_NE(frameHandler, nullptr);

        // Activate the display
        ASSERT_TRUE(pDisplay->setDisplayState(DisplayState::VISIBLE_ON_NEXT_FRAME).isOk());

        // Start the camera's video stream
        ASSERT_TRUE(frameHandler->startStream());

        // Wait a while to let the data flow
        static const int kSecondsToWait = 5;
        const int streamTimeMs =
                kSecondsToWait * kSecondsToMilliseconds - kMaxStreamStartMilliseconds;
        const unsigned minimumFramesExpected =
                streamTimeMs * kMinimumFramesPerSecond / kSecondsToMilliseconds;
        sleep(kSecondsToWait);
        unsigned framesReceived = 0;
        unsigned framesDisplayed = 0;
        frameHandler->getFramesCounters(&framesReceived, &framesDisplayed);
        EXPECT_EQ(framesReceived, framesDisplayed);
        EXPECT_GE(framesDisplayed, minimumFramesExpected);

        // Turn off the display (yes, before the stream stops -- it should be handled)
        ASSERT_TRUE(pDisplay->setDisplayState(DisplayState::NOT_VISIBLE).isOk());

        // Shut down the streamer
        frameHandler->shutdown();

        // Explicitly release the camera
        ASSERT_TRUE(mEnumerator->closeCamera(pCam).isOk());
        mActiveCameras.clear();

        // Explicitly release the display
        ASSERT_TRUE(mEnumerator->closeDisplay(pDisplay).isOk());
    }
}

/*
 * MultiCameraStreamUseConfig:
 * Verify that each client can start and stop video streams on the same
 * underlying camera with same configuration.
 */
TEST_P(EvsAidlTest, MultiCameraStreamUseConfig) {
    LOG(INFO) << "Starting MultiCameraStream test";

    if (mIsHwModule) {
        // This test is not for HW module implementation.
        return;
    }

    // Get the camera list
    loadCameraList();

    // Test each reported camera
    for (auto&& cam : mCameraInfo) {
        // choose a configuration that has a frame rate faster than minReqFps.
        Stream targetCfg = {};
        const int32_t minReqFps = 15;
        int32_t maxArea = 0;
        camera_metadata_entry_t streamCfgs;
        bool foundCfg = false;
        if (!find_camera_metadata_entry(reinterpret_cast<camera_metadata_t*>(cam.metadata.data()),
                                        ANDROID_SCALER_AVAILABLE_STREAM_CONFIGURATIONS,
                                        &streamCfgs)) {
            // Stream configurations are found in metadata
            RawStreamConfig* ptr = reinterpret_cast<RawStreamConfig*>(streamCfgs.data.i32);
            for (unsigned offset = 0; offset < streamCfgs.count; offset += kStreamCfgSz) {
                if (ptr->direction == ANDROID_SCALER_AVAILABLE_STREAM_CONFIGURATIONS_OUTPUT) {
                    if (ptr->width * ptr->height > maxArea && ptr->framerate >= minReqFps) {
                        targetCfg.width = ptr->width;
                        targetCfg.height = ptr->height;
                        targetCfg.format = static_cast<PixelFormat>(ptr->format);

                        maxArea = ptr->width * ptr->height;
                        foundCfg = true;
                    }
                }
                ++ptr;
            }
        }

        if (!foundCfg) {
            LOG(INFO) << "Device " << cam.id
                      << " does not provide a list of supported stream configurations, skipped";
            continue;
        }

        // Create the first camera client with a selected stream configuration.
        std::shared_ptr<IEvsCamera> pCam0;
        ASSERT_TRUE(mEnumerator->openCamera(cam.id, targetCfg, &pCam0).isOk());
        EXPECT_NE(pCam0, nullptr);

        // Store a camera handle for a clean-up
        mActiveCameras.push_back(pCam0);

        // Try to create the second camera client with different stream
        // configuration.
        int32_t id = targetCfg.id;
        targetCfg.id += 1;  // EVS manager sees only the stream id.
        std::shared_ptr<IEvsCamera> pCam1;
        ASSERT_FALSE(mEnumerator->openCamera(cam.id, targetCfg, &pCam1).isOk());

        // Try again with same stream configuration.
        targetCfg.id = id;
        ASSERT_TRUE(mEnumerator->openCamera(cam.id, targetCfg, &pCam1).isOk());
        EXPECT_NE(pCam1, nullptr);

        // Set up per-client frame receiver objects which will fire up its own thread
        std::shared_ptr<FrameHandler> frameHandler0 = ndk::SharedRefBase::make<FrameHandler>(
                pCam0, cam, nullptr, FrameHandler::eAutoReturn);
        std::shared_ptr<FrameHandler> frameHandler1 = ndk::SharedRefBase::make<FrameHandler>(
                pCam1, cam, nullptr, FrameHandler::eAutoReturn);
        EXPECT_NE(frameHandler0, nullptr);
        EXPECT_NE(frameHandler1, nullptr);

        // Start the camera's video stream via client 0
        ASSERT_TRUE(frameHandler0->startStream());
        ASSERT_TRUE(frameHandler1->startStream());

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
        framesReceived0 = framesReceived0 - 1;  // Back out the first frame we already waited for
        framesReceived1 = framesReceived1 - 1;  // Back out the first frame we already waited for
        nsecs_t runTime = end - firstFrame;
        float framesPerSecond0 = framesReceived0 / (runTime * kNanoToSeconds);
        float framesPerSecond1 = framesReceived1 / (runTime * kNanoToSeconds);
        LOG(INFO) << "Measured camera rate " << std::scientific << framesPerSecond0 << " fps and "
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
        ASSERT_TRUE(mEnumerator->closeCamera(pCam0).isOk());
        ASSERT_TRUE(mEnumerator->closeCamera(pCam1).isOk());
        mActiveCameras.clear();
    }
}

/*
 * LogicalCameraMetadata:
 * Opens logical camera reported by the enumerator and validate its metadata by
 * checking its capability and locating supporting physical camera device
 * identifiers.
 */
TEST_P(EvsAidlTest, LogicalCameraMetadata) {
    LOG(INFO) << "Starting LogicalCameraMetadata test";

    // Get the camera list
    loadCameraList();

    // Open and close each camera twice
    for (auto&& cam : mCameraInfo) {
        bool isLogicalCam = false;
        auto devices = getPhysicalCameraIds(cam.id, isLogicalCam);
        if (isLogicalCam) {
            ASSERT_GE(devices.size(), 1) << "Logical camera device must have at least one physical "
                                            "camera device ID in its metadata.";
        }
    }
}

/*
 * CameraStreamExternalBuffering:
 * This is same with CameraStreamBuffering except frame buffers are allocated by
 * the test client and then imported by EVS framework.
 */
TEST_P(EvsAidlTest, CameraStreamExternalBuffering) {
    LOG(INFO) << "Starting CameraStreamExternalBuffering test";

    // Arbitrary constant (should be > 1 and not too big)
    static const unsigned int kBuffersToHold = 3;

    // Get the camera list
    loadCameraList();

    // Acquire the graphics buffer allocator
    android::GraphicBufferAllocator& alloc(android::GraphicBufferAllocator::get());
    const auto usage =
            GRALLOC_USAGE_HW_TEXTURE | GRALLOC_USAGE_SW_READ_RARELY | GRALLOC_USAGE_SW_WRITE_OFTEN;

    // Test each reported camera
    for (auto&& cam : mCameraInfo) {
        // Read a target resolution from the metadata
        Stream targetCfg = getFirstStreamConfiguration(
                reinterpret_cast<camera_metadata_t*>(cam.metadata.data()));
        ASSERT_GT(targetCfg.width, 0);
        ASSERT_GT(targetCfg.height, 0);

        // Allocate buffers to use
        std::vector<BufferDesc> buffers;
        buffers.resize(kBuffersToHold);
        for (auto i = 0; i < kBuffersToHold; ++i) {
            unsigned pixelsPerLine;
            buffer_handle_t memHandle = nullptr;
            android::status_t result =
                    alloc.allocate(targetCfg.width, targetCfg.height,
                                   static_cast<android::PixelFormat>(targetCfg.format),
                                   /* layerCount = */ 1, usage, &memHandle, &pixelsPerLine,
                                   /* graphicBufferId = */ 0,
                                   /* requestorName = */ "CameraStreamExternalBufferingTest");
            if (result != android::NO_ERROR) {
                LOG(ERROR) << __FUNCTION__ << " failed to allocate memory.";
                // Release previous allocated buffers
                for (auto j = 0; j < i; j++) {
                    alloc.free(::android::dupFromAidl(buffers[i].buffer.handle));
                }
                return;
            } else {
                BufferDesc buf;
                HardwareBufferDescription* pDesc =
                        reinterpret_cast<HardwareBufferDescription*>(&buf.buffer.description);
                pDesc->width = targetCfg.width;
                pDesc->height = targetCfg.height;
                pDesc->layers = 1;
                pDesc->format = targetCfg.format;
                pDesc->usage = static_cast<BufferUsage>(usage);
                pDesc->stride = pixelsPerLine;
                buf.buffer.handle = ::android::dupToAidl(memHandle);
                buf.bufferId = i;  // Unique number to identify this buffer
                buffers[i] = std::move(buf);
            }
        }

        bool isLogicalCam = false;
        getPhysicalCameraIds(cam.id, isLogicalCam);

        std::shared_ptr<IEvsCamera> pCam;
        ASSERT_TRUE(mEnumerator->openCamera(cam.id, targetCfg, &pCam).isOk());
        EXPECT_NE(pCam, nullptr);

        // Store a camera handle for a clean-up
        mActiveCameras.push_back(pCam);

        // Request to import buffers
        int delta = 0;
        auto status = pCam->importExternalBuffers(buffers, &delta);
        if (isLogicalCam) {
            ASSERT_FALSE(status.isOk());
            continue;
        }

        ASSERT_TRUE(status.isOk());
        EXPECT_GE(delta, kBuffersToHold);

        // Set up a frame receiver object which will fire up its own thread.
        std::shared_ptr<FrameHandler> frameHandler = ndk::SharedRefBase::make<FrameHandler>(
                pCam, cam, nullptr, FrameHandler::eNoAutoReturn);
        EXPECT_NE(frameHandler, nullptr);

        // Start the camera's video stream
        ASSERT_TRUE(frameHandler->startStream());

        // Check that the video stream stalls once we've gotten exactly the number of buffers
        // we requested since we told the frameHandler not to return them.
        sleep(1);  // 1 second should be enough for at least 5 frames to be delivered worst case
        unsigned framesReceived = 0;
        frameHandler->getFramesCounters(&framesReceived, nullptr);
        ASSERT_LE(kBuffersToHold, framesReceived) << "Stream didn't stall at expected buffer limit";

        // Give back one buffer
        EXPECT_TRUE(frameHandler->returnHeldBuffer());

        // Once we return a buffer, it shouldn't take more than 1/10 second to get a new one
        // filled since we require 10fps minimum -- but give a 10% allowance just in case.
        unsigned framesReceivedAfter = 0;
        usleep(110 * kMillisecondsToMicroseconds);
        frameHandler->getFramesCounters(&framesReceivedAfter, nullptr);
        EXPECT_EQ(framesReceived + 1, framesReceivedAfter) << "Stream should've resumed";

        // Even when the camera pointer goes out of scope, the FrameHandler object will
        // keep the stream alive unless we tell it to shutdown.
        // Also note that the FrameHandle and the Camera have a mutual circular reference, so
        // we have to break that cycle in order for either of them to get cleaned up.
        frameHandler->shutdown();

        // Explicitly release the camera
        ASSERT_TRUE(mEnumerator->closeCamera(pCam).isOk());
        mActiveCameras.clear();
        // Release buffers
        for (auto& b : buffers) {
            alloc.free(::android::dupFromAidl(b.buffer.handle));
        }
        buffers.resize(0);
    }
}

TEST_P(EvsAidlTest, DeviceStatusCallbackRegistration) {
    std::shared_ptr<IEvsEnumeratorStatusCallback> cb =
            ndk::SharedRefBase::make<DeviceStatusCallback>();
    ndk::ScopedAStatus status = mEnumerator->registerStatusCallback(cb);
    if (mIsHwModule) {
        ASSERT_TRUE(status.isOk());
    } else {
        // A callback registration may fail if a HIDL EVS HAL implementation is
        // running.
        ASSERT_TRUE(status.isOk() ||
                    status.getServiceSpecificError() == static_cast<int>(EvsResult::NOT_SUPPORTED));
    }
}

/*
 * UltrasonicsArrayOpenClean:
 * Opens each ultrasonics arrays reported by the enumerator and then explicitly closes it via a
 * call to closeUltrasonicsArray. Then repeats the test to ensure all ultrasonics arrays
 * can be reopened.
 */
TEST_P(EvsAidlTest, UltrasonicsArrayOpenClean) {
    LOG(INFO) << "Starting UltrasonicsArrayOpenClean test";

    // Get the ultrasonics array list
    loadUltrasonicsArrayList();

    // Open and close each ultrasonics array twice
    for (auto&& ultraInfo : mUltrasonicsArraysInfo) {
        for (int pass = 0; pass < 2; pass++) {
            std::shared_ptr<IEvsUltrasonicsArray> pUltrasonicsArray;
            ASSERT_TRUE(
                    mEnumerator
                            ->openUltrasonicsArray(ultraInfo.ultrasonicsArrayId, &pUltrasonicsArray)
                            .isOk());
            EXPECT_NE(pUltrasonicsArray, nullptr);

            // Verify that this ultrasonics array self-identifies correctly
            UltrasonicsArrayDesc desc;
            ASSERT_TRUE(pUltrasonicsArray->getUltrasonicArrayInfo(&desc).isOk());
            EXPECT_EQ(ultraInfo.ultrasonicsArrayId, desc.ultrasonicsArrayId);
            LOG(DEBUG) << "Found ultrasonics array " << ultraInfo.ultrasonicsArrayId;

            // Explicitly close the ultrasonics array so resources are released right away
            ASSERT_TRUE(mEnumerator->closeUltrasonicsArray(pUltrasonicsArray).isOk());
        }
    }
}

// Starts a stream and verifies all data received is valid.
TEST_P(EvsAidlTest, UltrasonicsVerifyStreamData) {
    LOG(INFO) << "Starting UltrasonicsVerifyStreamData";

    // Get the ultrasonics array list
    loadUltrasonicsArrayList();

    // For each ultrasonics array.
    for (auto&& ultraInfo : mUltrasonicsArraysInfo) {
        LOG(DEBUG) << "Testing ultrasonics array: " << ultraInfo.ultrasonicsArrayId;

        std::shared_ptr<IEvsUltrasonicsArray> pUltrasonicsArray;
        ASSERT_TRUE(
                mEnumerator->openUltrasonicsArray(ultraInfo.ultrasonicsArrayId, &pUltrasonicsArray)
                        .isOk());
        EXPECT_NE(pUltrasonicsArray, nullptr);

        std::shared_ptr<FrameHandlerUltrasonics> frameHandler =
                ndk::SharedRefBase::make<FrameHandlerUltrasonics>(pUltrasonicsArray);
        EXPECT_NE(frameHandler, nullptr);

        // Start stream.
        ASSERT_TRUE(pUltrasonicsArray->startStream(frameHandler).isOk());

        // Wait 5 seconds to receive frames.
        sleep(5);

        // Stop stream.
        ASSERT_TRUE(pUltrasonicsArray->stopStream().isOk());

        EXPECT_GT(frameHandler->getReceiveFramesCount(), 0);
        EXPECT_TRUE(frameHandler->areAllFramesValid());

        // Explicitly close the ultrasonics array so resources are released right away
        ASSERT_TRUE(mEnumerator->closeUltrasonicsArray(pUltrasonicsArray).isOk());
    }
}

// Sets frames in flight before and after start of stream and verfies success.
TEST_P(EvsAidlTest, UltrasonicsSetFramesInFlight) {
    LOG(INFO) << "Starting UltrasonicsSetFramesInFlight";

    // Get the ultrasonics array list
    loadUltrasonicsArrayList();

    // For each ultrasonics array.
    for (auto&& ultraInfo : mUltrasonicsArraysInfo) {
        LOG(DEBUG) << "Testing ultrasonics array: " << ultraInfo.ultrasonicsArrayId;

        std::shared_ptr<IEvsUltrasonicsArray> pUltrasonicsArray;
        ASSERT_TRUE(
                mEnumerator->openUltrasonicsArray(ultraInfo.ultrasonicsArrayId, &pUltrasonicsArray)
                        .isOk());
        EXPECT_NE(pUltrasonicsArray, nullptr);

        ASSERT_TRUE(pUltrasonicsArray->setMaxFramesInFlight(10).isOk());

        std::shared_ptr<FrameHandlerUltrasonics> frameHandler =
                ndk::SharedRefBase::make<FrameHandlerUltrasonics>(pUltrasonicsArray);
        EXPECT_NE(frameHandler, nullptr);

        // Start stream.
        ASSERT_TRUE(pUltrasonicsArray->startStream(frameHandler).isOk());
        ASSERT_TRUE(pUltrasonicsArray->setMaxFramesInFlight(5).isOk());

        // Stop stream.
        ASSERT_TRUE(pUltrasonicsArray->stopStream().isOk());

        // Explicitly close the ultrasonics array so resources are released right away
        ASSERT_TRUE(mEnumerator->closeUltrasonicsArray(pUltrasonicsArray).isOk());
    }
}

GTEST_ALLOW_UNINSTANTIATED_PARAMETERIZED_TEST(EvsAidlTest);
INSTANTIATE_TEST_SUITE_P(
        PerInstance, EvsAidlTest,
        testing::ValuesIn(android::getAidlHalInstanceNames(IEvsEnumerator::descriptor)),
        android::PrintInstanceNameToString);

int main(int argc, char** argv) {
    ::testing::InitGoogleTest(&argc, argv);
    ABinderProcess_setThreadPoolMaxThreadCount(1);
    ABinderProcess_startThreadPool();
    return RUN_ALL_TESTS();
}
