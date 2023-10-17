/*
 * Copyright (C) 2022 The Android Open Source Project
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

#include <aidl/android/hardware/broadcastradio/IdentifierType.h>
#include <aidl/android/hardware/broadcastradio/Metadata.h>
#include <aidl/android/hardware/broadcastradio/ProgramFilter.h>
#include <aidl/android/hardware/broadcastradio/ProgramIdentifier.h>
#include <aidl/android/hardware/broadcastradio/ProgramInfo.h>
#include <aidl/android/hardware/broadcastradio/ProgramListChunk.h>
#include <aidl/android/hardware/broadcastradio/ProgramSelector.h>
#include <aidl/android/hardware/broadcastradio/Properties.h>
#include <aidl/android/hardware/broadcastradio/Result.h>

#include <numeric>
#include <optional>
#include <thread>
#include <unordered_set>

namespace aidl::android::hardware::broadcastradio {

namespace utils {

enum class FrequencyBand {
    UNKNOWN,
    FM,
    AM_LW,
    AM_MW,
    AM_SW,
};

class IdentifierIterator final
    : public std::iterator<std::random_access_iterator_tag, ProgramIdentifier, ssize_t,
                           const ProgramIdentifier*, const ProgramIdentifier&> {
    using ptrType = typename std::iterator_traits<IdentifierIterator>::pointer;
    using refType = typename std::iterator_traits<IdentifierIterator>::reference;
    using diffType = typename std::iterator_traits<IdentifierIterator>::difference_type;

  public:
    explicit IdentifierIterator(const ProgramSelector& sel);

    const IdentifierIterator operator++(int);
    IdentifierIterator& operator++();
    refType operator*() const;
    inline ptrType operator->() const { return &operator*(); }
    IdentifierIterator operator+(diffType v) const { return IdentifierIterator(mSel, mPos + v); }
    bool operator==(const IdentifierIterator& rhs) const;
    inline bool operator!=(const IdentifierIterator& rhs) const { return !operator==(rhs); };

  private:
    explicit IdentifierIterator(const ProgramSelector& sel, size_t pos);

    std::reference_wrapper<const ProgramSelector> mSel;

    const ProgramSelector& getSelector() const { return mSel.get(); }

    /** 0 is the primary identifier, 1-n are secondary identifiers. */
    size_t mPos = 0;
};

/**
 * Convert Result to int
 */
int32_t resultToInt(Result result);

/**
 * Guesses band from the frequency value.
 *
 * The band bounds are not exact to cover multiple regions.
 * The function is biased towards success, i.e. it never returns
 * FrequencyBand::UNKNOWN for correct frequency, but a result for
 * incorrect one is undefined (it doesn't have to return UNKNOWN).
 */
FrequencyBand getBand(int64_t frequency);

/**
 * Checks, if {@code pointer} tunes to {@channel}.
 *
 * For example, having a channel {AMFM_FREQUENCY_KHZ = 103.3}:
 * - selector {AMFM_FREQUENCY_KHZ = 103.3, HD_SUBCHANNEL = 0} can tune to this channel;
 * - selector {AMFM_FREQUENCY_KHZ = 103.3, HD_SUBCHANNEL = 1} can't.
 *
 * @param pointer selector we're trying to match against channel.
 * @param channel existing channel.
 */
bool tunesTo(const ProgramSelector& pointer, const ProgramSelector& channel);

/**
 * Checks whether a given program selector has the given ID (either primary or secondary).
 */
bool hasId(const ProgramSelector& sel, const IdentifierType& type);

/**
 * Returns ID (either primary or secondary) for a given program selector.
 *
 * If the selector does not contain given type, returns kValueForNotFoundIdentifier
 * and emits a warning.
 */
int64_t getId(const ProgramSelector& sel, const IdentifierType& type);

/**
 * Returns ID (either primary or secondary) for a given program selector.
 *
 * If the selector does not contain given type, returns default value.
 */
int64_t getId(const ProgramSelector& sel, const IdentifierType& type, int64_t defaultValue);

/**
 * Returns all IDs of a given type.
 */
std::vector<int> getAllIds(const ProgramSelector& sel, const IdentifierType& type);

/**
 * Checks, if a given selector is supported by the radio module.
 *
 * @param prop Module description.
 * @param sel The selector to check.
 * @return True, if the selector is supported, false otherwise.
 */
bool isSupported(const Properties& prop, const ProgramSelector& sel);

bool isValid(const ProgramIdentifier& id);
bool isValid(const ProgramSelector& sel);

ProgramIdentifier makeIdentifier(IdentifierType type, int64_t value);
ProgramSelector makeSelectorAmfm(uint32_t frequency);
ProgramSelector makeSelectorDab(uint64_t sidExt);
ProgramSelector makeSelectorDab(uint64_t sidExt, uint32_t ensemble, uint64_t freq);

bool satisfies(const ProgramFilter& filter, const ProgramSelector& sel);

struct ProgramInfoHasher {
    size_t operator()(const ProgramInfo& info) const;
};

struct ProgramInfoKeyEqual {
    bool operator()(const ProgramInfo& info1, const ProgramInfo& info2) const;
};

typedef std::unordered_set<ProgramInfo, ProgramInfoHasher, ProgramInfoKeyEqual> ProgramInfoSet;

void updateProgramList(const ProgramListChunk& chunk, ProgramInfoSet* list);

std::optional<std::string> getMetadataString(const ProgramInfo& info, const Metadata::Tag& tag);

ProgramIdentifier makeHdRadioStationName(const std::string& name);

template <typename aidl_type>
inline std::string vectorToString(const std::vector<aidl_type>& in_values) {
    return std::accumulate(std::begin(in_values), std::end(in_values), std::string{},
                           [](const std::string& ls, const aidl_type& rs) {
                               return ls + (ls.empty() ? "" : ",") + toString(rs);
                           });
}

IdentifierType getType(int typeAsInt);

bool parseArgInt(const std::string& s, int* out);

bool parseArgLong(const std::string& s, long* out);

bool parseArgBool(const std::string& s, bool* out);

bool parseArgDirection(const std::string& s, bool* out);

bool parseArgIdentifierTypeArray(const std::string& s, std::vector<IdentifierType>* out);

bool parseProgramIdentifierList(const std::string& s, std::vector<ProgramIdentifier>* out);

}  // namespace utils

utils::IdentifierIterator begin(const ProgramSelector& sel);
utils::IdentifierIterator end(const ProgramSelector& sel);

}  // namespace aidl::android::hardware::broadcastradio
