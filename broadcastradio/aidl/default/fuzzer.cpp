/*
 * Copyright (C) 2023 The Android Open Source Project
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

#include <fuzzbinder/libbinder_ndk_driver.h>
#include <fuzzer/FuzzedDataProvider.h>
#include "BroadcastRadio.h"
#include "VirtualRadio.h"

using ::aidl::android::hardware::broadcastradio::BroadcastRadio;
using ::aidl::android::hardware::broadcastradio::VirtualRadio;
using ::android::fuzzService;

extern "C" int LLVMFuzzerTestOneInput(const uint8_t* data, size_t size) {
    const VirtualRadio& amFmRadioMock = VirtualRadio::getAmFmRadio();
    std::shared_ptr<BroadcastRadio> amFmRadio =
            ::ndk::SharedRefBase::make<BroadcastRadio>(amFmRadioMock);
    const VirtualRadio& dabRadioMock = VirtualRadio::getDabRadio();
    std::shared_ptr<BroadcastRadio> dabRadio =
            ::ndk::SharedRefBase::make<BroadcastRadio>(dabRadioMock);

    std::vector<ndk::SpAIBinder> binder_services = {amFmRadio->asBinder(), dabRadio->asBinder()};

    fuzzService(binder_services, FuzzedDataProvider(data, size));

    return 0;
}
