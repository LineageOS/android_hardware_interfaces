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

#include "RemotelyProvisionedComponent.h"

#include <assert.h>
#include <variant>

#include <cppbor.h>
#include <cppbor_parse.h>

#include <KeyMintUtils.h>
#include <keymaster/cppcose/cppcose.h>
#include <keymaster/keymaster_configuration.h>
#include <remote_prov/remote_prov_utils.h>

#include <openssl/bn.h>
#include <openssl/ec.h>
#include <openssl/rand.h>
#include <openssl/x509.h>

namespace aidl::android::hardware::security::keymint {

using ::std::string;
using ::std::tuple;
using ::std::unique_ptr;
using ::std::variant;
using ::std::vector;
using bytevec = ::std::vector<uint8_t>;

using namespace cppcose;
using namespace keymaster;

namespace {

constexpr auto STATUS_FAILED = RemotelyProvisionedComponent::STATUS_FAILED;

struct AStatusDeleter {
    void operator()(AStatus* p) { AStatus_delete(p); }
};

// TODO(swillden): Remove the dependency on AStatus stuff.  The COSE lib should use something like
// StatusOr, but it shouldn't depend on AStatus.
class Status {
  public:
    Status() {}
    Status(int32_t errCode, const std::string& errMsg)
        : status_(AStatus_fromServiceSpecificErrorWithMessage(errCode, errMsg.c_str())) {}
    explicit Status(const std::string& errMsg)
        : status_(AStatus_fromServiceSpecificErrorWithMessage(STATUS_FAILED, errMsg.c_str())) {}
    Status(AStatus* status) : status_(status) {}
    Status(Status&&) = default;
    Status(const Status&) = delete;

    operator ::ndk::ScopedAStatus() && { return ndk::ScopedAStatus(status_.release()); }

    bool isOk() { return !status_; }

    // Don't call getMessage() unless isOk() returns false;
    const char* getMessage() const { return AStatus_getMessage(status_.get()); }

  private:
    std::unique_ptr<AStatus, AStatusDeleter> status_;
};

template <typename T>
class StatusOr {
  public:
    StatusOr(AStatus* status) : status_(status) {}
    StatusOr(Status status) : status_(std::move(status)) {}
    StatusOr(T val) : value_(std::move(val)) {}

    bool isOk() { return status_.isOk(); }

    T* operator->() & {
        assert(isOk());
        return &value_.value();
    }
    T& operator*() & {
        assert(isOk());
        return value_.value();
    }
    T&& operator*() && {
        assert(isOk());
        return std::move(value_).value();
    }

    const char* getMessage() const {
        assert(!isOk());
        return status_.getMessage();
    }

    Status moveError() {
        assert(!isOk());
        return std::move(status_);
    }

    T moveValue() { return std::move(value_).value(); }

  private:
    Status status_;
    std::optional<T> value_;
};

}  // namespace

RemotelyProvisionedComponent::RemotelyProvisionedComponent(
        std::shared_ptr<keymint::AndroidKeyMintDevice> keymint) {
    impl_ = keymint->getKeymasterImpl();
}

RemotelyProvisionedComponent::~RemotelyProvisionedComponent() {}

ScopedAStatus RemotelyProvisionedComponent::generateEcdsaP256KeyPair(bool testMode,
                                                                     MacedPublicKey* macedPublicKey,
                                                                     bytevec* privateKeyHandle) {
    GenerateRkpKeyRequest request(impl_->message_version());
    request.test_mode = testMode;
    GenerateRkpKeyResponse response(impl_->message_version());
    impl_->GenerateRkpKey(request, &response);
    if (response.error != KM_ERROR_OK) {
        return Status(-static_cast<int32_t>(response.error), "Failure in key generation.");
    }

    macedPublicKey->macedKey = km_utils::kmBlob2vector(response.maced_public_key);
    *privateKeyHandle = km_utils::kmBlob2vector(response.key_blob);
    return ScopedAStatus::ok();
}

ScopedAStatus RemotelyProvisionedComponent::generateCertificateRequest(
        bool testMode, const vector<MacedPublicKey>& keysToSign,
        const bytevec& endpointEncCertChain, const bytevec& challenge, DeviceInfo* deviceInfo,
        ProtectedData* protectedData, bytevec* keysToSignMac) {
    GenerateCsrRequest request(impl_->message_version());
    request.test_mode = testMode;
    request.num_keys = keysToSign.size();
    request.keys_to_sign_array = new KeymasterBlob[keysToSign.size()];
    for (int i = 0; i < keysToSign.size(); i++) {
        request.SetKeyToSign(i, keysToSign[i].macedKey.data(), keysToSign[i].macedKey.size());
    }
    request.SetEndpointEncCertChain(endpointEncCertChain.data(), endpointEncCertChain.size());
    request.SetChallenge(challenge.data(), challenge.size());
    GenerateCsrResponse response(impl_->message_version());
    impl_->GenerateCsr(request, &response);

    if (response.error != KM_ERROR_OK) {
        return Status(-static_cast<int32_t>(response.error), "Failure in CSR Generation.");
    }
    deviceInfo->deviceInfo = km_utils::kmBlob2vector(response.device_info_blob);
    protectedData->protectedData = km_utils::kmBlob2vector(response.protected_data_blob);
    *keysToSignMac = km_utils::kmBlob2vector(response.keys_to_sign_mac);
    return ScopedAStatus::ok();
}

}  // namespace aidl::android::hardware::security::keymint
