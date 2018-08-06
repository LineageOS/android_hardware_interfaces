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
#include <optional>
#include <queue>
#include <thread>
#include <unordered_set>

namespace android {
namespace hardware {
namespace broadcastradio {
namespace utils {

enum class FrequencyBand {
    UNKNOWN,
    FM,
    AM_LW,
    AM_MW,
    AM_SW,
};

V2_0::IdentifierType getType(uint32_t typeAsInt);
V2_0::IdentifierType getType(const V2_0::ProgramIdentifier& id);

class IdentifierIterator
    : public std::iterator<std::random_access_iterator_tag, V2_0::ProgramIdentifier, ssize_t,
                           const V2_0::ProgramIdentifier*, const V2_0::ProgramIdentifier&> {
    using traits = std::iterator_traits<IdentifierIterator>;
    using ptr_type = typename traits::pointer;
    using ref_type = typename traits::reference;
    using diff_type = typename traits::difference_type;

   public:
    explicit IdentifierIterator(const V2_0::ProgramSelector& sel);

    IdentifierIterator operator++(int);
    IdentifierIterator& operator++();
    ref_type operator*() const;
    inline ptr_type operator->() const { return &operator*(); }
    IdentifierIterator operator+(diff_type v) const { return IdentifierIterator(mSel, mPos + v); }
    bool operator==(const IdentifierIterator& rhs) const;
    inline bool operator!=(const IdentifierIterator& rhs) const { return !operator==(rhs); };

   private:
    explicit IdentifierIterator(const V2_0::ProgramSelector& sel, size_t pos);

    std::reference_wrapper<const V2_0::ProgramSelector> mSel;

    const V2_0::ProgramSelector& sel() const { return mSel.get(); }

    /** 0 is the primary identifier, 1-n are secondary identifiers. */
    size_t mPos = 0;
};

/**
 * Guesses band from the frequency value.
 *
 * The band bounds are not exact to cover multiple regions.
 * The function is biased towards success, i.e. it never returns
 * FrequencyBand::UNKNOWN for correct frequency, but a result for
 * incorrect one is undefined (it doesn't have to return UNKNOWN).
 */
FrequencyBand getBand(uint64_t frequency);

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

bool isValid(const V2_0::ProgramIdentifier& id);
bool isValid(const V2_0::ProgramSelector& sel);

V2_0::ProgramIdentifier make_identifier(V2_0::IdentifierType type, uint64_t value);
V2_0::ProgramSelector make_selector_amfm(uint32_t frequency);
V2_0::Metadata make_metadata(V2_0::MetadataKey key, int64_t value);
V2_0::Metadata make_metadata(V2_0::MetadataKey key, std::string value);

bool satisfies(const V2_0::ProgramFilter& filter, const V2_0::ProgramSelector& sel);

struct ProgramInfoHasher {
    size_t operator()(const V2_0::ProgramInfo& info) const;
};

struct ProgramInfoKeyEqual {
    bool operator()(const V2_0::ProgramInfo& info1, const V2_0::ProgramInfo& info2) const;
};

typedef std::unordered_set<V2_0::ProgramInfo, ProgramInfoHasher, ProgramInfoKeyEqual>
    ProgramInfoSet;

void updateProgramList(ProgramInfoSet& list, const V2_0::ProgramListChunk& chunk);

std::optional<std::string> getMetadataString(const V2_0::ProgramInfo& info,
                                             const V2_0::MetadataKey key);

V2_0::ProgramIdentifier make_hdradio_station_name(const std::string& name);

}  // namespace utils

namespace V2_0 {

utils::IdentifierIterator begin(const ProgramSelector& sel);
utils::IdentifierIterator end(const ProgramSelector& sel);

}  // namespace V2_0
}  // namespace broadcastradio
}  // namespace hardware
}  // namespace android

#endif  // ANDROID_HARDWARE_BROADCASTRADIO_COMMON_UTILS_2X_H
