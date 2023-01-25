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

#include <gtest/gtest.h>

#include <android/hardware/graphics/mapper/utils/IMapperMetadataTypes.h>
#include <android/hardware/graphics/mapper/utils/IMapperProvider.h>
#include <drm/drm_fourcc.h>
#include <gralloctypes/Gralloc4.h>
#include <span>
#include <vector>

using namespace ::android;
using namespace ::android::hardware::graphics::mapper;
using namespace ::aidl::android::hardware::graphics::common;
namespace gralloc4 = ::android::gralloc4;
using ::android::hardware::hidl_vec;

// These tests are primarily interested in hitting all the different *types* that can be
// serialized/deserialized than in exhaustively testing all the StandardMetadataTypes.
// Exhaustive testing of the actual metadata types is relegated for IMapper's VTS suite
// where meaning & correctness of values are more narrowly defined (eg, read-only values)

static constexpr auto HeaderSize = 69;

static std::span<uint8_t> SkipHeader(std::vector<uint8_t>& buffer) {
    return std::span<uint8_t>(buffer).subspan(HeaderSize);
}

static std::vector<PlaneLayout> fakePlaneLayouts() {
    PlaneLayout myPlaneLayout;
    myPlaneLayout.offsetInBytes = 10;
    myPlaneLayout.sampleIncrementInBits = 11;
    myPlaneLayout.strideInBytes = 12;
    myPlaneLayout.widthInSamples = 13;
    myPlaneLayout.heightInSamples = 14;
    myPlaneLayout.totalSizeInBytes = 15;
    myPlaneLayout.horizontalSubsampling = 16;
    myPlaneLayout.verticalSubsampling = 17;

    myPlaneLayout.components.resize(3);
    for (int i = 0; i < myPlaneLayout.components.size(); i++) {
        auto& it = myPlaneLayout.components[i];
        it.type = ExtendableType{"Plane ID", 40 + i};
        it.offsetInBits = 20 + i;
        it.sizeInBits = 30 + i;
    }

    return std::vector<PlaneLayout>{myPlaneLayout, PlaneLayout{}};
}

TEST(Metadata, setGetBufferId) {
    using BufferId = StandardMetadata<StandardMetadataType::BUFFER_ID>::value;

    std::vector<uint8_t> buffer(10000, 0);
    int64_t* payload = reinterpret_cast<int64_t*>(SkipHeader(buffer).data());
    *payload = 42;

    EXPECT_EQ(8 + HeaderSize, BufferId::encode(18, buffer.data(), 0));
    EXPECT_EQ(42, *payload);
    EXPECT_EQ(8 + HeaderSize, BufferId::encode(18, buffer.data(), buffer.size()));
    EXPECT_EQ(18, *payload);
    EXPECT_FALSE(BufferId::decode(buffer.data(), 0));
    auto read = BufferId::decode(buffer.data(), buffer.size());
    EXPECT_TRUE(read.has_value());
    EXPECT_EQ(18, read.value_or(0));
}

TEST(Metadata, setGetDataspace) {
    using DataspaceValue = StandardMetadata<StandardMetadataType::DATASPACE>::value;
    using intType = std::underlying_type_t<Dataspace>;
    std::vector<uint8_t> buffer(10000, 0);
    auto data = SkipHeader(buffer);

    EXPECT_EQ(4 + HeaderSize, DataspaceValue::encode(Dataspace::BT2020, buffer.data(), 0));
    EXPECT_EQ(0, *reinterpret_cast<intType*>(data.data()));
    EXPECT_EQ(4 + HeaderSize,
              DataspaceValue::encode(Dataspace::BT2020, buffer.data(), buffer.size()));
    EXPECT_EQ(static_cast<intType>(Dataspace::BT2020), *reinterpret_cast<intType*>(data.data()));
    EXPECT_FALSE(DataspaceValue::decode(buffer.data(), 0));
    auto read = DataspaceValue::decode(buffer.data(), buffer.size());
    ASSERT_TRUE(read.has_value());
    EXPECT_EQ(Dataspace::BT2020, *read);
}

