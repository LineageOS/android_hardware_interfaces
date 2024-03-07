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

#include "VtsHalTvInputTargetTest.h"

#include <android-base/properties.h>
#include <android/binder_ibinder.h>
#include <android/binder_process.h>
#include <android/binder_status.h>

using namespace VtsHalTvInputTargetTest;

TvInputAidlTest::TvInputCallback::TvInputCallback(shared_ptr<TvInputAidlTest> parent)
    : parent_(parent) {}

::ndk::ScopedAStatus TvInputAidlTest::TvInputCallback::notify(const TvInputEvent& in_event) {
    unique_lock<mutex> lock(parent_->mutex_);

    switch (in_event.type) {
        case TvInputEventType::DEVICE_AVAILABLE:
            parent_->onDeviceAvailable(in_event.deviceInfo);
            break;
        case TvInputEventType::DEVICE_UNAVAILABLE:
            parent_->onDeviceUnavailable(in_event.deviceInfo.deviceId);
            break;
        case TvInputEventType::STREAM_CONFIGURATIONS_CHANGED:
            parent_->onStreamConfigurationsChanged(in_event.deviceInfo.deviceId);
            break;
    }
    return ::ndk::ScopedAStatus::ok();
}

::ndk::ScopedAStatus TvInputAidlTest::TvInputCallback::notifyTvMessageEvent(
        const TvMessageEvent& in_event) {
    return ::ndk::ScopedAStatus::ok();
}

void TvInputAidlTest::SetUp() {
    if (AServiceManager_isDeclared(GetParam().c_str())) {
        ::ndk::SpAIBinder binder(AServiceManager_waitForService(GetParam().c_str()));
        tv_input_ = ITvInput::fromBinder(binder);
    } else {
        tv_input_ = nullptr;
    }
    ASSERT_NE(tv_input_, nullptr);

    tv_input_callback_ =
            ::ndk::SharedRefBase::make<TvInputCallback>(shared_ptr<TvInputAidlTest>(this));
    ASSERT_NE(tv_input_callback_, nullptr);

    tv_input_->setCallback(tv_input_callback_);
    // All events received within the timeout should be handled.
    sleep(WAIT_FOR_EVENT_TIMEOUT);
}

void TvInputAidlTest::TearDown() {
    tv_input_ = nullptr;
}

void TvInputAidlTest::onDeviceAvailable(const TvInputDeviceInfo& deviceInfo) {
    ALOGD("onDeviceAvailable for device id %d", deviceInfo.deviceId);
    device_info_.add(deviceInfo.deviceId, deviceInfo);
}

void TvInputAidlTest::onDeviceUnavailable(int32_t deviceId) {
    ALOGD("onDeviceUnavailable for device id %d", deviceId);
    device_info_.removeItem(deviceId);
    stream_config_.removeItem(deviceId);
}

::ndk::ScopedAStatus TvInputAidlTest::onStreamConfigurationsChanged(int32_t deviceId) {
    ALOGD("onStreamConfigurationsChanged for device id %d", deviceId);
    return updateStreamConfigurations(deviceId);
}

::ndk::ScopedAStatus TvInputAidlTest::updateStreamConfigurations(int32_t deviceId) {
    stream_config_.removeItem(deviceId);
    vector<TvStreamConfig> list;
    ::ndk::ScopedAStatus status = tv_input_->getStreamConfigurations(deviceId, &list);
    if (status.isOk()) {
        stream_config_.add(deviceId, list);
    }
    return status;
}

void TvInputAidlTest::updateAllStreamConfigurations() {
    for (size_t i = 0; i < device_info_.size(); i++) {
        int32_t device_id = device_info_.keyAt(i);
        updateStreamConfigurations(device_id);
    }
}

vector<size_t> TvInputAidlTest::getConfigIndices() {
    vector<size_t> indices;
    for (size_t i = 0; i < stream_config_.size(); i++) {
        if (stream_config_.valueAt(i).size() != 0) {
            indices.push_back(i);
        }
    }
    return indices;
}

int32_t TvInputAidlTest::getNumNotIn(vector<int32_t>& nums) {
    int32_t result = DEFAULT_ID;
    int32_t size = static_cast<int32_t>(nums.size());
    for (int32_t i = 0; i < size; i++) {
        // Put every element to its target position, if possible.
        int32_t target_pos = nums[i];
        while (target_pos >= 0 && target_pos < size && i != target_pos &&
               nums[i] != nums[target_pos]) {
            swap(nums[i], nums[target_pos]);
            target_pos = nums[i];
        }
    }

    for (int32_t i = 0; i < size; i++) {
        if (nums[i] != i) {
            return i;
        }
    }
    return result;
}

bool TvInputAidlTest::isValidHandle(NativeHandle& handle) {
    if (handle.fds.empty()) {
        return false;
    }
    for (size_t i = 0; i < handle.fds.size(); i++) {
        int fd = handle.fds[i].get();
        if (fcntl(fd, F_GETFL) < 0) {
            return false;
        }
    }
    return true;
}

