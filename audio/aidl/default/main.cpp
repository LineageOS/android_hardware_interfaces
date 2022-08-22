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

#include "core-impl/Config.h"
#include "core-impl/Module.h"

#include <android-base/logging.h>
#include <android/binder_manager.h>
#include <android/binder_process.h>

using aidl::android::hardware::audio::core::Config;
using aidl::android::hardware::audio::core::Module;

int main() {
    ABinderProcess_setThreadPoolMaxThreadCount(16);

    // make the default config service
    auto config = ndk::SharedRefBase::make<Config>();
    const std::string configName = std::string() + Config::descriptor + "/default";
    binder_status_t status =
            AServiceManager_addService(config->asBinder().get(), configName.c_str());
    CHECK(status == STATUS_OK);

    // make the default module
    auto moduleDefault = ndk::SharedRefBase::make<Module>();
    const std::string moduleDefaultName = std::string() + Module::descriptor + "/default";
    status = AServiceManager_addService(moduleDefault->asBinder().get(), moduleDefaultName.c_str());
    CHECK(status == STATUS_OK);

    ABinderProcess_joinThreadPool();
    return EXIT_FAILURE;  // should not reach
}
