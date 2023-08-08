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

#include <pthread.h>
#include <sched.h>
#include <sys/resource.h>
#include <unistd.h>

#include <atomic>

#include <StreamWorker.h>

#include <gtest/gtest.h>
#define LOG_TAG "StreamWorker_Test"
#include <log/log.h>

using android::hardware::audio::common::StreamLogic;
using android::hardware::audio::common::StreamWorker;

class TestWorkerLogic : public StreamLogic {
  public:
    struct Stream {
        void setErrorStatus() { status = Status::ABORT; }
        void setStopStatus() { status = Status::EXIT; }
        std::atomic<Status> status = Status::CONTINUE;
    };

    // Use nullptr to test error reporting from the worker thread.
    explicit TestWorkerLogic(Stream* stream) : mStream(stream) {}

    size_t getWorkerCycles() const { return mWorkerCycles; }
    int getPriority() const { return mPriority; }
    bool hasWorkerCycleCalled() const { return mWorkerCycles != 0; }
    bool hasNoWorkerCycleCalled(useconds_t usec) {
        const size_t cyclesBefore = mWorkerCycles;
        usleep(usec);
        return mWorkerCycles == cyclesBefore;
    }

  protected:
    // StreamLogic implementation
    std::string init() override { return mStream != nullptr ? "" : "Expected error"; }
    Status cycle() override {
        mPriority = getpriority(PRIO_PROCESS, 0);
        do {
            mWorkerCycles++;
        } while (mWorkerCycles == 0);
        return mStream->status;
    }

  private:
    Stream* const mStream;
    std::atomic<size_t> mWorkerCycles = 0;
    std::atomic<int> mPriority = ANDROID_PRIORITY_DEFAULT;
};
using TestWorker = StreamWorker<TestWorkerLogic>;

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
    StreamWorkerInvalidTest(TestWorker::Stream* stream)
        : testing::TestWithParam<bool>(), worker(stream) {}
    TestWorker worker;
};

TEST_P(StreamWorkerInvalidTest, Uninitialized) {
    EXPECT_FALSE(worker.hasWorkerCycleCalled());
    EXPECT_FALSE(worker.hasError());
    EXPECT_LE(worker.getTid(), 0);
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
#if defined(__ANDROID__)
    EXPECT_GT(worker.getTid(), 0);
#endif
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
    TestWorker::Stream stream;
};

static constexpr unsigned kWorkerIdleCheckTime = 50 * 1000;

TEST_P(StreamWorkerTest, Uninitialized) {
    EXPECT_FALSE(worker.hasWorkerCycleCalled());
    EXPECT_FALSE(worker.hasError());
    EXPECT_LE(worker.getTid(), 0);
}

TEST_P(StreamWorkerTest, Start) {
    ASSERT_TRUE(worker.start());
    EXPECT_TRUE(worker.waitForAtLeastOneCycle());
    EXPECT_FALSE(worker.hasError());
#if defined(__ANDROID__)
    EXPECT_GT(worker.getTid(), 0);
#endif
}

TEST_P(StreamWorkerTest, StartStop) {
    ASSERT_TRUE(worker.start());
    EXPECT_TRUE(worker.waitForAtLeastOneCycle());
    EXPECT_FALSE(worker.hasError());
    worker.stop();
    EXPECT_FALSE(worker.hasError());
}

TEST_P(StreamWorkerTest, WorkerExit) {
    ASSERT_TRUE(worker.start());
    stream.setStopStatus();
    worker.waitForAtLeastOneCycle();
    EXPECT_FALSE(worker.hasError());
    EXPECT_TRUE(worker.hasNoWorkerCycleCalled(kWorkerIdleCheckTime));
}

TEST_P(StreamWorkerTest, WorkerJoin) {
    ASSERT_TRUE(worker.start());
    stream.setStopStatus();
    worker.join();
    EXPECT_FALSE(worker.hasError());
    EXPECT_TRUE(worker.hasNoWorkerCycleCalled(kWorkerIdleCheckTime));
}

TEST_P(StreamWorkerTest, WorkerError) {
    ASSERT_TRUE(worker.start());
    stream.setErrorStatus();
    worker.waitForAtLeastOneCycle();
    EXPECT_TRUE(worker.hasError());
    EXPECT_TRUE(worker.hasNoWorkerCycleCalled(kWorkerIdleCheckTime));
}

