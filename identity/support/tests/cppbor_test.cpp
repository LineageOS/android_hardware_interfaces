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

#include <iomanip>
#include <sstream>

#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include "cppbor.h"
#include "cppbor_parse.h"

using namespace cppbor;
using namespace std;

using ::testing::_;
using ::testing::AllOf;
using ::testing::ByRef;
using ::testing::InSequence;
using ::testing::IsNull;
using ::testing::NotNull;
using ::testing::Return;
using ::testing::Truly;
using ::testing::Unused;

string hexDump(const string& str) {
    stringstream s;
    for (auto c : str) {
        s << setfill('0') << setw(2) << hex << (static_cast<unsigned>(c) & 0xff);
    }
    return s.str();
}

TEST(SimpleValueTest, UnsignedValueSizes) {
    // Check that unsigned integers encode to correct lengths, and that encodedSize() is correct.
    vector<pair<uint64_t /* value */, size_t /* expected encoded size */>> testCases{
            {0, 1},
            {1, 1},
            {23, 1},
            {24, 2},
            {255, 2},
            {256, 3},
            {65535, 3},
            {65536, 5},
            {4294967295, 5},
            {4294967296, 9},
            {std::numeric_limits<uint64_t>::max(), 9},
    };
    for (auto& testCase : testCases) {
        Uint val(testCase.first);
        EXPECT_EQ(testCase.second, val.encodedSize()) << "Wrong size for value " << testCase.first;
        EXPECT_EQ(val.encodedSize(), val.toString().size())
                << "encodedSize and encoding disagree for value " << testCase.first;
    }
}

TEST(SimpleValueTest, UnsignedValueEncodings) {
    EXPECT_EQ("\x00"s, Uint(0u).toString());
    EXPECT_EQ("\x01"s, Uint(1u).toString());
    EXPECT_EQ("\x0a"s, Uint(10u).toString());
    EXPECT_EQ("\x17"s, Uint(23u).toString());
    EXPECT_EQ("\x18\x18"s, Uint(24u).toString());
    EXPECT_EQ("\x18\x19"s, Uint(25u).toString());
    EXPECT_EQ("\x18\x64"s, Uint(100u).toString());
    EXPECT_EQ("\x19\x03\xe8"s, Uint(1000u).toString());
    EXPECT_EQ("\x1a\x00\x0f\x42\x40"s, Uint(1000000u).toString());
    EXPECT_EQ("\x1b\x00\x00\x00\xe8\xd4\xa5\x10\x00"s, Uint(1000000000000u).toString());
    EXPECT_EQ("\x1B\x7f\xff\xff\xff\xff\xff\xff\xff"s,
              Uint(std::numeric_limits<int64_t>::max()).toString());
}

TEST(SimpleValueTest, NegativeValueEncodings) {
    EXPECT_EQ("\x20"s, Nint(-1).toString());
    EXPECT_EQ("\x28"s, Nint(-9).toString());
    EXPECT_EQ("\x29"s, Nint(-10).toString());
    EXPECT_EQ("\x36"s, Nint(-23).toString());
    EXPECT_EQ("\x37"s, Nint(-24).toString());
    EXPECT_EQ("\x38\x18"s, Nint(-25).toString());
    EXPECT_EQ("\x38\x62"s, Nint(-99).toString());
    EXPECT_EQ("\x38\x63"s, Nint(-100).toString());
    EXPECT_EQ("\x39\x03\xe6"s, Nint(-999).toString());
    EXPECT_EQ("\x39\x03\xe7"s, Nint(-1000).toString());
    EXPECT_EQ("\x3a\x00\x0f\x42\x3F"s, Nint(-1000000).toString());
    EXPECT_EQ("\x3b\x00\x00\x00\xe8\xd4\xa5\x0f\xff"s, Nint(-1000000000000).toString());
    EXPECT_EQ("\x3B\x7f\xff\xff\xff\xff\xff\xff\xff"s,
              Nint(std::numeric_limits<int64_t>::min()).toString());
}

TEST(SimpleValueDeathTest, NegativeValueEncodings) {
    EXPECT_DEATH(Nint(0), "");
    EXPECT_DEATH(Nint(1), "");
}

TEST(SimpleValueTest, BooleanEncodings) {
    EXPECT_EQ("\xf4"s, Bool(false).toString());
    EXPECT_EQ("\xf5"s, Bool(true).toString());
}

TEST(SimpleValueTest, ByteStringEncodings) {
    EXPECT_EQ("\x40", Bstr("").toString());
    EXPECT_EQ("\x41\x61", Bstr("a").toString());
    EXPECT_EQ("\x41\x41", Bstr("A").toString());
    EXPECT_EQ("\x44\x49\x45\x54\x46", Bstr("IETF").toString());
    EXPECT_EQ("\x42\x22\x5c", Bstr("\"\\").toString());
    EXPECT_EQ("\x42\xc3\xbc", Bstr("\xc3\xbc").toString());
    EXPECT_EQ("\x43\xe6\xb0\xb4", Bstr("\xe6\xb0\xb4").toString());
    EXPECT_EQ("\x44\xf0\x90\x85\x91", Bstr("\xf0\x90\x85\x91").toString());
    EXPECT_EQ("\x44\x01\x02\x03\x04", Bstr("\x01\x02\x03\x04").toString());
    EXPECT_EQ("\x44\x40\x40\x40\x40", Bstr("@@@@").toString());
}

TEST(SimpleValueTest, TextStringEncodings) {
    EXPECT_EQ("\x60"s, Tstr("").toString());
    EXPECT_EQ("\x61\x61"s, Tstr("a").toString());
    EXPECT_EQ("\x61\x41"s, Tstr("A").toString());
    EXPECT_EQ("\x64\x49\x45\x54\x46"s, Tstr("IETF").toString());
    EXPECT_EQ("\x62\x22\x5c"s, Tstr("\"\\").toString());
    EXPECT_EQ("\x62\xc3\xbc"s, Tstr("\xc3\xbc").toString());
    EXPECT_EQ("\x63\xe6\xb0\xb4"s, Tstr("\xe6\xb0\xb4").toString());
    EXPECT_EQ("\x64\xf0\x90\x85\x91"s, Tstr("\xf0\x90\x85\x91").toString());
    EXPECT_EQ("\x64\x01\x02\x03\x04"s, Tstr("\x01\x02\x03\x04").toString());
}

