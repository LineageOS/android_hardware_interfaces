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

#ifndef CONFIRMATIONUI_SUPPORT_INCLUDE_ANDROID_HARDWARE_CONFIRMATIONUI_SUPPORT_MSG_FORMATTING_H_
#define CONFIRMATIONUI_SUPPORT_INCLUDE_ANDROID_HARDWARE_CONFIRMATIONUI_SUPPORT_MSG_FORMATTING_H_

#include <android/hardware/confirmationui/1.0/types.h>
#include <android/hardware/keymaster/4.0/types.h>
#include <stddef.h>
#include <stdint.h>
#include <algorithm>
#include <tuple>
#include <type_traits>

#include <android/hardware/confirmationui/support/confirmationui_utils.h>

namespace android {
namespace hardware {
namespace confirmationui {
namespace support {

template <size_t... I>
class IntegerSequence {};

namespace integer_sequence {

template <typename Lhs, typename Rhs>
struct conc {};

template <size_t... ILhs, size_t... IRhs>
struct conc<IntegerSequence<ILhs...>, IntegerSequence<IRhs...>> {
    using type = IntegerSequence<ILhs..., IRhs...>;
};

template <typename Lhs, typename Rhs>
using conc_t = typename conc<Lhs, Rhs>::type;

template <size_t... n>
struct make {};

template <size_t n>
struct make<n> {
    using type = conc_t<typename make<n - 1>::type, IntegerSequence<n - 1>>;
};
template <size_t start, size_t n>
struct make<start, n> {
    using type = conc_t<typename make<start, n - 1>::type, IntegerSequence<start + n - 1>>;
};

template <size_t start>
struct make<start, start> {
    using type = IntegerSequence<start>;
};

template <>
struct make<0> {
    using type = IntegerSequence<>;
};

template <size_t... n>
using make_t = typename make<n...>::type;

}  // namespace integer_sequence

template <size_t... idx, typename... T>
std::tuple<std::remove_reference_t<T>&&...> tuple_move_helper(IntegerSequence<idx...>,
                                                              std::tuple<T...>&& t) {
    return {std::move(std::get<idx>(t))...};
}

template <typename... T>
std::tuple<std::remove_reference_t<T>&&...> tuple_move(std::tuple<T...>&& t) {
    return tuple_move_helper(integer_sequence::make_t<sizeof...(T)>(), std::move(t));
}

template <typename... T>
std::tuple<std::remove_reference_t<T>&&...> tuple_move(std::tuple<T...>& t) {
    return tuple_move_helper(integer_sequence::make_t<sizeof...(T)>(), std::move(t));
}

using ::android::hardware::confirmationui::V1_0::ResponseCode;
using ::android::hardware::confirmationui::V1_0::UIOption;
using ::android::hardware::keymaster::V4_0::HardwareAuthToken;
using ::android::hardware::hidl_string;
using ::android::hardware::hidl_vec;

template <typename... fields>
class Message {};

enum class Command : uint32_t {
    PromptUserConfirmation,
    DeliverSecureInputEvent,
    Abort,
    Vendor,
};

template <Command cmd>
struct Cmd {};

#define DECLARE_COMMAND(cmd) using cmd##_t = Cmd<Command::cmd>

DECLARE_COMMAND(PromptUserConfirmation);
DECLARE_COMMAND(DeliverSecureInputEvent);
DECLARE_COMMAND(Abort);
DECLARE_COMMAND(Vendor);

using PromptUserConfirmationMsg = Message<PromptUserConfirmation_t, hidl_string, hidl_vec<uint8_t>,
                                          hidl_string, hidl_vec<UIOption>>;
using PromptUserConfirmationResponse = Message<ResponseCode>;
using DeliverSecureInputEventMsg = Message<DeliverSecureInputEvent_t, HardwareAuthToken>;
using DeliverSecureInputEventRespose = Message<ResponseCode>;
using AbortMsg = Message<Abort_t>;
using ResultMsg = Message<ResponseCode, hidl_vec<uint8_t>, hidl_vec<uint8_t>>;

template <typename T>
struct StreamState {
    using ptr_t = volatile T*;
    volatile T* pos_;
    size_t bytes_left_;
    bool good_;
    template <size_t size>
    StreamState(T (&buffer)[size]) : pos_(buffer), bytes_left_(size), good_(size > 0) {}
    StreamState(T* buffer, size_t size) : pos_(buffer), bytes_left_(size), good_(size > 0) {}
    StreamState() : pos_(nullptr), bytes_left_(0), good_(false) {}
    StreamState& operator++() {
        if (good_ && bytes_left_) {
            ++pos_;
            --bytes_left_;
        } else {
            good_ = false;
        }
        return *this;
    }
    StreamState& operator+=(size_t offset) {
        if (!good_ || offset > bytes_left_) {
            good_ = false;
        } else {
            pos_ += offset;
            bytes_left_ -= offset;
        }
        return *this;
    }
    operator bool() const { return good_; }
    volatile T* pos() const { return pos_; };
};

using WriteStream = StreamState<uint8_t>;
using ReadStream = StreamState<const uint8_t>;

inline void zero(volatile uint8_t* begin, const volatile uint8_t* end) {
    while (begin != end) {
        *begin++ = 0xaa;
    }
}
inline void zero(const volatile uint8_t*, const volatile uint8_t*) {}
// This odd alignment function aligns the stream position to a 4byte and never 8byte boundary
// It is to accommodate the 4 byte size field which is then followed by 8byte aligned data.
template <typename T>
StreamState<T> unalign(StreamState<T> s) {
    uint8_t unalignment = uintptr_t(s.pos_) & 0x3;
    auto pos = s.pos_;
    if (unalignment) {
        s += 4 - unalignment;
    }
    // now s.pos_ is aligned on a 4byte boundary
    if ((uintptr_t(s.pos_) & 0x4) == 0) {
        // if we are 8byte aligned add 4
        s += 4;
    }
    // zero out the gaps when writing
    zero(pos, s.pos_);
    return s;
}

inline WriteStream write(WriteStream out, const uint8_t* buffer, size_t size) {
    auto pos = out.pos();
    uint32_t v = size;
    out += 4 + size;
    if (out) {
        if (size != v) {
            out.good_ = false;
            return out;
        }
        auto& s = bytes_cast(v);
        pos = std::copy(s, s + 4, pos);
        std::copy(buffer, buffer + size, pos);
    }
    return out;
}
template <size_t size>
WriteStream write(WriteStream out, const uint8_t (&v)[size]) {
    return write(out, v, size);
}

inline std::tuple<ReadStream, ReadStream::ptr_t, size_t> read(ReadStream in) {
    auto pos = in.pos();
    in += 4;
    if (!in) return {in, nullptr, 0};
    uint32_t size;
    std::copy(pos, pos + 4, bytes_cast(size));
    pos = in.pos();
    in += size;
    if (!in) return {in, nullptr, 0};
    return {in, pos, size};
}

template <typename T>
std::tuple<ReadStream, T> readSimpleType(ReadStream in) {
    T result;
    ReadStream::ptr_t pos = nullptr;
    size_t read_size = 0;
    std::tie(in, pos, read_size) = read(in);
    if (!in || read_size != sizeof(T)) {
        in.good_ = false;
        return {in, {}};
    }
    std::copy(pos, pos + sizeof(T), bytes_cast(result));
    return {in, std::move(result)};
}

template <typename T>
std::tuple<ReadStream, hidl_vec<T>> readSimpleHidlVecInPlace(ReadStream in) {
    std::tuple<ReadStream, hidl_vec<T>> result;
    ReadStream::ptr_t pos = nullptr;
    size_t read_size = 0;
    std::tie(std::get<0>(result), pos, read_size) = read(in);
    if (!std::get<0>(result) || read_size % sizeof(T)) {
        std::get<0>(result).good_ = false;
        return result;
    }
    std::get<1>(result).setToExternal(reinterpret_cast<T*>(const_cast<uint8_t*>(pos)),
                                      read_size / sizeof(T));
    return result;
}

template <typename T>
WriteStream writeSimpleHidlVec(WriteStream out, const hidl_vec<T>& vec) {
    return write(out, reinterpret_cast<const uint8_t*>(vec.data()), vec.size() * sizeof(T));
}

// HardwareAuthToken
constexpr size_t hatSizeNoMac() {
    HardwareAuthToken* hat = nullptr;
    return sizeof hat->challenge + sizeof hat->userId + sizeof hat->authenticatorId +
           sizeof hat->authenticatorType + sizeof hat->timestamp;
}

template <typename T>
inline volatile const uint8_t* copyField(T& field, volatile const uint8_t*(&pos)) {
    auto& s = bytes_cast(field);
    std::copy(pos, pos + sizeof(T), s);
    return pos + sizeof(T);
}
inline std::tuple<ReadStream, HardwareAuthToken> read(Message<HardwareAuthToken>, ReadStream in_) {
    std::tuple<ReadStream, HardwareAuthToken> result;
    ReadStream& in = std::get<0>(result) = in_;
    auto& hat = std::get<1>(result);
    constexpr size_t hatSize = hatSizeNoMac();
    ReadStream::ptr_t pos = nullptr;
    size_t read_size = 0;
    std::tie(in, pos, read_size) = read(in);
    if (!in || read_size != hatSize) {
        in.good_ = false;
        return result;
    }
    pos = copyField(hat.challenge, pos);
    pos = copyField(hat.userId, pos);
    pos = copyField(hat.authenticatorId, pos);
    pos = copyField(hat.authenticatorType, pos);
    pos = copyField(hat.timestamp, pos);
    std::tie(in, hat.mac) = readSimpleHidlVecInPlace<uint8_t>(in);
    return result;
}

template <typename T>
inline volatile uint8_t* copyField(const T& field, volatile uint8_t*(&pos)) {
    auto& s = bytes_cast(field);
    return std::copy(s, &s[sizeof(T)], pos);
}

inline WriteStream write(WriteStream out, const HardwareAuthToken& v) {
    auto pos = out.pos();
    uint32_t size_field = hatSizeNoMac();
    out += 4 + size_field;
    if (!out) return out;
    pos = copyField(size_field, pos);
    pos = copyField(v.challenge, pos);
    pos = copyField(v.userId, pos);
    pos = copyField(v.authenticatorId, pos);
    pos = copyField(v.authenticatorType, pos);
    pos = copyField(v.timestamp, pos);
    return writeSimpleHidlVec(out, v.mac);
}

// ResponseCode
inline std::tuple<ReadStream, ResponseCode> read(Message<ResponseCode>, ReadStream in) {
    return readSimpleType<ResponseCode>(in);
}
inline WriteStream write(WriteStream out, const ResponseCode& v) {
    return write(out, bytes_cast(v));
}

// hidl_vec<uint8_t>
inline std::tuple<ReadStream, hidl_vec<uint8_t>> read(Message<hidl_vec<uint8_t>>, ReadStream in) {
    return readSimpleHidlVecInPlace<uint8_t>(in);
}
inline WriteStream write(WriteStream out, const hidl_vec<uint8_t>& v) {
    return writeSimpleHidlVec(out, v);
}

// hidl_vec<UIOption>
inline std::tuple<ReadStream, hidl_vec<UIOption>> read(Message<hidl_vec<UIOption>>, ReadStream in) {
    in = unalign(in);
    return readSimpleHidlVecInPlace<UIOption>(in);
}
inline WriteStream write(WriteStream out, const hidl_vec<UIOption>& v) {
    out = unalign(out);
    return writeSimpleHidlVec(out, v);
}

// hidl_string
inline std::tuple<ReadStream, hidl_string> read(Message<hidl_string>, ReadStream in) {
    std::tuple<ReadStream, hidl_string> result;
    ReadStream& in_ = std::get<0>(result);
    hidl_string& result_ = std::get<1>(result);
    ReadStream::ptr_t pos = nullptr;
    size_t read_size = 0;
    std::tie(in_, pos, read_size) = read(in);
    auto terminating_zero = in_.pos();
    ++in_;  // skip the terminating zero. Does nothing if the stream was already bad
    if (!in_) return result;
    if (*terminating_zero) {
        in_.good_ = false;
        return result;
    }
    result_.setToExternal(reinterpret_cast<const char*>(const_cast<const uint8_t*>(pos)),
                          read_size);
    return result;
}
inline WriteStream write(WriteStream out, const hidl_string& v) {
    out = write(out, reinterpret_cast<const uint8_t*>(v.c_str()), v.size());
    auto terminating_zero = out.pos();
    ++out;
    if (out) {
        *terminating_zero = 0;
    }
    return out;
}

inline WriteStream write(WriteStream out, Command cmd) {
    volatile Command* pos = reinterpret_cast<volatile Command*>(out.pos_);
    out += sizeof(Command);
    if (out) {
        *pos = cmd;
    }
    return out;
}
template <Command cmd>
WriteStream write(WriteStream out, Cmd<cmd>) {
    return write(out, cmd);
}

inline std::tuple<ReadStream, bool> read(ReadStream in, Command cmd) {
    volatile const Command* pos = reinterpret_cast<volatile const Command*>(in.pos_);
    in += sizeof(Command);
    if (!in) return {in, false};
    return {in, *pos == cmd};
}

template <Command cmd>
std::tuple<ReadStream, bool> read(Message<Cmd<cmd>>, ReadStream in) {
    return read(in, cmd);
}

inline WriteStream write(Message<>, WriteStream out) {
    return out;
}

template <typename Head, typename... Tail>
WriteStream write(Message<Head, Tail...>, WriteStream out, const Head& head, const Tail&... tail) {
    out = write(out, head);
    return write(Message<Tail...>(), out, tail...);
}

template <Command cmd, typename... Tail>
WriteStream write(Message<Cmd<cmd>, Tail...>, WriteStream out, const Tail&... tail) {
    out = write(out, cmd);
    return write(Message<Tail...>(), out, tail...);
}

template <Command cmd, typename HEAD, typename... Tail>
std::tuple<ReadStream, bool, HEAD, Tail...> read(Message<Cmd<cmd>, HEAD, Tail...>, ReadStream in) {
    bool command_matches;
    std::tie(in, command_matches) = read(in, cmd);
    if (!command_matches) return {in, false, HEAD(), Tail()...};

    return {in, true,
            [&]() -> HEAD {
                HEAD result;
                std::tie(in, result) = read(Message<HEAD>(), in);
                return result;
            }(),
            [&]() -> Tail {
                Tail result;
                std::tie(in, result) = read(Message<Tail>(), in);
                return result;
            }()...};
}

template <typename... Msg>
std::tuple<ReadStream, Msg...> read(Message<Msg...>, ReadStream in) {
    return {in, [&in]() -> Msg {
                Msg result;
                std::tie(in, result) = read(Message<Msg>(), in);
                return result;
            }()...};
}

template <typename T>
struct msg2tuple {};

template <typename... T>
struct msg2tuple<Message<T...>> {
    using type = std::tuple<T...>;
};
template <Command cmd, typename... T>
struct msg2tuple<Message<Cmd<cmd>, T...>> {
    using type = std::tuple<T...>;
};

template <typename T>
using msg2tuple_t = typename msg2tuple<T>::type;

template <size_t... idx, typename HEAD, typename... T>
std::tuple<T&&...> tuple_tail(IntegerSequence<idx...>, std::tuple<HEAD, T...>&& t) {
    return {std::move(std::get<idx>(t))...};
}

template <size_t... idx, typename HEAD, typename... T>
std::tuple<const T&...> tuple_tail(IntegerSequence<idx...>, const std::tuple<HEAD, T...>& t) {
    return {std::get<idx>(t)...};
}

template <typename HEAD, typename... Tail>
std::tuple<Tail&&...> tuple_tail(std::tuple<HEAD, Tail...>&& t) {
    return tuple_tail(integer_sequence::make_t<1, sizeof...(Tail)>(), std::move(t));
}

template <typename HEAD, typename... Tail>
std::tuple<const Tail&...> tuple_tail(const std::tuple<HEAD, Tail...>& t) {
    return tuple_tail(integer_sequence::make_t<1, sizeof...(Tail)>(), t);
}

}  // namespace support
}  // namespace confirmationui
}  // namespace hardware
}  // namespace android

#endif  // CONFIRMATIONUI_SUPPORT_INCLUDE_ANDROID_HARDWARE_CONFIRMATIONUI_SUPPORT_MSG_FORMATTING_H_
