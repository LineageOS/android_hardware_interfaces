/*
 * Copyright (C) 2017 The Android Open Source Project
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
#ifndef ANDROID_HARDWARE_BROADCASTRADIO_COMMON_UTILS_2X_H
#define ANDROID_HARDWARE_BROADCASTRADIO_COMMON_UTILS_2X_H

#include <android/hardware/broadcastradio/2.0/types.h>
#include <chrono>
#include <queue>
#include <thread>

namespace android {
namespace hardware {
namespace broadcastradio {
namespace utils {

V2_0::IdentifierType getType(const V2_0::ProgramIdentifier& id);

/**
 * Checks, if {@code pointer} tunes to {@channel}.
 *
 * For example, having a channel {AMFM_FREQUENCY = 103.3}:
 * - selector {AMFM_FREQUENCY = 103.3, HD_SUBCHANNEL = 0} can tune to this channel;
 * - selector {AMFM_FREQUENCY = 103.3, HD_SUBCHANNEL = 1} can't.
 *
 * @param pointer selector we're trying to match against channel.
 * @param channel existing channel.
 */
bool tunesTo(const V2_0::ProgramSelector& pointer, const V2_0::ProgramSelector& channel);

bool hasId(const V2_0::ProgramSelector& sel, const V2_0::IdentifierType type);

/**
 * Returns ID (either primary or secondary) for a given program selector.
 *
 * If the selector does not contain given type, returns 0 and emits a warning.
 */
uint64_t getId(const V2_0::ProgramSelector& sel, const V2_0::IdentifierType type);

/**
 * Returns ID (either primary or secondary) for a given program selector.
 *
 * If the selector does not contain given type, returns default value.
 */
uint64_t getId(const V2_0::ProgramSelector& sel, const V2_0::IdentifierType type, uint64_t defval);

/**
 * Returns all IDs of a given type.
 */
std::vector<uint64_t> getAllIds(const V2_0::ProgramSelector& sel, const V2_0::IdentifierType type);

/**
 * Checks, if a given selector is supported by the radio module.
 *
 * @param prop Module description.
 * @param sel The selector to check.
 * @return True, if the selector is supported, false otherwise.
 */
bool isSupported(const V2_0::Properties& prop, const V2_0::ProgramSelector& sel);

bool isValid(const V2_0::ProgramSelector& sel);

V2_0::ProgramIdentifier make_identifier(V2_0::IdentifierType type, uint64_t value);
V2_0::ProgramSelector make_selector_amfm(uint32_t frequency);
V2_0::Metadata make_metadata(V2_0::MetadataKey key, int64_t value);
V2_0::Metadata make_metadata(V2_0::MetadataKey key, std::string value);

}  // namespace utils
}  // namespace broadcastradio
}  // namespace hardware
}  // namespace android

#endif  // ANDROID_HARDWARE_BROADCASTRADIO_COMMON_UTILS_2X_H