TEST(Metadata, setGetValidName) {
    using NameValue = StandardMetadata<StandardMetadataType::NAME>::value;

    std::vector<uint8_t> buffer(10000, 'a');

    // len("Hello") + sizeof(int64)
    constexpr int expectedSize = 5 + sizeof(int64_t) + HeaderSize;
    EXPECT_EQ(expectedSize, NameValue::encode("Hello", buffer.data(), buffer.size()));
    EXPECT_EQ(5, *reinterpret_cast<int64_t*>(SkipHeader(buffer).data()));
    // Verify didn't write past the end of the desired size
    EXPECT_EQ('a', buffer[expectedSize]);

    auto readValue = NameValue::decode(buffer.data(), buffer.size());
    ASSERT_TRUE(readValue.has_value());
    EXPECT_EQ(5, readValue->length());
    EXPECT_EQ("Hello", *readValue);
}

TEST(Metadata, setGetInvalidName) {
    using NameValue = StandardMetadata<StandardMetadataType::NAME>::value;

    std::vector<uint8_t> buffer;
    buffer.resize(12 + HeaderSize, 'a');
    buffer[buffer.size() - 1] = '\0';

    // len("This is a long string") + sizeof(int64)
    constexpr int expectedSize = 21 + sizeof(int64_t) + HeaderSize;
    EXPECT_EQ(expectedSize,
              NameValue::encode("This is a long string", buffer.data(), buffer.size()));
    EXPECT_EQ(21, *reinterpret_cast<int64_t*>(SkipHeader(buffer).data()));

    auto readValue = NameValue::decode(buffer.data(), buffer.size());
    EXPECT_FALSE(readValue.has_value());
    readValue = NameValue::decode(buffer.data(), 0);
    ASSERT_FALSE(readValue.has_value());
}

TEST(Metadata, wouldOverflowName) {
    using NameValue = StandardMetadata<StandardMetadataType::NAME>::value;
    std::vector<uint8_t> buffer(10000, 0);

    // int_max + sizeof(int64) overflows int32
    std::string_view bad_string{"badbeef", std::numeric_limits<int32_t>::max()};
    EXPECT_EQ(-AIMAPPER_ERROR_BAD_VALUE,
              NameValue::encode(bad_string, buffer.data(), buffer.size()));

    // check barely overflows
    bad_string = std::string_view{"badbeef", std::numeric_limits<int32_t>::max() - 7};
    EXPECT_EQ(-AIMAPPER_ERROR_BAD_VALUE,
              NameValue::encode(bad_string, buffer.data(), buffer.size()));
}

TEST(Metadata, setGetMismatchedWidthHight) {
    // Validates that the header is properly validated on decode
    using WidthValue = StandardMetadata<StandardMetadataType::WIDTH>::value;
    using HeightValue = StandardMetadata<StandardMetadataType::HEIGHT>::value;
    std::vector<uint8_t> buffer(10000, 0);

    EXPECT_EQ(8 + HeaderSize, WidthValue::encode(100, buffer.data(), buffer.size()));
    EXPECT_EQ(100, *reinterpret_cast<uint64_t*>(SkipHeader(buffer).data()));
    auto read = WidthValue::decode(buffer.data(), buffer.size());
    ASSERT_TRUE(read.has_value());
    EXPECT_EQ(100, *read);
    read = HeightValue::decode(buffer.data(), buffer.size());
    EXPECT_FALSE(read.has_value());
}

TEST(Metadata, setGetCompression) {
    using CompressionValue = StandardMetadata<StandardMetadataType::COMPRESSION>::value;
    ExtendableType myCompression{"bestest_compression_ever", 42};
    std::vector<uint8_t> buffer(10000, 0);
    const int expectedSize =
            myCompression.name.length() + sizeof(int64_t) + sizeof(int64_t) + HeaderSize;
    EXPECT_EQ(expectedSize, CompressionValue::encode(myCompression, buffer.data(), 0));
    EXPECT_EQ(0, buffer[0]);
    EXPECT_EQ(expectedSize, CompressionValue::encode(myCompression, buffer.data(), buffer.size()));
    EXPECT_EQ(myCompression.name.length(), *reinterpret_cast<int64_t*>(SkipHeader(buffer).data()));
    EXPECT_FALSE(CompressionValue::decode(buffer.data(), 0).has_value());
    auto read = CompressionValue::decode(buffer.data(), buffer.size());
    ASSERT_TRUE(read.has_value());
    EXPECT_EQ(myCompression, read.value());
}

