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

#include <aidl/android/hardware/threadnetwork/IThreadChip.h>
#include <android-base/logging.h>
#include <android/binder_manager.h>
#include <android/binder_process.h>
#include <utils/Log.h>

#include "service.hpp"
#include "thread_chip.hpp"

using aidl::android::hardware::threadnetwork::IThreadChip;
using aidl::android::hardware::threadnetwork::ThreadChip;

int main(int argc, char* argv[]) {
    CHECK_GT(argc, 1);
    std::vector<std::shared_ptr<ThreadChip>> threadChips;
    aidl::android::hardware::threadnetwork::Service service;

    for (int id = 0; id < argc - 1; id++) {
        binder_status_t status;
        const std::string serviceName(std::string() + IThreadChip::descriptor + "/chip" +
                                      std::to_string(id));
        auto threadChip = ndk::SharedRefBase::make<ThreadChip>(argv[id + 1]);

        CHECK_NE(threadChip, nullptr);

        status = AServiceManager_addService(threadChip->asBinder().get(), serviceName.c_str());
        CHECK_EQ(status, STATUS_OK);

        ALOGI("ServiceName: %s, Url: %s", serviceName.c_str(), argv[id + 1]);
        threadChips.push_back(std::move(threadChip));
    }

    ALOGI("Thread Network HAL is running");

    service.startLoop();
    return EXIT_FAILURE;  // should not reach
}