TEST(IsIteratorPairOverTest, All) {
    EXPECT_TRUE((
            details::is_iterator_pair_over<pair<string::iterator, string::iterator>, char>::value));
    EXPECT_TRUE((details::is_iterator_pair_over<pair<string::const_iterator, string::iterator>,
                                                char>::value));
    EXPECT_TRUE((details::is_iterator_pair_over<pair<string::iterator, string::const_iterator>,
                                                char>::value));
    EXPECT_TRUE((details::is_iterator_pair_over<pair<char*, char*>, char>::value));
    EXPECT_TRUE((details::is_iterator_pair_over<pair<const char*, char*>, char>::value));
    EXPECT_TRUE((details::is_iterator_pair_over<pair<char*, const char*>, char>::value));
    EXPECT_FALSE((details::is_iterator_pair_over<pair<string::iterator, string::iterator>,
                                                 uint8_t>::value));
    EXPECT_FALSE((details::is_iterator_pair_over<pair<char*, char*>, uint8_t>::value));
    EXPECT_TRUE((details::is_iterator_pair_over<
                 pair<vector<uint8_t>::iterator, vector<uint8_t>::iterator>, uint8_t>::value));
    EXPECT_TRUE((details::is_iterator_pair_over<
                 pair<vector<uint8_t>::const_iterator, vector<uint8_t>::iterator>,
                 uint8_t>::value));
    EXPECT_TRUE((details::is_iterator_pair_over<
                 pair<vector<uint8_t>::iterator, vector<uint8_t>::const_iterator>,
                 uint8_t>::value));
    EXPECT_TRUE((details::is_iterator_pair_over<pair<uint8_t*, uint8_t*>, uint8_t>::value));
    EXPECT_TRUE((details::is_iterator_pair_over<pair<const uint8_t*, uint8_t*>, uint8_t>::value));
    EXPECT_TRUE((details::is_iterator_pair_over<pair<uint8_t*, const uint8_t*>, uint8_t>::value));
    EXPECT_FALSE((details::is_iterator_pair_over<
                  pair<vector<uint8_t>::iterator, vector<uint8_t>::iterator>, char>::value));
    EXPECT_FALSE((details::is_iterator_pair_over<pair<uint8_t*, const uint8_t*>, char>::value));
}

TEST(MakeEntryTest, Boolean) {
    EXPECT_EQ("\xf4"s, details::makeItem(false)->toString());
}

TEST(MakeEntryTest, Integers) {
    EXPECT_EQ("\x00"s, details::makeItem(static_cast<uint8_t>(0))->toString());
    EXPECT_EQ("\x00"s, details::makeItem(static_cast<uint16_t>(0))->toString());
    EXPECT_EQ("\x00"s, details::makeItem(static_cast<uint32_t>(0))->toString());
    EXPECT_EQ("\x00"s, details::makeItem(static_cast<uint64_t>(0))->toString());
    EXPECT_EQ("\x00"s, details::makeItem(static_cast<int8_t>(0))->toString());
    EXPECT_EQ("\x00"s, details::makeItem(static_cast<int16_t>(0))->toString());
    EXPECT_EQ("\x00"s, details::makeItem(static_cast<int32_t>(0))->toString());
    EXPECT_EQ("\x00"s, details::makeItem(static_cast<int64_t>(0))->toString());
    EXPECT_EQ("\x20"s, details::makeItem(static_cast<int8_t>(-1))->toString());
    EXPECT_EQ("\x20"s, details::makeItem(static_cast<int16_t>(-1))->toString());
    EXPECT_EQ("\x20"s, details::makeItem(static_cast<int32_t>(-1))->toString());
    EXPECT_EQ("\x20"s, details::makeItem(static_cast<int64_t>(-1))->toString());

    EXPECT_EQ("\x1b\xff\xff\xff\xff\xff\xff\xff\xff"s,
              details::makeItem(static_cast<uint64_t>(std::numeric_limits<uint64_t>::max()))
                      ->toString());
}

TEST(MakeEntryTest, StdStrings) {
    string s1("hello");
    const string s2("hello");
    EXPECT_EQ("\x65\x68\x65\x6c\x6c\x6f"s, details::makeItem(s1)->toString());  // copy of string
    EXPECT_EQ("\x65\x68\x65\x6c\x6c\x6f"s,
              details::makeItem(s2)->toString());  // copy of const string
    EXPECT_EQ("\x65\x68\x65\x6c\x6c\x6f"s,
              details::makeItem(std::move(s1))->toString());  // move string
    EXPECT_EQ(0U, s1.size());                                 // Prove string was moved, not copied.
}

TEST(MakeEntryTest, StdStringViews) {
    string_view s1("hello");
    const string_view s2("hello");
    EXPECT_EQ("\x65\x68\x65\x6c\x6c\x6f"s, details::makeItem(s1)->toString());
    EXPECT_EQ("\x65\x68\x65\x6c\x6c\x6f"s, details::makeItem(s2)->toString());
}

TEST(MakeEntryTest, CStrings) {
    char s1[] = "hello";
    const char s2[] = "hello";
    const char* s3 = "hello";
    EXPECT_EQ("\x65\x68\x65\x6c\x6c\x6f"s, details::makeItem(s1)->toString());
    EXPECT_EQ("\x65\x68\x65\x6c\x6c\x6f"s, details::makeItem(s2)->toString());
    EXPECT_EQ("\x65\x68\x65\x6c\x6c\x6f"s, details::makeItem(s3)->toString());
}

