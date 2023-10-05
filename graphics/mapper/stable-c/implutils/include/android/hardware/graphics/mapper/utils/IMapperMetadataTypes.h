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

#include <aidl/android/hardware/graphics/common/BlendMode.h>
#include <aidl/android/hardware/graphics/common/BufferUsage.h>
#include <aidl/android/hardware/graphics/common/Cta861_3.h>
#include <aidl/android/hardware/graphics/common/Dataspace.h>
#include <aidl/android/hardware/graphics/common/ExtendableType.h>
#include <aidl/android/hardware/graphics/common/PixelFormat.h>
#include <aidl/android/hardware/graphics/common/PlaneLayout.h>
#include <aidl/android/hardware/graphics/common/PlaneLayoutComponent.h>
#include <aidl/android/hardware/graphics/common/Rect.h>
#include <aidl/android/hardware/graphics/common/Smpte2086.h>
#include <aidl/android/hardware/graphics/common/StandardMetadataType.h>
#include <aidl/android/hardware/graphics/common/XyColor.h>
#include <android/hardware/graphics/mapper/IMapper.h>

#include <cinttypes>
#include <string_view>
#include <type_traits>
#include <vector>

namespace android::hardware::graphics::mapper {

using ::aidl::android::hardware::graphics::common::BlendMode;
using ::aidl::android::hardware::graphics::common::BufferUsage;
using ::aidl::android::hardware::graphics::common::Cta861_3;
using ::aidl::android::hardware::graphics::common::Dataspace;
using ::aidl::android::hardware::graphics::common::ExtendableType;
using ::aidl::android::hardware::graphics::common::PixelFormat;
using ::aidl::android::hardware::graphics::common::PlaneLayout;
using ::aidl::android::hardware::graphics::common::PlaneLayoutComponent;
using ::aidl::android::hardware::graphics::common::Rect;
using ::aidl::android::hardware::graphics::common::Smpte2086;
using ::aidl::android::hardware::graphics::common::StandardMetadataType;
using ::aidl::android::hardware::graphics::common::XyColor;

class MetadataWriter {
  private:
    uint8_t* _Nonnull mDest;
    size_t mSizeRemaining = 0;
    int32_t mDesiredSize = 0;

    void* _Nullable reserve(size_t sizeToWrite) {
        if (mDesiredSize < 0) {
            // Error state
            return nullptr;
        }
        if (__builtin_add_overflow(mDesiredSize, sizeToWrite, &mDesiredSize)) {
            // Overflowed, abort writing any further data
            mDesiredSize = -AIMAPPER_ERROR_BAD_VALUE;
            mSizeRemaining = 0;
            return nullptr;
        }
        if (sizeToWrite > mSizeRemaining) {
            mSizeRemaining = 0;
            return nullptr;
        } else {
            mSizeRemaining -= sizeToWrite;
            uint8_t* whereToWrite = mDest;
            mDest += sizeToWrite;
            return whereToWrite;
        }
    }

  public:
    explicit MetadataWriter(void* _Nullable destBuffer, size_t destBufferSize)
        : mDest(reinterpret_cast<uint8_t*>(destBuffer)), mSizeRemaining(destBufferSize) {}

    [[nodiscard]] int32_t desiredSize() const { return mDesiredSize; }

    template <typename HEADER>
    MetadataWriter& writeHeader() {
        return write(HEADER::name).template write<int64_t>(HEADER::value);
    }

    template <typename T, typename = std::enable_if_t<std::is_integral_v<T>>>
    MetadataWriter& write(T value) {
        auto sizeToWrite = sizeof(T);
        if (void* dest = reserve(sizeToWrite)) {
            memcpy(dest, &value, sizeToWrite);
        }
        return *this;
    }

    MetadataWriter& write(float value) {
        auto sizeToWrite = sizeof(float);
        if (void* dest = reserve(sizeToWrite)) {
            memcpy(dest, &value, sizeToWrite);
        }
        return *this;
    }

