/*
 * Copyright (C) 2021 The Android Open Source Project
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

#define LOG_TAG "keymint_2_se_provisioning_test"

#include <map>
#include <memory>
#include <vector>

#include <android-base/logging.h>
#include <android/binder_manager.h>

#include <cppbor_parse.h>
#include <keymaster/cppcose/cppcose.h>
#include <keymint_support/key_param_output.h>

#include "KeyMintAidlTestBase.h"

namespace aidl::android::hardware::security::keymint::test {

using std::array;
using std::map;
using std::shared_ptr;
using std::vector;

constexpr int kRoTVersion1 = 40001;

class SecureElementProvisioningTest : public testing::Test {
  protected:
    static void SetUpTestSuite() {
        auto params = ::android::getAidlHalInstanceNames(IKeyMintDevice::descriptor);
        for (auto& param : params) {
            ASSERT_TRUE(AServiceManager_isDeclared(param.c_str()))
                    << "IKeyMintDevice instance " << param << " found but not declared.";
            ::ndk::SpAIBinder binder(AServiceManager_waitForService(param.c_str()));
            auto keymint = IKeyMintDevice::fromBinder(binder);
            ASSERT_NE(keymint, nullptr) << "Failed to get IKeyMintDevice instance " << param;

            KeyMintHardwareInfo info;
            ASSERT_TRUE(keymint->getHardwareInfo(&info).isOk());
            ASSERT_EQ(keymints_.count(info.securityLevel), 0)
                    << "There must be exactly one IKeyMintDevice with security level "
                    << info.securityLevel;

            keymints_[info.securityLevel] = std::move(keymint);
        }
    }

    void validateMacedRootOfTrust(const vector<uint8_t>& rootOfTrust) {
        SCOPED_TRACE(testing::Message() << "RoT: " << bin2hex(rootOfTrust));

        const auto [macItem, macEndPos, macErrMsg] = cppbor::parse(rootOfTrust);
        ASSERT_TRUE(macItem) << "Root of trust parsing failed: " << macErrMsg;
        ASSERT_EQ(macItem->semanticTagCount(), 1);
        ASSERT_EQ(macItem->semanticTag(0), cppcose::kCoseMac0SemanticTag);
        ASSERT_TRUE(macItem->asArray());
        ASSERT_EQ(macItem->asArray()->size(), cppcose::kCoseMac0EntryCount);

        const auto& protectedItem = macItem->asArray()->get(cppcose::kCoseMac0ProtectedParams);
        ASSERT_TRUE(protectedItem);
        ASSERT_TRUE(protectedItem->asBstr());
        const auto [protMap, protEndPos, protErrMsg] = cppbor::parse(protectedItem->asBstr());
        ASSERT_TRUE(protMap);
        ASSERT_TRUE(protMap->asMap());
        ASSERT_EQ(protMap->asMap()->size(), 1);

        const auto& algorithm = protMap->asMap()->get(cppcose::ALGORITHM);
        ASSERT_TRUE(algorithm);
        ASSERT_TRUE(algorithm->asInt());
        ASSERT_EQ(algorithm->asInt()->value(), cppcose::HMAC_256);

        const auto& unprotItem = macItem->asArray()->get(cppcose::kCoseMac0UnprotectedParams);
        ASSERT_TRUE(unprotItem);
        ASSERT_TRUE(unprotItem->asMap());
        ASSERT_EQ(unprotItem->asMap()->size(), 0);

        const auto& payload = macItem->asArray()->get(cppcose::kCoseMac0Payload);
        ASSERT_TRUE(payload);
        ASSERT_TRUE(payload->asBstr());
        validateRootOfTrust(payload->asBstr()->value());

        const auto& tag = macItem->asArray()->get(cppcose::kCoseMac0Tag);
        ASSERT_TRUE(tag);
        ASSERT_TRUE(tag->asBstr());
        ASSERT_EQ(tag->asBstr()->value().size(), 32);
        // Cannot validate tag correctness.  Only the secure side has the necessary key.
    }

    void validateRootOfTrust(const vector<uint8_t>& payload) {
        SCOPED_TRACE(testing::Message() << "RoT payload: " << bin2hex(payload));

        const auto [rot, rotPos, rotErrMsg] = cppbor::parse(payload);
        ASSERT_TRUE(rot);
        ASSERT_EQ(rot->semanticTagCount(), 1);
        ASSERT_EQ(rot->semanticTag(), kRoTVersion1);
        ASSERT_TRUE(rot->asArray());
        ASSERT_EQ(rot->asArray()->size(), 5);

        size_t pos = 0;

        const auto& vbKey = rot->asArray()->get(pos++);
        ASSERT_TRUE(vbKey);
        ASSERT_TRUE(vbKey->asBstr());
        if (get_vsr_api_level() >= __ANDROID_API_V__) {
            // The attestation should contain the SHA-256 hash of the verified boot
            // key.  However, this not was checked for earlier versions of the KeyMint
            // HAL so only be strict for VSR-V and above.
            ASSERT_LE(vbKey->asBstr()->value().size(), 32);
        }

        const auto& deviceLocked = rot->asArray()->get(pos++);
        ASSERT_TRUE(deviceLocked);
        ASSERT_TRUE(deviceLocked->asBool());

        const auto& verifiedBootState = rot->asArray()->get(pos++);
        ASSERT_TRUE(verifiedBootState);
        ASSERT_TRUE(verifiedBootState->asInt());

        const auto& verifiedBootHash = rot->asArray()->get(pos++);
        ASSERT_TRUE(verifiedBootHash);
        ASSERT_TRUE(verifiedBootHash->asBstr());

        const auto& bootPatchLevel = rot->asArray()->get(pos++);
        ASSERT_TRUE(bootPatchLevel);
        ASSERT_TRUE(bootPatchLevel->asInt());

        verify_root_of_trust(vbKey->asBstr()->value(), deviceLocked->asBool()->value(),
                             static_cast<VerifiedBoot>(verifiedBootState->asInt()->value()),
                             verifiedBootHash->asBstr()->value());
    }

    int32_t AidlVersion(shared_ptr<IKeyMintDevice> keymint) {
        int32_t version = 0;
        auto status = keymint->getInterfaceVersion(&version);
        if (!status.isOk()) {
            ADD_FAILURE() << "Failed to determine interface version";
        }
        return version;
    }

    static map<SecurityLevel, shared_ptr<IKeyMintDevice>> keymints_;
};

map<SecurityLevel, shared_ptr<IKeyMintDevice>> SecureElementProvisioningTest::keymints_;

TEST_F(SecureElementProvisioningTest, ValidConfigurations) {
    if (keymints_.empty()) {
        GTEST_SKIP() << "Test not applicable to device with no KeyMint devices";
    }
    // TEE is required
    ASSERT_EQ(keymints_.count(SecurityLevel::TRUSTED_ENVIRONMENT), 1);
    // StrongBox is optional
    ASSERT_LE(keymints_.count(SecurityLevel::STRONGBOX), 1);
}

TEST_F(SecureElementProvisioningTest, TeeOnly) {
    if (keymints_.count(SecurityLevel::TRUSTED_ENVIRONMENT) == 0) {
        GTEST_SKIP() << "Test not applicable to device with no TEE KeyMint device";
    }
    auto tee = keymints_.find(SecurityLevel::TRUSTED_ENVIRONMENT)->second;
    // Execute the test only for KeyMint version >= 2.
    if (AidlVersion(tee) < 2) {
        GTEST_SKIP() << "Test not applicable to TEE KeyMint device before v2";
    }

    array<uint8_t, 16> challenge1 = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};
    array<uint8_t, 16> challenge2 = {1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};

    vector<uint8_t> rootOfTrust1;
    Status result = tee->getRootOfTrust(challenge1, &rootOfTrust1);
    ASSERT_TRUE(result.isOk()) << "getRootOfTrust returned " << result.getServiceSpecificError();
    validateMacedRootOfTrust(rootOfTrust1);

    vector<uint8_t> rootOfTrust2;
    result = tee->getRootOfTrust(challenge2, &rootOfTrust2);
    ASSERT_TRUE(result.isOk());
    validateMacedRootOfTrust(rootOfTrust2);
    ASSERT_NE(rootOfTrust1, rootOfTrust2);

    vector<uint8_t> rootOfTrust3;
    result = tee->getRootOfTrust(challenge1, &rootOfTrust3);
    ASSERT_TRUE(result.isOk());
    ASSERT_EQ(rootOfTrust1, rootOfTrust3);
}

TEST_F(SecureElementProvisioningTest, TeeDoesNotImplementStrongBoxMethods) {
    if (keymints_.count(SecurityLevel::TRUSTED_ENVIRONMENT) == 0) {
        GTEST_SKIP() << "Test not applicable to device with no TEE KeyMint device";
    }
    auto tee = keymints_.find(SecurityLevel::TRUSTED_ENVIRONMENT)->second;
    // Execute the test only for KeyMint version >= 2.
    if (AidlVersion(tee) < 2) {
        GTEST_SKIP() << "Test not applicable to TEE KeyMint device before v2";
    }

    array<uint8_t, 16> challenge;
    Status result = tee->getRootOfTrustChallenge(&challenge);
    ASSERT_FALSE(result.isOk());
    ASSERT_EQ(result.getExceptionCode(), EX_SERVICE_SPECIFIC);
    ASSERT_EQ(static_cast<ErrorCode>(result.getServiceSpecificError()), ErrorCode::UNIMPLEMENTED);

    result = tee->sendRootOfTrust({});
    ASSERT_FALSE(result.isOk());
    ASSERT_EQ(result.getExceptionCode(), EX_SERVICE_SPECIFIC);
    ASSERT_EQ(static_cast<ErrorCode>(result.getServiceSpecificError()), ErrorCode::UNIMPLEMENTED);
}

TEST_F(SecureElementProvisioningTest, StrongBoxDoesNotImplementTeeMethods) {
    if (keymints_.count(SecurityLevel::STRONGBOX) == 0) {
        // Need a StrongBox to provision.
        GTEST_SKIP() << "Test not applicable to device with no StrongBox KeyMint device";
    }
    // Execute the test only for KeyMint version >= 2.
    auto sb = keymints_.find(SecurityLevel::STRONGBOX)->second;
    if (AidlVersion(sb) < 2) {
        GTEST_SKIP() << "Test not applicable to StrongBox KeyMint device before v2";
    }

    vector<uint8_t> rootOfTrust;
    Status result = sb->getRootOfTrust({}, &rootOfTrust);
    ASSERT_FALSE(result.isOk());
    ASSERT_EQ(result.getExceptionCode(), EX_SERVICE_SPECIFIC);
    ASSERT_EQ(static_cast<ErrorCode>(result.getServiceSpecificError()), ErrorCode::UNIMPLEMENTED);
}

TEST_F(SecureElementProvisioningTest, UnimplementedTest) {
    if (keymints_.count(SecurityLevel::STRONGBOX) == 0) {
        // Need a StrongBox to provision.
        GTEST_SKIP() << "Test not applicable to device with no StrongBox KeyMint device";
    }
    // Execute the test only for KeyMint version >= 2.
    auto sb = keymints_.find(SecurityLevel::STRONGBOX)->second;
    if (AidlVersion(sb) < 2) {
        GTEST_SKIP() << "Test not applicable to StrongBox KeyMint device before v2";
    }

    if (keymints_.count(SecurityLevel::TRUSTED_ENVIRONMENT) == 0) {
        GTEST_SKIP() << "Test not applicable to device with no TEE KeyMint device";
    }
    auto tee = keymints_.find(SecurityLevel::TRUSTED_ENVIRONMENT)->second;
    if (AidlVersion(tee) < 2) {
        GTEST_SKIP() << "Test not applicable to TEE KeyMint device before v2";
    }

    array<uint8_t, 16> challenge;
    Status result = sb->getRootOfTrustChallenge(&challenge);
    if (!result.isOk()) {
        // Strongbox does not have to implement this feature if it has uses an alternative mechanism
        // to provision the root of trust.  In that case it MUST return UNIMPLEMENTED, both from
        // getRootOfTrustChallenge() and from sendRootOfTrust().
        ASSERT_EQ(result.getExceptionCode(), EX_SERVICE_SPECIFIC);
        ASSERT_EQ(static_cast<ErrorCode>(result.getServiceSpecificError()),
                  ErrorCode::UNIMPLEMENTED);

        result = sb->sendRootOfTrust({});
        ASSERT_EQ(result.getExceptionCode(), EX_SERVICE_SPECIFIC);
        ASSERT_EQ(static_cast<ErrorCode>(result.getServiceSpecificError()),
                  ErrorCode::UNIMPLEMENTED);

        SUCCEED() << "This Strongbox implementation does not use late root of trust delivery.";
        return;
    }
}

TEST_F(SecureElementProvisioningTest, ChallengeQualityTest) {
    if (keymints_.count(SecurityLevel::STRONGBOX) == 0) {
        // Need a StrongBox to provision.
        GTEST_SKIP() << "Test not applicable to device with no StrongBox KeyMint device";
    }
    // Execute the test only for KeyMint version >= 2.
    auto sb = keymints_.find(SecurityLevel::STRONGBOX)->second;
    if (AidlVersion(sb) < 2) {
        GTEST_SKIP() << "Test not applicable to StrongBox KeyMint device before v2";
    }

    array<uint8_t, 16> challenge1;
    Status result = sb->getRootOfTrustChallenge(&challenge1);
    if (!result.isOk()) return;

    array<uint8_t, 16> challenge2;
    result = sb->getRootOfTrustChallenge(&challenge2);
    ASSERT_TRUE(result.isOk());
    ASSERT_NE(challenge1, challenge2);

    // TODO: When we add entropy testing in other relevant places in these tests, add it here, too,
    // to verify that challenges appear to have adequate entropy.
}

TEST_F(SecureElementProvisioningTest, ProvisioningTest) {
    if (keymints_.count(SecurityLevel::STRONGBOX) == 0) {
        // Need a StrongBox to provision.
        GTEST_SKIP() << "Test not applicable to device with no StrongBox KeyMint device";
    }
    // Execute the test only for KeyMint version >= 2.
    auto sb = keymints_.find(SecurityLevel::STRONGBOX)->second;
    if (AidlVersion(sb) < 2) {
        GTEST_SKIP() << "Test not applicable to StrongBox KeyMint device before v2";
    }

    if (keymints_.count(SecurityLevel::TRUSTED_ENVIRONMENT) == 0) {
        GTEST_SKIP() << "Test not applicable to device with no TEE KeyMint device";
    }
    // Execute the test only for KeyMint version >= 2.
    auto tee = keymints_.find(SecurityLevel::TRUSTED_ENVIRONMENT)->second;
    if (AidlVersion(tee) < 2) {
        GTEST_SKIP() << "Test not applicable to TEE KeyMint device before v2";
    }

    array<uint8_t, 16> challenge;
    Status result = sb->getRootOfTrustChallenge(&challenge);
    if (!result.isOk()) return;

    vector<uint8_t> rootOfTrust;
    result = tee->getRootOfTrust(challenge, &rootOfTrust);
    ASSERT_TRUE(result.isOk());

    validateMacedRootOfTrust(rootOfTrust);

    result = sb->sendRootOfTrust(rootOfTrust);
    ASSERT_TRUE(result.isOk());

    // Sending again must fail, because a new challenge is required.
    result = sb->sendRootOfTrust(rootOfTrust);
    ASSERT_FALSE(result.isOk());
}

TEST_F(SecureElementProvisioningTest, InvalidProvisioningTest) {
    if (keymints_.count(SecurityLevel::STRONGBOX) == 0) {
        // Need a StrongBox to provision.
        GTEST_SKIP() << "Test not applicable to device with no StrongBox KeyMint device";
    }
    // Execute the test only for KeyMint version >= 2.
    auto sb = keymints_.find(SecurityLevel::STRONGBOX)->second;
    if (AidlVersion(sb) < 2) {
        GTEST_SKIP() << "Test not applicable to StrongBox KeyMint device before v2";
    }

    if (keymints_.count(SecurityLevel::TRUSTED_ENVIRONMENT) == 0) {
        GTEST_SKIP() << "Test not applicable to device with no TEE KeyMint device";
    }
    // Execute the test only for KeyMint version >= 2.
    auto tee = keymints_.find(SecurityLevel::TRUSTED_ENVIRONMENT)->second;
    if (AidlVersion(tee) < 2) {
        GTEST_SKIP() << "Test not applicable to TEE KeyMint device before v2";
    }

    array<uint8_t, 16> challenge;
    Status result = sb->getRootOfTrustChallenge(&challenge);
    if (!result.isOk()) return;

    result = sb->sendRootOfTrust({});
    ASSERT_FALSE(result.isOk());
    ASSERT_EQ(result.getExceptionCode(), EX_SERVICE_SPECIFIC);
    ASSERT_EQ(static_cast<ErrorCode>(result.getServiceSpecificError()),
              ErrorCode::VERIFICATION_FAILED);

    vector<uint8_t> rootOfTrust;
    result = tee->getRootOfTrust(challenge, &rootOfTrust);
    ASSERT_TRUE(result.isOk());

    validateMacedRootOfTrust(rootOfTrust);

    vector<uint8_t> corruptedRootOfTrust = rootOfTrust;
    corruptedRootOfTrust[corruptedRootOfTrust.size() / 2]++;
    result = sb->sendRootOfTrust(corruptedRootOfTrust);
    ASSERT_FALSE(result.isOk());
    ASSERT_EQ(result.getExceptionCode(), EX_SERVICE_SPECIFIC);
    ASSERT_EQ(static_cast<ErrorCode>(result.getServiceSpecificError()),
              ErrorCode::VERIFICATION_FAILED);

    // Now try the correct RoT
    result = sb->sendRootOfTrust(rootOfTrust);
    ASSERT_TRUE(result.isOk());
}

}  // namespace aidl::android::hardware::security::keymint::test
