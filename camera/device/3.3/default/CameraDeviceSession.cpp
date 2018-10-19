/*
 * Copyright (C) 2017 The Android Open Source Project
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

#define LOG_TAG "CamDevSession@3.3-impl"
#include <android/log.h>

#include <set>
#include <utils/Trace.h>
#include <hardware/gralloc.h>
#include <hardware/gralloc1.h>
#include "CameraDeviceSession.h"

namespace android {
namespace hardware {
namespace camera {
namespace device {
namespace V3_3 {
namespace implementation {

CameraDeviceSession::CameraDeviceSession(
    camera3_device_t* device,
    const camera_metadata_t* deviceInfo,
    const sp<V3_2::ICameraDeviceCallback>& callback) :
        V3_2::implementation::CameraDeviceSession(device, deviceInfo, callback) {
}

CameraDeviceSession::~CameraDeviceSession() {
}

Return<void> CameraDeviceSession::configureStreams_3_3(
        const StreamConfiguration& requestedConfiguration,
        ICameraDeviceSession::configureStreams_3_3_cb _hidl_cb)  {
    Status status = initStatus();
    HalStreamConfiguration outStreams;

    // hold the inflight lock for entire configureStreams scope since there must not be any
    // inflight request/results during stream configuration.
    Mutex::Autolock _l(mInflightLock);
    if (!mInflightBuffers.empty()) {
        ALOGE("%s: trying to configureStreams while there are still %zu inflight buffers!",
                __FUNCTION__, mInflightBuffers.size());
        _hidl_cb(Status::INTERNAL_ERROR, outStreams);
        return Void();
    }

    if (!mInflightAETriggerOverrides.empty()) {
        ALOGE("%s: trying to configureStreams while there are still %zu inflight"
                " trigger overrides!", __FUNCTION__,
                mInflightAETriggerOverrides.size());
        _hidl_cb(Status::INTERNAL_ERROR, outStreams);
        return Void();
    }

    if (!mInflightRawBoostPresent.empty()) {
        ALOGE("%s: trying to configureStreams while there are still %zu inflight"
                " boost overrides!", __FUNCTION__,
                mInflightRawBoostPresent.size());
        _hidl_cb(Status::INTERNAL_ERROR, outStreams);
        return Void();
    }

    if (status != Status::OK) {
        _hidl_cb(status, outStreams);
        return Void();
    }

    camera3_stream_configuration_t stream_list{};
    hidl_vec<camera3_stream_t*> streams;
    if (!preProcessConfigurationLocked(requestedConfiguration, &stream_list, &streams)) {
        _hidl_cb(Status::INTERNAL_ERROR, outStreams);
        return Void();
    }

    ATRACE_BEGIN("camera3->configure_streams");
    status_t ret = mDevice->ops->configure_streams(mDevice, &stream_list);
    ATRACE_END();

    // In case Hal returns error most likely it was not able to release
    // the corresponding resources of the deleted streams.
    if (ret == OK) {
        postProcessConfigurationLocked(requestedConfiguration);
    } else {
        postProcessConfigurationFailureLocked(requestedConfiguration);
    }

    if (ret == -EINVAL) {
        status = Status::ILLEGAL_ARGUMENT;
    } else if (ret != OK) {
        status = Status::INTERNAL_ERROR;
    } else {
        convertToHidl(stream_list, &outStreams);
        mFirstRequest = true;
    }

    _hidl_cb(status, outStreams);
    return Void();
}

} // namespace implementation
}  // namespace V3_3
}  // namespace device
}  // namespace camera
}  // namespace hardware
}  // namespace android