    MetadataWriter& write(const std::string_view& value) {
        auto sizeToWrite = value.length();
        write<int64_t>(sizeToWrite);
        if (void* dest = reserve(sizeToWrite)) {
            memcpy(dest, value.data(), sizeToWrite);
        }
        return *this;
    }

    MetadataWriter& write(const std::vector<uint8_t>& value) {
        auto sizeToWrite = value.size();
        write<int64_t>(sizeToWrite);
        if (void* dest = reserve(sizeToWrite)) {
            memcpy(dest, value.data(), sizeToWrite);
        }
        return *this;
    }

    MetadataWriter& write(const ExtendableType& value) {
        return write(value.name).write(value.value);
    }

    MetadataWriter& write(const XyColor& value) { return write(value.x).write(value.y); }
};

class MetadataReader {
  private:
    const uint8_t* _Nonnull mSrc;
    size_t mSizeRemaining = 0;
    bool mOk = true;

    const void* _Nullable advance(size_t size) {
        if (mOk && mSizeRemaining >= size) {
            const void* buf = mSrc;
            mSrc += size;
            mSizeRemaining -= size;
            return buf;
        }
        mOk = false;
        return nullptr;
    }

  public:
    explicit MetadataReader(const void* _Nonnull metadata, size_t metadataSize)
        : mSrc(reinterpret_cast<const uint8_t*>(metadata)), mSizeRemaining(metadataSize) {}

    [[nodiscard]] size_t remaining() const { return mSizeRemaining; }
    [[nodiscard]] bool ok() const { return mOk; }

    template <typename HEADER>
    MetadataReader& checkHeader() {
        if (HEADER::name != readString()) {
            mOk = false;
        }
        auto value = readInt<int64_t>();
        if (!value || *value != HEADER::value) {
            mOk = false;
        }
        return *this;
    }

    template <typename T, typename = std::enable_if_t<std::is_integral_v<T>>>
    MetadataReader& read(T& dest) {
        if (const void* src = advance(sizeof(T))) {
            memcpy(&dest, src, sizeof(T));
        }
        return *this;
    }

    MetadataReader& read(float& dest) {
        if (const void* src = advance(sizeof(float))) {
            memcpy(&dest, src, sizeof(float));
        }
        return *this;
    }

    MetadataReader& read(std::string& dest) {
        dest = readString();
        return *this;
    }

    MetadataReader& read(ExtendableType& dest) {
        dest.name = readString();
        read(dest.value);
        return *this;
    }

    MetadataReader& read(XyColor& dest) {
        read(dest.x);
        read(dest.y);
        return *this;
    }

    template <typename T, typename = std::enable_if_t<std::is_integral_v<T>>>
    [[nodiscard]] std::optional<T> readInt() {
        auto sizeToRead = sizeof(T);
        if (const void* src = advance(sizeof(T))) {
            T ret;
            memcpy(&ret, src, sizeToRead);
            return ret;
        }
        return std::nullopt;
    }

    [[nodiscard]] std::string_view readString() {
        auto lengthOpt = readInt<int64_t>();
        if (!lengthOpt) {
            return std::string_view{};
        }
        size_t length = lengthOpt.value();
        if (const void* src = advance(length)) {
            return std::string_view{reinterpret_cast<const char*>(src), length};
        }
        return std::string_view{};
    }

    [[nodiscard]] std::optional<ExtendableType> readExtendable() {
        ExtendableType ret;
        ret.name = readString();
        auto value = readInt<int64_t>();
        if (value) {
            ret.value = value.value();
            return ret;
        } else {
            return std::nullopt;
        }
    }

    [[nodiscard]] std::vector<uint8_t> readBuffer() {
        std::vector<uint8_t> ret;
        size_t length = readInt<int64_t>().value_or(0);
        if (const void* src = advance(length)) {
            ret.resize(length);
            memcpy(ret.data(), src, length);
        }
        return ret;
    }
};

template <typename HEADER, typename T, class Enable = void>
struct MetadataValue {};

template <typename HEADER, typename T>
struct MetadataValue<HEADER, T, std::enable_if_t<std::is_integral_v<T>>> {
    [[nodiscard]] static int32_t encode(T value, void* _Nullable destBuffer,
                                        size_t destBufferSize) {
        return MetadataWriter{destBuffer, destBufferSize}
                .template writeHeader<HEADER>()
                .write(value)
                .desiredSize();
    }

