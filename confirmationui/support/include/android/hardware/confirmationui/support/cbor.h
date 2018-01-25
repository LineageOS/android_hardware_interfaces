/*
**
** Copyright 2017, The Android Open Source Project
**
** Licensed under the Apache License, Version 2.0 (the "License");
** you may not use this file except in compliance with the License.
** You may obtain a copy of the License at
**
**     http://www.apache.org/licenses/LICENSE-2.0
**
** Unless required by applicable law or agreed to in writing, software
** distributed under the License is distributed on an "AS IS" BASIS,
** WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
** See the License for the specific language governing permissions and
** limitations under the License.
*/

#ifndef CONFIRMATIONUI_1_0_DEFAULT_CBOR_H_
#define CONFIRMATIONUI_1_0_DEFAULT_CBOR_H_

#include <stddef.h>
#include <stdint.h>
#include <type_traits>

namespace android {
namespace hardware {
namespace confirmationui {
namespace support {

template <typename In, typename Out>
Out copy(In begin, In end, Out out) {
    while (begin != end) {
        *out++ = *begin++;
    }
    return out;
}

enum class Type : uint8_t {
    NUMBER = 0,
    NEGATIVE = 1,
    BYTE_STRING = 2,
    TEXT_STRING = 3,
    ARRAY = 4,
    MAP = 5,
    TAG = 6,
    FLOAT = 7,
};

enum class Error : uint32_t {
    OK = 0,
    OUT_OF_DATA = 1,
    MALFORMED = 2,
    MALFORMED_UTF8 = 3,
};

template <typename Key, typename Value>
struct MapElement {
    const Key& key_;
    const Value& value_;
    MapElement(const Key& key, const Value& value) : key_(key), value_(value) {}
};

template <typename... Elems>
struct Array;

template <typename Head, typename... Tail>
struct Array<Head, Tail...> {
    const Head& head_;
    Array<Tail...> tail_;
    Array(const Head& head, const Tail&... tail) : head_(head), tail_(tail...) {}
    constexpr size_t size() const { return sizeof...(Tail) + 1; };
};

template <>
struct Array<> {};

struct TextStr {};
struct ByteStr {};

template <typename T, typename Variant>
struct StringBuffer {
    const T* data_;
    size_t size_;
    StringBuffer(const T* data, size_t size) : data_(data), size_(size) {
        static_assert(sizeof(T) == 1, "elements too large");
    }
    const T* data() const { return data_; }
    size_t size() const { return size_; }
};

/**
 * Takes a char array turns it into a StringBuffer of TextStr type. The length of the resulting
 * StringBuffer is size - 1, effectively stripping the 0 character from the region being considered.
 * If the terminating 0 shall not be stripped use text_keep_last.
 */
template <size_t size>
StringBuffer<char, TextStr> text(const char (&str)[size]) {
    if (size > 0) return StringBuffer<char, TextStr>(str, size - 1);
    return StringBuffer<char, TextStr>(str, size);
}

/**
 * As opposed to text(const char (&str)[size] this function does not strips the last character.
 */
template <size_t size>
StringBuffer<char, TextStr> text_keep_last(const char (&str)[size]) {
    return StringBuffer<char, TextStr>(str, size);
}

template <typename T>
auto getData(const T& v) -> decltype(v.data()) {
    return v.data();
}

template <typename T>
auto getData(const T& v) -> decltype(v.c_str()) {
    return v.c_str();
}

template <typename T>
auto text(const T& str) -> StringBuffer<std::decay_t<decltype(*getData(str))>, TextStr> {
    return StringBuffer<std::decay_t<decltype(*getData(str))>, TextStr>(getData(str), str.size());
}

inline StringBuffer<char, TextStr> text(const char* str, size_t size) {
    return StringBuffer<char, TextStr>(str, size);
}

template <typename T, size_t size>
StringBuffer<T, ByteStr> bytes(const T (&str)[size]) {
    return StringBuffer<T, ByteStr>(str, size);
}

template <typename T>
StringBuffer<T, ByteStr> bytes(const T* str, size_t size) {
    return StringBuffer<T, ByteStr>(str, size);
}

template <typename T>
auto bytes(const T& str) -> StringBuffer<std::decay_t<decltype(*getData(str))>, ByteStr> {
    return StringBuffer<std::decay_t<decltype(*getData(str))>, ByteStr>(getData(str), str.size());
}

template <typename... Elems>
struct Map;

template <typename HeadKey, typename HeadValue, typename... Tail>
struct Map<MapElement<HeadKey, HeadValue>, Tail...> {
    const MapElement<HeadKey, HeadValue>& head_;
    Map<Tail...> tail_;
    Map(const MapElement<HeadKey, HeadValue>& head, const Tail&... tail)
        : head_(head), tail_(tail...) {}
    constexpr size_t size() const { return sizeof...(Tail) + 1; };
};

template <>
struct Map<> {};

template <typename... Keys, typename... Values>
Map<MapElement<Keys, Values>...> map(const MapElement<Keys, Values>&... elements) {
    return Map<MapElement<Keys, Values>...>(elements...);
}

template <typename... Elements>
Array<Elements...> arr(const Elements&... elements) {
    return Array<Elements...>(elements...);
}

template <typename Key, typename Value>
MapElement<Key, Value> pair(const Key& k, const Value& v) {
    return MapElement<Key, Value>(k, v);
}

template <size_t size>
struct getUnsignedType;

template <>
struct getUnsignedType<sizeof(uint8_t)> {
    typedef uint8_t type;
};
template <>
struct getUnsignedType<sizeof(uint16_t)> {
    typedef uint16_t type;
};
template <>
struct getUnsignedType<sizeof(uint32_t)> {
    typedef uint32_t type;
};
template <>
struct getUnsignedType<sizeof(uint64_t)> {
    typedef uint64_t type;
};

template <size_t size>
using Unsigned = typename getUnsignedType<size>::type;

class WriteState {
   public:
    WriteState() : data_(nullptr), size_(0), error_(Error::OK) {}
    WriteState(uint8_t* buffer, size_t size) : data_(buffer), size_(size), error_(Error::OK) {}
    WriteState(uint8_t* buffer, size_t size, Error error)
        : data_(buffer), size_(size), error_(error) {}
    template <size_t size>
    WriteState(uint8_t (&buffer)[size]) : data_(buffer), size_(size), error_(Error::OK) {}

