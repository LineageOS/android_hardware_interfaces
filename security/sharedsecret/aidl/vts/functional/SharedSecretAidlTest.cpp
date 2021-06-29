/*
 * Copyright (C) 2020 The Android Open Source Project
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

#define LOG_TAG "sharedsecret_test"
#include <android-base/logging.h>

#include <aidl/Vintf.h>
#include <aidl/android/hardware/security/keymint/ErrorCode.h>
#include <aidl/android/hardware/security/sharedsecret/ISharedSecret.h>
#include <android/binder_manager.h>
#include <gtest/gtest.h>
#include <vector>

namespace aidl::android::hardware::security::sharedsecret::test {
using ::aidl::android::hardware::security::keymint::ErrorCode;
using ::std::shared_ptr;
using ::std::vector;
using Status = ::ndk::ScopedAStatus;

class SharedSecretAidlTest : public ::testing::Test {
  public:
    struct GetParamsResult {
        ErrorCode error;
        SharedSecretParameters params;
        auto tie() { return std::tie(error, params); }
    };

    struct ComputeResult {
        ErrorCode error;
        vector<uint8_t> sharing_check;
        auto tie() { return std::tie(error, sharing_check); }
    };

    GetParamsResult getSharedSecretParameters(shared_ptr<ISharedSecret>& sharedSecret) {
        SharedSecretParameters params;
        auto error = GetReturnErrorCode(sharedSecret->getSharedSecretParameters(&params));
        EXPECT_EQ(ErrorCode::OK, error);
        EXPECT_TRUE(params.seed.size() == 0 || params.seed.size() == 32);
        EXPECT_TRUE(params.nonce.size() == 32);

        GetParamsResult result;
        result.tie() = std::tie(error, params);
        return result;
    }

    vector<SharedSecretParameters> getAllSharedSecretParameters() {
        vector<SharedSecretParameters> paramsVec;
        for (auto& sharedSecret : allSharedSecrets_) {
            auto result = getSharedSecretParameters(sharedSecret);
            EXPECT_EQ(ErrorCode::OK, result.error);
            if (result.error == ErrorCode::OK) paramsVec.push_back(std::move(result.params));
        }
        return paramsVec;
    }

    ComputeResult computeSharedSecret(shared_ptr<ISharedSecret>& sharedSecret,
                                      const vector<SharedSecretParameters>& params) {
        std::vector<uint8_t> sharingCheck;
        auto error = GetReturnErrorCode(sharedSecret->computeSharedSecret(params, &sharingCheck));
        ComputeResult result;
        result.tie() = std::tie(error, sharingCheck);
        return result;
    }

    vector<ComputeResult> computeAllSharedSecrets(const vector<SharedSecretParameters>& params) {
        vector<ComputeResult> result;
        for (auto& sharedSecret : allSharedSecrets_) {
            result.push_back(computeSharedSecret(sharedSecret, params));
        }
        return result;
    }

    vector<vector<uint8_t>> copyNonces(const vector<SharedSecretParameters>& paramsVec) {
        vector<vector<uint8_t>> nonces;
        for (auto& param : paramsVec) {
            nonces.push_back(param.nonce);
        }
        return nonces;
    }

    void verifyResponses(const vector<uint8_t>& expected, const vector<ComputeResult>& responses) {
        for (auto& response : responses) {
            EXPECT_EQ(ErrorCode::OK, response.error);
            EXPECT_EQ(expected, response.sharing_check) << "Sharing check values should match.";
        }
    }

    ErrorCode GetReturnErrorCode(const Status& result) {
        if (result.isOk()) return ErrorCode::OK;
        if (result.getExceptionCode() == EX_SERVICE_SPECIFIC) {
            return static_cast<ErrorCode>(result.getServiceSpecificError());
        }
        return ErrorCode::UNKNOWN_ERROR;
    }

    static shared_ptr<ISharedSecret> getSharedSecretService(const char* name) {
        if (AServiceManager_isDeclared(name)) {
            ::ndk::SpAIBinder binder(AServiceManager_waitForService(name));
            return ISharedSecret::fromBinder(binder);
        }
        return nullptr;
    }

    const vector<shared_ptr<ISharedSecret>>& allSharedSecrets() { return allSharedSecrets_; }

    static void SetUpTestCase() {
        ASSERT_TRUE(allSharedSecrets_.empty()) << "The Shared Secret vector is not empty.";
        auto names = ::android::getAidlHalInstanceNames(ISharedSecret::descriptor);
        for (const auto& name : names) {
            auto servicePtr = getSharedSecretService(name.c_str());
            if (servicePtr != nullptr) allSharedSecrets_.push_back(std::move(servicePtr));
        }
    }

    static void TearDownTestCase() {}
    void SetUp() override {}
    void TearDown() override {}

  private:
    static vector<shared_ptr<ISharedSecret>> allSharedSecrets_;
};

vector<shared_ptr<ISharedSecret>> SharedSecretAidlTest::allSharedSecrets_;

TEST_F(SharedSecretAidlTest, GetParameters) {
    auto sharedSecrets = allSharedSecrets();
    if (sharedSecrets.empty()) {
        GTEST_SKIP() << "Skipping the test because no shared secret service is found.";
    }
    for (auto sharedSecret : sharedSecrets) {
        auto result1 = getSharedSecretParameters(sharedSecret);
        EXPECT_EQ(ErrorCode::OK, result1.error);
        auto result2 = getSharedSecretParameters(sharedSecret);
        EXPECT_EQ(ErrorCode::OK, result2.error);
        ASSERT_EQ(result1.params.seed, result2.params.seed)
                << "A given shared secret service should always return the same seed.";
        ASSERT_EQ(result1.params.nonce, result2.params.nonce)
                << "A given shared secret service should always return the same nonce until "
                   "restart.";
    }
}

TEST_F(SharedSecretAidlTest, ComputeSharedSecret) {
    auto sharedSecrets = allSharedSecrets();
    if (sharedSecrets.empty()) {
        GTEST_SKIP() << "Skipping the test as no shared secret service is found.";
    }
    auto params = getAllSharedSecretParameters();
    ASSERT_EQ(sharedSecrets.size(), params.size())
            << "One or more shared secret services failed to provide parameters.";
    auto nonces = copyNonces(params);
    EXPECT_EQ(sharedSecrets.size(), nonces.size());
    std::sort(nonces.begin(), nonces.end());
    std::unique(nonces.begin(), nonces.end());
    EXPECT_EQ(sharedSecrets.size(), nonces.size());

    auto responses = computeAllSharedSecrets(params);
    ASSERT_GT(responses.size(), 0U);
    verifyResponses(responses[0].sharing_check, responses);

    // Do it a second time.  Should get the same answers.
    params = getAllSharedSecretParameters();
    ASSERT_EQ(sharedSecrets.size(), params.size())
            << "One or more shared secret services failed to provide parameters.";

    responses = computeAllSharedSecrets(params);
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

TEST_F(SharedSecretAidlTest, ComputeSharedSecretCorruptNonce) {
    auto sharedSecrets = allSharedSecrets();
    if (sharedSecrets.empty()) {
        GTEST_SKIP() << "Skipping the test as no shared secret service is found.";
    }
    auto fixup_hmac = finally([&]() { computeAllSharedSecrets(getAllSharedSecretParameters()); });

    auto params = getAllSharedSecretParameters();
    ASSERT_EQ(sharedSecrets.size(), params.size())
            << "One or more shared secret services failed to provide parameters.";

    // All should be well in the normal case
    auto responses = computeAllSharedSecrets(params);

    ASSERT_GT(responses.size(), 0U);
    vector<uint8_t> correct_response = responses[0].sharing_check;
    verifyResponses(correct_response, responses);

    // Pick a random param, a random byte within the param's nonce, and a random bit within
    // the byte.  Flip that bit.
    size_t param_to_tweak = rand() % params.size();
    uint8_t byte_to_tweak = rand() % sizeof(params[param_to_tweak].nonce);
    uint8_t bit_to_tweak = rand() % 8;
    params[param_to_tweak].nonce[byte_to_tweak] ^= (1 << bit_to_tweak);

    responses = computeAllSharedSecrets(params);
    for (size_t i = 0; i < responses.size(); ++i) {
        if (i == param_to_tweak) {
            EXPECT_EQ(ErrorCode::INVALID_ARGUMENT, responses[i].error)
                    << "Shared secret service that provided tweaked param should fail to compute "
                       "shared secret";
        } else {
            EXPECT_EQ(ErrorCode::OK, responses[i].error) << "Others should succeed";
            EXPECT_NE(correct_response, responses[i].sharing_check)
                    << "Others should calculate a different shared secret, due to the tweaked "
                       "nonce.";
        }
    }
}

TEST_F(SharedSecretAidlTest, ComputeSharedSecretShortNonce) {
    auto sharedSecrets = allSharedSecrets();
    if (sharedSecrets.empty()) {
        GTEST_SKIP() << "Skipping the test as no shared secret service is found.";
    }
    auto fixup_hmac = finally([&]() { computeAllSharedSecrets(getAllSharedSecretParameters()); });

    auto params = getAllSharedSecretParameters();
    ASSERT_EQ(sharedSecrets.size(), params.size())
            << "One or more shared secret services failed to provide parameters.";

    // All should be well in the normal case
    auto responses = computeAllSharedSecrets(params);

    ASSERT_GT(responses.size(), 0U);
    vector<uint8_t> correct_response = responses[0].sharing_check;
    verifyResponses(correct_response, responses);

    // Pick a random param and shorten that nonce by one.
    size_t param_to_tweak = rand() % params.size();
    auto& to_tweak = params[param_to_tweak].nonce;
    ASSERT_TRUE(to_tweak.size() == 32);
    to_tweak.resize(31);

    responses = computeAllSharedSecrets(params);
    for (size_t i = 0; i < responses.size(); ++i) {
        if (i == param_to_tweak) {
            EXPECT_EQ(ErrorCode::INVALID_ARGUMENT, responses[i].error)
                    << "Shared secret service that provided tweaked param should fail to compute "
                       "shared secret";
        } else {
            // Other services *may* succeed, or may notice the invalid size for the nonce.
            // However, if another service completes the computation, it should get the 'wrong'
            // answer.
            if (responses[i].error == ErrorCode::OK) {
                EXPECT_NE(correct_response, responses[i].sharing_check)
                        << "Others should calculate a different shared secret, due to the tweaked "
                           "nonce.";
            } else {
                EXPECT_EQ(ErrorCode::INVALID_ARGUMENT, responses[i].error);
            }
        }
    }
}

TEST_F(SharedSecretAidlTest, ComputeSharedSecretCorruptSeed) {
    auto sharedSecrets = allSharedSecrets();
    if (sharedSecrets.empty()) {
        GTEST_SKIP() << "Skipping the test as no shared secret service is found.";
    }
    auto fixup_hmac = finally([&]() { computeAllSharedSecrets(getAllSharedSecretParameters()); });
    auto params = getAllSharedSecretParameters();
    ASSERT_EQ(sharedSecrets.size(), params.size())
            << "One or more shared secret service failed to provide parameters.";

    // All should be well in the normal case
    auto responses = computeAllSharedSecrets(params);

    ASSERT_GT(responses.size(), 0U);
    vector<uint8_t> correct_response = responses[0].sharing_check;
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

    responses = computeAllSharedSecrets(params);
    for (size_t i = 0; i < responses.size(); ++i) {
        if (i == param_to_tweak) {
            EXPECT_EQ(ErrorCode::INVALID_ARGUMENT, responses[i].error)
                    << "Shared secret service that provided tweaked param should fail to compute "
                       "shared secret";
        } else {
            EXPECT_EQ(ErrorCode::OK, responses[i].error) << "Others should succeed";
            EXPECT_NE(correct_response, responses[i].sharing_check)
                    << "Others should calculate a different shared secret, due to the tweaked "
                       "nonce.";
        }
    }
}

TEST_F(SharedSecretAidlTest, ComputeSharedSecretShortSeed) {
    auto sharedSecrets = allSharedSecrets();
    if (sharedSecrets.empty()) {
        GTEST_SKIP() << "Skipping the test as no shared secret service is found.";
    }
    auto fixup_hmac = finally([&]() { computeAllSharedSecrets(getAllSharedSecretParameters()); });
    auto params = getAllSharedSecretParameters();
    ASSERT_EQ(sharedSecrets.size(), params.size())
            << "One or more shared secret service failed to provide parameters.";

    // All should be well in the normal case
    auto responses = computeAllSharedSecrets(params);

    ASSERT_GT(responses.size(), 0U);
    vector<uint8_t> correct_response = responses[0].sharing_check;
    verifyResponses(correct_response, responses);

    // Pick a random param and modify the seed to be of (invalid) length 31.
    auto param_to_tweak = rand() % params.size();
    auto& to_tweak = params[param_to_tweak].seed;
    ASSERT_TRUE(to_tweak.size() == 32 || to_tweak.size() == 0);
    to_tweak.resize(31);

    responses = computeAllSharedSecrets(params);
    for (size_t i = 0; i < responses.size(); ++i) {
        if (i == param_to_tweak) {
            EXPECT_EQ(ErrorCode::INVALID_ARGUMENT, responses[i].error)
                    << "Shared secret service that provided tweaked param should fail to compute "
                       "shared secret";
        } else {
            // Other services *may* succeed, or may notice the invalid size for the seed.
            // However, if another service completes the computation, it should get the 'wrong'
            // answer.
            if (responses[i].error == ErrorCode::OK) {
                EXPECT_NE(correct_response, responses[i].sharing_check)
                        << "Others should calculate a different shared secret, due to the tweaked "
                           "seed.";
            } else {
                EXPECT_EQ(ErrorCode::INVALID_ARGUMENT, responses[i].error);
            }
        }
    }
}

}  // namespace aidl::android::hardware::security::sharedsecret::test

int main(int argc, char** argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
