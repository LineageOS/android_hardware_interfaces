/*
 * Copyright (C) 2020 The Android Open Source Project
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

#define LOG_TAG "android.hardware.usb.gadget.aidl-service"

#include "UsbGadget.h"
#include <dirent.h>
#include <fcntl.h>
#include <stdio.h>
#include <sys/inotify.h>
#include <sys/mount.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

#include <aidl/android/frameworks/stats/IStats.h>

namespace aidl {
namespace android {
namespace hardware {
namespace usb {
namespace gadget {

string enabledPath;
constexpr char kHsi2cPath[] = "/sys/devices/platform/10d50000.hsi2c";
constexpr char kI2CPath[] = "/sys/devices/platform/10d50000.hsi2c/i2c-";
constexpr char kAccessoryLimitCurrent[] = "i2c-max77759tcpc/usb_limit_accessory_current";
constexpr char kAccessoryLimitCurrentEnable[] = "i2c-max77759tcpc/usb_limit_accessory_enable";

UsbGadget::UsbGadget() : mGadgetIrqPath("") {
}

Status UsbGadget::getUsbGadgetIrqPath() {
    std::string irqs;
    size_t read_pos = 0;
    size_t found_pos = 0;

    if (!ReadFileToString(kProcInterruptsPath, &irqs)) {
        ALOGE("cannot read all interrupts");
        return Status::ERROR;
    }

    while (true) {
        found_pos = irqs.find_first_of("\n", read_pos);
        if (found_pos == std::string::npos) {
            ALOGI("the string of all interrupts is unexpected");
            return Status::ERROR;
        }

        std::string single_irq = irqs.substr(read_pos, found_pos - read_pos);

        if (single_irq.find("dwc3", 0) != std::string::npos) {
            unsigned int dwc3_irq_number;
            size_t dwc3_pos = single_irq.find_first_of(":");
            if (!ParseUint(single_irq.substr(0, dwc3_pos), &dwc3_irq_number)) {
                ALOGI("unknown IRQ strings");
                return Status::ERROR;
            }

            mGadgetIrqPath = kProcIrqPath + single_irq.substr(0, dwc3_pos) + kSmpAffinityList;
            break;
        }

        if (found_pos == irqs.npos) {
            ALOGI("USB gadget doesn't start");
            return Status::ERROR;
        }

        read_pos = found_pos + 1;
    }

    return Status::SUCCESS;
}

void currentFunctionsAppliedCallback(bool functionsApplied, void *payload) {
    UsbGadget *gadget = (UsbGadget *)payload;
    gadget->mCurrentUsbFunctionsApplied = functionsApplied;
}

ScopedAStatus UsbGadget::getCurrentUsbFunctions(const shared_ptr<IUsbGadgetCallback>& callback,
                                                int64_t in_transactionId) {
    if (callback == nullptr) {
        return ScopedAStatus::fromExceptionCode(EX_NULL_POINTER);
    }
    ScopedAStatus ret = callback->getCurrentUsbFunctionsCb(
        mCurrentUsbFunctions,
        mCurrentUsbFunctionsApplied ? Status::FUNCTIONS_APPLIED : Status::FUNCTIONS_NOT_APPLIED,
	in_transactionId);
    if (!ret.isOk())
        ALOGE("Call to getCurrentUsbFunctionsCb failed %s", ret.getDescription().c_str());

    return ScopedAStatus::ok();
}

ScopedAStatus UsbGadget::getUsbSpeed(const shared_ptr<IUsbGadgetCallback> &callback,
	int64_t in_transactionId) {
    std::string current_speed;
    if (ReadFileToString(SPEED_PATH, &current_speed)) {
        current_speed = Trim(current_speed);
        ALOGI("current USB speed is %s", current_speed.c_str());
        if (current_speed == "low-speed")
            mUsbSpeed = UsbSpeed::LOWSPEED;
        else if (current_speed == "full-speed")
            mUsbSpeed = UsbSpeed::FULLSPEED;
        else if (current_speed == "high-speed")
            mUsbSpeed = UsbSpeed::HIGHSPEED;
        else if (current_speed == "super-speed")
            mUsbSpeed = UsbSpeed::SUPERSPEED;
        else if (current_speed == "super-speed-plus")
            mUsbSpeed = UsbSpeed::SUPERSPEED_10Gb;
        else if (current_speed == "UNKNOWN")
            mUsbSpeed = UsbSpeed::UNKNOWN;
        else
            mUsbSpeed = UsbSpeed::UNKNOWN;
    } else {
        ALOGE("Fail to read current speed");
        mUsbSpeed = UsbSpeed::UNKNOWN;
    }

    if (callback) {
        ScopedAStatus ret = callback->getUsbSpeedCb(mUsbSpeed, in_transactionId);

        if (!ret.isOk())
            ALOGE("Call to getUsbSpeedCb failed %s", ret.getDescription().c_str());
    }

    return ScopedAStatus::ok();
}

Status UsbGadget::tearDownGadget() {
    return Status::SUCCESS;
}

ScopedAStatus UsbGadget::reset(const shared_ptr<IUsbGadgetCallback> &callback,
        int64_t in_transactionId) {
    if (callback)
        callback->resetCb(Status::SUCCESS, in_transactionId);
    return ScopedAStatus::ok();
}

Status UsbGadget::setupFunctions(long functions,
	const shared_ptr<IUsbGadgetCallback> &callback, uint64_t timeout,
	int64_t in_transactionId) {
    bool ffsEnabled = false;
    if (timeout == 0) {
	ALOGI("timeout not setup");
    }

    if ((functions & GadgetFunction::ADB) != 0) {
        ffsEnabled = true;
    }

    if ((functions & GadgetFunction::NCM) != 0) {
        ALOGI("setCurrentUsbFunctions ncm");
    }

    // Pull up the gadget right away when there are no ffs functions.
    if (!ffsEnabled) {
        mCurrentUsbFunctionsApplied = true;
        if (callback)
            callback->setCurrentUsbFunctionsCb(functions, Status::SUCCESS, in_transactionId);
        return Status::SUCCESS;
    }

    return Status::SUCCESS;
}

Status getI2cBusHelper(string *name) {
    DIR *dp;

    dp = opendir(kHsi2cPath);
    if (dp != NULL) {
        struct dirent *ep;

        while ((ep = readdir(dp))) {
            if (ep->d_type == DT_DIR) {
                if (string::npos != string(ep->d_name).find("i2c-")) {
                    std::strtok(ep->d_name, "-");
                    *name = std::strtok(NULL, "-");
                }
            }
        }
        closedir(dp);
        return Status::SUCCESS;
    }

    ALOGE("Failed to open %s", kHsi2cPath);
    return Status::ERROR;
}

ScopedAStatus UsbGadget::setCurrentUsbFunctions(int64_t functions,
                                               const shared_ptr<IUsbGadgetCallback> &callback,
					       int64_t timeoutMs,
					       int64_t in_transactionId) {
    std::unique_lock<std::mutex> lk(mLockSetCurrentFunction);
    std::string current_usb_power_operation_mode, current_usb_type;
    std::string usb_limit_sink_enable;

    string accessoryCurrentLimitEnablePath, accessoryCurrentLimitPath, path;

    mCurrentUsbFunctions = functions;
    mCurrentUsbFunctionsApplied = false;

    getI2cBusHelper(&path);
    accessoryCurrentLimitPath = kI2CPath + path + "/" + kAccessoryLimitCurrent;
    accessoryCurrentLimitEnablePath = kI2CPath + path + "/" + kAccessoryLimitCurrentEnable;

    // Get the gadget IRQ number before tearDownGadget()
    if (mGadgetIrqPath.empty())
        getUsbGadgetIrqPath();

    // Unlink the gadget and stop the monitor if running.
    Status status = tearDownGadget();
    if (status != Status::SUCCESS) {
        goto error;
    }

    ALOGI("Returned from tearDown gadget");

    // Leave the gadget pulled down to give time for the host to sense disconnect.
    //usleep(kDisconnectWaitUs);

    if (functions == GadgetFunction::NONE) {
        if (callback == NULL)
            return ScopedAStatus::fromServiceSpecificErrorWithMessage(
                -1, "callback == NULL");
        ScopedAStatus ret = callback->setCurrentUsbFunctionsCb(functions, Status::SUCCESS, in_transactionId);
        if (!ret.isOk())
            ALOGE("Error while calling setCurrentUsbFunctionsCb %s", ret.getDescription().c_str());
        return ScopedAStatus::fromServiceSpecificErrorWithMessage(
                -1, "Error while calling setCurrentUsbFunctionsCb");
    }

    status = setupFunctions(functions, callback, timeoutMs, in_transactionId);
    if (status != Status::SUCCESS) {
        goto error;
    }

    if (functions & GadgetFunction::NCM) {
        if (!mGadgetIrqPath.empty()) {
            if (!WriteStringToFile(BIG_CORE, mGadgetIrqPath))
                ALOGI("Cannot move gadget IRQ to big core, path:%s", mGadgetIrqPath.c_str());
        }
    } else {
        if (!mGadgetIrqPath.empty()) {
            if (!WriteStringToFile(MEDIUM_CORE, mGadgetIrqPath))
                ALOGI("Cannot move gadget IRQ to medium core, path:%s", mGadgetIrqPath.c_str());
        }
    }

    if (ReadFileToString(CURRENT_USB_TYPE_PATH, &current_usb_type))
        current_usb_type = Trim(current_usb_type);

    if (ReadFileToString(CURRENT_USB_POWER_OPERATION_MODE_PATH, &current_usb_power_operation_mode))
        current_usb_power_operation_mode = Trim(current_usb_power_operation_mode);

    if (functions & GadgetFunction::ACCESSORY &&
        current_usb_type == "Unknown SDP [CDP] DCP" &&
        (current_usb_power_operation_mode == "default" ||
        current_usb_power_operation_mode == "1.5A")) {
        if (!WriteStringToFile("1300000", accessoryCurrentLimitPath)) {
            ALOGI("Write 1.3A to limit current fail");
        } else {
            if (!WriteStringToFile("1", accessoryCurrentLimitEnablePath)) {
                ALOGI("Enable limit current fail");
            }
        }
    } else {
        if (!WriteStringToFile("0", accessoryCurrentLimitEnablePath))
            ALOGI("unvote accessory limit current failed");
    }

    ALOGI("Usb Gadget setcurrent functions called successfully");
    return ScopedAStatus::fromServiceSpecificErrorWithMessage(
                -1, "Usb Gadget setcurrent functions called successfully");


error:
    ALOGI("Usb Gadget setcurrent functions failed");
    if (callback == NULL)
        return ScopedAStatus::fromServiceSpecificErrorWithMessage(
                -1, "Usb Gadget setcurrent functions failed");
    ScopedAStatus ret = callback->setCurrentUsbFunctionsCb(functions, status, in_transactionId);
    if (!ret.isOk())
        ALOGE("Error while calling setCurrentUsbFunctionsCb %s", ret.getDescription().c_str());
    return ScopedAStatus::fromServiceSpecificErrorWithMessage(
                -1, "Error while calling setCurrentUsbFunctionsCb");
}
}  // namespace gadget
}  // namespace usb
}  // namespace hardware
}  // namespace android
}  // aidl