TEST(MakeEntryTest, StringIteratorPairs) {
    // Use iterators from string to prove that "real" iterators work
    string s1 = "hello"s;
    pair<string::iterator, string::iterator> p1 = make_pair(s1.begin(), s1.end());

    const pair<string::iterator, string::iterator> p2 = p1;
    EXPECT_EQ("\x65\x68\x65\x6c\x6c\x6f"s, details::makeItem(p1)->toString());
    EXPECT_EQ("\x65\x68\x65\x6c\x6c\x6f"s, details::makeItem(p2)->toString());

    // Use char*s  as iterators
    const char* s2 = "hello";
    pair p3 = make_pair(s2, s2 + 5);
    const pair p4 = p3;
    EXPECT_EQ("\x65\x68\x65\x6c\x6c\x6f"s, details::makeItem(p3)->toString());
    EXPECT_EQ("\x65\x68\x65\x6c\x6c\x6f"s, details::makeItem(p4)->toString());
}

TEST(MakeEntryTest, ByteStrings) {
    vector<uint8_t> v1 = {0x00, 0x01, 0x02};
    const vector<uint8_t> v2 = {0x00, 0x01, 0x02};
    EXPECT_EQ("\x43\x00\x01\x02"s, details::makeItem(v1)->toString());  // copy of vector
    EXPECT_EQ("\x43\x00\x01\x02"s, details::makeItem(v2)->toString());  // copy of const vector
    EXPECT_EQ("\x43\x00\x01\x02"s, details::makeItem(std::move(v1))->toString());  // move vector
    EXPECT_EQ(0U, v1.size());  // Prove vector was moved, not copied.
}

TEST(MakeEntryTest, ByteStringIteratorPairs) {
    using vec = vector<uint8_t>;
    using iter = vec::iterator;
    vec v1 = {0x00, 0x01, 0x02};
    pair<iter, iter> p1 = make_pair(v1.begin(), v1.end());
    const pair<iter, iter> p2 = make_pair(v1.begin(), v1.end());
    EXPECT_EQ("\x43\x00\x01\x02"s, details::makeItem(p1)->toString());
    EXPECT_EQ("\x43\x00\x01\x02"s, details::makeItem(p2)->toString());

    // Use uint8_t*s as iterators
    uint8_t v2[] = {0x00, 0x01, 0x02};
    uint8_t* v3 = v2;
    pair<uint8_t*, uint8_t*> p3 = make_pair(v2, v2 + 3);
    const pair<uint8_t*, uint8_t*> p4 = make_pair(v2, v2 + 3);
    pair<uint8_t*, uint8_t*> p5 = make_pair(v3, v3 + 3);
    const pair<uint8_t*, uint8_t*> p6 = make_pair(v3, v3 + 3);
    EXPECT_EQ("\x43\x00\x01\x02"s, details::makeItem(p3)->toString());
    EXPECT_EQ("\x43\x00\x01\x02"s, details::makeItem(p4)->toString());
    EXPECT_EQ("\x43\x00\x01\x02"s, details::makeItem(p5)->toString());
    EXPECT_EQ("\x43\x00\x01\x02"s, details::makeItem(p6)->toString());
}

TEST(MakeEntryTest, ByteStringBuffers) {
    uint8_t v1[] = {0x00, 0x01, 0x02};
    EXPECT_EQ("\x43\x00\x01\x02"s, details::makeItem(make_pair(v1, 3))->toString());
}

TEST(MakeEntryTest, ItemPointer) {
    Uint* p1 = new Uint(0);
    EXPECT_EQ("\x00"s, details::makeItem(p1)->toString());
    EXPECT_EQ("\x60"s, details::makeItem(new Tstr(string()))->toString());
}

TEST(MakeEntryTest, ItemReference) {
    Tstr str("hello"s);
    Tstr& strRef = str;
    const Tstr& strConstRef = str;
    EXPECT_EQ("\x65\x68\x65\x6c\x6c\x6f"s, details::makeItem(str)->toString());
    EXPECT_EQ("\x65\x68\x65\x6c\x6c\x6f"s, details::makeItem(strRef)->toString());
    EXPECT_EQ("\x65\x68\x65\x6c\x6c\x6f"s, details::makeItem(strConstRef)->toString());
    EXPECT_EQ("\x65\x68\x65\x6c\x6c\x6f"s, details::makeItem(std::move(str))->toString());
    EXPECT_EQ("\x60"s, details::makeItem(str)->toString());  // Prove that it moved

    EXPECT_EQ("\x00"s, details::makeItem(Uint(0))->toString());

    EXPECT_EQ("\x43\x00\x01\x02"s,
              details::makeItem(Bstr(vector<uint8_t>{0x00, 0x01, 0x02}))->toString());

    EXPECT_EQ("\x80"s, details::makeItem(Array())->toString());
    EXPECT_EQ("\xa0"s, details::makeItem(Map())->toString());
}

TEST(CompoundValueTest, ArrayOfInts) {
    EXPECT_EQ("\x80"s, Array().toString());
    Array(Uint(0)).toString();

    EXPECT_EQ("\x81\x00"s, Array(Uint(0U)).toString());
    EXPECT_EQ("\x82\x00\x01"s, Array(Uint(0), Uint(1)).toString());
    EXPECT_EQ("\x83\x00\x01\x38\x62"s, Array(Uint(0), Uint(1), Nint(-99)).toString());

    EXPECT_EQ("\x81\x00"s, Array(0).toString());
    EXPECT_EQ("\x82\x00\x01"s, Array(0, 1).toString());
    EXPECT_EQ("\x83\x00\x01\x38\x62"s, Array(0, 1, -99).toString());
}

TEST(CompoundValueTest, MapOfInts) {
    EXPECT_EQ("\xA0"s, Map().toString());
    EXPECT_EQ("\xA1\x00\x01"s, Map(Uint(0), Uint(1)).toString());
    // Maps with an odd number of arguments will fail to compile.  Uncomment the next lines to test.
    // EXPECT_EQ("\xA1\x00"s, Map(Int(0)).toString());
    // EXPECT_EQ("\xA1\x00\x01\x02"s, Map(Int(0), Int(1), Int(2)).toString());
}

