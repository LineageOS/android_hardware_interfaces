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

#include <gtest/gtest.h>

#include "V2_0/ScopedWakelock.h"

namespace android {
namespace hardware {
namespace sensors {
namespace V2_0 {
namespace implementation {

class RefCounter : public IScopedWakelockRefCounter {
  public:
    size_t incCount = 0;
    size_t decCount = 0;

    bool incrementRefCountAndMaybeAcquireWakelock(size_t /* delta */,
                                                  int64_t* /* timeoutStart */) override {
        incCount++;
        return true;
    }

    void decrementRefCountAndMaybeReleaseWakelock(size_t /* delta */,
                                                  int64_t /* timeoutStart */) override {
        decCount++;
    }
};

class ScopedWakelockTest : public testing::Test {
  public:
    ScopedWakelock createScopedWakelock(bool locked) {
        return ScopedWakelock(&mRefCounter, locked);
    }

    RefCounter mRefCounter;
};

TEST_F(ScopedWakelockTest, UnlockedAfterMoved) {
    ScopedWakelock wakelock = createScopedWakelock(false /* locked */);

    ScopedWakelock movedWakelock(std::move(wakelock));

    EXPECT_FALSE(wakelock.isLocked());
    EXPECT_FALSE(movedWakelock.isLocked());
}

TEST_F(ScopedWakelockTest, LockedAfterMoved) {
    ScopedWakelock wakelock = createScopedWakelock(true /* locked */);

    ScopedWakelock movedWakelock(std::move(wakelock));

    EXPECT_FALSE(wakelock.isLocked());
    EXPECT_TRUE(movedWakelock.isLocked());
}

TEST_F(ScopedWakelockTest, Locked) {
    ScopedWakelock wakelock = createScopedWakelock(true /* locked */);

    EXPECT_TRUE(wakelock.isLocked());
}

TEST_F(ScopedWakelockTest, Unlocked) {
    ScopedWakelock wakelock = createScopedWakelock(false /* locked */);

    EXPECT_FALSE(wakelock.isLocked());
}

TEST_F(ScopedWakelockTest, ScopedLocked) {
    { createScopedWakelock(true /* locked */); }

    EXPECT_EQ(mRefCounter.incCount, 1);
    EXPECT_EQ(mRefCounter.decCount, 1);
}

TEST_F(ScopedWakelockTest, ScopedUnlockIsNoop) {
    { createScopedWakelock(false /* locked */); }

    EXPECT_EQ(mRefCounter.incCount, 0);
    EXPECT_EQ(mRefCounter.decCount, 0);
}

TEST_F(ScopedWakelockTest, ScopedLockedMove) {
    {
        ScopedWakelock wakelock = createScopedWakelock(true /* locked */);
        ScopedWakelock movedWakelock(std::move(wakelock));
    }

    EXPECT_EQ(mRefCounter.incCount, 1);
    EXPECT_EQ(mRefCounter.decCount, 1);
}

}  // namespace implementation
}  // namespace V2_0
}  // namespace sensors
}  // namespace hardware
}  // namespace android