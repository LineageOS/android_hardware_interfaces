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
#include <netinet/in.h>
#include <net/if.h>
#include <utils/Log.h>
#include <cutils/properties.h>
#include <sys/stat.h>

#include "service.hpp"
#include "thread_chip.hpp"

using aidl::android::hardware::threadnetwork::IThreadChip;
using aidl::android::hardware::threadnetwork::ThreadChip;

#define THREADNETWORK_COPROCESSOR_SIMULATION_PATH "/apex/com.android.hardware.threadnetwork/bin/ot-rcp"

namespace {
void addThreadChip(int id, const char* url) {
    binder_status_t status;
    const std::string serviceName(std::string() + IThreadChip::descriptor + "/chip" +
            std::to_string(id));

    ALOGI("ServiceName: %s, Url: %s", serviceName.c_str(), url);

    auto threadChip = ndk::SharedRefBase::make<ThreadChip>(url);

    CHECK_NE(threadChip, nullptr);

    status = AServiceManager_addService(threadChip->asBinder().get(), serviceName.c_str());
    CHECK_EQ(status, STATUS_OK);
}

void addSimulatedThreadChip() {
    char local_interface[PROP_VALUE_MAX];

    CHECK_GT(property_get("persist.vendor.otsim.local_interface",
                local_interface, "eth1"), 0);

    int node_id = property_get_int32("ro.boot.openthread_node_id", 0);
    CHECK_GT(node_id,0);

    std::string url = std::string("spinel+hdlc+forkpty://" \
            THREADNETWORK_COPROCESSOR_SIMULATION_PATH "?forkpty-arg=-L") \
                      + local_interface + "&forkpty-arg=" + std::to_string(node_id);
    addThreadChip(0, url.c_str());
}
}

int main(int argc, char* argv[]) {
    aidl::android::hardware::threadnetwork::Service service;

    if (argc > 1) {
        for (int id = 0; id < argc - 1; id++) {
            addThreadChip(id, argv[id + 1]);
        }
    } else {
        struct stat sb;

        CHECK_EQ(stat(THREADNETWORK_COPROCESSOR_SIMULATION_PATH, &sb), 0);
        CHECK(sb.st_mode & S_IXUSR);
        addSimulatedThreadChip();
    }

    ALOGI("Thread Network HAL is running");

    service.startLoop();
    return EXIT_FAILURE;  // should not reach
}
