/*
 * Copyright (C) 2023 The Android Open Source Project
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
#define LOG_TAG "authgraph_session_test"
#include <android-base/logging.h>

#include <aidl/Gtest.h>
#include <aidl/Vintf.h>
#include <aidl/android/hardware/security/authgraph/Error.h>
#include <aidl/android/hardware/security/authgraph/IAuthGraphKeyExchange.h>
#include <android/binder_manager.h>
#include <binder/ProcessState.h>
#include <gtest/gtest.h>
#include <vector>

namespace aidl::android::hardware::security::authgraph::test {
using ::aidl::android::hardware::security::authgraph::Error;

namespace {

// Check that the signature in the encoded COSE_Sign1 data is correct, and that the payload matches.
// TODO: maybe drop separate payload, and extract it from cose_sign1.payload (and return it).
void CheckSignature(std::vector<uint8_t>& /*pub_cose_key*/, std::vector<uint8_t>& /*payload*/,
                    std::vector<uint8_t>& /*cose_sign1*/) {
    // TODO: implement me
}

void CheckSignature(std::vector<uint8_t>& pub_cose_key, std::vector<uint8_t>& payload,
                    SessionIdSignature& signature) {
    return CheckSignature(pub_cose_key, payload, signature.signature);
}

std::vector<uint8_t> SigningKeyFromIdentity(const Identity& identity) {
    // TODO: This is a CBOR-encoded `Identity` which currently happens to be a COSE_Key with the
    // pubkey This will change in future.
    return identity.identity;
}

}  // namespace

class AuthGraphSessionTest : public ::testing::TestWithParam<std::string> {
  public:
    enum ErrorType { AIDL_ERROR, BINDER_ERROR };

    union ErrorValue {
        Error aidl_error;
        int32_t binder_error;
    };

    struct ReturnedError {
        ErrorType err_type;
        ErrorValue err_val;

        friend bool operator==(const ReturnedError& lhs, const ReturnedError& rhs) {
            return lhs.err_type == rhs.err_type;
            switch (lhs.err_type) {
                case ErrorType::AIDL_ERROR:
                    return lhs.err_val.aidl_error == rhs.err_val.aidl_error;
                case ErrorType::BINDER_ERROR:
                    return lhs.err_val.binder_error == rhs.err_val.binder_error;
            }
        }
    };

    const ReturnedError OK = {.err_type = ErrorType::AIDL_ERROR, .err_val.aidl_error = Error::OK};

    ReturnedError GetReturnError(const ::ndk::ScopedAStatus& result) {
        if (result.isOk()) {
            return OK;
        }
        int32_t exception_code = result.getExceptionCode();
        int32_t error_code = result.getServiceSpecificError();
        if (exception_code == EX_SERVICE_SPECIFIC && error_code != 0) {
            ReturnedError re = {.err_type = ErrorType::AIDL_ERROR,
                                .err_val.aidl_error = static_cast<Error>(error_code)};
            return re;
        }
        ReturnedError re = {.err_type = ErrorType::BINDER_ERROR,
                            .err_val.binder_error = exception_code};
        return re;
    }

    // Build the parameters for the VTS test by enumerating the available HAL instances
    static std::vector<std::string> build_params() {
        auto params = ::android::getAidlHalInstanceNames(IAuthGraphKeyExchange::descriptor);
        return params;
    }

    void SetUp() override {
        ASSERT_TRUE(AServiceManager_isDeclared(GetParam().c_str()))
                << "No instance declared for " << GetParam();
        ::ndk::SpAIBinder binder(AServiceManager_waitForService(GetParam().c_str()));
        authNode_ = IAuthGraphKeyExchange::fromBinder(binder);
        ASSERT_NE(authNode_, nullptr) << "Failed to get Binder reference for " << GetParam();
    }

    void TearDown() override {}

  protected:
    std::shared_ptr<IAuthGraphKeyExchange> authNode_;
};

