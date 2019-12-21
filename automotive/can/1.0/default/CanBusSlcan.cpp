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

#include "CanBusSlcan.h"

#include <android-base/logging.h>
#include <libnetdevice/can.h>
#include <libnetdevice/libnetdevice.h>

#include <net/if.h>
#include <sys/ioctl.h>
#include <sys/stat.h>
#include <termios.h>

namespace android::hardware::automotive::can::V1_0::implementation {

namespace slcanprotocol {
static const std::string kOpenCommand = "O\r";
static const std::string kCloseCommand = "C\r";
static constexpr int kSlcanDiscipline = N_SLCAN;
static constexpr int kDefaultDiscipline = N_TTY;

static const std::map<uint32_t, std::string> kBitrateCommands = {
        {10000, "C\rS0\r"},  {20000, "C\rS1\r"},  {50000, "C\rS2\r"},
        {100000, "C\rS3\r"}, {125000, "C\rS4\r"}, {250000, "C\rS5\r"},
        {500000, "C\rS6\r"}, {800000, "C\rS7\r"}, {1000000, "C\rS8\r"}};
}  // namespace slcanprotocol

/**
 * Serial Line CAN constructor
 * \param string uartName - name of slcan device (e.x. /dev/ttyUSB0)
 * \param uint32_t bitrate - speed of the CAN bus (125k = MSCAN, 500k = HSCAN)
 */
CanBusSlcan::CanBusSlcan(const std::string& uartName, uint32_t bitrate)
    : CanBus(), mUartName(uartName), kBitrate(bitrate) {}

ICanController::Result CanBusSlcan::preUp() {
    // verify valid bitrate and translate to serial command format
    const auto lookupIt = slcanprotocol::kBitrateCommands.find(kBitrate);
    if (lookupIt == slcanprotocol::kBitrateCommands.end()) {
        return ICanController::Result::BAD_BAUDRATE;
    }
    const auto canBitrateCommand = lookupIt->second;

    /* Attempt to open the uart in r/w without blocking or becoming the
     * controlling terminal */
    mFd = base::unique_fd(open(mUartName.c_str(), O_RDWR | O_NONBLOCK | O_NOCTTY));
    if (!mFd.ok()) {
        LOG(ERROR) << "SLCAN Failed to open " << mUartName << ": " << strerror(errno);
        return ICanController::Result::BAD_ADDRESS;
    }

    // blank terminal settings and pull them from the device
    struct termios terminalSettings = {};
    if (tcgetattr(mFd.get(), &terminalSettings) < 0) {
        LOG(ERROR) << "Failed to read attrs of" << mUartName << ": " << strerror(errno);
        return ICanController::Result::UNKNOWN_ERROR;
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
        LOG(ERROR) << "Failed to read serial settings from " << mUartName << ": "
                   << strerror(errno);
        return ICanController::Result::UNKNOWN_ERROR;
    }
    // set low latency mode
    serialSettings.flags |= ASYNC_LOW_LATENCY;
    // apply serial settings
    if (ioctl(mFd.get(), TIOCSSERIAL, &serialSettings) < 0) {
        LOG(ERROR) << "Failed to set low latency mode on " << mUartName << ": " << strerror(errno);
        return ICanController::Result::UNKNOWN_ERROR;
    }

    /* TCSADRAIN applies settings after we finish writing the rest of our
     * changes (as opposed to TCSANOW, which changes immediately) */
    if (tcsetattr(mFd.get(), TCSADRAIN, &terminalSettings) < 0) {
        LOG(ERROR) << "Failed to apply terminal settings to " << mUartName << ": "
                   << strerror(errno);
        return ICanController::Result::UNKNOWN_ERROR;
    }

    // apply speed setting for CAN
    if (write(mFd.get(), canBitrateCommand.c_str(), canBitrateCommand.length()) <= 0) {
        LOG(ERROR) << "Failed to apply CAN bitrate: " << strerror(errno);
        return ICanController::Result::UNKNOWN_ERROR;
    }

    // set open flag TODO: also support listen only
    if (write(mFd.get(), slcanprotocol::kOpenCommand.c_str(),
              slcanprotocol::kOpenCommand.length()) <= 0) {
        LOG(ERROR) << "Failed to set open flag: " << strerror(errno);
        return ICanController::Result::UNKNOWN_ERROR;
    }

    // set line discipline to slcan
    if (ioctl(mFd.get(), TIOCSETD, &slcanprotocol::kSlcanDiscipline) < 0) {
        LOG(ERROR) << "Failed to set line discipline to slcan: " << strerror(errno);
        return ICanController::Result::UNKNOWN_ERROR;
    }

    // get the name of the device we created
    struct ifreq ifrequest = {};
    if (ioctl(mFd.get(), SIOCGIFNAME, ifrequest.ifr_name) < 0) {
        LOG(ERROR) << "Failed to get the name of the created device: " << strerror(errno);
        return ICanController::Result::UNKNOWN_ERROR;
    }

    // Update the CanBus object with name that was assigned to it
    mIfname = ifrequest.ifr_name;

    return ICanController::Result::OK;
}

bool CanBusSlcan::postDown() {
    // reset the line discipline to TTY mode
    if (ioctl(mFd.get(), TIOCSETD, &slcanprotocol::kDefaultDiscipline) < 0) {
        LOG(ERROR) << "Failed to reset line discipline!";
        return false;
    }

    // issue the close command
    if (write(mFd.get(), slcanprotocol::kCloseCommand.c_str(),
              slcanprotocol::kCloseCommand.length()) <= 0) {
        LOG(ERROR) << "Failed to close tty!";
        return false;
    }

    // close our unique_fd
    mFd.reset();

    return true;
}

}  // namespace android::hardware::automotive::can::V1_0::implementation