    [[nodiscard]] static std::optional<T> decode(const void* _Nonnull metadata,
                                                 size_t metadataSize) {
        return MetadataReader{metadata, metadataSize}
                .template checkHeader<HEADER>()
                .template readInt<T>();
    }
};

template <typename HEADER, typename T>
struct MetadataValue<HEADER, T, std::enable_if_t<std::is_enum_v<T>>> {
    [[nodiscard]] static int32_t encode(T value, void* _Nullable destBuffer,
                                        size_t destBufferSize) {
        return MetadataWriter{destBuffer, destBufferSize}
                .template writeHeader<HEADER>()
                .write(static_cast<std::underlying_type_t<T>>(value))
                .desiredSize();
    }

    [[nodiscard]] static std::optional<T> decode(const void* _Nonnull metadata,
                                                 size_t metadataSize) {
        std::underlying_type_t<T> temp;
        return MetadataReader{metadata, metadataSize}.template checkHeader<HEADER>().read(temp).ok()
                       ? std::optional<T>(static_cast<T>(temp))
                       : std::nullopt;
    }
};

template <typename HEADER>
struct MetadataValue<HEADER, std::string> {
    [[nodiscard]] static int32_t encode(const std::string_view& value, void* _Nullable destBuffer,
                                        size_t destBufferSize) {
        return MetadataWriter{destBuffer, destBufferSize}
                .template writeHeader<HEADER>()
                .write(value)
                .desiredSize();
    }

    [[nodiscard]] static std::optional<std::string> decode(const void* _Nonnull metadata,
                                                           size_t metadataSize) {
        auto reader = MetadataReader{metadata, metadataSize}.template checkHeader<HEADER>();
        auto result = reader.readString();
        return reader.ok() ? std::optional<std::string>{result} : std::nullopt;
    }
};

template <typename HEADER>
struct MetadataValue<HEADER, ExtendableType> {
    static_assert(sizeof(int64_t) == sizeof(ExtendableType::value));

    [[nodiscard]] static int32_t encode(const ExtendableType& value, void* _Nullable destBuffer,
                                        size_t destBufferSize) {
        return MetadataWriter{destBuffer, destBufferSize}
                .template writeHeader<HEADER>()
                .write(value)
                .desiredSize();
    }

    [[nodiscard]] static std::optional<ExtendableType> decode(const void* _Nonnull metadata,
                                                              size_t metadataSize) {
        return MetadataReader{metadata, metadataSize}
                .template checkHeader<HEADER>()
                .readExtendable();
    }
};

template <typename HEADER>
struct MetadataValue<HEADER, std::vector<PlaneLayout>> {
    [[nodiscard]] static int32_t encode(const std::vector<PlaneLayout>& values,
                                        void* _Nullable destBuffer, size_t destBufferSize) {
        MetadataWriter writer{destBuffer, destBufferSize};
        writer.template writeHeader<HEADER>();
        writer.write<int64_t>(values.size());
        for (const auto& value : values) {
            writer.write<int64_t>(value.components.size());
            for (const auto& component : value.components) {
                writer.write(component.type)
                        .write<int64_t>(component.offsetInBits)
                        .write<int64_t>(component.sizeInBits);
            }
            writer.write<int64_t>(value.offsetInBytes)
                    .write<int64_t>(value.sampleIncrementInBits)
                    .write<int64_t>(value.strideInBytes)
                    .write<int64_t>(value.widthInSamples)
                    .write<int64_t>(value.heightInSamples)
                    .write<int64_t>(value.totalSizeInBytes)
                    .write<int64_t>(value.horizontalSubsampling)
                    .write<int64_t>(value.verticalSubsampling);
        }
        return writer.desiredSize();
    }

