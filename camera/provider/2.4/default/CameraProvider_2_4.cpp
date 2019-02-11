/*
 * Copyright 2019 The Android Open Source Project
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

#include "CameraProvider_2_4.h"
#include "LegacyCameraProviderImpl_2_4.h"
#include "ExternalCameraProviderImpl_2_4.h"

const char *kLegacyProviderName = "legacy/0";
const char *kExternalProviderName = "external/0";

namespace android {
namespace hardware {
namespace camera {
namespace provider {
namespace V2_4 {
namespace implementation {

using android::hardware::camera::provider::V2_4::ICameraProvider;

extern "C" ICameraProvider* HIDL_FETCH_ICameraProvider(const char* name);

template<typename IMPL>
CameraProvider<IMPL>* getProviderImpl() {
    CameraProvider<IMPL> *provider = new CameraProvider<IMPL>();
    if (provider == nullptr) {
        ALOGE("%s: cannot allocate camera provider!", __FUNCTION__);
        return nullptr;
    }
    if (provider->isInitFailed()) {
        ALOGE("%s: camera provider init failed!", __FUNCTION__);
        delete provider;
        return nullptr;
    }
    return provider;
}

ICameraProvider* HIDL_FETCH_ICameraProvider(const char* name) {
    using namespace android::hardware::camera::provider::V2_4::implementation;
    ICameraProvider* provider = nullptr;
    if (strcmp(name, kLegacyProviderName) == 0) {
        provider = getProviderImpl<LegacyCameraProviderImpl_2_4>();
    } else if (strcmp(name, kExternalProviderName) == 0) {
        provider = getProviderImpl<ExternalCameraProviderImpl_2_4>();
    } else {
        ALOGE("%s: unknown instance name: %s", __FUNCTION__, name);
    }

    return provider;
}

}  // namespace implementation
}  // namespace V2_4
}  // namespace provider
}  // namespace camera
}  // namespace hardware
}  // namespace android
