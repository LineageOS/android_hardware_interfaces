/*
 * Copyright (C) 2025 The Android Open Source Project
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

#include "FakeFingerprintEngineUdfpsUltraSonic.h"

#include <android-base/logging.h>

#include <fingerprint.sysprop.h>

#include "Fingerprint.h"
#include "util/CancellationSignal.h"
#include "util/Util.h"

#undef LOG_TAG
#define LOG_TAG "FingerprintVirtualHalUdfpsUltraSonic"

using namespace ::android::fingerprint::virt;

namespace aidl::android::hardware::biometrics::fingerprint {

FakeFingerprintEngineUdfpsUltraSonic::FakeFingerprintEngineUdfpsUltraSonic()
    : FakeFingerprintEngineUdfps() {}

ndk::ScopedAStatus FakeFingerprintEngineUdfpsUltraSonic::onPointerDownImpl(
        int32_t /*pointerId*/, int32_t /*x*/, int32_t /*y*/, float /*minor*/, float /*major*/) {
    BEGIN_OP(0);

    // UltraSonic sensor does not need illumination, so fingerprint capture can start on
    //   PointerDown action rather UiReady action which is required for optical sensor
    fingerDownAction();

    return ndk::ScopedAStatus::ok();
}

}  // namespace aidl::android::hardware::biometrics::fingerprint
