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

#define LOG_TAG "keymint_1_bootloader_test"

#include <memory>
#include <optional>
#include <string>
#include <vector>

#include <android/binder_manager.h>
#include <remote_prov/remote_prov_utils.h>

#include "KeyMintAidlTestBase.h"

namespace aidl::android::hardware::security::keymint::test {

using ::android::getAidlHalInstanceNames;
using ::std::string;
using ::std::vector;

// Since this test needs to talk to KeyMint HAL, it can only run as root. Thus,
// bootloader can not be locked.
class BootloaderStateTest : public KeyMintAidlTestBase {};

// Check that attested bootloader state is set to unlocked.
TEST_P(BootloaderStateTest, IsUnlocked) {
    // Generate a key with attestation.
    vector<uint8_t> key_blob;
    vector<KeyCharacteristics> key_characteristics;
    AuthorizationSet keyDesc = AuthorizationSetBuilder()
                                       .Authorization(TAG_NO_AUTH_REQUIRED)
                                       .EcdsaSigningKey(EcCurve::P_256)
                                       .AttestationChallenge("foo")
                                       .AttestationApplicationId("bar")
                                       .Digest(Digest::NONE)
                                       .SetDefaultValidity();
    auto result = GenerateKey(keyDesc, &key_blob, &key_characteristics);
    // If factory provisioned attestation key is not supported by Strongbox,
    // then create a key with self-signed attestation and use it as the
    // attestation key instead.
    if (SecLevel() == SecurityLevel::STRONGBOX &&
        result == ErrorCode::ATTESTATION_KEYS_NOT_PROVISIONED) {
        result = GenerateKeyWithSelfSignedAttestKey(
                AuthorizationSetBuilder()
                        .EcdsaKey(EcCurve::P_256)
                        .AttestKey()
                        .SetDefaultValidity(), /* attest key params */
                keyDesc, &key_blob, &key_characteristics);
    }
    ASSERT_EQ(ErrorCode::OK, result);

    // Parse attested AVB values.
    X509_Ptr cert(parse_cert_blob(cert_chain_[0].encodedCertificate));
    ASSERT_TRUE(cert.get());

    ASN1_OCTET_STRING* attest_rec = get_attestation_record(cert.get());
    ASSERT_TRUE(attest_rec);

    vector<uint8_t> key;
    VerifiedBoot attestedVbState;
    bool attestedBootloaderState;
    vector<uint8_t> attestedVbmetaDigest;
    auto error = parse_root_of_trust(attest_rec->data, attest_rec->length, &key, &attestedVbState,
                                     &attestedBootloaderState, &attestedVbmetaDigest);
    ASSERT_EQ(error, ErrorCode::OK);
    ASSERT_FALSE(attestedBootloaderState) << "This test runs as root. Bootloader must be unlocked.";
}

INSTANTIATE_KEYMINT_AIDL_TEST(BootloaderStateTest);

}  // namespace aidl::android::hardware::security::keymint::test
