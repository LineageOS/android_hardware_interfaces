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

#include "FakeFingerprintEngineUdfps.h"

#include <android-base/logging.h>

#include <fingerprint.sysprop.h>

#include "util/CancellationSignal.h"
#include "util/Util.h"

using namespace ::android::fingerprint::virt;

namespace aidl::android::hardware::biometrics::fingerprint {

SensorLocation FakeFingerprintEngineUdfps::defaultSensorLocation() {
    return {0 /* displayId (not used) */, defaultSensorLocationX /* sensorLocationX */,
            defaultSensorLocationY /* sensorLocationY */, defaultSensorRadius /* sensorRadius */,
            "" /* display */};
}

ndk::ScopedAStatus FakeFingerprintEngineUdfps::onPointerDownImpl(int32_t /*pointerId*/,
                                                                 int32_t /*x*/, int32_t /*y*/,
                                                                 float /*minor*/, float /*major*/) {
    BEGIN_OP(0);

    // TODO(b/230515082): if need to handle display touch events

    return ndk::ScopedAStatus::ok();
}

ndk::ScopedAStatus FakeFingerprintEngineUdfps::onPointerUpImpl(int32_t /*pointerId*/) {
    BEGIN_OP(0);
    // TODO(b/230515082)
    return ndk::ScopedAStatus::ok();
}

ndk::ScopedAStatus FakeFingerprintEngineUdfps::onUiReadyImpl() {
    BEGIN_OP(0);
    // TODO(b/230515082)
    return ndk::ScopedAStatus::ok();
}

}  // namespace aidl::android::hardware::biometrics::fingerprint
