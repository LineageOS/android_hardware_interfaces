/*
 * Copyright (c) 2021, The Android Open Source Project
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

#include <gtest/gtest.h>
#include <optional>
#include <string>
#include <vector>

#include "FakeSecureHardwareProxy.h"

// Most of libeic is tested as part of VTS since there's almost a 1:1 mapping between
// the HAL and libeic interfaces. This test suite is mainly for the few things which
// doesn't map directly.
//

using std::optional;
using std::string;
using std::vector;

using android::hardware::identity::AccessCheckResult;
using android::hardware::identity::FakeSecureHardwarePresentationProxy;
using android::hardware::identity::FakeSecureHardwareProvisioningProxy;

TEST(EicTest, AccessControlIsEnforced) {
    // First provision the credential...
    //
    FakeSecureHardwareProvisioningProxy provisioningProxy;
    bool isTestCredential = false;
    provisioningProxy.initialize(isTestCredential);
    optional<vector<uint8_t>> credKey =
            provisioningProxy.createCredentialKey({0x01, 0x02}, {0x03, 0x04});
    ASSERT_TRUE(credKey.has_value());
    string docType = "org.iso.18013.5.1.mDL";
    ASSERT_TRUE(provisioningProxy.startPersonalization(0, {1}, docType, 125));

    vector<int> acpIds = {};
    string nameSpace = "org.iso.18013.5.1";
    string name = "NonAccessibleElement";
    vector<uint8_t> content = {0x63, 0x46, 0x6f, 0x6f};  // "Foo" tstr
    ASSERT_TRUE(provisioningProxy.beginAddEntry(acpIds, nameSpace, name, content.size()));
    optional<vector<uint8_t>> encContent =
            provisioningProxy.addEntryValue(acpIds, nameSpace, name, content);
    ASSERT_TRUE(encContent.has_value());
    ASSERT_EQ(encContent->size(), content.size() + 28);

    optional<vector<uint8_t>> signatureOfToBeSigned = provisioningProxy.finishAddingEntries();
    ASSERT_TRUE(signatureOfToBeSigned.has_value());

    optional<vector<uint8_t>> credData = provisioningProxy.finishGetCredentialData(docType);
    ASSERT_TRUE(credData.has_value());
    ASSERT_TRUE(provisioningProxy.shutdown());

    // Then present data from it...
    //
    FakeSecureHardwarePresentationProxy presentationProxy;
    ASSERT_TRUE(presentationProxy.initialize(isTestCredential, docType, credData.value()));
    AccessCheckResult res =
            presentationProxy.startRetrieveEntryValue(nameSpace, name, 1, content.size(), acpIds);
    ASSERT_EQ(res, AccessCheckResult::kNoAccessControlProfiles);

    // Ensure that we can't get the data out if startRetrieveEntryValue() returned
    // something other than kOk... See b/190757775 for details.
    //
    optional<vector<uint8_t>> decContent =
            presentationProxy.retrieveEntryValue(encContent.value(), nameSpace, name, acpIds);
    ASSERT_FALSE(decContent.has_value());
}

int main(int argc, char** argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