TEST(CompoundValueTest, MixedArray) {
    vector<uint8_t> vec = {3, 2, 1};
    EXPECT_EQ("\x84\x01\x20\x43\x03\x02\x01\x65\x68\x65\x6C\x6C\x6F"s,
              Array(Uint(1), Nint(-1), Bstr(vec), Tstr("hello")).toString());

    EXPECT_EQ("\x84\x01\x20\x43\x03\x02\x01\x65\x68\x65\x6C\x6C\x6F"s,
              Array(1, -1, vec, "hello").toString());
}

TEST(CompoundValueTest, MixedMap) {
    vector<uint8_t> vec = {3, 2, 1};
    EXPECT_EQ("\xA2\x01\x20\x43\x03\x02\x01\x65\x68\x65\x6C\x6C\x6F"s,
              Map(Uint(1), Nint(-1), Bstr(vec), Tstr("hello")).toString());

    EXPECT_EQ("\xA2\x01\x20\x43\x03\x02\x01\x65\x68\x65\x6C\x6C\x6F"s,
              Map(1, -1, vec, "hello").toString());
}

TEST(CompoundValueTest, NestedStructures) {
    vector<uint8_t> vec = {3, 2, 1};

    string expectedEncoding =
            "\xA2\x66\x4F\x75\x74\x65\x72\x31\x82\xA2\x66\x49\x6E\x6E\x65\x72\x31\x18\x63\x66\x49"
            "\x6E"
            "\x6E\x65\x72\x32\x43\x03\x02\x01\x63\x66\x6F\x6F\x66\x4F\x75\x74\x65\x72\x32\x0A"s;

    // Do it with explicitly-created Items
    EXPECT_EQ(expectedEncoding,
              Map(Tstr("Outer1"),
                  Array(  //
                          Map(Tstr("Inner1"), Uint(99), Tstr("Inner2"), Bstr(vec)), Tstr("foo")),
                  Tstr("Outer2"),  //
                  Uint(10))
                      .toString());
    EXPECT_EQ(3U, vec.size());

    // Now just use convertible types
    EXPECT_EQ(expectedEncoding, Map("Outer1",
                                    Array(Map("Inner1", 99,  //
                                              "Inner2", vec),
                                          "foo"),
                                    "Outer2", 10)
                                        .toString());
    EXPECT_EQ(3U, vec.size());

    // Finally, do it with the .add() method.  This is slightly less efficient, but has the
    // advantage you can build a structure up incrementally, or somewhat fluently if you like.
    // First, fluently.
    EXPECT_EQ(expectedEncoding, Map().add("Outer1", Array().add(Map()  //
                                                                        .add("Inner1", 99)
                                                                        .add("Inner2", vec))
                                                            .add("foo"))
                                        .add("Outer2", 10)
                                        .toString());
    EXPECT_EQ(3U, vec.size());

    // Next, more incrementally
    Array arr;
    arr.add(Map()  //
                    .add("Inner1", 99)
                    .add("Inner2", vec))
            .add("foo");
    EXPECT_EQ(3U, vec.size());

    Map m;
    m.add("Outer1", std::move(arr));  // Moving is necessary; Map and Array cannot be copied.
    m.add("Outer2", 10);
    auto s = m.toString();
    EXPECT_EQ(expectedEncoding, s);
}

TEST(EncodingMethodsTest, AllVariants) {
    Map map;
    map.add("key1", Array().add(Map()  //
                                        .add("key_a", 9999999)
                                        .add("key_b", std::vector<uint8_t>{0x01, 0x02, 0x03})
                                        .add("key_c", std::numeric_limits<uint64_t>::max())
                                        .add("key_d", std::numeric_limits<int16_t>::min()))
                            .add("foo"))
            .add("key2", true);

    std::vector<uint8_t> buf;
    buf.resize(map.encodedSize());

    EXPECT_EQ(buf.data() + buf.size(), map.encode(buf.data(), buf.data() + buf.size()));

    EXPECT_EQ(buf, map.encode());

    std::vector<uint8_t> buf2;
    map.encode(std::back_inserter(buf2));
    EXPECT_EQ(buf, buf2);

    auto iter = buf.begin();
    map.encode([&](uint8_t c) { EXPECT_EQ(c, *iter++); });
}

TEST(EncodingMethodsTest, UintWithTooShortBuf) {
    Uint val(100000);
    vector<uint8_t> buf(val.encodedSize() - 1);
    EXPECT_EQ(nullptr, val.encode(buf.data(), buf.data() + buf.size()));
}

TEST(EncodingMethodsTest, TstrWithTooShortBuf) {
    Tstr val("01234567890123456789012345"s);
    vector<uint8_t> buf(1);
    EXPECT_EQ(nullptr, val.encode(buf.data(), buf.data() + buf.size()));

    buf.resize(val.encodedSize() - 1);
    EXPECT_EQ(nullptr, val.encode(buf.data(), buf.data() + buf.size()));
}

TEST(EncodingMethodsTest, BstrWithTooShortBuf) {
    Bstr val("01234567890123456789012345"s);
    vector<uint8_t> buf(1);
    EXPECT_EQ(nullptr, val.encode(buf.data(), buf.data() + buf.size()));

    buf.resize(val.encodedSize() - 1);
    EXPECT_EQ(nullptr, val.encode(buf.data(), buf.data() + buf.size()));
}

TEST(EncodingMethodsTest, ArrayWithTooShortBuf) {
    Array val("a", 5, -100);

    std::vector<uint8_t> buf(val.encodedSize() - 1);
    EXPECT_EQ(nullptr, val.encode(buf.data(), buf.data() + buf.size()));
}

