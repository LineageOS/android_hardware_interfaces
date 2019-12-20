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

#include <string>

namespace android::netdevice {

/**
 * Returns the index of a given network interface.
 *
 * If the syscall to check the index fails with other error than ENODEV, it gets logged and the
 * return value indicates the interface doesn't exists.
 *
 * \param ifname Interface to check
 * \return Interface index, or 0 if the interface doesn't exist
 */
unsigned int nametoindex(const std::string& ifname);

}  // namespace android::netdevice