TEST(Metadata, setGetPlaneLayout) {
    using PlaneLayoutValue = StandardMetadata<StandardMetadataType::PLANE_LAYOUTS>::value;

    std::vector<PlaneLayout> layouts = fakePlaneLayouts();

    std::vector<uint8_t> buffer(10000, 0);
    constexpr int componentSize = 8 + (4 * sizeof(int64_t));
    constexpr int firstLayoutSize = (8 + 1) * sizeof(int64_t) + (3 * componentSize);
    constexpr int secondLayoutSize = (8 + 1) * sizeof(int64_t);
    constexpr int expectedSize = firstLayoutSize + secondLayoutSize + sizeof(int64_t) + HeaderSize;
    EXPECT_EQ(expectedSize, PlaneLayoutValue::encode(layouts, buffer.data(), 0));
    EXPECT_EQ(0, buffer[0]);
    EXPECT_EQ(expectedSize, PlaneLayoutValue::encode(layouts, buffer.data(), buffer.size()));
    int64_t* payload = reinterpret_cast<int64_t*>(SkipHeader(buffer).data());
    EXPECT_EQ(3, payload[1]);
    EXPECT_EQ(8, payload[2]);
    EXPECT_EQ(40, payload[4]);
    EXPECT_EQ(31, payload[11]);
    EXPECT_EQ(22, payload[15]);
    EXPECT_EQ(10, payload[17]);
    EXPECT_EQ(11, payload[18]);
    EXPECT_FALSE(PlaneLayoutValue::decode(buffer.data(), 0).has_value());
    auto read = PlaneLayoutValue::decode(buffer.data(), buffer.size());
    ASSERT_TRUE(read.has_value());
    EXPECT_EQ(layouts, *read);
}

TEST(Metadata, setGetRects) {
    using RectsValue = StandardMetadata<StandardMetadataType::CROP>::value;
    std::vector<uint8_t> buffer(10000, 0);
    std::vector<Rect> cropRects{2};
    cropRects[0] = Rect{10, 11, 12, 13};
    cropRects[1] = Rect{20, 21, 22, 23};

    constexpr int expectedSize = sizeof(int64_t) + (8 * sizeof(int32_t)) + HeaderSize;
    EXPECT_EQ(expectedSize, RectsValue::encode(cropRects, buffer.data(), buffer.size()));
    EXPECT_EQ(2, reinterpret_cast<int64_t*>(SkipHeader(buffer).data())[0]);
    EXPECT_EQ(10, reinterpret_cast<int32_t*>(SkipHeader(buffer).data())[2]);
    auto read = RectsValue::decode(buffer.data(), buffer.size());
    ASSERT_TRUE(read.has_value());
    EXPECT_EQ(cropRects.size(), read->size());
    EXPECT_EQ(cropRects, *read);
}

TEST(Metadata, setGetSmpte2086) {
    using Smpte2086Value = StandardMetadata<StandardMetadataType::SMPTE2086>::value;
    Smpte2086 source;
    source.minLuminance = 12.335f;
    source.maxLuminance = 452.889f;
    source.whitePoint = XyColor{-6.f, -9.f};
    source.primaryRed = XyColor{.1f, .2f};
    source.primaryGreen = XyColor{.3f, .4f};
    source.primaryBlue = XyColor{.5f, .6f};

    constexpr int expectedSize = 10 * sizeof(float) + HeaderSize;
    std::vector<uint8_t> buffer(10000, 0);
    EXPECT_EQ(expectedSize, Smpte2086Value::encode(source, buffer.data(), buffer.size()));
    auto read = Smpte2086Value::decode(buffer.data(), buffer.size());
    ASSERT_TRUE(read.has_value());
    ASSERT_TRUE(read->has_value());
    EXPECT_EQ(source, read->value());

    // A valid encoding of a nullopt
    read = Smpte2086Value::decode(nullptr, 0);
    ASSERT_TRUE(read.has_value());
    EXPECT_FALSE(read->has_value());
}