TEST_P(AuthGraphSessionTest, Mainline) {
    std::shared_ptr<IAuthGraphKeyExchange> source = authNode_;
    std::shared_ptr<IAuthGraphKeyExchange> sink = authNode_;

    // Step 1: create an ephemeral ECDH key at the source.
    SessionInitiationInfo source_init_info;
    ASSERT_EQ(OK, GetReturnError(source->create(&source_init_info)));
    ASSERT_TRUE(source_init_info.key.pubKey.has_value());
    ASSERT_TRUE(source_init_info.key.arcFromPBK.has_value());

    // Step 2: pass the source's ECDH public key and other session info to the sink.
    KeInitResult init_result;
    ASSERT_EQ(OK, GetReturnError(sink->init(source_init_info.key.pubKey.value(),
                                            source_init_info.identity, source_init_info.nonce,
                                            source_init_info.version, &init_result)));
    SessionInitiationInfo sink_init_info = init_result.sessionInitiationInfo;
    ASSERT_TRUE(sink_init_info.key.pubKey.has_value());
    // The sink_init_info.arcFromPBK need not be populated, as the ephemeral key agreement
    // key is no longer needed.

    SessionInfo sink_info = init_result.sessionInfo;
    ASSERT_EQ((int)sink_info.sharedKeys.size(), 2) << "Expect two symmetric keys from init()";
    ASSERT_GT((int)sink_info.sessionId.size(), 0) << "Expect non-empty session ID from sink";
    std::vector<uint8_t> sink_signing_key = SigningKeyFromIdentity(sink_init_info.identity);
    CheckSignature(sink_signing_key, sink_info.sessionId, sink_info.signature);

    // Step 3: pass the sink's ECDH public key and other session info to the source, so it can
    // calculate the same pair of symmetric keys.
    SessionInfo source_info;
    ASSERT_EQ(OK, GetReturnError(source->finish(sink_init_info.key.pubKey.value(),
                                                sink_init_info.identity, sink_info.signature,
                                                sink_init_info.nonce, sink_init_info.version,
                                                source_init_info.key, &source_info)));
    ASSERT_EQ((int)source_info.sharedKeys.size(), 2) << "Expect two symmetric keys from finsh()";
    ASSERT_GT((int)source_info.sessionId.size(), 0) << "Expect non-empty session ID from source";
    std::vector<uint8_t> source_signing_key = SigningKeyFromIdentity(source_init_info.identity);
    CheckSignature(source_signing_key, source_info.sessionId, source_info.signature);

    // Both ends should agree on the session ID.
    ASSERT_EQ(source_info.sessionId, sink_info.sessionId);

    // Step 4: pass the source's session ID info back to the sink, so it can check it and
    // update the symmetric keys so they're marked as authentication complete.
    std::array<Arc, 2> auth_complete_result;
    ASSERT_EQ(OK, GetReturnError(sink->authenticationComplete(
                          source_info.signature, sink_info.sharedKeys, &auth_complete_result)));
    ASSERT_EQ((int)auth_complete_result.size(), 2)
            << "Expect two symmetric keys from authComplete()";
    sink_info.sharedKeys = auth_complete_result;

    // At this point the sink and source have agreed on the same pair of symmetric keys,
    // encoded as `sink_info.sharedKeys` and `source_info.sharedKeys`.
}

TEST_P(AuthGraphSessionTest, FreshNonces) {
    std::shared_ptr<IAuthGraphKeyExchange> source = authNode_;
    std::shared_ptr<IAuthGraphKeyExchange> sink = authNode_;

    SessionInitiationInfo source_init_info1;
    ASSERT_EQ(OK, GetReturnError(source->create(&source_init_info1)));
    SessionInitiationInfo source_init_info2;
    ASSERT_EQ(OK, GetReturnError(source->create(&source_init_info2)));

    // Two calls to create() should result in the same identity but different nonce values.
    ASSERT_EQ(source_init_info1.identity, source_init_info2.identity);
    ASSERT_NE(source_init_info1.nonce, source_init_info2.nonce);
    ASSERT_NE(source_init_info1.key.pubKey, source_init_info2.key.pubKey);
    ASSERT_NE(source_init_info1.key.arcFromPBK, source_init_info2.key.arcFromPBK);

    KeInitResult init_result1;
    ASSERT_EQ(OK, GetReturnError(sink->init(source_init_info1.key.pubKey.value(),
                                            source_init_info1.identity, source_init_info1.nonce,
                                            source_init_info1.version, &init_result1)));
    KeInitResult init_result2;
    ASSERT_EQ(OK, GetReturnError(sink->init(source_init_info2.key.pubKey.value(),
                                            source_init_info2.identity, source_init_info2.nonce,
                                            source_init_info2.version, &init_result2)));

    // Two calls to init() should result in the same identity buf different nonces and session IDs.
    ASSERT_EQ(init_result1.sessionInitiationInfo.identity,
              init_result2.sessionInitiationInfo.identity);
    ASSERT_NE(init_result1.sessionInitiationInfo.nonce, init_result2.sessionInitiationInfo.nonce);
    ASSERT_NE(init_result1.sessionInfo.sessionId, init_result2.sessionInfo.sessionId);
}

INSTANTIATE_TEST_SUITE_P(PerInstance, AuthGraphSessionTest,
                         testing::ValuesIn(AuthGraphSessionTest::build_params()),
                         ::android::PrintInstanceNameToString);
GTEST_ALLOW_UNINSTANTIATED_PARAMETERIZED_TEST(AuthGraphSessionTest);

}  // namespace aidl::android::hardware::security::authgraph::test

int main(int argc, char** argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
