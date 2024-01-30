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
  public:
    using EvsCamera::increaseAvailableFrames_unsafe;
    using EvsCamera::returnBuffer_unsafe;
    using EvsCamera::useBuffer_unsafe;

    ~EvsCameraForTest() override { shutdown(); }

    ::android::status_t allocateOneFrame(buffer_handle_t* handle) override {
        static std::intptr_t handle_cnt = 0;
        *handle = reinterpret_cast<buffer_handle_t>(++handle_cnt);
        return ::android::OK;
    }

    void freeOneFrame(const buffer_handle_t /* handle */) override {
        // Nothing to free because the handles are fake.
    }

    void checkBufferOrder() {
        for (std::size_t idx = 0; idx < mBuffers.size(); ++idx) {
            const auto& buffer = mBuffers[idx];
            EXPECT_EQ(idx < mFramesInUse, buffer.inUse);
            EXPECT_EQ(idx < mAvailableFrames, buffer.handle != nullptr);
            EXPECT_LE(mFramesInUse, mAvailableFrames);
        }
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
    MOCK_METHOD(bool, startVideoStreamImpl_locked,
                (const std::shared_ptr<evs::IEvsCameraStream>& receiver, ndk::ScopedAStatus& status,
                 std::unique_lock<std::mutex>& lck),
                (override));
    MOCK_METHOD(bool, stopVideoStreamImpl_locked,
                (ndk::ScopedAStatus & status, std::unique_lock<std::mutex>& lck), (override));
};

TEST(EvsCameraBufferTest, ChangeBufferPoolSize) {
    auto evsCam = ndk::SharedRefBase::make<EvsCameraForTest>();
    EXPECT_TRUE(evsCam->setMaxFramesInFlight(100).isOk());
    evsCam->checkBufferOrder();
    EXPECT_TRUE(evsCam->setMaxFramesInFlight(50).isOk());
    evsCam->checkBufferOrder();

    // 2 buffers in use.
    const auto [id1, handle1] = evsCam->useBuffer_unsafe();
    const auto [id2, handle2] = evsCam->useBuffer_unsafe();
    std::ignore = evsCam->useBuffer_unsafe();

    // It allows you to set the buffer pool size to 1, but it will keep the space for the in use
    // buffers.
    EXPECT_TRUE(evsCam->setMaxFramesInFlight(1).isOk());
    evsCam->checkBufferOrder();

    evsCam->returnBuffer_unsafe(id1);
    evsCam->checkBufferOrder();
    evsCam->returnBuffer_unsafe(id2);
    evsCam->checkBufferOrder();
}

TEST(EvsCameraBufferTest, UseAndReturn) {
    constexpr std::size_t kNumOfHandles = 20;
    auto evsCam = ndk::SharedRefBase::make<EvsCameraForTest>();

    // Our "fake handles" of this test case is 1 to kNumOfHandles.
    for (std::size_t i = 1; i <= kNumOfHandles; ++i) {
        evsCam->increaseAvailableFrames_unsafe(reinterpret_cast<buffer_handle_t>(i));
    }
    evsCam->checkBufferOrder();

    {
        std::vector<std::pair<std::size_t, std::intptr_t>> inUseIDHandlePairs;
        std::unordered_set<std::size_t> inUseIDs;
        std::unordered_set<std::intptr_t> inUseHandles;
        for (std::size_t i = 0; i < kNumOfHandles; ++i) {
            const auto [id, handle] = evsCam->useBuffer_unsafe();
            const std::size_t handleInt = reinterpret_cast<std::size_t>(handle);
            EXPECT_TRUE(EvsCamera::IsBufferIDValid(id));
            EXPECT_NE(handle, nullptr);
            EXPECT_LT(id, kNumOfHandles);

            // handleInt must be between [1, kNumOfHandles] as we "allocated" above.
            EXPECT_LT(0u, handleInt);
            EXPECT_LE(handleInt, kNumOfHandles);

            inUseIDHandlePairs.push_back({id, handleInt});
            EXPECT_TRUE(inUseIDs.insert(id).second);
            EXPECT_TRUE(inUseHandles.insert(handleInt).second);
            evsCam->checkBufferOrder();
        }
        // Return buffers in the order of acquiring.
        for (const auto [id, handleInt] : inUseIDHandlePairs) {
            evsCam->returnBuffer_unsafe(id);
            evsCam->checkBufferOrder();
        }
    }

    {
        std::vector<std::pair<std::size_t, std::intptr_t>> inUseIDHandlePairs;
        std::unordered_set<std::size_t> inUseIDs;
        std::unordered_set<std::intptr_t> inUseHandles;
        for (std::size_t i = 0; i < kNumOfHandles; ++i) {
            const auto [id, handle] = evsCam->useBuffer_unsafe();
            const std::size_t handleInt = reinterpret_cast<std::size_t>(handle);
            EXPECT_TRUE(EvsCamera::IsBufferIDValid(id));
            EXPECT_NE(handle, nullptr);
            EXPECT_LT(id, kNumOfHandles);

            // handleInt must be between [1, kNumOfHandles] as we "allocated" above.
            EXPECT_LT(0u, handleInt);
            EXPECT_LE(handleInt, kNumOfHandles);

            inUseIDHandlePairs.push_back({id, handleInt});
            EXPECT_TRUE(inUseIDs.insert(id).second);
            EXPECT_TRUE(inUseHandles.insert(handleInt).second);
            evsCam->checkBufferOrder();
        }
        // Return buffers in the reverse order of acquiring.
        std::reverse(inUseIDHandlePairs.begin(), inUseIDHandlePairs.end());
        for (const auto [id, handleInt] : inUseIDHandlePairs) {
            evsCam->returnBuffer_unsafe(id);
            evsCam->checkBufferOrder();
        }
    }

    {
        // Making sure the handles are still in [1, kNumOfHandles] and IDs are still [0,
        // kNumOfHandles). The mapping may be different, though.
        std::vector<std::pair<std::size_t, std::intptr_t>> inUseIDHandlePairs;
        std::unordered_set<std::size_t> inUseIDs;
        std::unordered_set<std::intptr_t> inUseHandles;
        for (std::size_t i = 0; i < kNumOfHandles; ++i) {
            const auto [id, handle] = evsCam->useBuffer_unsafe();
            const std::size_t handleInt = reinterpret_cast<std::size_t>(handle);
            EXPECT_TRUE(EvsCamera::IsBufferIDValid(id));
            EXPECT_NE(handle, nullptr);
            EXPECT_LT(id, kNumOfHandles);

            // handleInt must be between [1, kNumOfHandles] as we "allocated" above.
            EXPECT_LT(0u, handleInt);
            EXPECT_LE(handleInt, kNumOfHandles);

            inUseIDHandlePairs.push_back({id, handleInt});
            EXPECT_TRUE(inUseIDs.insert(id).second);
            EXPECT_TRUE(inUseHandles.insert(handleInt).second);
            evsCam->checkBufferOrder();
        }
    }
}

}  // namespace aidl::android::hardware::automotive::evs::implementation