TEST(Metadata, setGetCta861_3) {
    using Cta861_3Value = StandardMetadata<StandardMetadataType::CTA861_3>::value;
    Cta861_3 source;
    source.maxFrameAverageLightLevel = 244.55f;
    source.maxContentLightLevel = 202.202f;

    constexpr int expectedSize = 2 * sizeof(float) + HeaderSize;
    std::vector<uint8_t> buffer(10000, 0);
    EXPECT_EQ(expectedSize, Cta861_3Value::encode(source, buffer.data(), buffer.size()));
    auto read = Cta861_3Value::decode(buffer.data(), buffer.size());
    ASSERT_TRUE(read.has_value());
    ASSERT_TRUE(read->has_value());
    EXPECT_EQ(source, read->value());

    // A valid encoding of a nullopt
    read = Cta861_3Value::decode(nullptr, 0);
    ASSERT_TRUE(read.has_value());
    EXPECT_FALSE(read->has_value());
}

TEST(Metadata, setGetSmpte2094_10) {
    using SMPTE2094_10Value = StandardMetadata<StandardMetadataType::SMPTE2094_10>::value;

    std::vector<uint8_t> buffer(10000, 0);
    EXPECT_EQ(0, SMPTE2094_10Value::encode(std::nullopt, buffer.data(), buffer.size()));
    auto read = SMPTE2094_10Value::decode(buffer.data(), 0);
    ASSERT_TRUE(read.has_value());
    EXPECT_FALSE(read->has_value());

    const std::vector<uint8_t> emptyBuffer;
    EXPECT_EQ(sizeof(int64_t) + HeaderSize,
              SMPTE2094_10Value::encode(emptyBuffer, buffer.data(), buffer.size()));
    read = SMPTE2094_10Value::decode(buffer.data(), buffer.size());
    ASSERT_TRUE(read.has_value());
    ASSERT_TRUE(read->has_value());
    EXPECT_EQ(0, read->value().size());

    const std::vector<uint8_t> simpleBuffer{0, 1, 2, 3, 4, 5};
    EXPECT_EQ(sizeof(int64_t) + 6 + HeaderSize,
              SMPTE2094_10Value::encode(simpleBuffer, buffer.data(), buffer.size()));
    read = SMPTE2094_10Value::decode(buffer.data(), buffer.size());
    ASSERT_TRUE(read.has_value());
    ASSERT_TRUE(read->has_value());
    EXPECT_EQ(6, read->value().size());
    EXPECT_EQ(simpleBuffer, read->value());
}

TEST(MetadataProvider, bufferId) {
    using BufferId = StandardMetadata<StandardMetadataType::BUFFER_ID>::value;
    std::vector<uint8_t> buffer(10000, 0);
    int result = provideStandardMetadata(StandardMetadataType::BUFFER_ID, buffer.data(),
                                         buffer.size(), []<StandardMetadataType T>(auto&& provide) {
                                             if constexpr (T == StandardMetadataType::BUFFER_ID) {
                                                 return provide(42);
                                             }
                                             return 0;
                                         });

    EXPECT_EQ(8 + HeaderSize, result);
    auto read = BufferId::decode(buffer.data(), buffer.size());
    EXPECT_EQ(42, read.value_or(0));
}

TEST(MetadataProvider, allJumpsWork) {
    const auto& values = ndk::internal::enum_values<StandardMetadataType>;
    auto get = [](StandardMetadataType type) -> int {
        return provideStandardMetadata(type, nullptr, 0, []<StandardMetadataType T>(auto&&) {
            return static_cast<int>(T) + 100;
        });
    };

    for (auto& type : values) {
        const int expected = type == StandardMetadataType::INVALID ? -AIMAPPER_ERROR_UNSUPPORTED
                                                                   : static_cast<int>(type) + 100;
        EXPECT_EQ(expected, get(type));
    }
}

