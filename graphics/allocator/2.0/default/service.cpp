/*
 * Copyright 2016 The Android Open Source Project
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

#define LOG_TAG "android.hardware.graphics.allocator@2.0-service"

#include <android/hardware/graphics/allocator/2.0/IAllocator.h>

#include <hidl/LegacySupport.h>
#include <hwbinder/ProcessState.h>

using android::hardware::defaultPassthroughServiceImplementation;
using android::hardware::graphics::allocator::V2_0::IAllocator;

int main() {

#ifdef ARCH_ARM_32
    android::hardware::ProcessState::initWithMmapSize((size_t)(32768));
#endif
    return defaultPassthroughServiceImplementation<IAllocator>(4);
}