    using DecodeResult = std::optional<std::vector<PlaneLayout>>;
    [[nodiscard]] static DecodeResult decode(const void* _Nonnull metadata, size_t metadataSize) {
        std::vector<PlaneLayout> values;
        MetadataReader reader{metadata, metadataSize};
        reader.template checkHeader<HEADER>();
        auto numPlanes = reader.readInt<int64_t>().value_or(0);
        values.reserve(numPlanes);
        for (int i = 0; i < numPlanes && reader.ok(); i++) {
            PlaneLayout& value = values.emplace_back();
            auto numPlaneComponents = reader.readInt<int64_t>().value_or(0);
            value.components.reserve(numPlaneComponents);
            for (int j = 0; j < numPlaneComponents && reader.ok(); j++) {
                PlaneLayoutComponent& component = value.components.emplace_back();
                reader.read(component.type)
                        .read<int64_t>(component.offsetInBits)
                        .read<int64_t>(component.sizeInBits);
            }
            reader.read<int64_t>(value.offsetInBytes)
                    .read<int64_t>(value.sampleIncrementInBits)
                    .read<int64_t>(value.strideInBytes)
                    .read<int64_t>(value.widthInSamples)
                    .read<int64_t>(value.heightInSamples)
                    .read<int64_t>(value.totalSizeInBytes)
                    .read<int64_t>(value.horizontalSubsampling)
                    .read<int64_t>(value.verticalSubsampling);
        }
        return reader.ok() ? DecodeResult{std::move(values)} : std::nullopt;
    }
};

template <typename HEADER>
struct MetadataValue<HEADER, std::vector<Rect>> {
    [[nodiscard]] static int32_t encode(const std::vector<Rect>& value, void* _Nullable destBuffer,
                                        size_t destBufferSize) {
        MetadataWriter writer{destBuffer, destBufferSize};
        writer.template writeHeader<HEADER>();
        writer.write<int64_t>(value.size());
        for (auto& rect : value) {
            writer.write<int32_t>(rect.left)
                    .write<int32_t>(rect.top)
                    .write<int32_t>(rect.right)
                    .write<int32_t>(rect.bottom);
        }
        return writer.desiredSize();
    }

    using DecodeResult = std::optional<std::vector<Rect>>;
    [[nodiscard]] static DecodeResult decode(const void* _Nonnull metadata, size_t metadataSize) {
        MetadataReader reader{metadata, metadataSize};
        reader.template checkHeader<HEADER>();
        std::vector<Rect> value;
        auto numRects = reader.readInt<int64_t>().value_or(0);
        value.reserve(numRects);
        for (int i = 0; i < numRects && reader.ok(); i++) {
            Rect& rect = value.emplace_back();
            reader.read<int32_t>(rect.left)
                    .read<int32_t>(rect.top)
                    .read<int32_t>(rect.right)
                    .read<int32_t>(rect.bottom);
        }
        return reader.ok() ? DecodeResult{std::move(value)} : std::nullopt;
    }
};

template <typename HEADER>
struct MetadataValue<HEADER, std::optional<Smpte2086>> {
    [[nodiscard]] static int32_t encode(const std::optional<Smpte2086>& optValue,
                                        void* _Nullable destBuffer, size_t destBufferSize) {
        if (optValue.has_value()) {
            const auto& value = *optValue;
            return MetadataWriter{destBuffer, destBufferSize}
                    .template writeHeader<HEADER>()
                    .write(value.primaryRed)
                    .write(value.primaryGreen)
                    .write(value.primaryBlue)
                    .write(value.whitePoint)
                    .write(value.maxLuminance)
                    .write(value.minLuminance)
                    .desiredSize();
        } else {
            return 0;
        }
    }

