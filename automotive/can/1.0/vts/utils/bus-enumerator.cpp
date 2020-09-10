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

#include <android/hidl/manager/1.2/IServiceManager.h>
#include <can-vts-utils/bus-enumerator.h>
#include <hidl-utils/hidl-utils.h>

namespace android::hardware::automotive::can::V1_0::vts::utils {

hidl_vec<hidl_string> getBusNames() {
    auto manager = hidl::manager::V1_2::IServiceManager::getService();
    hidl_vec<hidl_string> services;
    manager->listManifestByInterface(ICanBus::descriptor, hidl_utils::fill(&services));
    return services;
}

}  // namespace android::hardware::automotive::can::V1_0::vts::utils
