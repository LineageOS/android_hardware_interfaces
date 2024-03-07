/*
 * Copyright (C) 2023 The Android Open Source Project
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

#include "EvsCamera.h"

#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include <cstdint>
#include <unordered_set>
#include <vector>

namespace aidl::android::hardware::automotive::evs::implementation {

class EvsCameraForTest : public EvsCamera {
  private:
    using Base = EvsCamera;

  public:
    using EvsCamera::mStreamState;
    using EvsCamera::shutdown;
    using EvsCamera::StreamState;

    ~EvsCameraForTest() override { shutdown(); }

    ::android::status_t allocateOneFrame(buffer_handle_t* handle) override {
        static std::intptr_t handle_cnt = 0;
        *handle = reinterpret_cast<buffer_handle_t>(++handle_cnt);
        return ::android::OK;
    }

    void freeOneFrame(const buffer_handle_t /* handle */) override {
        // Nothing to free because the handles are fake.
    }

    bool preVideoStreamStart_locked(const std::shared_ptr<evs::IEvsCameraStream>& receiver,
                                    ndk::ScopedAStatus& status,
                                    std::unique_lock<std::mutex>& lck) override {
        mPreStartCalled = true;
        EXPECT_EQ(mStreamState, StreamState::STOPPED);
        EXPECT_FALSE(mStreamStarted);
        EXPECT_FALSE(mStreamStopped);
        return Base::preVideoStreamStart_locked(receiver, status, lck);
    }

    bool startVideoStreamImpl_locked(const std::shared_ptr<evs::IEvsCameraStream>& /* receiver */,
                                     ndk::ScopedAStatus& /* status */,
                                     std::unique_lock<std::mutex>& /* lck */) override {
        EXPECT_EQ(mStreamState, StreamState::RUNNING);
        EXPECT_FALSE(mStreamStarted);
        EXPECT_FALSE(mStreamStopped);
        mStreamStarted = true;
        return true;
    }

    bool postVideoStreamStart_locked(const std::shared_ptr<evs::IEvsCameraStream>& receiver,
                                     ndk::ScopedAStatus& status,
                                     std::unique_lock<std::mutex>& lck) override {
        mPostStartCalled = true;
        EXPECT_EQ(mStreamState, StreamState::RUNNING);
        EXPECT_TRUE(mStreamStarted);
        EXPECT_FALSE(mStreamStopped);
        return Base::postVideoStreamStart_locked(receiver, status, lck);
    }

    bool preVideoStreamStop_locked(ndk::ScopedAStatus& status,
                                   std::unique_lock<std::mutex>& lck) override {
        // Skip the check if stop was called before.
        if (!mPreStopCalled) {
            mPreStopCalled = true;
            EXPECT_EQ(mStreamState, StreamState::RUNNING);
            EXPECT_TRUE(mStreamStarted);
            EXPECT_FALSE(mStreamStopped);
        }
        return Base::preVideoStreamStop_locked(status, lck);
    }

    bool stopVideoStreamImpl_locked(ndk::ScopedAStatus& /* status */,
                                    std::unique_lock<std::mutex>& /* lck */) override {
        EXPECT_EQ(mStreamState, StreamState::STOPPING);
        EXPECT_TRUE(mStreamStarted);
        EXPECT_FALSE(mStreamStopped);
        mStreamStopped = true;
        return true;
    }

    bool postVideoStreamStop_locked(ndk::ScopedAStatus& status,
                                    std::unique_lock<std::mutex>& lck) override {
        mPostStopCalled = true;
        const auto ret = Base::postVideoStreamStop_locked(status, lck);
        EXPECT_EQ(mStreamState, StreamState::STOPPED);
        EXPECT_TRUE(mStreamStarted);
        EXPECT_TRUE(mStreamStopped);
        return ret;
    }

    MOCK_METHOD(::ndk::ScopedAStatus, forcePrimaryClient,
                (const std::shared_ptr<::aidl::android::hardware::automotive::evs::IEvsDisplay>&
                         in_display),
                (override));
    MOCK_METHOD(::ndk::ScopedAStatus, getCameraInfo,
                (::aidl::android::hardware::automotive::evs::CameraDesc * _aidl_return),
                (override));
    MOCK_METHOD(::ndk::ScopedAStatus, getExtendedInfo,
                (int32_t in_opaqueIdentifier, std::vector<uint8_t>* _aidl_return), (override));
    MOCK_METHOD(::ndk::ScopedAStatus, getIntParameter,
                (::aidl::android::hardware::automotive::evs::CameraParam in_id,
                 std::vector<int32_t>* _aidl_return),
                (override));
    MOCK_METHOD(::ndk::ScopedAStatus, getIntParameterRange,
                (::aidl::android::hardware::automotive::evs::CameraParam in_id,
                 ::aidl::android::hardware::automotive::evs::ParameterRange* _aidl_return),
                (override));
    MOCK_METHOD(::ndk::ScopedAStatus, getParameterList,
                (std::vector<::aidl::android::hardware::automotive::evs::CameraParam> *
                 _aidl_return),
                (override));
    MOCK_METHOD(::ndk::ScopedAStatus, getPhysicalCameraInfo,
                (const std::string& in_deviceId,
                 ::aidl::android::hardware::automotive::evs::CameraDesc* _aidl_return),
                (override));
    MOCK_METHOD(::ndk::ScopedAStatus, setExtendedInfo,
                (int32_t in_opaqueIdentifier, const std::vector<uint8_t>& in_opaqueValue),
                (override));
    MOCK_METHOD(::ndk::ScopedAStatus, setIntParameter,
                (::aidl::android::hardware::automotive::evs::CameraParam in_id, int32_t in_value,
                 std::vector<int32_t>* _aidl_return),
                (override));
    MOCK_METHOD(::ndk::ScopedAStatus, setPrimaryClient, (), (override));
    MOCK_METHOD(::ndk::ScopedAStatus, unsetPrimaryClient, (), (override));

    bool mStreamStarted = false;
    bool mStreamStopped = false;
    bool mPreStartCalled = false;
    bool mPostStartCalled = false;
    bool mPreStopCalled = false;
    bool mPostStopCalled = false;
};