TEST(EncodingMethodsTest, MapWithTooShortBuf) {
    Map map;
    map.add("key1", Array().add(Map()  //
                                        .add("key_a", 99)
                                        .add("key_b", std::vector<uint8_t>{0x01, 0x02, 0x03}))
                            .add("foo"))
            .add("key2", true);

    std::vector<uint8_t> buf(map.encodedSize() - 1);
    EXPECT_EQ(nullptr, map.encode(buf.data(), buf.data() + buf.size()));
}

TEST(EqualityTest, Uint) {
    Uint val(99);
    EXPECT_EQ(val, Uint(99));

    EXPECT_NE(val, Uint(98));
    EXPECT_NE(val, Nint(-1));
    EXPECT_NE(val, Tstr("99"));
    EXPECT_NE(val, Bstr("99"));
    EXPECT_NE(val, Bool(false));
    EXPECT_NE(val, Array(99, 1));
    EXPECT_NE(val, Map(99, 1));
}

TEST(EqualityTest, Nint) {
    Nint val(-1);
    EXPECT_EQ(val, Nint(-1));

    EXPECT_NE(val, Uint(99));
    EXPECT_NE(val, Nint(-4));
    EXPECT_NE(val, Tstr("99"));
    EXPECT_NE(val, Bstr("99"));
    EXPECT_NE(val, Bool(false));
    EXPECT_NE(val, Array(99));
    EXPECT_NE(val, Map(99, 1));
}

TEST(EqualityTest, Tstr) {
    Tstr val("99");
    EXPECT_EQ(val, Tstr("99"));

    EXPECT_NE(val, Uint(99));
    EXPECT_NE(val, Nint(-1));
    EXPECT_NE(val, Nint(-4));
    EXPECT_NE(val, Tstr("98"));
    EXPECT_NE(val, Bstr("99"));
    EXPECT_NE(val, Bool(false));
    EXPECT_NE(val, Array(99, 1));
    EXPECT_NE(val, Map(99, 1));
}

TEST(EqualityTest, Bstr) {
    Bstr val("99");
    EXPECT_EQ(val, Bstr("99"));

    EXPECT_NE(val, Uint(99));
    EXPECT_NE(val, Nint(-1));
    EXPECT_NE(val, Nint(-4));
    EXPECT_NE(val, Tstr("99"));
    EXPECT_NE(val, Bstr("98"));
    EXPECT_NE(val, Bool(false));
    EXPECT_NE(val, Array(99, 1));
    EXPECT_NE(val, Map(99, 1));
}

TEST(EqualityTest, Bool) {
    Bool val(false);
    EXPECT_EQ(val, Bool(false));

    EXPECT_NE(val, Uint(99));
    EXPECT_NE(val, Nint(-1));
    EXPECT_NE(val, Nint(-4));
    EXPECT_NE(val, Tstr("99"));
    EXPECT_NE(val, Bstr("98"));
    EXPECT_NE(val, Bool(true));
    EXPECT_NE(val, Array(99, 1));
    EXPECT_NE(val, Map(99, 1));
}

TEST(EqualityTest, Array) {
    Array val(99, 1);
    EXPECT_EQ(val, Array(99, 1));

    EXPECT_NE(val, Uint(99));
    EXPECT_NE(val, Nint(-1));
    EXPECT_NE(val, Nint(-4));
    EXPECT_NE(val, Tstr("99"));
    EXPECT_NE(val, Bstr("98"));
    EXPECT_NE(val, Bool(true));
    EXPECT_NE(val, Array(99, 2));
    EXPECT_NE(val, Array(98, 1));
    EXPECT_NE(val, Array(99, 1, 2));
    EXPECT_NE(val, Map(99, 1));
}

TEST(EqualityTest, Map) {
    Map val(99, 1);
    EXPECT_EQ(val, Map(99, 1));

    EXPECT_NE(val, Uint(99));
    EXPECT_NE(val, Nint(-1));
    EXPECT_NE(val, Nint(-4));
    EXPECT_NE(val, Tstr("99"));
    EXPECT_NE(val, Bstr("98"));
    EXPECT_NE(val, Bool(true));
    EXPECT_NE(val, Array(99, 1));
    EXPECT_NE(val, Map(99, 2));
    EXPECT_NE(val, Map(99, 1, 99, 2));
}

TEST(ConvertTest, Uint) {
    unique_ptr<Item> item = details::makeItem(10);

    EXPECT_EQ(UINT, item->type());
    EXPECT_NE(nullptr, item->asInt());
    EXPECT_NE(nullptr, item->asUint());
    EXPECT_EQ(nullptr, item->asNint());
    EXPECT_EQ(nullptr, item->asTstr());
    EXPECT_EQ(nullptr, item->asBstr());
    EXPECT_EQ(nullptr, item->asSimple());
    EXPECT_EQ(nullptr, item->asMap());
    EXPECT_EQ(nullptr, item->asArray());

    EXPECT_EQ(10, item->asInt()->value());
    EXPECT_EQ(10, item->asUint()->value());
}

TEST(ConvertTest, Nint) {
    unique_ptr<Item> item = details::makeItem(-10);

    EXPECT_EQ(NINT, item->type());
    EXPECT_NE(nullptr, item->asInt());
    EXPECT_EQ(nullptr, item->asUint());
    EXPECT_NE(nullptr, item->asNint());
    EXPECT_EQ(nullptr, item->asTstr());
    EXPECT_EQ(nullptr, item->asBstr());
    EXPECT_EQ(nullptr, item->asSimple());
    EXPECT_EQ(nullptr, item->asMap());
    EXPECT_EQ(nullptr, item->asArray());

    EXPECT_EQ(-10, item->asInt()->value());
    EXPECT_EQ(-10, item->asNint()->value());
}