    // Double optional because the value type itself is an optional<>
    using DecodeResult = std::optional<std::optional<Smpte2086>>;
    [[nodiscard]] static DecodeResult decode(const void* _Nullable metadata, size_t metadataSize) {
        std::optional<Smpte2086> optValue{std::nullopt};
        if (metadataSize > 0) {
            Smpte2086 value;
            MetadataReader reader{metadata, metadataSize};
            reader.template checkHeader<HEADER>();
            reader.read(value.primaryRed)
                    .read(value.primaryGreen)
                    .read(value.primaryBlue)
                    .read(value.whitePoint)
                    .read(value.maxLuminance)
                    .read(value.minLuminance);
            if (reader.ok()) {
                optValue = std::move(value);
            } else {
                return std::nullopt;
            }
        }
        return DecodeResult{std::move(optValue)};
    }
};

template <typename HEADER>
struct MetadataValue<HEADER, std::optional<Cta861_3>> {
    [[nodiscard]] static int32_t encode(const std::optional<Cta861_3>& optValue,
                                        void* _Nullable destBuffer, size_t destBufferSize) {
        if (optValue.has_value()) {
            const auto& value = *optValue;
            return MetadataWriter{destBuffer, destBufferSize}
                    .template writeHeader<HEADER>()
                    .write(value.maxContentLightLevel)
                    .write(value.maxFrameAverageLightLevel)
                    .desiredSize();
        } else {
            return 0;
        }
    }

    // Double optional because the value type itself is an optional<>
    using DecodeResult = std::optional<std::optional<Cta861_3>>;
    [[nodiscard]] static DecodeResult decode(const void* _Nullable metadata, size_t metadataSize) {
        std::optional<Cta861_3> optValue{std::nullopt};
        if (metadataSize > 0) {
            MetadataReader reader{metadata, metadataSize};
            reader.template checkHeader<HEADER>();
            Cta861_3 value;
            reader.read(value.maxContentLightLevel).read(value.maxFrameAverageLightLevel);
            if (reader.ok()) {
                optValue = std::move(value);
            } else {
                return std::nullopt;
            }
        }
        return DecodeResult{std::move(optValue)};
    }
};

template <typename HEADER>
struct MetadataValue<HEADER, std::optional<std::vector<uint8_t>>> {
    [[nodiscard]] static int32_t encode(const std::optional<std::vector<uint8_t>>& value,
                                        void* _Nullable destBuffer, size_t destBufferSize) {
        if (!value.has_value()) {
            return 0;
        }
        return MetadataWriter{destBuffer, destBufferSize}
                .template writeHeader<HEADER>()
                .write(*value)
                .desiredSize();
    }