class MockEvsCameraStream : public evs::IEvsCameraStream {
    MOCK_METHOD(::ndk::SpAIBinder, asBinder, (), (override));
    MOCK_METHOD(bool, isRemote, (), (override));
    MOCK_METHOD(
            ::ndk::ScopedAStatus, deliverFrame,
            (const std::vector<::aidl::android::hardware::automotive::evs::BufferDesc>& in_buffer),
            (override));
    MOCK_METHOD(::ndk::ScopedAStatus, notify,
                (const ::aidl::android::hardware::automotive::evs::EvsEventDesc& in_event),
                (override));
    MOCK_METHOD(::ndk::ScopedAStatus, getInterfaceVersion, (int32_t * _aidl_return), (override));
    MOCK_METHOD(::ndk::ScopedAStatus, getInterfaceHash, (std::string * _aidl_return), (override));
};

using StreamState = EvsCameraForTest::StreamState;

TEST(EvsCameraStateTest, StateChangeHooks) {
    auto evsCam = ndk::SharedRefBase::make<EvsCameraForTest>();
    auto mockStream = ndk::SharedRefBase::make<MockEvsCameraStream>();
    EXPECT_FALSE(evsCam->mPreStartCalled);
    EXPECT_FALSE(evsCam->mPostStartCalled);
    EXPECT_FALSE(evsCam->mPreStopCalled);
    EXPECT_FALSE(evsCam->mPostStopCalled);
    EXPECT_FALSE(evsCam->mStreamStarted);
    EXPECT_FALSE(evsCam->mStreamStopped);
    EXPECT_EQ(evsCam->mStreamState, StreamState::STOPPED);
    evsCam->startVideoStream(mockStream);

    EXPECT_TRUE(evsCam->mPreStartCalled);
    EXPECT_TRUE(evsCam->mPostStartCalled);
    EXPECT_FALSE(evsCam->mPreStopCalled);
    EXPECT_FALSE(evsCam->mPostStopCalled);
    EXPECT_TRUE(evsCam->mStreamStarted);
    EXPECT_FALSE(evsCam->mStreamStopped);
    EXPECT_EQ(evsCam->mStreamState, StreamState::RUNNING);
    evsCam->stopVideoStream();

    EXPECT_TRUE(evsCam->mPreStartCalled);
    EXPECT_TRUE(evsCam->mPostStartCalled);
    EXPECT_TRUE(evsCam->mPreStopCalled);
    EXPECT_TRUE(evsCam->mPostStopCalled);
    EXPECT_TRUE(evsCam->mStreamStarted);
    EXPECT_TRUE(evsCam->mStreamStopped);
    EXPECT_EQ(evsCam->mStreamState, StreamState::STOPPED);

    evsCam->shutdown();
    EXPECT_EQ(evsCam->mStreamState, StreamState::DEAD);
}

}  // namespace aidl::android::hardware::automotive::evs::implementation
