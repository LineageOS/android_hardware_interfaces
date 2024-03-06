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

#define LOG_TAG "android.hardware.tv.input-service.example"

#include <utils/Log.h>

#include "TvInput.h"

namespace aidl {
namespace android {
namespace hardware {
namespace tv {
namespace input {

TvInput::TvInput() {}

void TvInput::init() {
    // Set up TvInputDeviceInfo and TvStreamConfig
    mDeviceInfos[0] = shared_ptr<TvInputDeviceInfoWrapper>(
            new TvInputDeviceInfoWrapper(0, TvInputType::TUNER, true));
    mDeviceInfos[1] = shared_ptr<TvInputDeviceInfoWrapper>(
            new TvInputDeviceInfoWrapper(1, TvInputType::HDMI, true));
    mDeviceInfos[3] = shared_ptr<TvInputDeviceInfoWrapper>(
            new TvInputDeviceInfoWrapper(3, TvInputType::DISPLAY_PORT, true));

    mStreamConfigs[0] = {
            {1, shared_ptr<TvStreamConfigWrapper>(new TvStreamConfigWrapper(1, 720, 1080, false))}};
    mStreamConfigs[1] = {{11, shared_ptr<TvStreamConfigWrapper>(
                                      new TvStreamConfigWrapper(11, 360, 480, false))}};
    mStreamConfigs[3] = {{5, shared_ptr<TvStreamConfigWrapper>(
                                     new TvStreamConfigWrapper(5, 1080, 1920, false))}};

    mQueue = shared_ptr<AidlMessageQueue<int8_t, SynchronizedReadWrite>>(
            new (std::nothrow) AidlMessageQueue<int8_t, SynchronizedReadWrite>(8));
}

::ndk::ScopedAStatus TvInput::setCallback(const shared_ptr<ITvInputCallback>& in_callback) {
    ALOGV("%s", __FUNCTION__);

    mCallback = in_callback;

    TvInputEvent event;
    event.type = TvInputEventType::DEVICE_AVAILABLE;

    event.deviceInfo = mDeviceInfos[0]->deviceInfo;
    mCallback->notify(event);

    event.deviceInfo = mDeviceInfos[1]->deviceInfo;
    mCallback->notify(event);

    event.deviceInfo = mDeviceInfos[3]->deviceInfo;
    mCallback->notify(event);

    return ::ndk::ScopedAStatus::ok();
}

::ndk::ScopedAStatus TvInput::setTvMessageEnabled(int32_t deviceId, int32_t streamId,
                                                  TvMessageEventType in_type, bool enabled) {
    ALOGV("%s", __FUNCTION__);

    if (mStreamConfigs.count(deviceId) == 0) {
        ALOGW("Device with id %d isn't available", deviceId);
        return ::ndk::ScopedAStatus::fromServiceSpecificError(STATUS_INVALID_ARGUMENTS);
    }

    // When calling notifyTvMessage, make sure to verify against this map.
    mTvMessageEventEnabled[deviceId][streamId][in_type] = enabled;

    return ::ndk::ScopedAStatus::ok();
}

::ndk::ScopedAStatus TvInput::getTvMessageQueueDesc(
        MQDescriptor<int8_t, SynchronizedReadWrite>* out_queue, int32_t in_deviceId,
        int32_t in_streamId) {
    ALOGV("%s", __FUNCTION__);
    ::ndk::ScopedAStatus status = ::ndk::ScopedAStatus::ok();
    if (mStreamConfigs.count(in_deviceId) == 0) {
        ALOGW("Device with id %d isn't available", in_deviceId);
        status = ::ndk::ScopedAStatus::fromServiceSpecificError(STATUS_INVALID_ARGUMENTS);
    } else if (!mQueue->isValid()) {
        ALOGE("Tv Message Queue was not properly initialized");
        status = ::ndk::ScopedAStatus::fromServiceSpecificError(STATUS_INVALID_STATE);
    } else {
        *out_queue = mQueue->dupeDesc();
    }
    return status;
}

::ndk::ScopedAStatus TvInput::getStreamConfigurations(int32_t in_deviceId,
                                                      vector<TvStreamConfig>* _aidl_return) {
    ALOGV("%s", __FUNCTION__);

    if (mStreamConfigs.count(in_deviceId) == 0) {
        ALOGW("Device with id %d isn't available", in_deviceId);
        return ::ndk::ScopedAStatus::fromServiceSpecificError(STATUS_INVALID_ARGUMENTS);
    }

    for (auto const& iconfig : mStreamConfigs[in_deviceId]) {
        _aidl_return->push_back(iconfig.second->streamConfig);
    }

    return ::ndk::ScopedAStatus::ok();
}

::ndk::ScopedAStatus TvInput::openStream(int32_t in_deviceId, int32_t in_streamId,
                                         NativeHandle* _aidl_return) {
    ALOGV("%s", __FUNCTION__);

    if (mStreamConfigs.count(in_deviceId) == 0 ||
        mStreamConfigs[in_deviceId].count(in_streamId) == 0) {
        ALOGW("Stream with device id %d, stream id %d isn't available", in_deviceId, in_streamId);
        return ::ndk::ScopedAStatus::fromServiceSpecificError(STATUS_INVALID_ARGUMENTS);
    }
    if (mStreamConfigs[in_deviceId][in_streamId]->isOpen) {
        ALOGW("Stream with device id %d, stream id %d is already opened", in_deviceId, in_streamId);
        return ::ndk::ScopedAStatus::fromServiceSpecificError(STATUS_INVALID_STATE);
    }
    mStreamConfigs[in_deviceId][in_streamId]->handle = createNativeHandle(in_streamId);
    *_aidl_return = makeToAidl(mStreamConfigs[in_deviceId][in_streamId]->handle);
    mStreamConfigs[in_deviceId][in_streamId]->isOpen = true;
    return ::ndk::ScopedAStatus::ok();
}

::ndk::ScopedAStatus TvInput::closeStream(int32_t in_deviceId, int32_t in_streamId) {
    ALOGV("%s", __FUNCTION__);

    if (mStreamConfigs.count(in_deviceId) == 0 ||
        mStreamConfigs[in_deviceId].count(in_streamId) == 0) {
        ALOGW("Stream with device id %d, stream id %d isn't available", in_deviceId, in_streamId);
        return ::ndk::ScopedAStatus::fromServiceSpecificError(STATUS_INVALID_ARGUMENTS);
    }
    if (!mStreamConfigs[in_deviceId][in_streamId]->isOpen) {
        ALOGW("Stream with device id %d, stream id %d is already closed", in_deviceId, in_streamId);
        return ::ndk::ScopedAStatus::fromServiceSpecificError(STATUS_INVALID_STATE);
    }
    native_handle_delete(mStreamConfigs[in_deviceId][in_streamId]->handle);
    mStreamConfigs[in_deviceId][in_streamId]->handle = nullptr;
    mStreamConfigs[in_deviceId][in_streamId]->isOpen = false;
    return ::ndk::ScopedAStatus::ok();
}

native_handle_t* TvInput::createNativeHandle(int fd) {
    native_handle_t* handle = native_handle_create(1, 1);
    if (handle == nullptr) {
        ALOGE("[TVInput] Failed to create native_handle %d", errno);
        return nullptr;
    }
    handle->data[0] = dup(0);
    handle->data[1] = fd;
    return handle;
}

}  // namespace input
}  // namespace tv
}  // namespace hardware
}  // namespace android
}  // namespace aidl
