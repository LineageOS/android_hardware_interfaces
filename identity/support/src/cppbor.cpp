/*
 * Copyright (c) 2019, The Android Open Source Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include "cppbor.h"
#include "cppbor_parse.h"

#define LOG_TAG "CppBor"
#include <android-base/logging.h>

namespace cppbor {

namespace {

template <typename T, typename Iterator, typename = std::enable_if<std::is_unsigned<T>::value>>
Iterator writeBigEndian(T value, Iterator pos) {
    for (unsigned i = 0; i < sizeof(value); ++i) {
        *pos++ = static_cast<uint8_t>(value >> (8 * (sizeof(value) - 1)));
        value = static_cast<T>(value << 8);
    }
    return pos;
}

template <typename T, typename = std::enable_if<std::is_unsigned<T>::value>>
void writeBigEndian(T value, std::function<void(uint8_t)>& cb) {
    for (unsigned i = 0; i < sizeof(value); ++i) {
        cb(static_cast<uint8_t>(value >> (8 * (sizeof(value) - 1))));
        value = static_cast<T>(value << 8);
    }
}

}  // namespace

size_t headerSize(uint64_t addlInfo) {
    if (addlInfo < ONE_BYTE_LENGTH) return 1;
    if (addlInfo <= std::numeric_limits<uint8_t>::max()) return 2;
    if (addlInfo <= std::numeric_limits<uint16_t>::max()) return 3;
    if (addlInfo <= std::numeric_limits<uint32_t>::max()) return 5;
    return 9;
}

uint8_t* encodeHeader(MajorType type, uint64_t addlInfo, uint8_t* pos, const uint8_t* end) {
    size_t sz = headerSize(addlInfo);
    if (end - pos < static_cast<ssize_t>(sz)) return nullptr;
    switch (sz) {
        case 1:
            *pos++ = type | static_cast<uint8_t>(addlInfo);
            return pos;
        case 2:
            *pos++ = type | ONE_BYTE_LENGTH;
            *pos++ = static_cast<uint8_t>(addlInfo);
            return pos;
        case 3:
            *pos++ = type | TWO_BYTE_LENGTH;
            return writeBigEndian(static_cast<uint16_t>(addlInfo), pos);
        case 5:
            *pos++ = type | FOUR_BYTE_LENGTH;
            return writeBigEndian(static_cast<uint32_t>(addlInfo), pos);
        case 9:
            *pos++ = type | EIGHT_BYTE_LENGTH;
            return writeBigEndian(addlInfo, pos);
        default:
            CHECK(false);  // Impossible to get here.
            return nullptr;
    }
}

void encodeHeader(MajorType type, uint64_t addlInfo, EncodeCallback encodeCallback) {
    size_t sz = headerSize(addlInfo);
    switch (sz) {
        case 1:
            encodeCallback(type | static_cast<uint8_t>(addlInfo));
            break;
        case 2:
            encodeCallback(type | ONE_BYTE_LENGTH);
            encodeCallback(static_cast<uint8_t>(addlInfo));
            break;
        case 3:
            encodeCallback(type | TWO_BYTE_LENGTH);
            writeBigEndian(static_cast<uint16_t>(addlInfo), encodeCallback);
            break;
        case 5:
            encodeCallback(type | FOUR_BYTE_LENGTH);
            writeBigEndian(static_cast<uint32_t>(addlInfo), encodeCallback);
            break;
        case 9:
            encodeCallback(type | EIGHT_BYTE_LENGTH);
            writeBigEndian(addlInfo, encodeCallback);
            break;
        default:
            CHECK(false);  // Impossible to get here.
    }
}

bool Item::operator==(const Item& other) const& {
    if (type() != other.type()) return false;
    switch (type()) {
        case UINT:
            return *asUint() == *(other.asUint());
        case NINT:
            return *asNint() == *(other.asNint());
        case BSTR:
            return *asBstr() == *(other.asBstr());
        case TSTR:
            return *asTstr() == *(other.asTstr());
        case ARRAY:
            return *asArray() == *(other.asArray());
        case MAP:
            return *asMap() == *(other.asMap());
        case SIMPLE:
            return *asSimple() == *(other.asSimple());
        case SEMANTIC:
            return *asSemantic() == *(other.asSemantic());
        default:
            CHECK(false);  // Impossible to get here.
            return false;
    }
}

Nint::Nint(int64_t v) : mValue(v) {
    CHECK(v < 0) << "Only negative values allowed";
}

bool Simple::operator==(const Simple& other) const& {
    if (simpleType() != other.simpleType()) return false;

    switch (simpleType()) {
        case BOOLEAN:
            return *asBool() == *(other.asBool());
        case NULL_T:
            return true;
        default:
            CHECK(false);  // Impossible to get here.
            return false;
    }
}

uint8_t* Bstr::encode(uint8_t* pos, const uint8_t* end) const {
    pos = encodeHeader(mValue.size(), pos, end);
    if (!pos || end - pos < static_cast<ptrdiff_t>(mValue.size())) return nullptr;
    return std::copy(mValue.begin(), mValue.end(), pos);
}

void Bstr::encodeValue(EncodeCallback encodeCallback) const {
    for (auto c : mValue) {
        encodeCallback(c);
    }
}

uint8_t* Tstr::encode(uint8_t* pos, const uint8_t* end) const {
    pos = encodeHeader(mValue.size(), pos, end);
    if (!pos || end - pos < static_cast<ptrdiff_t>(mValue.size())) return nullptr;
    return std::copy(mValue.begin(), mValue.end(), pos);
}

void Tstr::encodeValue(EncodeCallback encodeCallback) const {
    for (auto c : mValue) {
        encodeCallback(static_cast<uint8_t>(c));
    }
}

bool CompoundItem::operator==(const CompoundItem& other) const& {
    return type() == other.type()             //
           && addlInfo() == other.addlInfo()  //
           // Can't use vector::operator== because the contents are pointers.  std::equal lets us
           // provide a predicate that does the dereferencing.
           && std::equal(mEntries.begin(), mEntries.end(), other.mEntries.begin(),
                         [](auto& a, auto& b) -> bool { return *a == *b; });
}

uint8_t* CompoundItem::encode(uint8_t* pos, const uint8_t* end) const {
    pos = encodeHeader(addlInfo(), pos, end);
    if (!pos) return nullptr;
    for (auto& entry : mEntries) {
        pos = entry->encode(pos, end);
        if (!pos) return nullptr;
    }
    return pos;
}

void CompoundItem::encode(EncodeCallback encodeCallback) const {
    encodeHeader(addlInfo(), encodeCallback);
    for (auto& entry : mEntries) {
        entry->encode(encodeCallback);
    }
}

void Map::assertInvariant() const {
    CHECK(mEntries.size() % 2 == 0);
}

std::unique_ptr<Item> Map::clone() const {
    assertInvariant();
    auto res = std::make_unique<Map>();
    for (size_t i = 0; i < mEntries.size(); i += 2) {
        res->add(mEntries[i]->clone(), mEntries[i + 1]->clone());
    }
    return res;
}

std::unique_ptr<Item> Array::clone() const {
    auto res = std::make_unique<Array>();
    for (size_t i = 0; i < mEntries.size(); i++) {
        res->add(mEntries[i]->clone());
    }
    return res;
}

void Semantic::assertInvariant() const {
    CHECK(mEntries.size() == 1);
}

}  // namespace cppbor
