/*
 * Copyright (C) 2019 The Android Open Source Project
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

#define LOG_TAG "android.hardware.tv.tuner@1.0-Lnb"

#include "Lnb.h"
#include <utils/Log.h>

namespace android {
namespace hardware {
namespace tv {
namespace tuner {
namespace V1_0 {
namespace implementation {

Lnb::Lnb() {}
Lnb::Lnb(int id) {
    mId = id;
}

Lnb::~Lnb() {}

Return<Result> Lnb::setCallback(const sp<ILnbCallback>& /* callback */) {
    ALOGV("%s", __FUNCTION__);

    return Result::SUCCESS;
}

Return<Result> Lnb::setVoltage(LnbVoltage /* voltage */) {
    ALOGV("%s", __FUNCTION__);

    return Result::SUCCESS;
}

Return<Result> Lnb::setTone(LnbTone /* tone */) {
    ALOGV("%s", __FUNCTION__);

    return Result::SUCCESS;
}

Return<Result> Lnb::setSatellitePosition(LnbPosition /* position */) {
    ALOGV("%s", __FUNCTION__);

    return Result::SUCCESS;
}

Return<Result> Lnb::sendDiseqcMessage(const hidl_vec<uint8_t>& /* diseqcMessage */) {
    ALOGV("%s", __FUNCTION__);

    return Result::SUCCESS;
}

Return<Result> Lnb::close() {
    ALOGV("%s", __FUNCTION__);

    return Result::SUCCESS;
}

int Lnb::getId() {
    return mId;
}

}  // namespace implementation
}  // namespace V1_0
}  // namespace tuner
}  // namespace tv
}  // namespace hardware
}  // namespace android
