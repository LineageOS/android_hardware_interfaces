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

#include "KeymasterHidlTest.h"

namespace android {
namespace hardware {
namespace keymaster {
namespace V4_0 {
namespace test {

/**
 * HmacKeySharingTest extends KeymasterHidlTest with some utilities that make writing HMAC sharing
 * tests easier.
 */
class HmacKeySharingTest : public KeymasterHidlTest {
   protected:
     const std::vector<sp<IKeymasterDevice>>& allKeymasters() {
         if (all_keymasters_.empty()) {
             auto names = android::hardware::getAllHalInstanceNames(IKeymasterDevice::descriptor);
             for (const auto& name : names) {
                 all_keymasters_.push_back(IKeymasterDevice::getService(name));
             }
         }
         return all_keymasters_;
     }

    struct GetParamsResult {
        ErrorCode error;
        HmacSharingParameters params;
        auto tie() { return std::tie(error, params); }
    };

    struct ComputeHmacResult {
        ErrorCode error;
        HidlBuf sharing_check;
        auto tie() { return std::tie(error, sharing_check); }
    };

    using KeymasterVec = std::vector<sp<IKeymasterDevice>>;
    using ByteString = std::basic_string<uint8_t>;
    // using NonceVec = std::vector<HidlBuf>;

    GetParamsResult getHmacSharingParameters(IKeymasterDevice& keymaster) {
        GetParamsResult result;
        EXPECT_TRUE(keymaster
                        .getHmacSharingParameters([&](auto error, auto params) {
                            result.tie() = std::tie(error, params);
                        })
                        .isOk());
        return result;
    }

    hidl_vec<HmacSharingParameters> getHmacSharingParameters(const KeymasterVec& keymasters) {
        std::vector<HmacSharingParameters> paramsVec;
        for (auto& keymaster : keymasters) {
            auto result = getHmacSharingParameters(*keymaster);
            EXPECT_EQ(ErrorCode::OK, result.error);
            if (result.error == ErrorCode::OK) paramsVec.push_back(std::move(result.params));
        }
        return paramsVec;
    }

    ComputeHmacResult computeSharedHmac(IKeymasterDevice& keymaster,
                                        const hidl_vec<HmacSharingParameters>& params) {
        ComputeHmacResult result;
        EXPECT_TRUE(keymaster
                        .computeSharedHmac(params,
                                           [&](auto error, auto params) {
                                               result.tie() = std::tie(error, params);
                                           })
                        .isOk());
        return result;
    }

    std::vector<ComputeHmacResult> computeSharedHmac(
        const KeymasterVec& keymasters, const hidl_vec<HmacSharingParameters>& paramsVec) {
        std::vector<ComputeHmacResult> resultVec;
        for (auto& keymaster : keymasters) {
            resultVec.push_back(computeSharedHmac(*keymaster, paramsVec));
        }
        return resultVec;
    }

    std::vector<ByteString> copyNonces(const hidl_vec<HmacSharingParameters>& paramsVec) {
        std::vector<ByteString> nonces;
        for (auto& param : paramsVec) {
            nonces.emplace_back(param.nonce.data(), param.nonce.size());
        }
        return nonces;
    }

    void verifyResponses(const HidlBuf& expected, const std::vector<ComputeHmacResult>& responses) {
        for (auto& response : responses) {
            EXPECT_EQ(ErrorCode::OK, response.error);
            EXPECT_EQ(expected, response.sharing_check) << "Sharing check values should match.";
        }
    }

  private:
    static std::vector<sp<IKeymasterDevice>> all_keymasters_;
};

std::vector<sp<IKeymasterDevice>> HmacKeySharingTest::all_keymasters_;

TEST_P(HmacKeySharingTest, GetParameters) {
    auto result1 = getHmacSharingParameters(keymaster());
    EXPECT_EQ(ErrorCode::OK, result1.error);

    auto result2 = getHmacSharingParameters(keymaster());
    EXPECT_EQ(ErrorCode::OK, result2.error);

    ASSERT_EQ(result1.params.seed, result2.params.seed)
        << "A given keymaster should always return the same seed.";
    ASSERT_EQ(result1.params.nonce, result2.params.nonce)
        << "A given keymaster should always return the same nonce until restart.";
}

TEST_P(HmacKeySharingTest, ComputeSharedHmac) {
    auto params = getHmacSharingParameters(allKeymasters());
    ASSERT_EQ(allKeymasters().size(), params.size())
            << "One or more keymasters failed to provide parameters.";

    auto nonces = copyNonces(params);
    EXPECT_EQ(allKeymasters().size(), nonces.size());
    std::sort(nonces.begin(), nonces.end());
    std::unique(nonces.begin(), nonces.end());
    EXPECT_EQ(allKeymasters().size(), nonces.size());

    auto responses = computeSharedHmac(allKeymasters(), params);
    ASSERT_GT(responses.size(), 0U);
    verifyResponses(responses[0].sharing_check, responses);

    // Do it a second time.  Should get the same answers.
    params = getHmacSharingParameters(allKeymasters());
    ASSERT_EQ(allKeymasters().size(), params.size())
            << "One or more keymasters failed to provide parameters.";

    responses = computeSharedHmac(allKeymasters(), params);
    ASSERT_GT(responses.size(), 0U);
    ASSERT_EQ(32U, responses[0].sharing_check.size());
    verifyResponses(responses[0].sharing_check, responses);
}

template <class F>
class final_action {
   public:
     explicit final_action(F f) : f_(std::move(f)) {}
     ~final_action() { f_(); }