    using DecodeResult = std::optional<std::optional<std::vector<uint8_t>>>;
    [[nodiscard]] static DecodeResult decode(const void* _Nonnull metadata, size_t metadataSize) {
        std::optional<std::vector<uint8_t>> optValue;
        if (metadataSize > 0) {
            MetadataReader reader{metadata, metadataSize};
            reader.template checkHeader<HEADER>();
            auto value = reader.readBuffer();
            if (reader.ok()) {
                optValue = std::move(value);
            } else {
                return std::nullopt;
            }
        }
        return DecodeResult{std::move(optValue)};
    }
};

template <StandardMetadataType>
struct StandardMetadata {};

#define DEFINE_TYPE(typeName, typeArg)                                                            \
    template <>                                                                                   \
    struct StandardMetadata<StandardMetadataType::typeName> {                                     \
        using value_type = typeArg;                                                               \
        struct Header {                                                                           \
            static constexpr auto name = "android.hardware.graphics.common.StandardMetadataType"; \
            static constexpr auto value = static_cast<int64_t>(StandardMetadataType::typeName);   \
        };                                                                                        \
        using value = MetadataValue<Header, value_type>;                                          \
        static_assert(                                                                            \
                StandardMetadataType::typeName ==                                                 \
                        ndk::internal::enum_values<StandardMetadataType>[static_cast<size_t>(     \
                                StandardMetadataType::typeName)],                                 \
                "StandardMetadataType must have equivalent value to index");                      \
    }

DEFINE_TYPE(BUFFER_ID, uint64_t);
DEFINE_TYPE(NAME, std::string);
DEFINE_TYPE(WIDTH, uint64_t);
DEFINE_TYPE(HEIGHT, uint64_t);
DEFINE_TYPE(LAYER_COUNT, uint64_t);
DEFINE_TYPE(PIXEL_FORMAT_REQUESTED, PixelFormat);
DEFINE_TYPE(PIXEL_FORMAT_FOURCC, uint32_t);
DEFINE_TYPE(PIXEL_FORMAT_MODIFIER, uint64_t);
DEFINE_TYPE(USAGE, BufferUsage);
DEFINE_TYPE(ALLOCATION_SIZE, uint64_t);
DEFINE_TYPE(PROTECTED_CONTENT, uint64_t);
DEFINE_TYPE(COMPRESSION, ExtendableType);
DEFINE_TYPE(INTERLACED, ExtendableType);
DEFINE_TYPE(CHROMA_SITING, ExtendableType);
DEFINE_TYPE(PLANE_LAYOUTS, std::vector<PlaneLayout>);
DEFINE_TYPE(CROP, std::vector<Rect>);
DEFINE_TYPE(DATASPACE, Dataspace);
DEFINE_TYPE(BLEND_MODE, BlendMode);
DEFINE_TYPE(SMPTE2086, std::optional<Smpte2086>);
DEFINE_TYPE(CTA861_3, std::optional<Cta861_3>);
DEFINE_TYPE(SMPTE2094_10, std::optional<std::vector<uint8_t>>);
DEFINE_TYPE(SMPTE2094_40, std::optional<std::vector<uint8_t>>);
DEFINE_TYPE(STRIDE, uint32_t);

#undef DEFINE_TYPE

#if defined(__cplusplus) && __cplusplus >= 202002L

template <typename F, std::size_t... I>
void invokeWithStandardMetadata(F&& f, StandardMetadataType type, std::index_sequence<I...>) {
    // Setup the jump table, mapping from each type to a springboard that invokes the template
    // function with the appropriate concrete type
    using F_PTR = decltype(&f);
    using THUNK = void (*)(F_PTR);
    static constexpr auto jump = std::array<THUNK, sizeof...(I)>{[](F_PTR fp) {
        constexpr StandardMetadataType type = ndk::internal::enum_values<StandardMetadataType>[I];
        if constexpr (type != StandardMetadataType::INVALID) {
            (*fp)(StandardMetadata<type>{});
        }
    }...};

    auto index = static_cast<size_t>(type);
    if (index >= 0 && index < jump.size()) {
        jump[index](&f);
    }
}

template <typename F, typename StandardMetadataSequence = std::make_index_sequence<
                              ndk::internal::enum_values<StandardMetadataType>.size()>>
int32_t provideStandardMetadata(StandardMetadataType type, void* _Nullable destBuffer,
                                size_t destBufferSize, F&& f) {
    int32_t retVal = -AIMAPPER_ERROR_UNSUPPORTED;
    invokeWithStandardMetadata(
            [&]<StandardMetadataType T>(StandardMetadata<T>) {
                retVal = f.template operator()<T>(
                        [&](const typename StandardMetadata<T>::value_type& value) -> int32_t {
                            return StandardMetadata<T>::value::encode(value, destBuffer,
                                                                      destBufferSize);
                        });
            },
            type, StandardMetadataSequence{});
    return retVal;
}

template <typename F, typename StandardMetadataSequence = std::make_index_sequence<
                              ndk::internal::enum_values<StandardMetadataType>.size()>>
AIMapper_Error applyStandardMetadata(StandardMetadataType type, const void* _Nonnull metadata,
                                     size_t metadataSize, F&& f) {
    AIMapper_Error retVal = AIMAPPER_ERROR_UNSUPPORTED;
    invokeWithStandardMetadata(
            [&]<StandardMetadataType T>(StandardMetadata<T>) {
                auto value = StandardMetadata<T>::value::decode(metadata, metadataSize);
                if (value.has_value()) {
                    retVal = f.template operator()<T>(std::move(*value));
                } else {
                    retVal = AIMAPPER_ERROR_BAD_VALUE;
                }
            },
            type, StandardMetadataSequence{});
    return retVal;
}

#endif

}  // namespace android::hardware::graphics::mapper