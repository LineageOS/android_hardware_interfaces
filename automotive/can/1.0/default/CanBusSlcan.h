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

#pragma once

#include <linux/serial.h>
#include <linux/tty.h>
#include <net/if.h>
#include <termios.h>
#include "CanBus.h"

namespace android::hardware::automotive::can::V1_0::implementation {

struct CanBusSlcan : public CanBus {
    CanBusSlcan(const std::string& uartName, uint32_t bitrate);

  protected:
    virtual ICanController::Result preUp() override;
    virtual bool postDown() override;

  private:
    ICanController::Result updateIfaceName(base::unique_fd& uartFd);

    const std::string mUartName;
    const uint32_t kBitrate;
    base::unique_fd mFd;
};

}  // namespace android::hardware::automotive::can::V1_0::implementation
