/*
 * Copyright (C) 2020 The Android Open Source Project
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

#include "VehicleHalClient.h"

#include <android-base/logging.h>

namespace android::hardware::automotive::vehicle::V2_0::impl {

void VehicleHalClient::onPropertyValue(const VehiclePropValue& value, bool updateStatus) {
    if (!mPropCallback) {
        LOG(ERROR) << __func__ << ": PropertyCallBackType is not registered!";
        return;
    }
    return mPropCallback(value, updateStatus);
}

void VehicleHalClient::registerPropertyValueCallback(PropertyCallBackType&& callback) {
    if (mPropCallback) {
        LOG(ERROR) << __func__ << ": Cannot register multiple callbacks!";
        return;
    }
    mPropCallback = std::move(callback);
}

}  // namespace android::hardware::automotive::vehicle::V2_0::impl