/*
 * GetStreamConfigTest:
 * Calls updateStreamConfigurations() for each existing device
 * Checks returned results
 */
TEST_P(TvInputAidlTest, GetStreamConfigTest) {
    unique_lock<mutex> lock(mutex_);

    for (size_t i = 0; i < device_info_.size(); i++) {
        int32_t device_id = device_info_.keyAt(i);
        ALOGD("GetStreamConfigTest: device_id=%d", device_id);
        ASSERT_TRUE(updateStreamConfigurations(device_id).isOk());
    }
}

/*
 * OpenAndCloseStreamTest:
 * Calls openStream() and then closeStream() for each existing stream
 * Checks returned results
 */
TEST_P(TvInputAidlTest, OpenAndCloseStreamTest) {
    unique_lock<mutex> lock(mutex_);

    updateAllStreamConfigurations();

    for (size_t j = 0; j < stream_config_.size(); j++) {
        int32_t device_id = stream_config_.keyAt(j);
        vector<TvStreamConfig> config = stream_config_.valueAt(j);
        for (size_t i = 0; i < config.size(); i++) {
            NativeHandle handle;
            int32_t stream_id = config[i].streamId;
            ALOGD("OpenAndCloseStreamTest: open stream, device_id=%d, stream_id=%d", device_id,
                  stream_id);
            ASSERT_TRUE(tv_input_->openStream(device_id, stream_id, &handle).isOk());
            ASSERT_TRUE(isValidHandle(handle));

            ALOGD("OpenAndCloseStreamTest: close stream, device_id=%d, stream_id=%d", device_id,
                  stream_id);
            ASSERT_TRUE(tv_input_->closeStream(device_id, stream_id).isOk());
        }
    }
}

/*
 * InvalidDeviceIdTest:
 * Calls updateStreamConfigurations(), openStream(), and closeStream()
 * for a non-existing device
 * Checks returned results
 * The results should be ITvInput::STATUS_INVALID_ARGUMENTS
 */
TEST_P(TvInputAidlTest, InvalidDeviceIdTest) {
    unique_lock<mutex> lock(mutex_);

    vector<int32_t> device_ids;
    for (size_t i = 0; i < device_info_.size(); i++) {
        device_ids.push_back(device_info_.keyAt(i));
    }
    // Get a non-existing device ID.
    int32_t id = getNumNotIn(device_ids);
    ALOGD("InvalidDeviceIdTest: update stream config, device_id=%d", id);
    ASSERT_TRUE(updateStreamConfigurations(id).getServiceSpecificError() ==
                ITvInput::STATUS_INVALID_ARGUMENTS);

    int32_t stream_id = 0;
    NativeHandle handle;

    ALOGD("InvalidDeviceIdTest: open stream, device_id=%d, stream_id=%d", id, stream_id);
    ASSERT_TRUE(tv_input_->openStream(id, stream_id, &handle).getServiceSpecificError() ==
                ITvInput::STATUS_INVALID_ARGUMENTS);

    ALOGD("InvalidDeviceIdTest: close stream, device_id=%d, stream_id=%d", id, stream_id);
    ASSERT_TRUE(tv_input_->closeStream(id, stream_id).getServiceSpecificError() ==
                ITvInput::STATUS_INVALID_ARGUMENTS);
}

/*
 * InvalidStreamIdTest:
 * Calls openStream(), and closeStream() for a non-existing stream
 * Checks returned results
 * The results should be ITvInput::STATUS_INVALID_ARGUMENTS
 */
TEST_P(TvInputAidlTest, InvalidStreamIdTest) {
    unique_lock<mutex> lock(mutex_);

    if (device_info_.isEmpty()) {
        return;
    }
    updateAllStreamConfigurations();

    int32_t device_id = device_info_.keyAt(0);
    // Get a non-existing stream ID.
    int32_t id = DEFAULT_ID;
    if (stream_config_.indexOfKey(device_id) >= 0) {
        vector<int32_t> stream_ids;
        vector<TvStreamConfig> config = stream_config_.valueFor(device_id);
        for (size_t i = 0; i < config.size(); i++) {
            stream_ids.push_back(config[i].streamId);
        }
        id = getNumNotIn(stream_ids);
    }

    NativeHandle handle;

    ALOGD("InvalidStreamIdTest: open stream, device_id=%d, stream_id=%d", device_id, id);
    ASSERT_TRUE(tv_input_->openStream(device_id, id, &handle).getServiceSpecificError() ==
                ITvInput::STATUS_INVALID_ARGUMENTS);

    ALOGD("InvalidStreamIdTest: close stream, device_id=%d, stream_id=%d", device_id, id);
    ASSERT_TRUE(tv_input_->closeStream(device_id, id).getServiceSpecificError() ==
                ITvInput::STATUS_INVALID_ARGUMENTS);
}

/*
 * OpenAnOpenedStreamsTest:
 * Calls openStream() twice for a stream (if any)
 * Checks returned results
 * The result of the second call should be ITvInput::STATUS_INVALID_STATE
 */