TEST(MetadataProvider, invalid) {
    int result = provideStandardMetadata(StandardMetadataType::INVALID, nullptr, 0,
                                         []<StandardMetadataType T>(auto&&) { return 10; });

    EXPECT_EQ(-AIMAPPER_ERROR_UNSUPPORTED, result);
}

TEST(MetadataProvider, outOfBounds) {
    int result = provideStandardMetadata(static_cast<StandardMetadataType>(-1), nullptr, 0,
                                         []<StandardMetadataType T>(auto&&) { return 10; });
    EXPECT_EQ(-AIMAPPER_ERROR_UNSUPPORTED, result) << "-1 should have resulted in UNSUPPORTED";

    result = provideStandardMetadata(static_cast<StandardMetadataType>(100), nullptr, 0,
                                     []<StandardMetadataType T>(auto&&) { return 10; });
    EXPECT_EQ(-AIMAPPER_ERROR_UNSUPPORTED, result)
            << "100 (out of range) should have resulted in UNSUPPORTED";
}

template <StandardMetadataType T>
std::vector<uint8_t> encode(const typename StandardMetadata<T>::value_type& value) {
    using Value = typename StandardMetadata<T>::value;

    int desiredSize = Value::encode(value, nullptr, 0);
    EXPECT_GE(desiredSize, 0);
    std::vector<uint8_t> buffer;
    buffer.resize(desiredSize);
    EXPECT_EQ(desiredSize, Value::encode(value, buffer.data(), buffer.size()));
    return buffer;
}

TEST(MetadataGralloc4Interop, BufferId) {
    auto mpbuf = encode<StandardMetadataType::BUFFER_ID>(42);
    hidl_vec<uint8_t> g4buf;
    ASSERT_EQ(NO_ERROR, gralloc4::encodeBufferId(42, &g4buf));
    EXPECT_EQ(g4buf, mpbuf);
}

TEST(MetadataGralloc4Interop, Name) {
    auto mpbuf = encode<StandardMetadataType::NAME>("Hello, Interop!");
    hidl_vec<uint8_t> g4buf;
    ASSERT_EQ(NO_ERROR, gralloc4::encodeName("Hello, Interop!", &g4buf));
    EXPECT_EQ(g4buf, mpbuf);
}

TEST(MetadataGralloc4Interop, Width) {
    auto mpbuf = encode<StandardMetadataType::WIDTH>(128);
    hidl_vec<uint8_t> g4buf;
    ASSERT_EQ(NO_ERROR, gralloc4::encodeWidth(128, &g4buf));
    EXPECT_EQ(g4buf, mpbuf);
}

TEST(MetadataGralloc4Interop, Height) {
    auto mpbuf = encode<StandardMetadataType::HEIGHT>(64);
    hidl_vec<uint8_t> g4buf;
    ASSERT_EQ(NO_ERROR, gralloc4::encodeHeight(64, &g4buf));
    EXPECT_EQ(g4buf, mpbuf);
}

TEST(MetadataGralloc4Interop, LayerCount) {
    auto mpbuf = encode<StandardMetadataType::LAYER_COUNT>(3);
    hidl_vec<uint8_t> g4buf;
    ASSERT_EQ(NO_ERROR, gralloc4::encodeLayerCount(3, &g4buf));
    EXPECT_EQ(g4buf, mpbuf);
}

TEST(MetadataGralloc4Interop, PixelFormatRequested) {
    auto mpbuf = encode<StandardMetadataType::PIXEL_FORMAT_REQUESTED>(PixelFormat::RGBX_8888);
    hidl_vec<uint8_t> g4buf;
    ASSERT_EQ(NO_ERROR, gralloc4::encodePixelFormatRequested(
                                hardware::graphics::common::V1_2::PixelFormat::RGBX_8888, &g4buf));
    EXPECT_EQ(g4buf, mpbuf);
}

