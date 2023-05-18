/*
 * Copyright 2022 The Android Open Source Project
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

#include <android/binder_manager.h>

#include <aidl/Gtest.h>
#include <aidl/Vintf.h>

#include <aidl/android/hardware/tv/input/BnTvInputCallback.h>
#include <aidl/android/hardware/tv/input/ITvInput.h>
#include <aidl/android/hardware/tv/input/TvInputDeviceInfo.h>
#include <aidl/android/hardware/tv/input/TvInputEvent.h>
#include <aidl/android/hardware/tv/input/TvMessageEvent.h>
#include <aidl/android/hardware/tv/input/TvMessageEventType.h>
#include <aidl/android/hardware/tv/input/TvStreamConfig.h>
#include <fmq/AidlMessageQueue.h>

#include <log/log.h>
#include <utils/KeyedVector.h>

using namespace aidl::android::hardware::tv::input;
using namespace std;
using ::aidl::android::hardware::common::NativeHandle;
using ::aidl::android::hardware::common::fmq::MQDescriptor;
using ::aidl::android::hardware::common::fmq::SynchronizedReadWrite;
using ::android::AidlMessageQueue;

#define WAIT_FOR_EVENT_TIMEOUT 5
#define DEFAULT_ID INT32_MIN

namespace VtsHalTvInputTargetTest {

class TvInputAidlTest : public testing::TestWithParam<string> {
  public:
    class TvInputCallback : public BnTvInputCallback {
      public:
        TvInputCallback(shared_ptr<TvInputAidlTest> parent);
        ::ndk::ScopedAStatus notify(const TvInputEvent& in_event) override;
        ::ndk::ScopedAStatus notifyTvMessageEvent(const TvMessageEvent& in_event) override;

      private:
        shared_ptr<TvInputAidlTest> parent_;
    };

    virtual void SetUp() override;
    virtual void TearDown() override;

    /* Called when a DEVICE_AVAILABLE event is received. */
    void onDeviceAvailable(const TvInputDeviceInfo& deviceInfo);

    /* Called when a DEVICE_UNAVAILABLE event is received. */
    void onDeviceUnavailable(int32_t deviceId);

    /* Called when a STREAM_CONFIGURATIONS_CHANGED event is received. */
    ::ndk::ScopedAStatus onStreamConfigurationsChanged(int32_t deviceId);

    /* Gets and updates the stream configurations for a device. */
    ::ndk::ScopedAStatus updateStreamConfigurations(int32_t deviceId);

    /* Gets and updates the stream configurations for all existing devices. */
    void updateAllStreamConfigurations();

    /* Returns a list of indices of stream_config_ whose corresponding values are not empty. */
    vector<size_t> getConfigIndices();

    /*
     * Returns DEFAULT_ID if there is no missing integer in the range [0, the size of nums).
     * Otherwise, returns the smallest missing non-negative integer.
     */
    int32_t getNumNotIn(vector<int32_t>& nums);

  protected:
    shared_ptr<ITvInput> tv_input_;
    shared_ptr<TvInputCallback> tv_input_callback_;
    android::KeyedVector<int32_t, TvInputDeviceInfo> device_info_;
    android::KeyedVector<int32_t, vector<TvStreamConfig>> stream_config_;
    mutex mutex_;
};

}  // namespace VtsHalTvInputTargetTest
