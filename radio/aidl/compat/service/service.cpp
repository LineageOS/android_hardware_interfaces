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

#include "hidl-utils.h"

#include <android-base/logging.h>
#include <android/binder_manager.h>
#include <android/binder_process.h>
#include <libradiocompat/RadioConfig.h>

namespace android::hardware::radio::service {

using namespace std::string_literals;

static std::vector<std::shared_ptr<ndk::ICInterface>> gPublishedHals;

static void publishRadioConfig() {
    auto hidlHal = config::V1_1::IRadioConfig::getService();
    CHECK(hidlHal) << "HIDL IRadioConfig not present in VINTF";

    hidl_utils::linkDeathToDeath(hidlHal);

    auto aidlHal = ndk::SharedRefBase::make<compat::RadioConfig>(hidlHal);
    gPublishedHals.push_back(aidlHal);
    const auto instance = compat::RadioConfig::descriptor + "/default"s;
    const auto status = AServiceManager_addService(aidlHal->asBinder().get(), instance.c_str());
    CHECK_EQ(status, STATUS_OK);
}

static void main() {
    base::SetDefaultTag("radiocompat");
    base::SetMinimumLogSeverity(base::VERBOSE);
    LOG(DEBUG) << "Radio HAL compat service starting...";

    publishRadioConfig();

    LOG(DEBUG) << "Radio HAL compat service is operational";
    ABinderProcess_joinThreadPool();
    LOG(FATAL) << "Radio HAL compat service has stopped";
}

}  // namespace android::hardware::radio::service

int main() {
    android::hardware::radio::service::main();
    return EXIT_FAILURE;  // should not reach
}