    WriteState& operator++() {
        if (size_) {
            ++data_;
            --size_;
        } else {
            error_ = Error::OUT_OF_DATA;
        }
        return *this;
    }
    WriteState& operator+=(size_t offset) {
        if (offset > size_) {
            error_ = Error::OUT_OF_DATA;
        } else {
            data_ += offset;
            size_ -= offset;
        }
        return *this;
    }
    operator bool() const { return error_ == Error::OK; }

    uint8_t* data_;
    size_t size_;
    Error error_;
};

WriteState writeHeader(WriteState wState, Type type, const uint64_t value);
bool checkUTF8Copy(const char* begin, const char* const end, uint8_t* out);

template <typename T>
WriteState writeNumber(WriteState wState, const T& v) {
    if (!wState) return wState;
    if (v >= 0) {
        return writeHeader(wState, Type::NUMBER, v);
    } else {
        return writeHeader(wState, Type::NEGATIVE, UINT64_C(-1) - v);
    }
}

inline WriteState write(const WriteState& wState, const uint8_t& v) {
    return writeNumber(wState, v);
}
inline WriteState write(const WriteState& wState, const int8_t& v) {
    return writeNumber(wState, v);
}
inline WriteState write(const WriteState& wState, const uint16_t& v) {
    return writeNumber(wState, v);
}
inline WriteState write(const WriteState& wState, const int16_t& v) {
    return writeNumber(wState, v);
}
inline WriteState write(const WriteState& wState, const uint32_t& v) {
    return writeNumber(wState, v);
}
inline WriteState write(const WriteState& wState, const int32_t& v) {
    return writeNumber(wState, v);
}
inline WriteState write(const WriteState& wState, const uint64_t& v) {
    return writeNumber(wState, v);
}
inline WriteState write(const WriteState& wState, const int64_t& v) {
    return writeNumber(wState, v);
}

template <typename T>
WriteState write(WriteState wState, const StringBuffer<T, TextStr>& v) {
    wState = writeHeader(wState, Type::TEXT_STRING, v.size());
    uint8_t* buffer = wState.data_;
    wState += v.size();
    if (!wState) return wState;
    if (!checkUTF8Copy(v.data(), v.data() + v.size(), buffer)) {
        wState.error_ = Error::MALFORMED_UTF8;
    }
    return wState;
}

template <typename T>
WriteState write(WriteState wState, const StringBuffer<T, ByteStr>& v) {
    wState = writeHeader(wState, Type::BYTE_STRING, v.size());
    uint8_t* buffer = wState.data_;
    wState += v.size();
    if (!wState) return wState;
    static_assert(sizeof(*v.data()) == 1, "elements too large");
    copy(v.data(), v.data() + v.size(), buffer);
    return wState;
}

template <template <typename...> class Arr>
WriteState writeArrayHelper(WriteState wState, const Arr<>&) {
    return wState;
}

template <template <typename...> class Arr, typename Head, typename... Tail>
WriteState writeArrayHelper(WriteState wState, const Arr<Head, Tail...>& arr) {
    wState = write(wState, arr.head_);
    return writeArrayHelper(wState, arr.tail_);
}

template <typename... Elems>
WriteState write(WriteState wState, const Map<Elems...>& map) {
    if (!wState) return wState;
    wState = writeHeader(wState, Type::MAP, map.size());
    return writeArrayHelper(wState, map);
}

template <typename... Elems>
WriteState write(WriteState wState, const Array<Elems...>& arr) {
    if (!wState) return wState;
    wState = writeHeader(wState, Type::ARRAY, arr.size());
    return writeArrayHelper(wState, arr);
}

template <typename Key, typename Value>
WriteState write(WriteState wState, const MapElement<Key, Value>& element) {
    if (!wState) return wState;
    wState = write(wState, element.key_);
    return write(wState, element.value_);
}

template <typename Head, typename... Tail>
WriteState write(WriteState wState, const Head& head, const Tail&... tail) {
    wState = write(wState, head);
    return write(wState, tail...);
}

}  // namespace support
}  // namespace confirmationui
}  // namespace hardware
}  // namespace android

#endif  // CONFIRMATIONUI_1_0_DEFAULT_CBOR_H_
