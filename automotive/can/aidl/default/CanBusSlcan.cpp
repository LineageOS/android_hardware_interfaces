/*
 * Copyright 2022, The Android Open Source Project
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

#include "CanBusSlcan.h"

#include <android-base/file.h>
#include <android-base/logging.h>
#include <libnetdevice/libnetdevice.h>

#include <linux/serial.h>
#include <linux/tty.h>
#include <net/if.h>
#include <termios.h>

#include <map>

namespace aidl::android::hardware::automotive::can {

using namespace std::string_view_literals;
using namespace ::android::base;

namespace slcanprotocol {
static constexpr std::string_view kOpenCommand = "O\r"sv;
static constexpr std::string_view kCloseCommand = "C\r"sv;
static constexpr int kSlcanDiscipline = N_SLCAN;
static constexpr int kDefaultDiscipline = N_TTY;

static const std::map<uint32_t, std::string_view> kBitrateCommands = {
        {10000, "C\rS0\r"sv},  {20000, "C\rS1\r"sv},  {50000, "C\rS2\r"sv},
        {100000, "C\rS3\r"sv}, {125000, "C\rS4\r"sv}, {250000, "C\rS5\r"sv},
        {500000, "C\rS6\r"sv}, {800000, "C\rS7\r"sv}, {1000000, "C\rS8\r"sv}};
}  // namespace slcanprotocol

/**
 * Serial Line CAN constructor
 * \param string uartName - name of slcan device (e.x. /dev/ttyUSB0)
 * \param uint32_t bitrate - speed of the CAN bus (125k = MSCAN, 500k = HSCAN)
 */
CanBusSlcan::CanBusSlcan(const std::string& uartName, uint32_t bitrate)
    : CanBus(), mTtyPath(uartName), kBitrate(bitrate) {}

/** helper function to update CanBusSlcan object's iface name */
Result CanBusSlcan::updateIfaceName(unique_fd& uartFd) {
    struct ifreq ifrequest = {};
    /*
     * Fetching the iface name with an ioctl won't interfere with an open socketCAN iface attached
     * to this tty. This is important in the event we are trying to register a SLCAN based iface
     * that has already been configured and brought up.
     */
    if (ioctl(uartFd.get(), SIOCGIFNAME, ifrequest.ifr_name) < 0) {
        PLOG(ERROR) << "Failed to get the name of the created device";
        return Result::UNKNOWN_ERROR;
    }

    // Update the CanBus object with name that was assigned to it
    mIfname = ifrequest.ifr_name;
    return Result::OK;
}

Result CanBusSlcan::preUp() {
    // verify valid bitrate and translate to serial command format
    std::optional<std::string_view> canBitrateCommand = std::nullopt;
    if (kBitrate != 0) {
        const auto lookupIt = slcanprotocol::kBitrateCommands.find(kBitrate);
        if (lookupIt == slcanprotocol::kBitrateCommands.end()) {
            return Result::BAD_BITRATE;
        }
        canBitrateCommand = lookupIt->second;
    }

    /* Attempt to open the uart in r/w without blocking or becoming the
     * controlling terminal */
    mFd = unique_fd(open(mTtyPath.c_str(), O_RDWR | O_NONBLOCK | O_NOCTTY | O_CLOEXEC));
    if (!mFd.ok()) {
        PLOG(ERROR) << "SLCAN Failed to open " << mTtyPath;
        return Result::BAD_INTERFACE_ID;
    }

    // If the device is already up, update the iface name in our CanBusSlcan object
    if (kBitrate == 0) {
        return updateIfaceName(mFd);
    }

    // blank terminal settings and pull them from the device
    struct termios terminalSettings = {};
    if (tcgetattr(mFd.get(), &terminalSettings) < 0) {
        PLOG(ERROR) << "Failed to read attrs of" << mTtyPath;
        return Result::UNKNOWN_ERROR;
    }

    // change settings to raw mode
    cfmakeraw(&terminalSettings);

    // disable software flow control
    terminalSettings.c_iflag &= ~IXOFF;
    // enable hardware flow control
    terminalSettings.c_cflag |= CRTSCTS;

    struct serial_struct serialSettings;
    // get serial settings
    if (ioctl(mFd.get(), TIOCGSERIAL, &serialSettings) < 0) {
        PLOG(ERROR) << "Failed to read serial settings from " << mTtyPath;
        return Result::UNKNOWN_ERROR;
    }
    // set low latency mode
    serialSettings.flags |= ASYNC_LOW_LATENCY;
    // apply serial settings
    if (ioctl(mFd.get(), TIOCSSERIAL, &serialSettings) < 0) {
        PLOG(ERROR) << "Failed to set low latency mode on " << mTtyPath;
        return Result::UNKNOWN_ERROR;
    }

    /* TCSADRAIN applies settings after we finish writing the rest of our
     * changes (as opposed to TCSANOW, which changes immediately) */
    if (tcsetattr(mFd.get(), TCSADRAIN, &terminalSettings) < 0) {
        PLOG(ERROR) << "Failed to apply terminal settings to " << mTtyPath;
        return Result::UNKNOWN_ERROR;
    }

    // apply speed setting for CAN
    if (!WriteStringToFd(*canBitrateCommand, mFd)) {
        PLOG(ERROR) << "Failed to apply CAN bitrate";
        return Result::UNKNOWN_ERROR;
    }

    // TODO(b/144775286): set open flag & support listen only
    if (!WriteStringToFd(slcanprotocol::kOpenCommand, mFd)) {
        PLOG(ERROR) << "Failed to set open flag";
        return Result::UNKNOWN_ERROR;
    }

    // set line discipline to slcan
    if (ioctl(mFd.get(), TIOCSETD, &slcanprotocol::kSlcanDiscipline) < 0) {
        PLOG(ERROR) << "Failed to set line discipline to slcan";
        return Result::UNKNOWN_ERROR;
    }

    // Update the CanBus object with name that was assigned to it
    return updateIfaceName(mFd);
}

bool CanBusSlcan::postDown() {
    // reset the line discipline to TTY mode
    if (ioctl(mFd.get(), TIOCSETD, &slcanprotocol::kDefaultDiscipline) < 0) {
        LOG(ERROR) << "Failed to reset line discipline!";
        return false;
    }

    // issue the close command
    if (!WriteStringToFd(slcanprotocol::kCloseCommand, mFd)) {
        LOG(ERROR) << "Failed to close tty!";
        return false;
    }

    // close our unique_fd
    mFd.reset();

    return true;
}

}  // namespace aidl::android::hardware::automotive::can