TEST_P(TvInputAidlTest, OpenAnOpenedStreamsTest) {
    unique_lock<mutex> lock(mutex_);

    updateAllStreamConfigurations();
    vector<size_t> indices = getConfigIndices();
    if (indices.empty()) {
        return;
    }
    int32_t device_id = stream_config_.keyAt(indices[0]);
    vector<TvStreamConfig> streamConfigs = stream_config_.valueAt(indices[0]);
    if (streamConfigs.empty()) {
        return;
    }
    int32_t stream_id = streamConfigs[0].streamId;
    NativeHandle handle;

    ALOGD("OpenAnOpenedStreamsTest: open stream, device_id=%d, stream_id=%d", device_id, stream_id);
    ASSERT_TRUE(tv_input_->openStream(device_id, stream_id, &handle).isOk());
    ASSERT_TRUE(isValidHandle(handle));

    ALOGD("OpenAnOpenedStreamsTest: open stream, device_id=%d, stream_id=%d", device_id, stream_id);
    ASSERT_TRUE(tv_input_->openStream(device_id, stream_id, &handle).getServiceSpecificError() ==
                ITvInput::STATUS_INVALID_STATE);

    // close stream as subsequent tests assume no open streams
    ALOGD("OpenAnOpenedStreamsTest: close stream, device_id=%d, stream_id=%d", device_id,
          stream_id);
    ASSERT_TRUE(tv_input_->closeStream(device_id, stream_id).isOk());
}

/*
 * CloseStreamBeforeOpenTest:
 * Calls closeStream() without calling openStream() for a stream (if any)
 * Checks the returned result
 * The result should be ITvInput::STATUS_INVALID_STATE
 */
TEST_P(TvInputAidlTest, CloseStreamBeforeOpenTest) {
    unique_lock<mutex> lock(mutex_);

    updateAllStreamConfigurations();
    vector<size_t> indices = getConfigIndices();
    if (indices.empty()) {
        return;
    }
    int32_t device_id = stream_config_.keyAt(indices[0]);
    vector<TvStreamConfig> streamConfigs = stream_config_.valueAt(indices[0]);
    if (streamConfigs.empty()) {
        return;
    }
    int32_t stream_id = streamConfigs[0].streamId;

    ALOGD("CloseStreamBeforeOpenTest: close stream, device_id=%d, stream_id=%d", device_id,
          stream_id);
    ASSERT_TRUE(tv_input_->closeStream(device_id, stream_id).getServiceSpecificError() ==
                ITvInput::STATUS_INVALID_STATE);
}

TEST_P(TvInputAidlTest, SetTvMessageEnabledTest) {
    unique_lock<mutex> lock(mutex_);

    updateAllStreamConfigurations();
    vector<size_t> indices = getConfigIndices();
    if (indices.empty()) {
        return;
    }
    int32_t device_id = stream_config_.keyAt(indices[0]);
    vector<TvStreamConfig> streamConfigs = stream_config_.valueAt(indices[0]);
    if (streamConfigs.empty()) {
        return;
    }
    int32_t stream_id = streamConfigs[0].streamId;
    ALOGD("SetTvMessageEnabledTest: device_id=%d, stream_id=%d", device_id, stream_id);
    tv_input_->setTvMessageEnabled(device_id, stream_id, TvMessageEventType::WATERMARK, true);
}

TEST_P(TvInputAidlTest, GetTvMessageQueueTest) {
    unique_lock<mutex> lock(mutex_);

    updateAllStreamConfigurations();
    vector<size_t> indices = getConfigIndices();
    if (indices.empty()) {
        return;
    }
    int32_t device_id = stream_config_.keyAt(indices[0]);
    vector<TvStreamConfig> streamConfigs = stream_config_.valueAt(indices[0]);
    if (streamConfigs.empty()) {
        return;
    }
    int32_t stream_id = streamConfigs[0].streamId;
    ALOGD("GetTvMessageQueueTest: device_id=%d, stream_id=%d", device_id, stream_id);
    MQDescriptor<int8_t, SynchronizedReadWrite> queueDescriptor;
    AidlMessageQueue<int8_t, SynchronizedReadWrite>* queue;
    tv_input_->getTvMessageQueueDesc(&queueDescriptor, device_id, stream_id);
    queue = new (std::nothrow) AidlMessageQueue<int8_t, SynchronizedReadWrite>(queueDescriptor);
    ASSERT_TRUE(queue->isValid());
    delete queue;
}

INSTANTIATE_TEST_SUITE_P(PerInstance, TvInputAidlTest,
                         testing::ValuesIn(android::getAidlHalInstanceNames(ITvInput::descriptor)),
                         android::PrintInstanceNameToString);

// TODO remove from the allow list once the cf tv target is enabled for testing
GTEST_ALLOW_UNINSTANTIATED_PARAMETERIZED_TEST(TvInputAidlTest);
