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

#include <broadcastradio-utils-2x/Utils.h>
#include <gtest/gtest.h>

namespace {

namespace V2_0 = android::hardware::broadcastradio::V2_0;
namespace utils = android::hardware::broadcastradio::utils;

using V2_0::IdentifierType;
using V2_0::ProgramSelector;

TEST(IdentifierIteratorTest, singleSecondary) {
    // clang-format off
    V2_0::ProgramSelector sel {
        utils::make_identifier(IdentifierType::RDS_PI, 0xBEEF),
        {utils::make_identifier(IdentifierType::AMFM_FREQUENCY, 100100)}
    };
    // clang-format on

    auto it = V2_0::begin(sel);
    auto end = V2_0::end(sel);

    ASSERT_NE(end, it);
    EXPECT_EQ(sel.primaryId, *it);
    ASSERT_NE(end, ++it);
    EXPECT_EQ(sel.secondaryIds[0], *it);
    ASSERT_EQ(end, ++it);
}

TEST(IdentifierIteratorTest, empty) {
    V2_0::ProgramSelector sel{};

    auto it = V2_0::begin(sel);
    auto end = V2_0::end(sel);

    ASSERT_NE(end, it++);  // primary id is always present
    ASSERT_EQ(end, it);
}

TEST(IdentifierIteratorTest, twoSelectors) {
    V2_0::ProgramSelector sel1{};
    V2_0::ProgramSelector sel2{};

    auto it1 = V2_0::begin(sel1);
    auto it2 = V2_0::begin(sel2);

    EXPECT_NE(it1, it2);
}

TEST(IdentifierIteratorTest, increments) {
    V2_0::ProgramSelector sel{{}, {{}, {}}};

    auto it = V2_0::begin(sel);
    auto end = V2_0::end(sel);
    auto pre = it;
    auto post = it;

    EXPECT_NE(++pre, post++);
    EXPECT_EQ(pre, post);
    EXPECT_EQ(pre, it + 1);
    ASSERT_NE(end, pre);
}

TEST(IdentifierIteratorTest, findType) {
    using namespace std::placeholders;

    uint64_t rds_pi1 = 0xDEAD;
    uint64_t rds_pi2 = 0xBEEF;
    uint64_t freq1 = 100100;
    uint64_t freq2 = 107900;

    // clang-format off
    V2_0::ProgramSelector sel {
        utils::make_identifier(IdentifierType::RDS_PI, rds_pi1),
        {
            utils::make_identifier(IdentifierType::AMFM_FREQUENCY, freq1),
            utils::make_identifier(IdentifierType::RDS_PI, rds_pi2),
            utils::make_identifier(IdentifierType::AMFM_FREQUENCY, freq2),
        }
    };
    // clang-format on

    auto typeEquals = [](const V2_0::ProgramIdentifier& id, V2_0::IdentifierType type) {
        return utils::getType(id) == type;
    };
    auto isRdsPi = std::bind(typeEquals, _1, IdentifierType::RDS_PI);
    auto isFreq = std::bind(typeEquals, _1, IdentifierType::AMFM_FREQUENCY);

    auto end = V2_0::end(sel);
    auto it = std::find_if(V2_0::begin(sel), end, isRdsPi);
    ASSERT_NE(end, it);
    EXPECT_EQ(rds_pi1, it->value);

    it = std::find_if(it + 1, end, isRdsPi);
    ASSERT_NE(end, it);
    EXPECT_EQ(rds_pi2, it->value);

    it = std::find_if(V2_0::begin(sel), end, isFreq);
    ASSERT_NE(end, it);
    EXPECT_EQ(freq1, it->value);

    it = std::find_if(++it, end, isFreq);
    ASSERT_NE(end, it);
    EXPECT_EQ(freq2, it->value);
}

TEST(IdentifierIteratorTest, rangeLoop) {
    V2_0::ProgramSelector sel{{}, {{}, {}, {}}};

    unsigned count = 0;
    for (auto&& id : sel) {
        ASSERT_EQ(0u, id.type);
        count++;
    }

    const auto expectedCount = 1 + sel.secondaryIds.size();
    ASSERT_EQ(expectedCount, count);
}

}  // anonymous namespace