TEST(ConvertTest, Tstr) {
    unique_ptr<Item> item = details::makeItem("hello");

    EXPECT_EQ(TSTR, item->type());
    EXPECT_EQ(nullptr, item->asInt());
    EXPECT_EQ(nullptr, item->asUint());
    EXPECT_EQ(nullptr, item->asNint());
    EXPECT_NE(nullptr, item->asTstr());
    EXPECT_EQ(nullptr, item->asBstr());
    EXPECT_EQ(nullptr, item->asSimple());
    EXPECT_EQ(nullptr, item->asMap());
    EXPECT_EQ(nullptr, item->asArray());

    EXPECT_EQ("hello"s, item->asTstr()->value());
}

TEST(ConvertTest, Bstr) {
    vector<uint8_t> vec{0x23, 0x24, 0x22};
    unique_ptr<Item> item = details::makeItem(vec);

    EXPECT_EQ(BSTR, item->type());
    EXPECT_EQ(nullptr, item->asInt());
    EXPECT_EQ(nullptr, item->asUint());
    EXPECT_EQ(nullptr, item->asNint());
    EXPECT_EQ(nullptr, item->asTstr());
    EXPECT_NE(nullptr, item->asBstr());
    EXPECT_EQ(nullptr, item->asSimple());
    EXPECT_EQ(nullptr, item->asMap());
    EXPECT_EQ(nullptr, item->asArray());

    EXPECT_EQ(vec, item->asBstr()->value());
}

TEST(ConvertTest, Bool) {
    unique_ptr<Item> item = details::makeItem(false);

    EXPECT_EQ(SIMPLE, item->type());
    EXPECT_EQ(nullptr, item->asInt());
    EXPECT_EQ(nullptr, item->asUint());
    EXPECT_EQ(nullptr, item->asNint());
    EXPECT_EQ(nullptr, item->asTstr());
    EXPECT_EQ(nullptr, item->asBstr());
    EXPECT_NE(nullptr, item->asSimple());
    EXPECT_EQ(nullptr, item->asMap());
    EXPECT_EQ(nullptr, item->asArray());

    EXPECT_EQ(BOOLEAN, item->asSimple()->simpleType());
    EXPECT_NE(nullptr, item->asSimple()->asBool());

    EXPECT_FALSE(item->asSimple()->asBool()->value());
}

TEST(ConvertTest, Map) {
    unique_ptr<Item> item(new Map);

    EXPECT_EQ(MAP, item->type());
    EXPECT_EQ(nullptr, item->asInt());
    EXPECT_EQ(nullptr, item->asUint());
    EXPECT_EQ(nullptr, item->asNint());
    EXPECT_EQ(nullptr, item->asTstr());
    EXPECT_EQ(nullptr, item->asBstr());
    EXPECT_EQ(nullptr, item->asSimple());
    EXPECT_NE(nullptr, item->asMap());
    EXPECT_EQ(nullptr, item->asArray());

    EXPECT_EQ(0U, item->asMap()->size());
}

TEST(ConvertTest, Array) {
    unique_ptr<Item> item(new Array);

    EXPECT_EQ(ARRAY, item->type());
    EXPECT_EQ(nullptr, item->asInt());
    EXPECT_EQ(nullptr, item->asUint());
    EXPECT_EQ(nullptr, item->asNint());
    EXPECT_EQ(nullptr, item->asTstr());
    EXPECT_EQ(nullptr, item->asBstr());
    EXPECT_EQ(nullptr, item->asSimple());
    EXPECT_EQ(nullptr, item->asMap());
    EXPECT_NE(nullptr, item->asArray());

    EXPECT_EQ(0U, item->asArray()->size());
}

class MockParseClient : public ParseClient {
  public:
    MOCK_METHOD4(item, ParseClient*(std::unique_ptr<Item>& item, const uint8_t* hdrBegin,
                                    const uint8_t* valueBegin, const uint8_t* end));
    MOCK_METHOD4(itemEnd, ParseClient*(std::unique_ptr<Item>& item, const uint8_t* hdrBegin,
                                       const uint8_t* valueBegin, const uint8_t* end));
    MOCK_METHOD2(error, void(const uint8_t* position, const std::string& errorMessage));
};

MATCHER_P(IsType, value, std::string("Type ") + (negation ? "doesn't match" : "matches")) {
    return arg->type() == value;
}

MATCHER_P(MatchesItem, value, "") {
    return arg && *arg == value;
}

MATCHER_P(IsArrayOfSize, value, "") {
    return arg->type() == ARRAY && arg->asArray()->size() == value;
}

MATCHER_P(IsMapOfSize, value, "") {
    return arg->type() == MAP && arg->asMap()->size() == value;
}

TEST(StreamParseTest, Uint) {
    MockParseClient mpc;

    Uint val(100);
    auto encoded = val.encode();
    uint8_t* encBegin = encoded.data();
    uint8_t* encEnd = encoded.data() + encoded.size();

    EXPECT_CALL(mpc, item(MatchesItem(val), encBegin, encEnd, encEnd)).WillOnce(Return(&mpc));
    EXPECT_CALL(mpc, itemEnd(_, _, _, _)).Times(0);
    EXPECT_CALL(mpc, error(_, _)).Times(0);

    parse(encoded.data(), encoded.data() + encoded.size(), &mpc);
}

TEST(StreamParseTest, Nint) {
    MockParseClient mpc;

    Nint val(-10);
    auto encoded = val.encode();
    uint8_t* encBegin = encoded.data();
    uint8_t* encEnd = encoded.data() + encoded.size();

    EXPECT_CALL(mpc, item(MatchesItem(val), encBegin, encEnd, encEnd)).WillOnce(Return(&mpc));

    EXPECT_CALL(mpc, itemEnd(_, _, _, _)).Times(0);
    EXPECT_CALL(mpc, error(_, _)).Times(0);

    parse(encoded.data(), encoded.data() + encoded.size(), &mpc);
}

TEST(StreamParseTest, Bool) {
    MockParseClient mpc;

    Bool val(true);
    auto encoded = val.encode();
    uint8_t* encBegin = encoded.data();
    uint8_t* encEnd = encoded.data() + encoded.size();

    EXPECT_CALL(mpc, item(MatchesItem(val), encBegin, encEnd, encEnd)).WillOnce(Return(&mpc));
    EXPECT_CALL(mpc, itemEnd(_, _, _, _)).Times(0);
    EXPECT_CALL(mpc, error(_, _)).Times(0);

    parse(encoded.data(), encoded.data() + encoded.size(), &mpc);
}