TEST(MetadataGralloc4Interop, PixelFormatFourcc) {
    auto mpbuf = encode<StandardMetadataType::PIXEL_FORMAT_FOURCC>(DRM_FORMAT_ABGR8888);
    hidl_vec<uint8_t> g4buf;
    ASSERT_EQ(NO_ERROR, gralloc4::encodePixelFormatFourCC(DRM_FORMAT_ABGR8888, &g4buf));
    EXPECT_EQ(g4buf, mpbuf);
}

TEST(MetadataGralloc4Interop, PixelFormatModifier) {
    auto mpbuf = encode<StandardMetadataType::PIXEL_FORMAT_MODIFIER>(123456);
    hidl_vec<uint8_t> g4buf;
    ASSERT_EQ(NO_ERROR, gralloc4::encodePixelFormatModifier(123456, &g4buf));
    EXPECT_EQ(g4buf, mpbuf);
}

TEST(MetadataGralloc4Interop, Usage) {
    auto mpbuf = encode<StandardMetadataType::USAGE>(BufferUsage::COMPOSER_OVERLAY);
    hidl_vec<uint8_t> g4buf;
    ASSERT_EQ(NO_ERROR,
              gralloc4::encodeUsage(
                      static_cast<uint64_t>(
                              hardware::graphics::common::V1_2::BufferUsage::COMPOSER_OVERLAY),
                      &g4buf));
    EXPECT_EQ(g4buf, mpbuf);
}

TEST(MetadataGralloc4Interop, AllocationSize) {
    auto mpbuf = encode<StandardMetadataType::ALLOCATION_SIZE>(10200);
    hidl_vec<uint8_t> g4buf;
    ASSERT_EQ(NO_ERROR, gralloc4::encodeAllocationSize(10200, &g4buf));
    EXPECT_EQ(g4buf, mpbuf);
}

TEST(MetadataGralloc4Interop, ProtectedContent) {
    auto mpbuf = encode<StandardMetadataType::PROTECTED_CONTENT>(1);
    hidl_vec<uint8_t> g4buf;
    ASSERT_EQ(NO_ERROR, gralloc4::encodeProtectedContent(1, &g4buf));
    EXPECT_EQ(g4buf, mpbuf);
}

TEST(MetadataGralloc4Interop, Compression) {
    auto mpbuf = encode<StandardMetadataType::COMPRESSION>(
            gralloc4::Compression_DisplayStreamCompression);
    hidl_vec<uint8_t> g4buf;
    ASSERT_EQ(NO_ERROR,
              gralloc4::encodeCompression(gralloc4::Compression_DisplayStreamCompression, &g4buf));
    EXPECT_EQ(g4buf, mpbuf);
}

TEST(MetadataGralloc4Interop, Interlaced) {
    auto mpbuf = encode<StandardMetadataType::INTERLACED>(gralloc4::Interlaced_TopBottom);
    hidl_vec<uint8_t> g4buf;
    ASSERT_EQ(NO_ERROR, gralloc4::encodeInterlaced(gralloc4::Interlaced_TopBottom, &g4buf));
    EXPECT_EQ(g4buf, mpbuf);
}

TEST(MetadataGralloc4Interop, ChromeSitting) {
    auto mpbuf =
            encode<StandardMetadataType::CHROMA_SITING>(gralloc4::ChromaSiting_SitedInterstitial);
    hidl_vec<uint8_t> g4buf;
    ASSERT_EQ(NO_ERROR,
              gralloc4::encodeChromaSiting(gralloc4::ChromaSiting_SitedInterstitial, &g4buf));
    EXPECT_EQ(g4buf, mpbuf);
}

TEST(MetadataGralloc4Interop, PlaneLayouts) {
    auto mpbuf = encode<StandardMetadataType::PLANE_LAYOUTS>(fakePlaneLayouts());
    hidl_vec<uint8_t> g4buf;
    ASSERT_EQ(NO_ERROR, gralloc4::encodePlaneLayouts(fakePlaneLayouts(), &g4buf));
    EXPECT_EQ(g4buf, mpbuf);
}

