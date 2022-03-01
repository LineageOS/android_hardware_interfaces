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

    static map<SecurityLevel, shared_ptr<IKeyMintDevice>> keymints_;
};

map<SecurityLevel, shared_ptr<IKeyMintDevice>> SecureElementProvisioningTest::keymints_;

TEST_F(SecureElementProvisioningTest, ValidConfigurations) {
    // TEE is required
    ASSERT_EQ(keymints_.count(SecurityLevel::TRUSTED_ENVIRONMENT), 1);
    // StrongBox is optional
    ASSERT_LE(keymints_.count(SecurityLevel::STRONGBOX), 1);
}

TEST_F(SecureElementProvisioningTest, TeeOnly) {
    ASSERT_EQ(keymints_.count(SecurityLevel::TRUSTED_ENVIRONMENT), 1);
    auto tee = keymints_.find(SecurityLevel::TRUSTED_ENVIRONMENT)->second;
    ASSERT_NE(tee, nullptr);

    array<uint8_t, 16> challenge1 = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};
    array<uint8_t, 16> challenge2 = {1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};

    vector<uint8_t> rootOfTrust1;
    Status result = tee->getRootOfTrust(challenge1, &rootOfTrust1);

    // TODO: Remove the next line to require TEEs to succeed.
    if (!result.isOk()) return;

    ASSERT_TRUE(result.isOk());

    // TODO:  Parse and validate rootOfTrust1 here

    vector<uint8_t> rootOfTrust2;
    result = tee->getRootOfTrust(challenge2, &rootOfTrust2);
    ASSERT_TRUE(result.isOk());

    // TODO:  Parse and validate rootOfTrust2 here

    ASSERT_NE(rootOfTrust1, rootOfTrust2);

    vector<uint8_t> rootOfTrust3;
    result = tee->getRootOfTrust(challenge1, &rootOfTrust3);
    ASSERT_TRUE(result.isOk());

    ASSERT_EQ(rootOfTrust1, rootOfTrust3);

    // TODO:  Parse and validate rootOfTrust3 here
}

TEST_F(SecureElementProvisioningTest, TeeDoesNotImplementStrongBoxMethods) {
    ASSERT_EQ(keymints_.count(SecurityLevel::TRUSTED_ENVIRONMENT), 1);
    auto tee = keymints_.find(SecurityLevel::TRUSTED_ENVIRONMENT)->second;
    ASSERT_NE(tee, nullptr);

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
    if (keymints_.count(SecurityLevel::STRONGBOX) == 0) return;

    auto sb = keymints_.find(SecurityLevel::STRONGBOX)->second;
    ASSERT_NE(sb, nullptr);

    vector<uint8_t> rootOfTrust;
    Status result = sb->getRootOfTrust({}, &rootOfTrust);
    ASSERT_FALSE(result.isOk());
    ASSERT_EQ(result.getExceptionCode(), EX_SERVICE_SPECIFIC);
    ASSERT_EQ(static_cast<ErrorCode>(result.getServiceSpecificError()), ErrorCode::UNIMPLEMENTED);
}

TEST_F(SecureElementProvisioningTest, UnimplementedTest) {
    if (keymints_.count(SecurityLevel::STRONGBOX) == 0) return;  // Need a StrongBox to provision.

    ASSERT_EQ(keymints_.count(SecurityLevel::TRUSTED_ENVIRONMENT), 1);
    auto tee = keymints_.find(SecurityLevel::TRUSTED_ENVIRONMENT)->second;
    ASSERT_NE(tee, nullptr);

    ASSERT_EQ(keymints_.count(SecurityLevel::STRONGBOX), 1);
    auto sb = keymints_.find(SecurityLevel::STRONGBOX)->second;
    ASSERT_NE(sb, nullptr);

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
    if (keymints_.count(SecurityLevel::STRONGBOX) == 0) return;  // Need a StrongBox to provision.

    ASSERT_EQ(keymints_.count(SecurityLevel::STRONGBOX), 1);
    auto sb = keymints_.find(SecurityLevel::STRONGBOX)->second;
    ASSERT_NE(sb, nullptr);

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
    if (keymints_.count(SecurityLevel::STRONGBOX) == 0) return;  // Need a StrongBox to provision.

    ASSERT_EQ(keymints_.count(SecurityLevel::TRUSTED_ENVIRONMENT), 1);
    auto tee = keymints_.find(SecurityLevel::TRUSTED_ENVIRONMENT)->second;
    ASSERT_NE(tee, nullptr);

    ASSERT_EQ(keymints_.count(SecurityLevel::STRONGBOX), 1);
    auto sb = keymints_.find(SecurityLevel::STRONGBOX)->second;
    ASSERT_NE(sb, nullptr);

    array<uint8_t, 16> challenge;
    Status result = sb->getRootOfTrustChallenge(&challenge);
    if (!result.isOk()) return;

    vector<uint8_t> rootOfTrust;
    result = tee->getRootOfTrust(challenge, &rootOfTrust);
    ASSERT_TRUE(result.isOk());

    // TODO: Verify COSE_Mac0 structure and content here.

    result = sb->sendRootOfTrust(rootOfTrust);
    ASSERT_TRUE(result.isOk());

    // Sending again must fail, because a new challenge is required.
    result = sb->sendRootOfTrust(rootOfTrust);
    ASSERT_FALSE(result.isOk());
}

TEST_F(SecureElementProvisioningTest, InvalidProvisioningTest) {
    if (keymints_.count(SecurityLevel::STRONGBOX) == 0) return;  // Need a StrongBox to provision.

    ASSERT_EQ(keymints_.count(SecurityLevel::TRUSTED_ENVIRONMENT), 1);
    auto tee = keymints_.find(SecurityLevel::TRUSTED_ENVIRONMENT)->second;
    ASSERT_NE(tee, nullptr);

    ASSERT_EQ(keymints_.count(SecurityLevel::STRONGBOX), 1);
    auto sb = keymints_.find(SecurityLevel::STRONGBOX)->second;
    ASSERT_NE(sb, nullptr);

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
