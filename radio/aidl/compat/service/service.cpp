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
#include <libradiocompat/CallbackManager.h>
#include <libradiocompat/RadioConfig.h>
#include <libradiocompat/RadioData.h>
#include <libradiocompat/RadioMessaging.h>
#include <libradiocompat/RadioModem.h>
#include <libradiocompat/RadioNetwork.h>
#include <libradiocompat/RadioSim.h>
#include <libradiocompat/RadioVoice.h>

namespace android::hardware::radio::service {

using namespace std::string_literals;

static std::vector<std::shared_ptr<ndk::ICInterface>> gPublishedHals;

template <typename T>
static void publishRadioHal(std::shared_ptr<compat::DriverContext> ctx, sp<V1_5::IRadio> hidlHal,
                            std::shared_ptr<compat::CallbackManager> cm, const std::string& slot) {
    const auto instance = T::descriptor + "/"s + slot;
    if (!AServiceManager_isDeclared(instance.c_str())) {
        LOG(INFO) << instance << " is not declared in VINTF (this may be intentional)";
        return;
    }
    LOG(DEBUG) << "Publishing " << instance;

    auto aidlHal = ndk::SharedRefBase::make<T>(ctx, hidlHal, cm);
    gPublishedHals.push_back(aidlHal);
    const auto status = AServiceManager_addService(aidlHal->asBinder().get(), instance.c_str());
    CHECK_EQ(status, STATUS_OK);
}

static void publishRadio(std::string slot) {
    auto radioHidl = V1_5::IRadio::getService(slot);
    CHECK(radioHidl) << "HIDL IRadio not present in VINTF";

    hidl_utils::linkDeathToDeath(radioHidl);

    auto context = std::make_shared<compat::DriverContext>();
    auto callbackMgr = std::make_shared<compat::CallbackManager>(context, radioHidl);

    publishRadioHal<compat::RadioData>(context, radioHidl, callbackMgr, slot);
    publishRadioHal<compat::RadioMessaging>(context, radioHidl, callbackMgr, slot);
    publishRadioHal<compat::RadioModem>(context, radioHidl, callbackMgr, slot);
    publishRadioHal<compat::RadioNetwork>(context, radioHidl, callbackMgr, slot);
    publishRadioHal<compat::RadioSim>(context, radioHidl, callbackMgr, slot);
    publishRadioHal<compat::RadioVoice>(context, radioHidl, callbackMgr, slot);
}

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
    base::InitLogging(nullptr, base::LogdLogger(base::RADIO));
    base::SetDefaultTag("radiocompat");
    base::SetMinimumLogSeverity(base::VERBOSE);
    LOG(DEBUG) << "Radio HAL compat service starting...";

    publishRadioConfig();

    const auto slots = hidl_utils::listManifestByInterface(V1_0::IRadio::descriptor);
    LOG(INFO) << "Found " << slots.size() << " slot(s)";
    for (const auto& slot : slots) {
        publishRadio(slot);
    }

    LOG(DEBUG) << "Radio HAL compat service is operational";
    ABinderProcess_joinThreadPool();
    LOG(FATAL) << "Radio HAL compat service has stopped";
}

}  // namespace android::hardware::radio::service

int main() {
    android::hardware::radio::service::main();
    return EXIT_FAILURE;  // should not reach
}
