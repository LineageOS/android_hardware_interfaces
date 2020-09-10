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

#include <android/hardware/automotive/can/1.0/ICanController.h>

namespace android::hardware::automotive::can::V1_0 {

/**
 * Define gTest printer for a given HIDL type, but skip definition for Return<T>.
 */
#define DEFINE_CAN_HAL_PRINTER_SIMPLE(T, converter) \
    std::ostream& operator<<(std::ostream& os, const T& v) { return os << converter(v); }

/**
 * Define gTest printer for a given HIDL type.
 */
#define DEFINE_CAN_HAL_PRINTER(T, converter)    \
    DEFINE_CAN_HAL_PRINTER_SIMPLE(T, converter) \
    std::ostream& operator<<(std::ostream& os, const Return<T>& v) { return os << converter(v); }

DEFINE_CAN_HAL_PRINTER(CanMessage, toString)
DEFINE_CAN_HAL_PRINTER(ErrorEvent, toString)
DEFINE_CAN_HAL_PRINTER(ICanController::BusConfig::InterfaceId, toString);
DEFINE_CAN_HAL_PRINTER(ICanController::InterfaceType, toString)
DEFINE_CAN_HAL_PRINTER(ICanController::Result, toString)
DEFINE_CAN_HAL_PRINTER(Result, toString)

#undef DEFINE_CAN_HAL_PRINTER
#undef DEFINE_CAN_HAL_PRINTER_SIMPLE

}  // namespace android::hardware::automotive::can::V1_0
