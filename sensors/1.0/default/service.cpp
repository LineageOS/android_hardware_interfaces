/*
 * Copyright (C) 2016 The Android Open Source Project
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

#define LOG_TAG "android.hardware.sensors@1.0-service"

#include <android/hardware/sensors/1.0/ISensors.h>
#include <hidl/LegacySupport.h>

#ifdef ARCH_ARM_32
#include <hwbinder/ProcessState.h>
#include <cutils/properties.h>
#endif

using android::hardware::sensors::V1_0::ISensors;
using android::hardware::defaultPassthroughServiceImplementation;

#ifdef ARCH_ARM_32
//default h/w binder memsize for sensors is 8 KB
#define DEFAULT_SENSORS_HW_BINDER_MEM_SIZE_KB 8
size_t getHWBinderMmapSize() {
    int32_t value = DEFAULT_SENSORS_HW_BINDER_MEM_SIZE_KB;

    value = property_get_int32("persist.vendor.sensor.hw.binder.size", value);
    ALOGD("Init hw binder with mem size = %d", value);
    return 1024 * value;
}
#endif

int main() {
#ifdef ARCH_ARM_32
    android::hardware::ProcessState::initWithMmapSize((size_t)getHWBinderMmapSize());
#endif
    /* Sensors framework service needs at least two threads.
     * One thread blocks on a "poll"
     * The second thread is needed for all other HAL methods.
     */
    return defaultPassthroughServiceImplementation<ISensors>(2);
}
