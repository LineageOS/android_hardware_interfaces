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

#include <android-base/properties.h>
#include <android/binder_manager.h>
#include <fstab/fstab.h>
#include <libavb/libavb.h>
#include <libavb_user/avb_ops_user.h>
#include <remote_prov/remote_prov_utils.h>

#include "KeyMintAidlTestBase.h"

namespace aidl::android::hardware::security::keymint::test {

using ::android::getAidlHalInstanceNames;
using ::std::string;
using ::std::vector;

// Since this test needs to talk to KeyMint HAL, it can only run as root. Thus,
// bootloader can not be locked.
class BootloaderStateTest : public KeyMintAidlTestBase {
  public:
    virtual void SetUp() override {
        KeyMintAidlTestBase::SetUp();

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
        ASSERT_EQ(ErrorCode::OK, result);

        // Parse attested AVB values.
        X509_Ptr cert(parse_cert_blob(cert_chain_[0].encodedCertificate));
        ASSERT_TRUE(cert.get());

        ASN1_OCTET_STRING* attest_rec = get_attestation_record(cert.get());
        ASSERT_TRUE(attest_rec);

        auto error = parse_root_of_trust(attest_rec->data, attest_rec->length, &attestedVbKey_,
                                         &attestedVbState_, &attestedBootloaderState_,
                                         &attestedVbmetaDigest_);
        ASSERT_EQ(error, ErrorCode::OK);
    }

    vector<uint8_t> attestedVbKey_;
    VerifiedBoot attestedVbState_;
    bool attestedBootloaderState_;
    vector<uint8_t> attestedVbmetaDigest_;
};

// Check that attested bootloader state is set to unlocked.
TEST_P(BootloaderStateTest, BootloaderIsUnlocked) {
    ASSERT_FALSE(attestedBootloaderState_)
            << "This test runs as root. Bootloader must be unlocked.";
}

// Check that verified boot state is set to "unverified", i.e. "orange".
TEST_P(BootloaderStateTest, VbStateIsUnverified) {
    // Unlocked bootloader implies that verified boot state must be "unverified".
    ASSERT_EQ(attestedVbState_, VerifiedBoot::UNVERIFIED)
            << "Verified boot state must be \"UNVERIFIED\" aka \"orange\".";

    // AVB spec stipulates that bootloader must set "androidboot.verifiedbootstate" parameter
    // on the kernel command-line. This parameter is exposed to userspace as
    // "ro.boot.verifiedbootstate" property.
    auto vbStateProp = ::android::base::GetProperty("ro.boot.verifiedbootstate", "");
    ASSERT_EQ(vbStateProp, "orange")
            << "Verified boot state must be \"UNVERIFIED\" aka \"orange\".";
}

// Check that the attested Verified Boot key is 32 bytes of zeroes since the bootloader is unlocked.
TEST_P(BootloaderStateTest, VerifiedBootKeyAllZeroes) {
    // Gate this test to avoid waiver issues.
    if (get_vsr_api_level() <= __ANDROID_API_V__) {
        return;
    }

    std::vector<uint8_t> expectedVbKey(32, 0);
    ASSERT_EQ(attestedVbKey_, expectedVbKey) << "Verified Boot key digest must be 32 bytes of "
                                                "zeroes since the bootloader is unlocked.";
}

// Following error codes from avb_slot_data() mean that slot data was loaded
// (even if verification failed).
static inline bool avb_slot_data_loaded(AvbSlotVerifyResult result) {
    switch (result) {
        case AVB_SLOT_VERIFY_RESULT_OK:
        case AVB_SLOT_VERIFY_RESULT_ERROR_VERIFICATION:
        case AVB_SLOT_VERIFY_RESULT_ERROR_ROLLBACK_INDEX:
        case AVB_SLOT_VERIFY_RESULT_ERROR_PUBLIC_KEY_REJECTED:
            return true;
        default:
            return false;
    }
}

// Check that the attested VBMeta digest is correct.
TEST_P(BootloaderStateTest, VbmetaDigest) {
    AvbSlotVerifyData* avbSlotData;
    auto suffix = fs_mgr_get_slot_suffix();
    const char* partitions[] = {nullptr};
    auto avbOps = avb_ops_user_new();

    // For VTS, devices run with vendor_boot-debug.img, which is not release key
    // signed. Use AVB_SLOT_VERIFY_FLAGS_ALLOW_VERIFICATION_ERROR to bypass avb
    // verification errors. This is OK since we only care about the digest for
    // this test case.
    auto result = avb_slot_verify(avbOps, partitions, suffix.c_str(),
                                  AVB_SLOT_VERIFY_FLAGS_ALLOW_VERIFICATION_ERROR,
                                  AVB_HASHTREE_ERROR_MODE_EIO, &avbSlotData);
    ASSERT_TRUE(avb_slot_data_loaded(result)) << "Failed to load avb slot data";

    vector<uint8_t> sha256Digest(AVB_SHA256_DIGEST_SIZE);
    avb_slot_verify_data_calculate_vbmeta_digest(avbSlotData, AVB_DIGEST_TYPE_SHA256,
                                                 sha256Digest.data());

    if (get_vsr_api_level() >= __ANDROID_API_V__) {
        ASSERT_TRUE(attestedVbmetaDigest_ == sha256Digest)
                << "Attested VBMeta digest (" << bin2hex(attestedVbmetaDigest_)
                << ") does not match the expected SHA-256 digest (" << bin2hex(sha256Digest)
                << ").";
    } else {
        // Prior to VSR-V, there was no MUST requirement for the algorithm used by the bootloader
        // to calculate the VBMeta digest. However, the only two supported options are SHA-256 and
        // SHA-512, so we expect the attested VBMeta digest to match one of these.
        vector<uint8_t> sha512Digest(AVB_SHA512_DIGEST_SIZE);
        avb_slot_verify_data_calculate_vbmeta_digest(avbSlotData, AVB_DIGEST_TYPE_SHA512,
                                                     sha512Digest.data());

        ASSERT_TRUE((attestedVbmetaDigest_ == sha256Digest) ||
                    (attestedVbmetaDigest_ == sha512Digest))
                << "Attested VBMeta digest (" << bin2hex(attestedVbmetaDigest_)
                << ") does not match the expected digest (SHA-256: " << bin2hex(sha256Digest)
                << " or SHA-512: " << bin2hex(sha512Digest) << ").";
    }
}

INSTANTIATE_KEYMINT_AIDL_TEST(BootloaderStateTest);

}  // namespace aidl::android::hardware::security::keymint::test
