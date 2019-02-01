/*
 **
 ** Copyright 2016, The Android Open Source Project
 **
 ** Licensed under the Apache License, Version 2.0 (the "License");
 ** you may not use this file except in compliance with the License.
 ** You may obtain a copy of the License at
 **
 **     http://www.apache.org/licenses/LICENSE-2.0
 **
 ** Unless required by applicable law or agreed to in writing, software
 ** distributed under the License is distributed on an "AS IS" BASIS,
 ** WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 ** See the License for the specific language governing permissions and
 ** limitations under the License.
 */

#define LOG_TAG "android.hardware.keymaster@3.0-impl"

#include "KeymasterDevice.h"

#include <log/log.h>

#include <AndroidKeymaster3Device.h>
#include <hardware/keymaster0.h>
#include <hardware/keymaster1.h>
#include <hardware/keymaster2.h>
#include <hardware/keymaster_defs.h>

namespace android {
namespace hardware {
namespace keymaster {
namespace V3_0 {
namespace implementation {

static int get_keymaster0_dev(keymaster0_device_t** dev, const hw_module_t* mod) {
    int rc = keymaster0_open(mod, dev);
    if (rc) {
        ALOGE("Error opening keystore keymaster0 device.");
        *dev = nullptr;
        return rc;
    }
    return 0;
}

static int get_keymaster1_dev(keymaster1_device_t** dev, const hw_module_t* mod) {
    int rc = keymaster1_open(mod, dev);
    if (rc) {
        ALOGE("Error %d opening keystore keymaster1 device", rc);
        if (*dev) {
            (*dev)->common.close(&(*dev)->common);
            *dev = nullptr;
        }
    }
    return rc;
}

static int get_keymaster2_dev(keymaster2_device_t** dev, const hw_module_t* mod) {
    int rc = keymaster2_open(mod, dev);
    if (rc) {
        ALOGE("Error %d opening keystore keymaster2 device", rc);
        *dev = nullptr;
    }
    return rc;
}

static IKeymasterDevice* createKeymaster3Device() {
    const hw_module_t* mod = nullptr;

    int rc = hw_get_module_by_class(KEYSTORE_HARDWARE_MODULE_ID, NULL, &mod);
    if (rc) {
        ALOGI("Could not find any keystore module, using software-only implementation.");
        // SoftKeymasterDevice will be deleted by keymaster_device_release()
        return ::keymaster::ng::CreateKeymasterDevice();
    }

    if (mod->module_api_version < KEYMASTER_MODULE_API_VERSION_1_0) {
        keymaster0_device_t* dev = nullptr;
        if (get_keymaster0_dev(&dev, mod)) {
            return nullptr;
        }
        return ::keymaster::ng::CreateKeymasterDevice(dev);
    } else if (mod->module_api_version == KEYMASTER_MODULE_API_VERSION_1_0) {
        keymaster1_device_t* dev = nullptr;
        if (get_keymaster1_dev(&dev, mod)) {
            return nullptr;
        }
        return ::keymaster::ng::CreateKeymasterDevice(dev);
    } else {
        keymaster2_device_t* dev = nullptr;
        if (get_keymaster2_dev(&dev, mod)) {
            return nullptr;
        }
        return ::keymaster::ng::CreateKeymasterDevice(dev);
    }
}

IKeymasterDevice* HIDL_FETCH_IKeymasterDevice(const char* name) {
    ALOGI("Fetching keymaster device name %s", name);

    if (name && strcmp(name, "softwareonly") == 0) {
        return ::keymaster::ng::CreateKeymasterDevice();
    } else if (name && strcmp(name, "default") == 0) {
        return createKeymaster3Device();
    }
    return nullptr;
}

}  // namespace implementation
}  // namespace V3_0
}  // namespace keymaster
}  // namespace hardware
}  // namespace android
