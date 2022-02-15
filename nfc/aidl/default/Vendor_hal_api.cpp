/*
 * Copyright (C) 2021 The Android Open Source Project
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

#include <android-base/properties.h>
#include <dlfcn.h>
#include <errno.h>
#include <string.h>

#include "Vendor_hal_api.h"

bool logging = false;

int Vendor_hal_open(nfc_stack_callback_t* p_cback, nfc_stack_data_callback_t* p_data_cback) {
    (void)p_cback;
    (void)p_data_cback;
    // nothing to open in this example
    return -1;
}

int Vendor_hal_write(uint16_t data_len, const uint8_t* p_data) {
    (void)data_len;
    (void)p_data;
    return -1;
}

int Vendor_hal_core_initialized() {
    return -1;
}

int Vendor_hal_pre_discover() {
    return -1;
}

int Vendor_hal_close() {
    return -1;
}

int Vendor_hal_close_off() {
    return -1;
}

int Vendor_hal_power_cycle() {
    return -1;
}

void Vendor_hal_factoryReset() {}

void Vendor_hal_getConfig(NfcConfig& config) {
    (void)config;
}

void Vendor_hal_setVerboseLogging(bool enable) {
    logging = enable;
}

bool Vendor_hal_getVerboseLogging() {
    return logging;
}