TEST(MetadataGralloc4Interop, Crop) {
    std::vector<Rect> cropRects{Rect{10, 11, 12, 13}, Rect{20, 21, 22, 23}};
    auto mpbuf = encode<StandardMetadataType::CROP>(cropRects);
    hidl_vec<uint8_t> g4buf;
    ASSERT_EQ(NO_ERROR, gralloc4::encodeCrop(cropRects, &g4buf));
    EXPECT_EQ(g4buf, mpbuf);
}

TEST(MetadataGralloc4Interop, Dataspace) {
    auto mpbuf = encode<StandardMetadataType::DATASPACE>(Dataspace::DISPLAY_P3);
    hidl_vec<uint8_t> g4buf;
    ASSERT_EQ(NO_ERROR, gralloc4::encodeDataspace(Dataspace::DISPLAY_P3, &g4buf));
    EXPECT_EQ(g4buf, mpbuf);
}

TEST(MetadataGralloc4Interop, BlendMode) {
    auto mpbuf = encode<StandardMetadataType::BLEND_MODE>(BlendMode::PREMULTIPLIED);
    hidl_vec<uint8_t> g4buf;
    ASSERT_EQ(NO_ERROR, gralloc4::encodeBlendMode(BlendMode::PREMULTIPLIED, &g4buf));
    EXPECT_EQ(g4buf, mpbuf);
}

TEST(MetadataGralloc4Interop, Smpte2086) {
    Smpte2086 hdrdata{XyColor{.1f, .2f}, XyColor{.3f, .4f}, XyColor{.5f, .6f},
                      XyColor{.7f, .8f}, 452.889f,          12.335f};

    auto mpbuf = encode<StandardMetadataType::SMPTE2086>(hdrdata);
    hidl_vec<uint8_t> g4buf;
    ASSERT_EQ(NO_ERROR, gralloc4::encodeSmpte2086(hdrdata, &g4buf));
    EXPECT_EQ(g4buf, mpbuf);
}

TEST(MetadataGralloc4Interop, Cta861_3) {
    Cta861_3 hdrdata{302.202f, 244.55f};
    auto mpbuf = encode<StandardMetadataType::CTA861_3>(hdrdata);
    hidl_vec<uint8_t> g4buf;
    ASSERT_EQ(NO_ERROR, gralloc4::encodeCta861_3(hdrdata, &g4buf));
    EXPECT_EQ(g4buf, mpbuf);
}

TEST(MetadataGralloc4Interop, Smpte2094_10) {
    auto mpbuf = encode<StandardMetadataType::SMPTE2094_10>(std::nullopt);
    hidl_vec<uint8_t> g4buf;
    ASSERT_EQ(NO_ERROR, gralloc4::encodeSmpte2094_10(std::nullopt, &g4buf));
    EXPECT_EQ(g4buf, mpbuf);

    std::vector<uint8_t> hdrdata{1, 2, 3, 4, 5, 6};
    mpbuf = encode<StandardMetadataType::SMPTE2094_10>(hdrdata);
    ASSERT_EQ(NO_ERROR, gralloc4::encodeSmpte2094_10(hdrdata, &g4buf));
    EXPECT_EQ(g4buf, mpbuf);
}

TEST(MetadataGralloc4Interop, Smpte2094_40) {
    auto mpbuf = encode<StandardMetadataType::SMPTE2094_40>(std::nullopt);
    hidl_vec<uint8_t> g4buf;
    ASSERT_EQ(NO_ERROR, gralloc4::encodeSmpte2094_40(std::nullopt, &g4buf));
    EXPECT_EQ(g4buf, mpbuf);

    std::vector<uint8_t> hdrdata{1, 2, 3, 4, 5, 6};
    mpbuf = encode<StandardMetadataType::SMPTE2094_40>(hdrdata);
    ASSERT_EQ(NO_ERROR, gralloc4::encodeSmpte2094_40(hdrdata, &g4buf));
    EXPECT_EQ(g4buf, mpbuf);
}
