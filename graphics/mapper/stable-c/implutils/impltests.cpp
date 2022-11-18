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
#include <vector>

using namespace ::android::hardware::graphics::mapper;
using namespace ::aidl::android::hardware::graphics::common;

// These tests are primarily interested in hitting all the different *types* that can be
// serialized/deserialized than in exhaustively testing all the StandardMetadataTypes.
// Exhaustive testing of the actual metadata types is relegated for IMapper's VTS suite
// where meaning & correctness of values are more narrowly defined (eg, read-only values)

TEST(Metadata, setGetBufferId) {
    using BufferId = StandardMetadata<StandardMetadataType::BUFFER_ID>::value;

    std::vector<char> buffer;
    buffer.resize(12, 0);
    *reinterpret_cast<int64_t*>(buffer.data()) = 42;

    EXPECT_EQ(8, BufferId::encode(18, buffer.data(), 0));
    EXPECT_EQ(42, *reinterpret_cast<int64_t*>(buffer.data()));
    EXPECT_EQ(8, BufferId::encode(18, buffer.data(), buffer.size()));
    EXPECT_EQ(18, *reinterpret_cast<int64_t*>(buffer.data()));
    EXPECT_FALSE(BufferId::decode(buffer.data(), 0));
    auto read = BufferId::decode(buffer.data(), buffer.size());
    EXPECT_TRUE(read.has_value());
    EXPECT_EQ(18, read.value_or(0));
}

TEST(Metadata, setGetDataspace) {
    using DataspaceValue = StandardMetadata<StandardMetadataType::DATASPACE>::value;
    using intType = std::underlying_type_t<Dataspace>;
    std::vector<char> buffer;
    buffer.resize(12, 0);

    EXPECT_EQ(4, DataspaceValue::encode(Dataspace::BT2020, buffer.data(), 0));
    EXPECT_EQ(0, *reinterpret_cast<intType*>(buffer.data()));
    EXPECT_EQ(4, DataspaceValue::encode(Dataspace::BT2020, buffer.data(), buffer.size()));
    EXPECT_EQ(static_cast<intType>(Dataspace::BT2020), *reinterpret_cast<intType*>(buffer.data()));
    EXPECT_FALSE(DataspaceValue::decode(buffer.data(), 0));
    auto read = DataspaceValue::decode(buffer.data(), buffer.size());
    ASSERT_TRUE(read.has_value());
    EXPECT_EQ(Dataspace::BT2020, *read);
}

TEST(Metadata, setGetValidName) {
    using NameValue = StandardMetadata<StandardMetadataType::NAME>::value;

    std::vector<char> buffer;
    buffer.resize(100, 'a');
    buffer[buffer.size() - 1] = '\0';

    // len("Hello") + sizeof(int64)
    constexpr int expectedSize = 5 + sizeof(int64_t);
    EXPECT_EQ(expectedSize, NameValue::encode("Hello", buffer.data(), buffer.size()));
    EXPECT_EQ(5, *reinterpret_cast<int64_t*>(buffer.data()));
    // Verify didn't write past the end of the desired size
    EXPECT_EQ('a', buffer[expectedSize]);

    auto readValue = NameValue::decode(buffer.data(), buffer.size());
    ASSERT_TRUE(readValue.has_value());
    EXPECT_EQ(5, readValue->length());
    EXPECT_EQ("Hello", *readValue);
}

TEST(Metadata, setGetInvalidName) {
    using NameValue = StandardMetadata<StandardMetadataType::NAME>::value;

    std::vector<char> buffer;
    buffer.resize(12, 'a');
    buffer[buffer.size() - 1] = '\0';

    // len("This is a long string") + sizeof(int64)
    constexpr int expectedSize = 21 + sizeof(int64_t);
    EXPECT_EQ(expectedSize,
              NameValue::encode("This is a long string", buffer.data(), buffer.size()));
    EXPECT_EQ(21, *reinterpret_cast<int64_t*>(buffer.data()));
    // Verify didn't write the too-long string
    EXPECT_EQ('a', buffer[9]);
    EXPECT_EQ('\0', buffer[buffer.size() - 1]);

    auto readValue = NameValue::decode(buffer.data(), buffer.size());
    EXPECT_FALSE(readValue.has_value());
    readValue = NameValue::decode(buffer.data(), 0);
    ASSERT_FALSE(readValue.has_value());
}

TEST(Metadata, wouldOverflowName) {
    using NameValue = StandardMetadata<StandardMetadataType::NAME>::value;
    std::vector<char> buffer(100, 0);

    // int_max + sizeof(int64) overflows int32
    std::string_view bad_string{"badbeef", std::numeric_limits<int32_t>::max()};
    EXPECT_EQ(-AIMAPPER_ERROR_BAD_VALUE,
              NameValue::encode(bad_string, buffer.data(), buffer.size()));

    // check barely overflows
    bad_string = std::string_view{"badbeef", std::numeric_limits<int32_t>::max() - 7};
    EXPECT_EQ(-AIMAPPER_ERROR_BAD_VALUE,
              NameValue::encode(bad_string, buffer.data(), buffer.size()));
}

