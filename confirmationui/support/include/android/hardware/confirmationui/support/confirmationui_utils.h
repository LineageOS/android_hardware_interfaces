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

#ifndef CONFIRMATIONUI_1_0_SUPPORT_INCLUDE_CONFIRMATIONUI_UTILS_H_
#define CONFIRMATIONUI_1_0_SUPPORT_INCLUDE_CONFIRMATIONUI_UTILS_H_

#include <stddef.h>
#include <stdint.h>
#include <algorithm>
#include <initializer_list>
#include <type_traits>

namespace android {
namespace hardware {
namespace confirmationui {
namespace support {

/**
 * This class wraps a (mostly return) value and stores whether or not the wrapped value is valid out
 * of band. Note that if the wrapped value is a reference it is unsafe to access the value if
 * !isOk(). If the wrapped type is a pointer or value and !isOk(), it is still safe to access the
 * wrapped value. In this case the pointer will be NULL though, and the value will be default
 * constructed.
 */
template <typename ValueT>
class NullOr {
    template <typename T>
    struct reference_initializer {
        static T&& init() { return *static_cast<std::remove_reference_t<T>*>(nullptr); }
    };
    template <typename T>
    struct pointer_initializer {
        static T init() { return nullptr; }
    };
    template <typename T>
    struct value_initializer {
        static T init() { return T(); }
    };
    template <typename T>
    using initializer_t =
        std::conditional_t<std::is_lvalue_reference<T>::value, reference_initializer<T>,
                           std::conditional_t<std::is_pointer<T>::value, pointer_initializer<T>,
                                              value_initializer<T>>>;

   public:
    NullOr() : value_(initializer_t<ValueT>::init()), null_(true) {}
    template <typename T>
    NullOr(T&& value) : value_(std::forward<T>(value)), null_(false) {}

    bool isOk() const { return !null_; }

    const ValueT& value() const & { return value_; }
    ValueT& value() & { return value_; }
    ValueT&& value() && { return std::move(value_); }

    const std::remove_reference_t<ValueT>* operator->() const { return &value_; }
    std::remove_reference_t<ValueT>* operator->() { return &value_; }

   private:
    ValueT value_;
    bool null_;
};

template <typename T, size_t elements>
class array {
    using array_type = T[elements];

   public:
    array() : data_{} {}
    array(const T (&data)[elements]) { std::copy(data, data + elements, data_); }
    explicit array(const T& v) { fill(v); }

    T* data() { return data_; }
    const T* data() const { return data_; }
    constexpr size_t size() const { return elements; }

    T* begin() { return data_; }
    T* end() { return data_ + elements; }
    const T* begin() const { return data_; }
    const T* end() const { return data_ + elements; }

    void fill(const T& v) {
        for (size_t i = 0; i < elements; ++i) {
            data_[i] = v;
        }
    }

   private:
    array_type data_;
};

template <typename T>
auto bytes_cast(const T& v) -> const uint8_t (&)[sizeof(T)] {
    return *reinterpret_cast<const uint8_t(*)[sizeof(T)]>(&v);
}
template <typename T>
auto bytes_cast(T& v) -> uint8_t (&)[sizeof(T)] {
    return *reinterpret_cast<uint8_t(*)[sizeof(T)]>(&v);
}

class ByteBufferProxy {
    template <typename T>
    struct has_data {
        template <typename U>
        static int f(const U*, const void*) {
            return 0;
        }
        template <typename U>
        static int* f(const U* u, decltype(u->data())) {
            return nullptr;
        }
        static constexpr bool value = std::is_pointer<decltype(f((T*)nullptr, ""))>::value;
    };

   public:
    template <typename T>
    ByteBufferProxy(const T& buffer, decltype(buffer.data()) = nullptr)
        : data_(reinterpret_cast<const uint8_t*>(buffer.data())), size_(buffer.size()) {
        static_assert(sizeof(decltype(*buffer.data())) == 1, "elements to large");
    }

    // this overload kicks in for types that have .c_str() but not .data(), such as hidl_string.
    // std::string has both so we need to explicitly disable this overload if .data() is present.
    template <typename T>
    ByteBufferProxy(const T& buffer,
                    std::enable_if_t<!has_data<T>::value, decltype(buffer.c_str())> = nullptr)
        : data_(reinterpret_cast<const uint8_t*>(buffer.c_str())), size_(buffer.size()) {
        static_assert(sizeof(decltype(*buffer.c_str())) == 1, "elements to large");
    }

    template <size_t size>
    ByteBufferProxy(const char (&buffer)[size])
        : data_(reinterpret_cast<const uint8_t*>(buffer)), size_(size - 1) {
        static_assert(size > 0, "even an empty string must be 0-terminated");
    }

    template <size_t size>
    ByteBufferProxy(const uint8_t (&buffer)[size]) : data_(buffer), size_(size) {}

    ByteBufferProxy() : data_(nullptr), size_(0) {}

    const uint8_t* data() const { return data_; }
    size_t size() const { return size_; }

    const uint8_t* begin() const { return data_; }
    const uint8_t* end() const { return data_ + size_; }

   private:
    const uint8_t* data_;
    size_t size_;
};

constexpr uint8_t auth_token_key_size = 32;
constexpr uint8_t hmac_size_bytes = support::auth_token_key_size;
using auth_token_key_t = array<uint8_t, auth_token_key_size>;
using hmac_t = auth_token_key_t;

/**
 * Implementer are expected to provide an implementation with the following prototype:
 *  static NullOr<array<uint8_t, 32>> hmac256(const uint8_t key[32],
 *                                     std::initializer_list<ByteBufferProxy> buffers);
 */
template <typename Impl>
class HMac {
   public:
    template <typename... Data>
    static NullOr<hmac_t> hmac256(const auth_token_key_t& key, const Data&... data) {
        return Impl::hmac256(key, {data...});
    }
};

bool operator==(const ByteBufferProxy& lhs, const ByteBufferProxy& rhs);

template <typename IntType, uint32_t byteOrder>
struct choose_hton;

template <typename IntType>
struct choose_hton<IntType, __ORDER_LITTLE_ENDIAN__> {
    inline static IntType hton(const IntType& value) {
        IntType result = {};
        const unsigned char* inbytes = reinterpret_cast<const unsigned char*>(&value);
        unsigned char* outbytes = reinterpret_cast<unsigned char*>(&result);
        for (int i = sizeof(IntType) - 1; i >= 0; --i) {
            *(outbytes++) = inbytes[i];
        }
        return result;
    }
};

template <typename IntType>
struct choose_hton<IntType, __ORDER_BIG_ENDIAN__> {
    inline static IntType hton(const IntType& value) { return value; }
};

template <typename IntType>
inline IntType hton(const IntType& value) {
    return choose_hton<IntType, __BYTE_ORDER__>::hton(value);
}

template <typename IntType>
inline IntType ntoh(const IntType& value) {
    // same operation as hton
    return choose_hton<IntType, __BYTE_ORDER__>::hton(value);
}

}  // namespace support
}  // namespace confirmationui
}  // namespace hardware
}  // namespace android

#endif  // CONFIRMATIONUI_1_0_SUPPORT_INCLUDE_CONFIRMATIONUI_UTILS_H_