TEST(StreamParseTest, Tstr) {
    MockParseClient mpc;

    Tstr val("Hello");
    auto encoded = val.encode();
    uint8_t* encBegin = encoded.data();
    uint8_t* encEnd = encoded.data() + encoded.size();

    EXPECT_CALL(mpc, item(MatchesItem(val), encBegin, encBegin + 1, encEnd)).WillOnce(Return(&mpc));
    EXPECT_CALL(mpc, itemEnd(_, _, _, _)).Times(0);
    EXPECT_CALL(mpc, error(_, _)).Times(0);

    parse(encoded.data(), encoded.data() + encoded.size(), &mpc);
}

TEST(StreamParseTest, Bstr) {
    MockParseClient mpc;

    Bstr val("Hello");
    auto encoded = val.encode();
    uint8_t* encBegin = encoded.data();
    uint8_t* encEnd = encoded.data() + encoded.size();

    EXPECT_CALL(mpc, item(MatchesItem(val), encBegin, encBegin + 1, encEnd)).WillOnce(Return(&mpc));
    EXPECT_CALL(mpc, itemEnd(_, _, _, _)).Times(0);
    EXPECT_CALL(mpc, error(_, _)).Times(0);

    parse(encoded.data(), encoded.data() + encoded.size(), &mpc);
}

TEST(StreamParseTest, Array) {
    MockParseClient mpc;

    Array val("Hello", 4, Array(-9, "Goodbye"), std::numeric_limits<uint64_t>::max());
    ASSERT_NE(val[2]->asArray(), nullptr);
    const Array& interior = *(val[2]->asArray());
    auto encoded = val.encode();
    uint8_t* encBegin = encoded.data();
    uint8_t* encEnd = encoded.data() + encoded.size();

    {
        InSequence s;
        const uint8_t* pos = encBegin;
        EXPECT_CALL(mpc, item(IsArrayOfSize(val.size()), pos, pos + 1, pos + 1))
                .WillOnce(Return(&mpc));
        ++pos;
        EXPECT_CALL(mpc, item(MatchesItem(ByRef(*val[0])), pos, pos + 1, pos + 6))
                .WillOnce(Return(&mpc));
        pos += 6;
        EXPECT_CALL(mpc, item(MatchesItem(ByRef(*val[1])), pos, pos + 1, pos + 1))
                .WillOnce(Return(&mpc));
        ++pos;
        const uint8_t* innerArrayBegin = pos;
        EXPECT_CALL(mpc, item(IsArrayOfSize(interior.size()), pos, pos + 1, pos + 1))
                .WillOnce(Return(&mpc));
        ++pos;
        EXPECT_CALL(mpc, item(MatchesItem(ByRef(*interior[0])), pos, pos + 1, pos + 1))
                .WillOnce(Return(&mpc));
        ++pos;
        EXPECT_CALL(mpc, item(MatchesItem(ByRef(*interior[1])), pos, pos + 1, pos + 8))
                .WillOnce(Return(&mpc));
        pos += 8;
        EXPECT_CALL(mpc, itemEnd(IsArrayOfSize(interior.size()), innerArrayBegin,
                                 innerArrayBegin + 1, pos))
                .WillOnce(Return(&mpc));
        EXPECT_CALL(mpc, item(MatchesItem(ByRef(*val[3])), pos, pos + 9, pos + 9))
                .WillOnce(Return(&mpc));
        EXPECT_CALL(mpc, itemEnd(IsArrayOfSize(val.size()), encBegin, encBegin + 1, encEnd))
                .WillOnce(Return(&mpc));
    }

    EXPECT_CALL(mpc, error(_, _))  //
            .Times(0);

    parse(encoded.data(), encoded.data() + encoded.size(), &mpc);
}

TEST(StreamParseTest, Map) {
    MockParseClient mpc;

    Map val("Hello", 4, Array(-9, "Goodbye"), std::numeric_limits<uint64_t>::max());
    ASSERT_NE(val[1].first->asArray(), nullptr);
    const Array& interior = *(val[1].first->asArray());
    auto encoded = val.encode();
    uint8_t* encBegin = encoded.data();
    uint8_t* encEnd = encoded.data() + encoded.size();

    {
        InSequence s;
        const uint8_t* pos = encBegin;
        EXPECT_CALL(mpc, item(_, pos, pos + 1, pos + 1)).WillOnce(Return(&mpc));
        ++pos;
        EXPECT_CALL(mpc, item(MatchesItem(ByRef(*val[0].first)), pos, pos + 1, pos + 6))
                .WillOnce(Return(&mpc));
        pos += 6;
        EXPECT_CALL(mpc, item(MatchesItem(ByRef(*val[0].second)), pos, pos + 1, pos + 1))
                .WillOnce(Return(&mpc));
        ++pos;
        const uint8_t* innerArrayBegin = pos;
        EXPECT_CALL(mpc, item(IsArrayOfSize(interior.size()), pos, pos + 1, pos + 1))
                .WillOnce(Return(&mpc));
        ++pos;
        EXPECT_CALL(mpc, item(MatchesItem(ByRef(*interior[0])), pos, pos + 1, pos + 1))
                .WillOnce(Return(&mpc));
        ++pos;
        EXPECT_CALL(mpc, item(MatchesItem(ByRef(*interior[1])), pos, pos + 1, pos + 8))
                .WillOnce(Return(&mpc));
        pos += 8;
        EXPECT_CALL(mpc, itemEnd(IsArrayOfSize(interior.size()), innerArrayBegin,
                                 innerArrayBegin + 1, pos))
                .WillOnce(Return(&mpc));
        EXPECT_CALL(mpc, item(MatchesItem(ByRef(*val[1].second)), pos, pos + 9, pos + 9))
                .WillOnce(Return(&mpc));
        EXPECT_CALL(mpc, itemEnd(IsMapOfSize(val.size()), encBegin, encBegin + 1, encEnd))
                .WillOnce(Return(&mpc));
    }

    EXPECT_CALL(mpc, error(_, _))  //
            .Times(0);

    parse(encoded.data(), encoded.data() + encoded.size(), &mpc);
}

