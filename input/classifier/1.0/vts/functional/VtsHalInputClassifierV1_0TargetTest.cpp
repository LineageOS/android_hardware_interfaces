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

#define LOG_TAG "input_classifier_hal_test"

#include <android-base/logging.h>
#include <android/hardware/input/classifier/1.0/IInputClassifier.h>
#include <android/hardware/input/common/1.0/types.h>
#include <gtest/gtest.h>
#include <hidl/GtestPrinter.h>
#include <hidl/ServiceManagement.h>
#include <input/InputDevice.h>
#include <unistd.h>

using ::android::ReservedInputDeviceId;
using ::android::sp;
using ::android::hardware::Return;
using ::android::hardware::input::classifier::V1_0::IInputClassifier;
using ::android::hardware::input::common::V1_0::Action;
using ::android::hardware::input::common::V1_0::Axis;
using ::android::hardware::input::common::V1_0::Button;
using ::android::hardware::input::common::V1_0::EdgeFlag;
using ::android::hardware::input::common::V1_0::MotionEvent;
using ::android::hardware::input::common::V1_0::PointerCoords;
using ::android::hardware::input::common::V1_0::PointerProperties;
using ::android::hardware::input::common::V1_0::Source;
using ::android::hardware::input::common::V1_0::ToolType;
using ::android::hardware::input::common::V1_0::VideoFrame;

static MotionEvent getSimpleMotionEvent() {
    MotionEvent event;
    event.action = Action::DOWN;
    event.actionButton = Button::NONE;
    event.actionIndex = 0;
    event.buttonState = 0;
    event.deviceId = 0;
    event.deviceTimestamp = 0;
    event.displayId = 1;
    event.downTime = 2;
    event.edgeFlags = 0;
    event.eventTime = 3;
    event.flags = 0;
    event.frames = {};
    event.metaState = 0;
    event.policyFlags = 0;
    event.source = Source::TOUCHSCREEN;
    event.xPrecision = 0;
    event.yPrecision = 0;

    PointerCoords coords;
    coords.bits = Axis::X | Axis::Y;
    coords.values = {1 /*X*/, 2 /*Y*/};
    event.pointerCoords = {coords};

    PointerProperties properties;
    properties.id = 0;
    properties.toolType = ToolType::FINGER;
    event.pointerProperties = {properties};

    return event;
}

// The main test class for INPUT CLASSIFIER HIDL HAL 1.0.
class InputClassifierHidlTest_1_0 : public ::testing::TestWithParam<std::string> {
  public:
    virtual void SetUp() override {
        classifier = IInputClassifier::getService(GetParam());
        ASSERT_NE(classifier, nullptr);
    }

    virtual void TearDown() override {}

    sp<IInputClassifier> classifier;
};

/**
 * Call resetDevice(..) for a few common device id values, and make sure that the HAL
 * can handle the resets gracefully.
 */
TEST_P(InputClassifierHidlTest_1_0, ResetDevice) {
    EXPECT_TRUE(classifier->resetDevice(ReservedInputDeviceId::VIRTUAL_KEYBOARD_ID).isOk());
    EXPECT_TRUE(classifier->resetDevice(ReservedInputDeviceId::BUILT_IN_KEYBOARD_ID).isOk());
    EXPECT_TRUE(classifier->resetDevice(1).isOk());
    EXPECT_TRUE(classifier->resetDevice(2).isOk());
}

/**
 * Call reset() on the HAL to ensure no fatal failure there.
 */
TEST_P(InputClassifierHidlTest_1_0, ResetHal) {
    EXPECT_TRUE(classifier->reset().isOk());
}

/**
 * Classify an event without any video frames.
 */
TEST_P(InputClassifierHidlTest_1_0, Classify_NoVideoFrame) {
    // Create a MotionEvent that does not have any video data
    MotionEvent event = getSimpleMotionEvent();

    EXPECT_TRUE(classifier->classify(event).isOk());
    // We are not checking the actual classification here,
    // because the HAL operation is highly device-specific.

    // Return HAL to a consistent state by doing a reset
    classifier->reset();
}

/**
 * Classify an event with one video frame. Should be the most common scenario.
 */
TEST_P(InputClassifierHidlTest_1_0, Classify_OneVideoFrame) {
    MotionEvent event = getSimpleMotionEvent();
    VideoFrame frame;
    frame.data = {1, 2, 3, 4};
    frame.height = 2;
    frame.width = 2;
    frame.timestamp = event.eventTime;
    event.frames = {frame};

    EXPECT_TRUE(classifier->classify(event).isOk());
    // We are not checking the actual classification here,
    // because the HAL operation is highly device-specific.

    // Return HAL to a consistent state by doing a reset
    classifier->reset();
}

/**
 * Classify an event with 2 video frames. This could happen if there's slowness in the system,
 * or if simply the video rate is somehow higher that the input event rate.
 * The HAL should be able to handle events with more than 1 video frame.
 *
 * The frames should be in chronological order, but it is not guaranteed that they will have
 * monotonically increasing timestamps. Still, we provide consistent timestamps here since that
 * is the most realistic mode of operation.
 */
TEST_P(InputClassifierHidlTest_1_0, Classify_TwoVideoFrames) {
    MotionEvent event = getSimpleMotionEvent();
    VideoFrame frame1;
    frame1.data = {1, 2, 3, 4};
    frame1.height = 2;
    frame1.width = 2;
    frame1.timestamp = event.eventTime;
    VideoFrame frame2 = frame1;
    frame2.data = {5, 5, 5, -1};
    frame2.timestamp += 1;
    event.frames = {frame1, frame2};

    EXPECT_TRUE(classifier->classify(event).isOk());
    // We are not checking the actual classification here,
    // because the HAL operation is highly device-specific.

    // Return HAL to a consistent state by doing a reset
    classifier->reset();
}

INSTANTIATE_TEST_SUITE_P(
        PerInstance, InputClassifierHidlTest_1_0,
        testing::ValuesIn(android::hardware::getAllHalInstanceNames(IInputClassifier::descriptor)),
        android::hardware::PrintInstanceNameToString);
