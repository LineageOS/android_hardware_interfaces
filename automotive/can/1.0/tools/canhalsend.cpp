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
#include <android-base/parseint.h>
#include <android-base/strings.h>
#include <android/hardware/automotive/can/1.0/ICanBus.h>
#include <android/hidl/manager/1.2/IServiceManager.h>

#include <iostream>
#include <string>

namespace android::hardware::automotive::can {

using ICanBus = V1_0::ICanBus;
using Result = V1_0::Result;

static void usage() {
    std::cerr << "canhalsend - simple command line tool to send raw CAN frames" << std::endl;
    std::cerr << std::endl << "usage:" << std::endl << std::endl;
    std::cerr << "canhalsend <bus name> <can id>#<data>" << std::endl;
    std::cerr << "where:" << std::endl;
    std::cerr << " bus name - name under which ICanBus is published" << std::endl;
    std::cerr << " can id - such as 1a5 or 1fab5982" << std::endl;
    std::cerr << " data - such as deadbeef, 010203, or R for a remote frame" << std::endl;
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

static int cansend(const std::string& busname, const V1_0::CanMessage& msg) {
    auto bus = tryOpen(busname);
    if (bus == nullptr) {
        std::cerr << "Bus " << busname << " is not available" << std::endl;
        return -1;
    }

    const auto result = bus->send(msg);
    if (result != Result::OK) {
        std::cerr << "Send call failed: " << toString(result) << std::endl;
        return -1;
    }
    return 0;
}

static std::optional<V1_0::CanMessage> parseCanMessage(const std::string& msg) {
    const auto hashpos = msg.find("#");
    if (hashpos == std::string::npos) return std::nullopt;

    const std::string msgidStr = msg.substr(0, hashpos);
    const std::string payloadStr = msg.substr(hashpos + 1);

    V1_0::CanMessageId msgid;
    // "0x" must be prepended to msgidStr, since ParseUint doesn't accept a base argument.
    if (!android::base::ParseUint("0x" + msgidStr, &msgid)) return std::nullopt;

    V1_0::CanMessage canmsg = {};
    canmsg.id = msgid;
    if (msgid > 0x7FF) {
        canmsg.isExtendedId = true;
    }

    if (android::base::StartsWith(payloadStr, "R")) {
        canmsg.remoteTransmissionRequest = true;

        /* The CAN bus HAL doesn't define a data length code (DLC) field, since it is inferrred
         * from the payload size. RTR messages indicate to the receiver how many bytes they are
         * expecting to receive back via the DLC sent with the RTR frame. */
        if (payloadStr.size() <= 1) return canmsg;

        unsigned int dlc = 0;

        /* The maximum DLC for CAN-FD is 64 bytes and CAN 2.0 is 8 bytes. Limit the size of the DLC
         * to something memory safe and let the HAL determine if the DLC is valid. */
        if (!android::base::ParseUint(payloadStr.substr(1), &dlc, 10000u)) {
            std::cerr << "Invalid DLC for RTR frame!" << std::endl;
            return std::nullopt;
        }
        canmsg.payload.resize(dlc);
        return canmsg;
    }

    std::vector<uint8_t> payload;
    if (payloadStr.size() % 2 != 0) return std::nullopt;
    for (size_t i = 0; i < payloadStr.size(); i += 2) {
        std::string byteStr(payloadStr, i, 2);
        uint8_t byteBuf;
        if (!android::base::ParseUint("0x" + byteStr, &byteBuf)) return std::nullopt;
        payload.emplace_back(byteBuf);
    }
    canmsg.payload = payload;

    return canmsg;
}

static int main(int argc, char* argv[]) {
    base::SetDefaultTag("CanHalSend");
    base::SetMinimumLogSeverity(android::base::VERBOSE);

    if (argc == 0) {
        usage();
        return 0;
    }

    if (argc != 2) {
        std::cerr << "Invalid number of arguments" << std::endl;
        usage();
        return -1;
    }

    std::string busname(argv[0]);
    const auto canmsg = parseCanMessage(argv[1]);
    if (!canmsg) {
        std::cerr << "Failed to parse CAN message argument" << std::endl;
        return -1;
    }

    return cansend(busname, *canmsg);
}

}  // namespace android::hardware::automotive::can

int main(int argc, char* argv[]) {
    if (argc < 1) return -1;
    return ::android::hardware::automotive::can::main(--argc, ++argv);
}
