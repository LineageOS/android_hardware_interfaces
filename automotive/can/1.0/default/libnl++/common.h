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

#include <libnl++/Buffer.h>

#include <linux/can.h>
#include <net/if.h>

#include <string>

namespace android::nl {

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

/**
 * Filter a string against non-printable characters.
 *
 * Replaces all non-printable characters with '?'.
 *
 * \param str String to filter.
 * \return Filtered string.
 */
std::string printableOnly(std::string str);

/**
 * Calculates a (optionally running) CRC16 checksum.
 *
 * CRC16 isn't a strong checksum, but is good for quick comparison purposes.
 * One benefit (and also a drawback too) is that all-zero payloads with any length will
 * always have a checksum of 0x0000.
 *
 * \param data Buffer to calculate checksum for
 * \param crc Previous CRC16 value to continue calculating running checksum
 * \return CRC16 checksum
 */
uint16_t crc16(const Buffer<uint8_t> data, uint16_t crc = 0);

}  // namespace android::nl