TEST(StreamParseTest, Semantic) {
    MockParseClient mpc;

    vector<uint8_t> encoded;
    auto iter = back_inserter(encoded);
    encodeHeader(SEMANTIC, 0, iter);
    Uint(999).encode(iter);

    EXPECT_CALL(mpc, item(_, _, _, _)).Times(0);
    EXPECT_CALL(mpc, itemEnd(_, _, _, _)).Times(0);
    EXPECT_CALL(mpc, error(encoded.data(), "Semantic tags not supported"));

    parse(encoded.data(), encoded.data() + encoded.size(), &mpc);
}

TEST(FullParserTest, Uint) {
    Uint val(10);

    auto [item, pos, message] = parse(val.encode());
    EXPECT_THAT(item, MatchesItem(val));
}

TEST(FullParserTest, Nint) {
    Nint val(-10);

    auto [item, pos, message] = parse(val.encode());
    EXPECT_THAT(item, MatchesItem(val));

    vector<uint8_t> minNint = {0x3B, 0x7F, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF};

    std::tie(item, pos, message) = parse(minNint);
    EXPECT_THAT(item, NotNull());
    EXPECT_EQ(item->asNint()->value(), std::numeric_limits<int64_t>::min());
}

TEST(FullParserTest, NintOutOfRange) {
    vector<uint8_t> outOfRangeNint = {0x3B, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF};

    auto [item, pos, message] = parse(outOfRangeNint);
    EXPECT_THAT(item, IsNull());
    EXPECT_EQ(pos, outOfRangeNint.data());
    EXPECT_EQ(message, "NINT values that don't fit in int64_t are not supported.");
}

TEST(FullParserTest, Tstr) {
    Tstr val("Hello");

    auto [item, pos, message] = parse(val.encode());
    EXPECT_THAT(item, MatchesItem(val));
}

TEST(FullParserTest, Bstr) {
    Bstr val("\x00\x01\0x02"s);

    auto [item, pos, message] = parse(val.encode());
    EXPECT_THAT(item, MatchesItem(val));
}

TEST(FullParserTest, Array) {
    Array val("hello", -4, 3);

    auto encoded = val.encode();
    auto [item, pos, message] = parse(encoded);
    EXPECT_THAT(item, MatchesItem(ByRef(val)));
    EXPECT_EQ(pos, encoded.data() + encoded.size());
    EXPECT_EQ("", message);

    // We've already checked it all, but walk it just for fun.
    ASSERT_NE(nullptr, item->asArray());
    const Array& arr = *(item->asArray());
    ASSERT_EQ(arr[0]->type(), TSTR);
    EXPECT_EQ(arr[0]->asTstr()->value(), "hello");
}

TEST(FullParserTest, Map) {
    Map val("hello", -4, 3, Bstr("hi"));

    auto [item, pos, message] = parse(val.encode());
    EXPECT_THAT(item, MatchesItem(ByRef(val)));
}

TEST(FullParserTest, Complex) {
    vector<uint8_t> vec = {0x01, 0x02, 0x08, 0x03};
    Map val("Outer1",
            Array(Map("Inner1", 99,  //
                      "Inner2", vec),
                  "foo"),
            "Outer2", 10);

    std::unique_ptr<Item> item;
    const uint8_t* pos;
    std::string message;
    std::tie(item, pos, message) = parse(val.encode());
    EXPECT_THAT(item, MatchesItem(ByRef(val)));
}

TEST(FullParserTest, IncompleteUint) {
    Uint val(1000);

    auto encoding = val.encode();
    auto [item, pos, message] = parse(encoding.data(), encoding.size() - 1);
    EXPECT_EQ(nullptr, item.get());
    EXPECT_EQ(encoding.data(), pos);
    EXPECT_EQ("Need 2 byte(s) for length field, have 1.", message);
}

TEST(FullParserTest, IncompleteString) {
    Tstr val("hello");

    auto encoding = val.encode();
    auto [item, pos, message] = parse(encoding.data(), encoding.size() - 2);
    EXPECT_EQ(nullptr, item.get());
    EXPECT_EQ(encoding.data(), pos);
    EXPECT_EQ("Need 5 byte(s) for text string, have 3.", message);
}

TEST(FullParserTest, ArrayWithInsufficientEntries) {
    Array val(1, 2, 3, 4);

    auto encoding = val.encode();
    auto [item, pos, message] = parse(encoding.data(), encoding.size() - 1);
    EXPECT_EQ(nullptr, item.get());
    EXPECT_EQ(encoding.data(), pos);
    EXPECT_EQ("Not enough entries for array.", message);
}

TEST(FullParserTest, ArrayWithTruncatedEntry) {
    Array val(1, 2, 3, 400000);

    auto encoding = val.encode();
    auto [item, pos, message] = parse(encoding.data(), encoding.size() - 1);
    EXPECT_EQ(nullptr, item.get());
    EXPECT_EQ(encoding.data() + encoding.size() - 5, pos);
    EXPECT_EQ("Need 4 byte(s) for length field, have 3.", message);
}

TEST(FullParserTest, MapWithTruncatedEntry) {
    Map val(1, 2, 300000, 4);

    auto encoding = val.encode();
    auto [item, pos, message] = parse(encoding.data(), encoding.size() - 2);
    EXPECT_EQ(nullptr, item.get());
    EXPECT_EQ(encoding.data() + 3, pos);
    EXPECT_EQ("Need 4 byte(s) for length field, have 3.", message);
}
int main(int argc, char** argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
