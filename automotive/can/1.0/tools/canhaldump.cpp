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

#include <android-base/logging.h>
#include <android/hardware/automotive/can/1.0/ICanBus.h>
#include <android/hardware/automotive/can/1.0/ICanMessageListener.h>
#include <android/hidl/manager/1.2/IServiceManager.h>
#include <hidl-utils/hidl-utils.h>

#include <linux/can.h>
#include <chrono>
#include <iomanip>
#include <iostream>
#include <string>
#include <thread>

namespace android::hardware::automotive::can {

using namespace std::chrono_literals;

using ICanBus = V1_0::ICanBus;
using Result = V1_0::Result;

struct CanMessageListener : public V1_0::ICanMessageListener {
    const std::string name;

    CanMessageListener(std::string name) : name(name) {}

    virtual Return<void> onReceive(const V1_0::CanMessage& message) {
        int msgIdWidth = 3;
        if (message.isExtendedId) msgIdWidth = 8;
        std::cout << "  " << name << "  " << std::hex << std::uppercase << std::setw(msgIdWidth)
                  << std::setfill('0') << message.id << std::setw(0);
        std::cout << "   [" << message.payload.size() << "] ";
        if (message.remoteTransmissionRequest) {
            std::cout << "remote request";
        } else {
            for (const auto byte : message.payload) {
                std::cout << " " << std::setfill('0') << std::setw(2) << unsigned(byte);
            }
        }
        std::cout << std::nouppercase << std::dec << std::endl;
        return {};
    }

    virtual Return<void> onError(V1_0::ErrorEvent error) {
        std::cout << "  " << name << "  " << toString(error) << std::endl;
        return {};
    }
};

static void usage() {
    std::cerr << "canhaldump - dump CAN bus traffic" << std::endl;
    std::cerr << std::endl << "usage:" << std::endl << std::endl;
    std::cerr << "canhaldump <bus name>" << std::endl;
    std::cerr << "where:" << std::endl;
    std::cerr << " bus name - name under which ICanBus is be published" << std::endl;
}

// TODO(b/135918744): extract to a new library
static sp<ICanBus> tryOpen(const std::string& busname) {
    auto bus = ICanBus::tryGetService(busname);
    if (bus != nullptr) return bus;

    /* Fallback for interfaces not registered in manifest. For testing purposes only,
     * one should not depend on this in production deployment. */
    auto manager = hidl::manager::V1_2::IServiceManager::getService();
    auto ret = manager->get(ICanBus::descriptor, busname).withDefault(nullptr);
    if (ret == nullptr) return nullptr;

    std::cerr << "WARNING: bus " << busname << " is not registered in device manifest, "
              << "trying to fetch it directly..." << std::endl;

    return ICanBus::castFrom(ret);
}

static int candump(const std::string& busname) {
    auto bus = tryOpen(busname);
    if (bus == nullptr) {
        std::cerr << "Bus " << busname << " is not available" << std::endl;
        return -1;
    }

    Result result;
    sp<V1_0::ICloseHandle> chnd;
    // TODO(b/135918744): extract to library
    bus->listen({}, new CanMessageListener(busname), hidl_utils::fill(&result, &chnd)).assertOk();

    if (result != Result::OK) {
        std::cerr << "Listen call failed: " << toString(result) << std::endl;
        return -1;
    }

    while (true) std::this_thread::sleep_for(1h);
}

static int main(int argc, char* argv[]) {
    base::SetDefaultTag("CanHalDump");
    base::SetMinimumLogSeverity(android::base::VERBOSE);

    if (argc == 0) {
        usage();
        return 0;
    }

    if (argc != 1) {
        std::cerr << "Invalid number of arguments" << std::endl;
        usage();
        return -1;
    }

    return candump(argv[0]);
}

}  // namespace android::hardware::automotive::can

int main(int argc, char* argv[]) {
    if (argc < 1) return -1;
    return ::android::hardware::automotive::can::main(--argc, ++argv);
}
