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

#include <libradiocompat/RadioResponse.h>

#include "commonStructs.h"
#include "debug.h"

#include "collections.h"

#define RADIO_MODULE "ImsResponse"

namespace android::hardware::radio::compat {

namespace aidl = ::aidl::android::hardware::radio::ims;

void RadioResponse::setResponseFunction(std::shared_ptr<aidl::IRadioImsResponse> imsCb) {
    mImsCb = imsCb;
}

std::shared_ptr<aidl::IRadioImsResponse> RadioResponse::imsCb() {
    return mImsCb.get();
}

}  // namespace android::hardware::radio::compat
