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

#pragma once

#include <aidl/android/hardware/automotive/remoteaccess/ApState.h>
#include <aidl/android/hardware/automotive/remoteaccess/BnRemoteAccess.h>
#include <aidl/android/hardware/automotive/remoteaccess/IRemoteTaskCallback.h>
#include <android-base/thread_annotations.h>
#include <wakeup_client.grpc.pb.h>

#include <string>
#include <thread>

namespace android {
namespace hardware {
namespace automotive {
namespace remoteaccess {

class RemoteAccessService
    : public aidl::android::hardware::automotive::remoteaccess::BnRemoteAccess {
  public:
    explicit RemoteAccessService(WakeupClient::StubInterface* grpcStub);

    ~RemoteAccessService();

    ndk::ScopedAStatus getDeviceId(std::string* deviceId) override;

    ndk::ScopedAStatus getWakeupServiceName(std::string* wakeupServiceName) override;

    ndk::ScopedAStatus setRemoteTaskCallback(
            const std::shared_ptr<
                    aidl::android::hardware::automotive::remoteaccess::IRemoteTaskCallback>&
                    callback) override;

    ndk::ScopedAStatus clearRemoteTaskCallback() override;

    ndk::ScopedAStatus notifyApStateChange(
            const aidl::android::hardware::automotive::remoteaccess::ApState& newState) override;

  private:
    // For testing.
    friend class RemoteAccessServiceUnitTest;

    WakeupClient::StubInterface* mGrpcStub;
    std::thread mThread;
    std::mutex mLock;
    std::condition_variable mCv;
    std::shared_ptr<aidl::android::hardware::automotive::remoteaccess::IRemoteTaskCallback>
            mRemoteTaskCallback GUARDED_BY(mLock);
    std::unique_ptr<grpc::ClientContext> mGetRemoteTasksContext GUARDED_BY(mLock);
    // Associated with mCv to notify the task loop to stop waiting and exit.
    bool mTaskWaitStopped GUARDED_BY(mLock);
    // A mutex to make sure startTaskLoop does not overlap with stopTaskLoop.
    std::mutex mStartStopTaskLoopLock;
    bool mTaskLoopRunning GUARDED_BY(mStartStopTaskLoopLock);
    // Default wait time before retry connecting to remote access client is 10s.
    size_t mRetryWaitInMs = 10'000;

    void runTaskLoop();
    void maybeStartTaskLoop();
    void maybeStopTaskLoop();

    void setRetryWaitInMs(size_t retryWaitInMs) { mRetryWaitInMs = retryWaitInMs; }
};

}  // namespace remoteaccess
}  // namespace automotive
}  // namespace hardware
}  // namespace android