TEST_P(StreamWorkerTest, StopAfterError) {
    ASSERT_TRUE(worker.start());
    stream.setErrorStatus();
    worker.waitForAtLeastOneCycle();
    EXPECT_TRUE(worker.hasError());
    EXPECT_TRUE(worker.hasNoWorkerCycleCalled(kWorkerIdleCheckTime));
    worker.stop();
    EXPECT_TRUE(worker.hasError());
}

TEST_P(StreamWorkerTest, PauseResume) {
    ASSERT_TRUE(worker.start());
    EXPECT_TRUE(worker.waitForAtLeastOneCycle());
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
    EXPECT_TRUE(worker.waitForAtLeastOneCycle());
    EXPECT_FALSE(worker.hasError());
    worker.pause();
    worker.stop();
    EXPECT_FALSE(worker.hasError());
}

TEST_P(StreamWorkerTest, PauseAfterErrorIgnored) {
    ASSERT_TRUE(worker.start());
    stream.setErrorStatus();
    worker.waitForAtLeastOneCycle();
    EXPECT_TRUE(worker.hasError());
    worker.pause();
    EXPECT_TRUE(worker.hasNoWorkerCycleCalled(kWorkerIdleCheckTime));
    EXPECT_TRUE(worker.hasError());
}

TEST_P(StreamWorkerTest, ResumeAfterErrorIgnored) {
    ASSERT_TRUE(worker.start());
    stream.setErrorStatus();
    worker.waitForAtLeastOneCycle();
    EXPECT_TRUE(worker.hasError());
    worker.resume();
    EXPECT_TRUE(worker.hasNoWorkerCycleCalled(kWorkerIdleCheckTime));
    EXPECT_TRUE(worker.hasError());
}

TEST_P(StreamWorkerTest, WorkerErrorOnResume) {
    ASSERT_TRUE(worker.start());
    EXPECT_TRUE(worker.waitForAtLeastOneCycle());
    EXPECT_FALSE(worker.hasError());
    worker.pause();
    EXPECT_FALSE(worker.hasError());
    stream.setErrorStatus();
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
    stream.setErrorStatus();
    EXPECT_FALSE(worker.waitForAtLeastOneCycle());
}

TEST_P(StreamWorkerTest, MutexDoesNotBlockWorker) {
    ASSERT_TRUE(worker.start());
    const size_t workerCyclesBefore = worker.getWorkerCycles();
    worker.testLockUnlockMutex(true);
    while (worker.getWorkerCycles() == workerCyclesBefore) {
        usleep(kWorkerIdleCheckTime);
    }
    worker.testLockUnlockMutex(false);
    EXPECT_TRUE(worker.waitForAtLeastOneCycle());
    EXPECT_FALSE(worker.hasError());
}

TEST_P(StreamWorkerTest, ThreadName) {
    const std::string workerName = "TestWorker";
    ASSERT_TRUE(worker.start(workerName)) << worker.getError();
    char nameBuf[128];
    ASSERT_EQ(0, pthread_getname_np(worker.testGetThreadNativeHandle(), nameBuf, sizeof(nameBuf)));
    EXPECT_EQ(workerName, nameBuf);
}

TEST_P(StreamWorkerTest, ThreadPriority) {
    const int priority = ANDROID_PRIORITY_LOWEST;
    ASSERT_TRUE(worker.start("", priority)) << worker.getError();
    EXPECT_TRUE(worker.waitForAtLeastOneCycle());
    EXPECT_EQ(priority, worker.getPriority());
}

TEST_P(StreamWorkerTest, DeferredStartCheckNoError) {
    stream.setStopStatus();
    EXPECT_TRUE(worker.start(android::hardware::audio::common::internal::kTestSingleThread));
    EXPECT_FALSE(worker.hasError());
}

TEST_P(StreamWorkerTest, DeferredStartCheckWithError) {
    stream.setErrorStatus();
    EXPECT_FALSE(worker.start(android::hardware::audio::common::internal::kTestSingleThread));
    EXPECT_TRUE(worker.hasError());
}

INSTANTIATE_TEST_SUITE_P(StreamWorker, StreamWorkerTest, testing::Bool());
