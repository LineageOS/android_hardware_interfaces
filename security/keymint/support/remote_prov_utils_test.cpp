/*
 * Copyright 2021 The Android Open Source Project
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

#include <android-base/properties.h>
#include <cppbor_parse.h>
#include <gmock/gmock.h>
#include <gtest/gtest.h>
#include <keymaster/android_keymaster_utils.h>
#include <keymaster/logger.h>
#include <keymaster/remote_provisioning_utils.h>
#include <openssl/curve25519.h>
#include <remote_prov/remote_prov_utils.h>
#include <cstdint>
#include "cppbor.h"
#include "keymaster/cppcose/cppcose.h"

namespace aidl::android::hardware::security::keymint::remote_prov {
namespace {

using ::keymaster::KeymasterBlob;
using ::keymaster::validateAndExtractEekPubAndId;
using ::testing::ElementsAreArray;

TEST(RemoteProvUtilsTest, GenerateEekChainInvalidLength) {
    ASSERT_FALSE(generateEekChain(1, /*eekId=*/{}));
}

TEST(RemoteProvUtilsTest, GenerateEekChain) {
    bytevec kTestEekId = {'t', 'e', 's', 't', 'I', 'd', 0};
    for (size_t length : {2, 3, 31}) {
        auto get_eek_result = generateEekChain(length, kTestEekId);
        ASSERT_TRUE(get_eek_result) << get_eek_result.message();

        auto& [chain, pubkey, privkey] = *get_eek_result;

        auto validation_result = validateAndExtractEekPubAndId(
                /*testMode=*/true, KeymasterBlob(chain.data(), chain.size()));
        ASSERT_TRUE(validation_result.isOk());

        auto& [eekPub, eekId] = *validation_result;
        EXPECT_THAT(eekId, ElementsAreArray(kTestEekId));
        EXPECT_THAT(eekPub, ElementsAreArray(pubkey));
    }
}

TEST(RemoteProvUtilsTest, GetProdEekChain) {
    auto chain = getProdEekChain();

    auto validation_result = validateAndExtractEekPubAndId(
            /*testMode=*/false, KeymasterBlob(chain.data(), chain.size()));
    ASSERT_TRUE(validation_result.isOk()) << "Error: " << validation_result.moveError();

    auto& [eekPub, eekId] = *validation_result;

    auto [geekCert, ignoredNewPos, error] =
            cppbor::parse(kCoseEncodedGeekCert, sizeof(kCoseEncodedGeekCert));
    ASSERT_NE(geekCert, nullptr) << "Error: " << error;
    ASSERT_NE(geekCert->asArray(), nullptr);

    auto& encodedGeekCoseKey = geekCert->asArray()->get(kCoseSign1Payload);
    ASSERT_NE(encodedGeekCoseKey, nullptr);
    ASSERT_NE(encodedGeekCoseKey->asBstr(), nullptr);

    auto geek = CoseKey::parse(encodedGeekCoseKey->asBstr()->value());
    ASSERT_TRUE(geek) << "Error: " << geek.message();

    const std::vector<uint8_t> empty;
    EXPECT_THAT(eekId, ElementsAreArray(geek->getBstrValue(CoseKey::KEY_ID).value_or(empty)));
    EXPECT_THAT(eekPub, ElementsAreArray(geek->getBstrValue(CoseKey::PUBKEY_X).value_or(empty)));
}

TEST(RemoteProvUtilsTest, JsonEncodeCsr) {
    cppbor::Array array;
    array.add(1);

    auto [json, error] = jsonEncodeCsrWithBuild(array);

    ASSERT_TRUE(error.empty()) << error;

    std::string expected = R"({"build_fingerprint":")" +
                           ::android::base::GetProperty("ro.build.fingerprint", /*default=*/"") +
                           R"(","csr":"gQE="})";

    ASSERT_EQ(json, expected);
}

}  // namespace
}  // namespace aidl::android::hardware::security::keymint::remote_prov