   private:
    F f_;
};

template <class F>
inline final_action<F> finally(const F& f) {
    return final_action<F>(f);
}

TEST_P(HmacKeySharingTest, ComputeSharedHmacCorruptNonce) {
    // Important: The execution of this test gets the keymaster implementations on the device out of
    // sync with respect to the HMAC key.  Granted that VTS tests aren't run on in-use production
    // devices, this still has the potential to cause confusion.  To mitigate that, we always
    // (barring crashes :-/) re-run the unmodified agreement process on our way out.
    auto fixup_hmac = finally([&]() {
        computeSharedHmac(allKeymasters(), getHmacSharingParameters(allKeymasters()));
    });

    auto params = getHmacSharingParameters(allKeymasters());
    ASSERT_EQ(allKeymasters().size(), params.size())
            << "One or more keymasters failed to provide parameters.";

    // All should be well in the normal case
    auto responses = computeSharedHmac(allKeymasters(), params);

    ASSERT_GT(responses.size(), 0U);
    HidlBuf correct_response = responses[0].sharing_check;
    verifyResponses(correct_response, responses);

    // Pick a random param, a random byte within the param's nonce, and a random bit within
    // the byte.  Flip that bit.
    size_t param_to_tweak = rand() % params.size();
    uint8_t byte_to_tweak = rand() % sizeof(params[param_to_tweak].nonce);
    uint8_t bit_to_tweak = rand() % 8;
    params[param_to_tweak].nonce[byte_to_tweak] ^= (1 << bit_to_tweak);

    responses = computeSharedHmac(allKeymasters(), params);
    for (size_t i = 0; i < responses.size(); ++i) {
        if (i == param_to_tweak) {
            EXPECT_EQ(ErrorCode::INVALID_ARGUMENT, responses[i].error)
                << "Keymaster that provided tweaked param should fail to compute HMAC key";
        } else {
            EXPECT_EQ(ErrorCode::OK, responses[i].error) << "Others should succeed";
            EXPECT_NE(correct_response, responses[i].sharing_check)
                << "Others should calculate a different HMAC key, due to the tweaked nonce.";
        }
    }
}

TEST_P(HmacKeySharingTest, ComputeSharedHmacCorruptSeed) {
    // Important: The execution of this test gets the keymaster implementations on the device out of
    // sync with respect to the HMAC key.  Granted that VTS tests aren't run on in-use production
    // devices, this still has the potential to cause confusion.  To mitigate that, we always
    // (barring crashes :-/) re-run the unmodified agreement process on our way out.
    auto fixup_hmac = finally([&]() {
        computeSharedHmac(allKeymasters(), getHmacSharingParameters(allKeymasters()));
    });

    auto params = getHmacSharingParameters(allKeymasters());
    ASSERT_EQ(allKeymasters().size(), params.size())
            << "One or more keymasters failed to provide parameters.";

    // All should be well in the normal case
    auto responses = computeSharedHmac(allKeymasters(), params);

    ASSERT_GT(responses.size(), 0U);
    HidlBuf correct_response = responses[0].sharing_check;
    verifyResponses(correct_response, responses);

    // Pick a random param and modify the seed.  We just increase the seed length by 1.  It doesn't
    // matter what value is in the additional byte; it changes the seed regardless.
    auto param_to_tweak = rand() % params.size();
    auto& to_tweak = params[param_to_tweak].seed;
    ASSERT_TRUE(to_tweak.size() == 32 || to_tweak.size() == 0);
    if (!to_tweak.size()) {
        to_tweak.resize(32);  // Contents don't matter; a little randomization is nice.
    }
    to_tweak[0]++;

    responses = computeSharedHmac(allKeymasters(), params);
    for (size_t i = 0; i < responses.size(); ++i) {
        if (i == param_to_tweak) {
            EXPECT_EQ(ErrorCode::INVALID_ARGUMENT, responses[i].error)
                << "Keymaster that provided tweaked param should fail to compute HMAC key ";
        } else {
            EXPECT_EQ(ErrorCode::OK, responses[i].error) << "Others should succeed";
            EXPECT_NE(correct_response, responses[i].sharing_check)
                << "Others should calculate a different HMAC key, due to the tweaked nonce.";
        }
    }
}

INSTANTIATE_KEYMASTER_HIDL_TEST(HmacKeySharingTest);

}  // namespace test
}  // namespace V4_0
}  // namespace keymaster
}  // namespace hardware
}  // namespace android
