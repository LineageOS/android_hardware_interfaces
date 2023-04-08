/*
 * Copyright 2021, The Android Open Source Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include <android-base/logging.h>
#include <android/binder_manager.h>
#include <android/binder_process.h>
#include <utils/StrongPointer.h>

#include "uwb.h"

using ::aidl::android::hardware::uwb::IUwb;
using ::android::sp;
using ::android::base::InitLogging;
using ::android::base::StderrLogger;
using ::android::hardware::uwb::impl::Uwb;

int main(int /*argc*/, char* argv[]) {
    InitLogging(argv, StderrLogger);
    LOG(INFO) << "UWB HAL starting up";

    ABinderProcess_setThreadPoolMaxThreadCount(0);
    std::shared_ptr<IUwb> uwb = ndk::SharedRefBase::make<Uwb>();
    const std::string instance = std::string() + IUwb::descriptor + "/default";
    binder_status_t status = AServiceManager_addService(uwb->asBinder().get(), instance.c_str());
    CHECK(status == STATUS_OK);

    ABinderProcess_joinThreadPool();
    return EXIT_FAILURE;  // should not reach
}
