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
#include <math.h>

// pull in all the <= 4.0 tests
#include "4.0/AudioPrimaryHidlHalTest.cpp"

TEST_P(InputStreamTest, SetMicrophoneDirection) {
    doc::test("Make sure setMicrophoneDirection correctly handles valid & invalid arguments");

    // MicrophoneDirection dir = MicrophoneDirection::FRONT;
    for (MicrophoneDirection dir : android::hardware::hidl_enum_range<MicrophoneDirection>()) {
        ASSERT_RESULT(okOrNotSupported, stream->setMicrophoneDirection(dir));
    }

    // Bogus values
    for (auto dir : {42, -1, 4}) {
        ASSERT_RESULT(invalidArgsOrNotSupported,
                      stream->setMicrophoneDirection(MicrophoneDirection(dir)));
    }
}

TEST_P(InputStreamTest, SetMicrophoneFieldDimension) {
    doc::test("Make sure setMicrophoneFieldDimension correctly handles valid & invalid arguments");

    // Valid zoom values -1.0 -> 1.0
    float incr = 0.1f;
    for (float val = -1.0f; val <= 1.0; val += incr) {
        ASSERT_RESULT(okOrNotSupported, stream->setMicrophoneFieldDimension(val));
    }

    // Bogus values
    for (float val = 1.0f + incr; val <= 10.0f; val += incr) {
        ASSERT_RESULT(invalidArgsOrNotSupported, stream->setMicrophoneFieldDimension(val));
        ASSERT_RESULT(invalidArgsOrNotSupported, stream->setMicrophoneFieldDimension(-val));
    }
    // Some extremes
    ASSERT_RESULT(invalidArgsOrNotSupported, stream->setMicrophoneFieldDimension(NAN));
    ASSERT_RESULT(invalidArgsOrNotSupported, stream->setMicrophoneFieldDimension(-NAN));
    ASSERT_RESULT(invalidArgsOrNotSupported, stream->setMicrophoneFieldDimension(INFINITY));
    ASSERT_RESULT(invalidArgsOrNotSupported, stream->setMicrophoneFieldDimension(-INFINITY));
}
