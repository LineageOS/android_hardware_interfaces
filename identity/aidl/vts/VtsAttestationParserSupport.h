
/*
 * Copyright 2019, The Android Open Source Project
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

#ifndef VTS_ATTESTATION_PARSER_SUPPORT_H
#define VTS_ATTESTATION_PARSER_SUPPORT_H

//#include <aidl/Gtest.h>
#include <android/hardware/identity/IIdentityCredentialStore.h>
#include <android/hardware/identity/support/IdentityCredentialSupport.h>
#include <android/hardware/keymaster/4.0/types.h>
#include <hardware/keymaster_defs.h>
#include <keymaster/android_keymaster_utils.h>
#include <keymaster/authorization_set.h>
#include <keymaster/contexts/pure_soft_keymaster_context.h>
#include <keymaster/contexts/soft_attestation_cert.h>
#include <keymaster/keymaster_tags.h>
#include <keymaster/km_openssl/attestation_utils.h>
#include <vector>

namespace android::hardware::identity::test_utils {

using ::std::optional;
using ::std::string;
using ::std::vector;

using ::keymaster::AuthorizationSet;
using ::keymaster::TypedTag;

class AttestationCertificateParser {
  public:
    AttestationCertificateParser(const vector<Certificate>& certChain)
        : origCertChain_(certChain) {}

    bool parse();

    uint32_t getKeymasterVersion();
    uint32_t getAttestationVersion();
    vector<uint8_t> getAttestationChallenge();
    keymaster_security_level_t getKeymasterSecurityLevel();
    keymaster_security_level_t getAttestationSecurityLevel();

    template <keymaster_tag_t Tag>
    bool getSwEnforcedBool(TypedTag<KM_BOOL, Tag> tag) {
        if (att_sw_enforced_.GetTagValue(tag)) {
            return true;
        }

        return false;
    }

    template <keymaster_tag_t Tag>
    bool getHwEnforcedBool(TypedTag<KM_BOOL, Tag> tag) {
        if (att_hw_enforced_.GetTagValue(tag)) {
            return true;
        }

        return false;
    }

    template <keymaster_tag_t Tag>
    optional<vector<uint8_t>> getHwEnforcedBlob(TypedTag<KM_BYTES, Tag> tag) {
        keymaster_blob_t blob;
        if (att_hw_enforced_.GetTagValue(tag, &blob)) {
            return {};
        }

        vector<uint8_t> ret(blob.data, blob.data + blob.data_length);
        return ret;
    }

    template <keymaster_tag_t Tag>
    optional<vector<uint8_t>> getSwEnforcedBlob(TypedTag<KM_BYTES, Tag> tag) {
        keymaster_blob_t blob;
        if (!att_sw_enforced_.GetTagValue(tag, &blob)) {
            return {};
        }

        vector<uint8_t> ret(blob.data, blob.data + blob.data_length);
        return ret;
    }

  private:
    // Helper functions.
    bool verifyChain(const keymaster_cert_chain_t& chain);

    ASN1_OCTET_STRING* getAttestationRecord(X509* certificate);

    X509* parseCertBlob(const keymaster_blob_t& blob);

    bool verifyAttestationRecord(const keymaster_blob_t& attestation_cert);

    optional<keymaster_cert_chain_t> certificateChainToKeymasterChain(
            const vector<Certificate>& certificates);

    // Private variables.
    vector<Certificate> origCertChain_;
    AuthorizationSet att_sw_enforced_;
    AuthorizationSet att_hw_enforced_;
    uint32_t att_attestation_version_;
    uint32_t att_keymaster_version_;
    keymaster_security_level_t att_attestation_security_level_;
    keymaster_security_level_t att_keymaster_security_level_;
    vector<uint8_t> att_challenge_;
};

}  // namespace android::hardware::identity::test_utils

#endif  // VTS_ATTESTATION_PARSER_SUPPORT_H
