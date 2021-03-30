/*
 * Copyright (C) 2021 The Android Open Source Project
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

#include "StreamWorker.h"

#include <sched.h>
#include <unistd.h>
#include <atomic>

#include <gtest/gtest.h>
#define LOG_TAG "StreamWorker_Test"
#include <log/log.h>

struct TestStream {
    std::atomic<bool> error = false;
};

class TestWorker : public StreamWorker<TestWorker> {
  public:
    // Use nullptr to test error reporting from the worker thread.
    explicit TestWorker(TestStream* stream) : mStream(stream) {}

    size_t getWorkerCycles() const { return mWorkerCycles; }
    bool hasWorkerCycleCalled() const { return mWorkerCycles != 0; }
    bool hasNoWorkerCycleCalled(useconds_t usec) {
        const size_t cyclesBefore = mWorkerCycles;
        usleep(usec);
        return mWorkerCycles == cyclesBefore;
    }

    bool workerInit() { return mStream; }
    bool workerCycle() {
        do {
            mWorkerCycles++;
        } while (mWorkerCycles == 0);
        return !mStream->error;
    }

  private:
    TestStream* const mStream;
    std::atomic<size_t> mWorkerCycles = 0;
};

// The parameter specifies whether an extra call to 'stop' is made at the end.
class StreamWorkerInvalidTest : public testing::TestWithParam<bool> {
  public:
    StreamWorkerInvalidTest() : StreamWorkerInvalidTest(nullptr) {}
    void TearDown() override {
        if (GetParam()) {
            worker.stop();
        }
    }

  protected:
    StreamWorkerInvalidTest(TestStream* stream) : testing::TestWithParam<bool>(), worker(stream) {}
    TestWorker worker;
};

TEST_P(StreamWorkerInvalidTest, Uninitialized) {
    EXPECT_FALSE(worker.hasWorkerCycleCalled());
    EXPECT_FALSE(worker.hasError());
}

TEST_P(StreamWorkerInvalidTest, UninitializedPauseIgnored) {
    EXPECT_FALSE(worker.hasError());
    worker.pause();
    EXPECT_FALSE(worker.hasError());
}

TEST_P(StreamWorkerInvalidTest, UninitializedResumeIgnored) {
    EXPECT_FALSE(worker.hasError());
    worker.resume();
    EXPECT_FALSE(worker.hasError());
}

TEST_P(StreamWorkerInvalidTest, Start) {
    EXPECT_FALSE(worker.start());
    EXPECT_FALSE(worker.hasWorkerCycleCalled());
    EXPECT_TRUE(worker.hasError());
}

TEST_P(StreamWorkerInvalidTest, PauseIgnored) {
    EXPECT_FALSE(worker.start());
    EXPECT_TRUE(worker.hasError());
    worker.pause();
    EXPECT_TRUE(worker.hasError());
}

TEST_P(StreamWorkerInvalidTest, ResumeIgnored) {
    EXPECT_FALSE(worker.start());
    EXPECT_TRUE(worker.hasError());
    worker.resume();
    EXPECT_TRUE(worker.hasError());
}

INSTANTIATE_TEST_SUITE_P(StreamWorkerInvalid, StreamWorkerInvalidTest, testing::Bool());

class StreamWorkerTest : public StreamWorkerInvalidTest {
  public:
    StreamWorkerTest() : StreamWorkerInvalidTest(&stream) {}

  protected:
    TestStream stream;
};

static constexpr unsigned kWorkerIdleCheckTime = 50 * 1000;

TEST_P(StreamWorkerTest, Uninitialized) {
    EXPECT_FALSE(worker.hasWorkerCycleCalled());
    EXPECT_FALSE(worker.hasError());
}

TEST_P(StreamWorkerTest, Start) {
    ASSERT_TRUE(worker.start());
    worker.waitForAtLeastOneCycle();
    EXPECT_FALSE(worker.hasError());
}

TEST_P(StreamWorkerTest, WorkerError) {
    ASSERT_TRUE(worker.start());
    stream.error = true;
    worker.waitForAtLeastOneCycle();
    EXPECT_TRUE(worker.hasError());
    EXPECT_TRUE(worker.hasNoWorkerCycleCalled(kWorkerIdleCheckTime));
}

TEST_P(StreamWorkerTest, PauseResume) {
    ASSERT_TRUE(worker.start());
    worker.waitForAtLeastOneCycle();
    EXPECT_FALSE(worker.hasError());
    worker.pause();
    EXPECT_TRUE(worker.hasNoWorkerCycleCalled(kWorkerIdleCheckTime));
    EXPECT_FALSE(worker.hasError());
    const size_t workerCyclesBefore = worker.getWorkerCycles();
    worker.resume();
    // 'resume' is synchronous and returns after the worker has looped at least once.
    EXPECT_GT(worker.getWorkerCycles(), workerCyclesBefore);
    EXPECT_FALSE(worker.hasError());
}

TEST_P(StreamWorkerTest, StopPaused) {
    ASSERT_TRUE(worker.start());
    worker.waitForAtLeastOneCycle();
    EXPECT_FALSE(worker.hasError());
    worker.pause();
    worker.stop();
    EXPECT_FALSE(worker.hasError());
}

TEST_P(StreamWorkerTest, PauseAfterErrorIgnored) {
    ASSERT_TRUE(worker.start());
    stream.error = true;
    worker.waitForAtLeastOneCycle();
    EXPECT_TRUE(worker.hasError());
    worker.pause();
    EXPECT_TRUE(worker.hasNoWorkerCycleCalled(kWorkerIdleCheckTime));
    EXPECT_TRUE(worker.hasError());
}

TEST_P(StreamWorkerTest, ResumeAfterErrorIgnored) {
    ASSERT_TRUE(worker.start());
    stream.error = true;
    worker.waitForAtLeastOneCycle();
    EXPECT_TRUE(worker.hasError());
    worker.resume();
    EXPECT_TRUE(worker.hasNoWorkerCycleCalled(kWorkerIdleCheckTime));
    EXPECT_TRUE(worker.hasError());
}

TEST_P(StreamWorkerTest, WorkerErrorOnResume) {
    ASSERT_TRUE(worker.start());
    worker.waitForAtLeastOneCycle();
    EXPECT_FALSE(worker.hasError());
    worker.pause();
    EXPECT_FALSE(worker.hasError());
    stream.error = true;
    EXPECT_FALSE(worker.hasError());
    worker.resume();
    worker.waitForAtLeastOneCycle();
    EXPECT_TRUE(worker.hasError());
    EXPECT_TRUE(worker.hasNoWorkerCycleCalled(kWorkerIdleCheckTime));
}

TEST_P(StreamWorkerTest, WaitForAtLeastOneCycle) {
    ASSERT_TRUE(worker.start());
    const size_t workerCyclesBefore = worker.getWorkerCycles();
    EXPECT_TRUE(worker.waitForAtLeastOneCycle());
    EXPECT_GT(worker.getWorkerCycles(), workerCyclesBefore);
}

TEST_P(StreamWorkerTest, WaitForAtLeastOneCycleError) {
    ASSERT_TRUE(worker.start());
    stream.error = true;
    EXPECT_FALSE(worker.waitForAtLeastOneCycle());
}

INSTANTIATE_TEST_SUITE_P(StreamWorker, StreamWorkerTest, testing::Bool());