TEST(Metadata, setGetCompression) {
    using CompressionValue = StandardMetadata<StandardMetadataType::COMPRESSION>::value;
    ExtendableType myCompression{"bestest_compression_ever", 42};
    std::vector<char> buffer(100, '\0');
    const int expectedSize = myCompression.name.length() + sizeof(int64_t) + sizeof(int64_t);
    EXPECT_EQ(expectedSize, CompressionValue::encode(myCompression, buffer.data(), 0));
    EXPECT_EQ(0, buffer[0]);
    EXPECT_EQ(expectedSize, CompressionValue::encode(myCompression, buffer.data(), buffer.size()));
    EXPECT_EQ(myCompression.name.length(), *reinterpret_cast<int64_t*>(buffer.data()));
    EXPECT_FALSE(CompressionValue::decode(buffer.data(), 0).has_value());
    auto read = CompressionValue::decode(buffer.data(), buffer.size());
    ASSERT_TRUE(read.has_value());
    EXPECT_EQ(myCompression, read.value());
}

TEST(Metadata, setGetPlaneLayout) {
    using PlaneLayoutValue = StandardMetadata<StandardMetadataType::PLANE_LAYOUTS>::value;
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

    std::vector<PlaneLayout> layouts{myPlaneLayout, PlaneLayout{}};

    std::vector<char> buffer(5000, '\0');
    constexpr int componentSize = 8 + (4 * sizeof(int64_t));
    constexpr int firstLayoutSize = (8 + 1) * sizeof(int64_t) + (3 * componentSize);
    constexpr int secondLayoutSize = (8 + 1) * sizeof(int64_t);
    constexpr int expectedSize = firstLayoutSize + secondLayoutSize + sizeof(int64_t);
    EXPECT_EQ(expectedSize, PlaneLayoutValue::encode(layouts, buffer.data(), 0));
    EXPECT_EQ(0, buffer[0]);
    EXPECT_EQ(expectedSize, PlaneLayoutValue::encode(layouts, buffer.data(), buffer.size()));
    EXPECT_EQ(3, reinterpret_cast<int64_t*>(buffer.data())[1]);
    EXPECT_EQ(8, reinterpret_cast<int64_t*>(buffer.data())[2]);
    EXPECT_EQ(40, reinterpret_cast<int64_t*>(buffer.data())[4]);
    EXPECT_EQ(31, reinterpret_cast<int64_t*>(buffer.data())[11]);
    EXPECT_EQ(22, reinterpret_cast<int64_t*>(buffer.data())[15]);
    EXPECT_EQ(10, reinterpret_cast<int64_t*>(buffer.data())[17]);
    EXPECT_EQ(11, reinterpret_cast<int64_t*>(buffer.data())[18]);
    EXPECT_FALSE(PlaneLayoutValue::decode(buffer.data(), 0).has_value());
    auto read = PlaneLayoutValue::decode(buffer.data(), buffer.size());
    ASSERT_TRUE(read.has_value());
    EXPECT_EQ(layouts, *read);
}

TEST(Metadata, setGetRects) {
    using RectsValue = StandardMetadata<StandardMetadataType::CROP>::value;
    std::vector<uint8_t> buffer(500, 0);
    std::vector<Rect> cropRects{2};
    cropRects[0] = Rect{10, 11, 12, 13};
    cropRects[1] = Rect{20, 21, 22, 23};

    constexpr int expectedSize = sizeof(int64_t) + (8 * sizeof(int32_t));
    EXPECT_EQ(expectedSize, RectsValue::encode(cropRects, buffer.data(), buffer.size()));
    EXPECT_EQ(2, reinterpret_cast<int64_t*>(buffer.data())[0]);
    EXPECT_EQ(10, reinterpret_cast<int32_t*>(buffer.data())[2]);
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

    constexpr int expectedSize = 10 * sizeof(float);
    std::vector<uint8_t> buffer(500, 0);
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

    constexpr int expectedSize = 2 * sizeof(float);
    std::vector<uint8_t> buffer(500, 0);
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

    std::vector<uint8_t> buffer(500, 0);
    EXPECT_EQ(0, SMPTE2094_10Value::encode(std::nullopt, buffer.data(), buffer.size()));
    auto read = SMPTE2094_10Value::decode(buffer.data(), 0);
    ASSERT_TRUE(read.has_value());
    EXPECT_FALSE(read->has_value());

    const std::vector<uint8_t> emptyBuffer;
    EXPECT_EQ(sizeof(int64_t),
              SMPTE2094_10Value::encode(emptyBuffer, buffer.data(), buffer.size()));
    read = SMPTE2094_10Value::decode(buffer.data(), buffer.size());
    ASSERT_TRUE(read.has_value());
    ASSERT_TRUE(read->has_value());
    EXPECT_EQ(0, read->value().size());

    const std::vector<uint8_t> simpleBuffer{0, 1, 2, 3, 4, 5};
    EXPECT_EQ(sizeof(int64_t) + 6,
              SMPTE2094_10Value::encode(simpleBuffer, buffer.data(), buffer.size()));
    read = SMPTE2094_10Value::decode(buffer.data(), buffer.size());
    ASSERT_TRUE(read.has_value());
    ASSERT_TRUE(read->has_value());
    EXPECT_EQ(6, read->value().size());
    EXPECT_EQ(simpleBuffer, read->value());
}

TEST(MetadataProvider, bufferId) {
    using BufferId = StandardMetadata<StandardMetadataType::BUFFER_ID>::value;
    std::vector<uint8_t> buffer(500, 0);
    int result = provideStandardMetadata(StandardMetadataType::BUFFER_ID, buffer.data(),
                                         buffer.size(), []<StandardMetadataType T>(auto&& provide) {
                                             if constexpr (T == StandardMetadataType::BUFFER_ID) {
                                                 return provide(42);
                                             }
                                             return 0;
                                         });

    EXPECT_EQ(8, result);
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
