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

#include <libradiocompat/RadioResponse.h>

#include "debug.h"

#define RADIO_MODULE "Common"

namespace android::hardware::radio::compat {

RadioResponse::RadioResponse(std::shared_ptr<DriverContext> context) : mContext(context) {}

Return<void> RadioResponse::acknowledgeRequest(int32_t serial) {
    LOG_CALL << serial;
    // TODO(b/203699028): send to correct requestor or confirm if spam is not a problem
    if (mDataCb) mDataCb.get()->acknowledgeRequest(serial);
    if (mMessagingCb) mMessagingCb.get()->acknowledgeRequest(serial);
    if (mModemCb) mModemCb.get()->acknowledgeRequest(serial);
    if (mNetworkCb) mNetworkCb.get()->acknowledgeRequest(serial);
    if (mSimCb) mSimCb.get()->acknowledgeRequest(serial);
    if (mVoiceCb) mVoiceCb.get()->acknowledgeRequest(serial);
    return {};
}

}  // namespace android::hardware::radio::compat
