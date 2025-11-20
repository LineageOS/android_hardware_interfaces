/*
 * Copyright (C) 2025 The Android Open Source Project
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

#include <aidl/android/hardware/security/see/hwcrypto/BnCryptoOperationContext.h>
#include <aidl/android/hardware/security/see/hwcrypto/BnHwCryptoOperations.h>
#include <aidl/android/hardware/security/see/hwcrypto/BnOpaqueKey.h>
#include <aidl/android/hardware/security/see/hwcrypto/IOpaqueKey.h>
#include <android-base/logging.h>
#include <android/hardware/security/see/hwcrypto/BnHwCryptoKey.h>
#include <binder/RpcTrusty.h>
#include <trusty/tipc.h>
#include <optional>
#include <string>
#include "hwcryptokeyimpl.h"

using android::IBinder;
using android::IInterface;
using android::RpcSession;
using android::RpcTrustyConnectWithSessionInitializer;
using android::sp;
using android::wp;
using android::base::ErrnoError;
using android::base::Error;
using android::base::Result;
using android::binder::Status;

namespace android {
namespace trusty {
namespace hwcryptohalservice {

#define HWCRYPTO_KEY_PORT "com.android.trusty.rust.hwcryptohal.V1"

// Even though we get the cpp_hwcrypto::IOpaqueKey and cpp_hwcrypto::ICryptoOperationContext and
// create the ndk_hwcrypto wrappers on this library we cannot cast them back when we need them
// because they are received on the function calls as binder objects and there is no reliable
// we to do this cast yet. Because of that we are creating maps to hold the wrapped objects
// and translate them on function calls.
// TODO: Add cleanup of both keyMapping and contextMapping once we have more test infrastructure in
//       place.
std::map<std::weak_ptr<ndk_hwcrypto::IOpaqueKey>, wp<cpp_hwcrypto::IOpaqueKey>, std::owner_less<>>
        keyMapping;
std::map<std::weak_ptr<ndk_hwcrypto::ICryptoOperationContext>,
         wp<cpp_hwcrypto::ICryptoOperationContext>, std::owner_less<>>
        contextMapping;

static ndk::ScopedAStatus convertStatus(Status status) {
    if (status.isOk()) {
        return ndk::ScopedAStatus::ok();
    } else {
        auto exCode = status.exceptionCode();
        if (exCode == Status::Exception::EX_SERVICE_SPECIFIC) {
            return ndk::ScopedAStatus::fromServiceSpecificErrorWithMessage(
                    status.serviceSpecificErrorCode(), status.exceptionMessage());
        } else {
            return ndk::ScopedAStatus::fromExceptionCodeWithMessage(exCode,
                                                                    status.exceptionMessage());
        }
    }
}

static std::optional<cpp_hwcrypto::types::ExplicitKeyMaterial> convertExplicitKeyMaterial(
        const ndk_hwcrypto::types::ExplicitKeyMaterial& keyMaterial) {
    auto explicitKeyCpp = cpp_hwcrypto::types::ExplicitKeyMaterial();

    if (keyMaterial.getTag() == ndk_hwcrypto::types::ExplicitKeyMaterial::aes) {
        auto aesKey = keyMaterial.get<ndk_hwcrypto::types::ExplicitKeyMaterial::aes>();
        auto aesKeyCpp = cpp_hwcrypto::types::AesKey();
        if (aesKey.getTag() == ndk_hwcrypto::types::AesKey::aes128) {
            aesKeyCpp.set<cpp_hwcrypto::types::AesKey::aes128>(
                    aesKey.get<ndk_hwcrypto::types::AesKey::aes128>());
            explicitKeyCpp.set<cpp_hwcrypto::types::ExplicitKeyMaterial::aes>(aesKeyCpp);
        } else if (aesKey.getTag() == ndk_hwcrypto::types::AesKey::aes256) {
            aesKeyCpp.set<cpp_hwcrypto::types::AesKey::aes256>(
                    aesKey.get<ndk_hwcrypto::types::AesKey::aes256>());
            explicitKeyCpp.set<cpp_hwcrypto::types::ExplicitKeyMaterial::aes>(aesKeyCpp);
        } else {
            LOG(ERROR) << "unknown AesKey type";
            return std::nullopt;
        }
    } else if (keyMaterial.getTag() == ndk_hwcrypto::types::ExplicitKeyMaterial::hmac) {
        auto hmacKey = keyMaterial.get<ndk_hwcrypto::types::ExplicitKeyMaterial::hmac>();
        auto hmacKeyCpp = cpp_hwcrypto::types::HmacKey();
        if (hmacKey.getTag() == ndk_hwcrypto::types::HmacKey::sha256) {
            hmacKeyCpp.set<cpp_hwcrypto::types::HmacKey::sha256>(
                    hmacKey.get<ndk_hwcrypto::types::HmacKey::sha256>());
            explicitKeyCpp.set<cpp_hwcrypto::types::ExplicitKeyMaterial::hmac>(hmacKeyCpp);
        } else if (hmacKey.getTag() == ndk_hwcrypto::types::HmacKey::sha512) {
            hmacKeyCpp.set<cpp_hwcrypto::types::HmacKey::sha512>(
                    hmacKey.get<ndk_hwcrypto::types::HmacKey::sha512>());
            explicitKeyCpp.set<cpp_hwcrypto::types::ExplicitKeyMaterial::hmac>(hmacKeyCpp);
        } else {
            LOG(ERROR) << "unknown HmacKey type";
            return std::nullopt;
        }
    } else {
        LOG(ERROR) << "unknown Key type";
        return std::nullopt;
    }
    return explicitKeyCpp;
}

class HwCryptoOperationContextNdk : public ndk_hwcrypto::BnCryptoOperationContext {
  private:
    sp<cpp_hwcrypto::ICryptoOperationContext> mContext;
    std::weak_ptr<ndk_hwcrypto::ICryptoOperationContext> self;

  public:
    HwCryptoOperationContextNdk(sp<cpp_hwcrypto::ICryptoOperationContext> operations)
        : mContext(std::move(operations)) {}

    ~HwCryptoOperationContextNdk() { contextMapping.erase(self); }

    static std::shared_ptr<HwCryptoOperationContextNdk> Create(
            sp<cpp_hwcrypto::ICryptoOperationContext> operations) {
        if (operations == nullptr) {
            return nullptr;
        }
        std::shared_ptr<HwCryptoOperationContextNdk> contextNdk =
                ndk::SharedRefBase::make<HwCryptoOperationContextNdk>(std::move(operations));

        if (!contextNdk) {
            LOG(ERROR) << "failed to allocate HwCryptoOperationContext";
            return nullptr;
        }
        contextNdk->self = contextNdk;
        return contextNdk;
    }
};

std::optional<cpp_hwcrypto::types::OperationData> convertOperationData(
        const ndk_hwcrypto::types::OperationData& ndkOperationData) {
    cpp_hwcrypto::types::OperationData cppOperationData = cpp_hwcrypto::types::OperationData();
    cpp_hwcrypto::types::MemoryBufferReference cppMemBuffRef;
    switch (ndkOperationData.getTag()) {
        case ndk_hwcrypto::types::OperationData::dataBuffer:
            cppOperationData.set<cpp_hwcrypto::types::OperationData::dataBuffer>(
                    ndkOperationData.get<ndk_hwcrypto::types::OperationData::dataBuffer>());
            break;
        case ndk_hwcrypto::types::OperationData::memoryBufferReference:
            cppMemBuffRef.startOffset =
                    ndkOperationData
                            .get<ndk_hwcrypto::types::OperationData::memoryBufferReference>()
                            .startOffset;
            cppMemBuffRef.sizeBytes =
                    ndkOperationData
                            .get<ndk_hwcrypto::types::OperationData::memoryBufferReference>()
                            .sizeBytes;
            cppOperationData.set<cpp_hwcrypto::types::OperationData::memoryBufferReference>(
                    std::move(cppMemBuffRef));
            break;
        default:
            LOG(ERROR) << "received unknown operation data type";
            return std::nullopt;
    }
    return cppOperationData;
}

std::optional<cpp_hwcrypto::PatternParameters> convertPatternParameters(
        const ndk_hwcrypto::PatternParameters& ndkpatternParameters) {
    int64_t numberBlocksProcess = ndkpatternParameters.numberBlocksProcess;
    int64_t numberBlocksCopy = ndkpatternParameters.numberBlocksCopy;
    if ((numberBlocksProcess < 0) || (numberBlocksCopy < 0)) {
        LOG(ERROR) << "received invalid pattern parameters";
        return std::nullopt;
    }
    cpp_hwcrypto::PatternParameters patternParameters = cpp_hwcrypto::PatternParameters();
    patternParameters.numberBlocksProcess = numberBlocksProcess;
    patternParameters.numberBlocksCopy = numberBlocksCopy;
    return patternParameters;
}

std::optional<cpp_hwcrypto::types::SymmetricOperation> convertSymmetricOperation(
        const ndk_hwcrypto::types::SymmetricOperation& ndkSymmetricOperation) {
    cpp_hwcrypto::types::SymmetricOperation symmetricOperation =
            cpp_hwcrypto::types::SymmetricOperation();
    switch (ndkSymmetricOperation) {
        case ndk_hwcrypto::types::SymmetricOperation::ENCRYPT:
            symmetricOperation = cpp_hwcrypto::types::SymmetricOperation::ENCRYPT;
            break;
        case ndk_hwcrypto::types::SymmetricOperation::DECRYPT:
            symmetricOperation = cpp_hwcrypto::types::SymmetricOperation::DECRYPT;
            break;
        default:
            LOG(ERROR) << "invalid symmetric operation type";
            return std::nullopt;
    }
    return symmetricOperation;
}

cpp_hwcrypto::types::CipherModeParameters convertSymmetricModeParameters(
        const ndk_hwcrypto::types::CipherModeParameters& ndkcipherModeParameters) {
    cpp_hwcrypto::types::CipherModeParameters cipherModeParameters =
            cpp_hwcrypto::types::CipherModeParameters();
    cipherModeParameters.nonce = ndkcipherModeParameters.nonce;
    return cipherModeParameters;
}

cpp_hwcrypto::types::AesGcmMode::AesGcmModeParameters convertSymmetricModeParameters(
        const ndk_hwcrypto::types::AesGcmMode::AesGcmModeParameters& ndkgcmModeParameters) {
    cpp_hwcrypto::types::AesGcmMode::AesGcmModeParameters gcmModeParameters =
            cpp_hwcrypto::types::AesGcmMode::AesGcmModeParameters();
    gcmModeParameters.nonce = ndkgcmModeParameters.nonce;
    return gcmModeParameters;
}

std::optional<cpp_hwcrypto::MemoryBufferParameter> convertMemoryBufferParameters(
        const ndk_hwcrypto::MemoryBufferParameter& ndkMemBuffParams) {
    cpp_hwcrypto::MemoryBufferParameter memBuffParams = cpp_hwcrypto::MemoryBufferParameter();
    memBuffParams.sizeBytes = ndkMemBuffParams.sizeBytes;
    android::os::ParcelFileDescriptor pfd;
    ndk::ScopedFileDescriptor ndkFd;
    switch (ndkMemBuffParams.bufferHandle.getTag()) {
        case ndk_hwcrypto::MemoryBufferParameter::MemoryBuffer::input:
            ndkFd = ndkMemBuffParams.bufferHandle
                            .get<ndk_hwcrypto::MemoryBufferParameter::MemoryBuffer::input>()
                            .dup();
            pfd.reset(binder::unique_fd(ndkFd.release()));
            memBuffParams.bufferHandle
                    .set<cpp_hwcrypto::MemoryBufferParameter::MemoryBuffer::input>(std::move(pfd));
            break;
        case ndk_hwcrypto::MemoryBufferParameter::MemoryBuffer::output:
            ndkFd = ndkMemBuffParams.bufferHandle
                            .get<ndk_hwcrypto::MemoryBufferParameter::MemoryBuffer::output>()
                            .dup();
            pfd.reset(binder::unique_fd(ndkFd.release()));
            memBuffParams.bufferHandle
                    .set<cpp_hwcrypto::MemoryBufferParameter::MemoryBuffer::output>(std::move(pfd));
            break;
        default:
            LOG(ERROR) << "unknown bufferHandle type";
            return std::nullopt;
    }
    return memBuffParams;
}

std::optional<cpp_hwcrypto::OperationParameters> convertOperationParameters(
        const ndk_hwcrypto::OperationParameters& ndkOperationParameters) {
    cpp_hwcrypto::OperationParameters operationParameters = cpp_hwcrypto::OperationParameters();
    sp<cpp_hwcrypto::IOpaqueKey> opaqueKey;
    cpp_hwcrypto::types::HmacOperationParameters hmacParameters =
            cpp_hwcrypto::types::HmacOperationParameters();
    std::optional<cpp_hwcrypto::types::SymmetricOperation> cppSymmetricOperation;
    cpp_hwcrypto::types::CipherModeParameters cipherModeParameters;
    cpp_hwcrypto::types::AesCipherMode cppAesCipherMode = cpp_hwcrypto::types::AesCipherMode();
    cpp_hwcrypto::types::SymmetricOperationParameters cppSymmetricOperationParameters =
            cpp_hwcrypto::types::SymmetricOperationParameters();
    cpp_hwcrypto::types::SymmetricAuthOperationParameters cppSymmetricAuthOperationParameters =
            cpp_hwcrypto::types::SymmetricAuthOperationParameters();
    cpp_hwcrypto::types::AesGcmMode::AesGcmModeParameters cppAesGcmModeParameters =
            cpp_hwcrypto::types::AesGcmMode::AesGcmModeParameters();
    cpp_hwcrypto::types::AesGcmMode cppAesGcmMode = cpp_hwcrypto::types::AesGcmMode();
    switch (ndkOperationParameters.getTag()) {
        case ndk_hwcrypto::OperationParameters::symmetricAuthCrypto:
            opaqueKey = retrieveCppBinder<cpp_hwcrypto::IOpaqueKey, ndk_hwcrypto::IOpaqueKey,
                                          keyMapping>(
                    ndkOperationParameters
                            .get<ndk_hwcrypto::OperationParameters::symmetricAuthCrypto>()
                            .key);
            if (!opaqueKey) {
                LOG(ERROR) << "couldn't get aes key";
                return std::nullopt;
            }
            cppSymmetricAuthOperationParameters.key = std::move(opaqueKey);
            cppSymmetricOperation = convertSymmetricOperation(
                    ndkOperationParameters
                            .get<ndk_hwcrypto::OperationParameters::symmetricAuthCrypto>()
                            .direction);
            if (!cppSymmetricOperation.has_value()) {
                LOG(ERROR) << "couldn't get aes direction";
                return std::nullopt;
            }
            cppSymmetricAuthOperationParameters.direction =
                    std::move(cppSymmetricOperation.value());
            switch (ndkOperationParameters
                            .get<ndk_hwcrypto::OperationParameters::symmetricAuthCrypto>()
                            .parameters.getTag()) {
                case ndk_hwcrypto::types::SymmetricAuthCryptoParameters::aes:
                    switch (ndkOperationParameters
                                    .get<ndk_hwcrypto::OperationParameters::symmetricAuthCrypto>()
                                    .parameters
                                    .get<ndk_hwcrypto::types::SymmetricAuthCryptoParameters::aes>()
                                    .getTag()) {
                        case ndk_hwcrypto::types::AesGcmMode::gcmTag16:
                            cppAesGcmModeParameters = convertSymmetricModeParameters(
                                    ndkOperationParameters
                                            .get<ndk_hwcrypto::OperationParameters::
                                                         symmetricAuthCrypto>()
                                            .parameters
                                            .get<ndk_hwcrypto::types::
                                                         SymmetricAuthCryptoParameters::aes>()
                                            .get<ndk_hwcrypto::types::AesGcmMode::gcmTag16>());
                            cppAesGcmMode.set<cpp_hwcrypto::types::AesGcmMode::gcmTag16>(
                                    std::move(cppAesGcmModeParameters));
                            cppSymmetricAuthOperationParameters.parameters
                                    .set<cpp_hwcrypto::types::SymmetricAuthCryptoParameters::aes>(
                                            std::move(cppAesGcmMode));
                            break;
                        default:
                            LOG(ERROR) << "received invalid aes gcm parameters";
                            return std::nullopt;
                    }
                    break;
                default:
                    LOG(ERROR) << "received invalid symmetric auth crypto parameters";
                    return std::nullopt;
            }
            operationParameters.set<cpp_hwcrypto::OperationParameters::symmetricAuthCrypto>(
                    std::move(cppSymmetricAuthOperationParameters));
            break;
        case ndk_hwcrypto::OperationParameters::symmetricCrypto:
            opaqueKey = retrieveCppBinder<cpp_hwcrypto::IOpaqueKey, ndk_hwcrypto::IOpaqueKey,
                                          keyMapping>(
                    ndkOperationParameters.get<ndk_hwcrypto::OperationParameters::symmetricCrypto>()
                            .key);
            if (!opaqueKey) {
                LOG(ERROR) << "couldn't get aes key";
                return std::nullopt;
            }
            cppSymmetricOperationParameters.key = std::move(opaqueKey);
            cppSymmetricOperation = convertSymmetricOperation(
                    ndkOperationParameters.get<ndk_hwcrypto::OperationParameters::symmetricCrypto>()
                            .direction);
            if (!cppSymmetricOperation.has_value()) {
                LOG(ERROR) << "couldn't get aes direction";
                return std::nullopt;
            }
            cppSymmetricOperationParameters.direction = std::move(cppSymmetricOperation.value());
            switch (ndkOperationParameters.get<ndk_hwcrypto::OperationParameters::symmetricCrypto>()
                            .parameters.getTag()) {
                case ndk_hwcrypto::types::SymmetricCryptoParameters::aes:
                    switch (ndkOperationParameters
                                    .get<ndk_hwcrypto::OperationParameters::symmetricCrypto>()
                                    .parameters
                                    .get<ndk_hwcrypto::types::SymmetricCryptoParameters::aes>()
                                    .getTag()) {
                        case ndk_hwcrypto::types::AesCipherMode::cbc:
                            cipherModeParameters = convertSymmetricModeParameters(
                                    ndkOperationParameters
                                            .get<ndk_hwcrypto::OperationParameters::
                                                         symmetricCrypto>()
                                            .parameters
                                            .get<ndk_hwcrypto::types::SymmetricCryptoParameters::
                                                         aes>()
                                            .get<ndk_hwcrypto::types::AesCipherMode::cbc>());
                            cppAesCipherMode.set<cpp_hwcrypto::types::AesCipherMode::cbc>(
                                    std::move(cipherModeParameters));
                            cppSymmetricOperationParameters.parameters
                                    .set<cpp_hwcrypto::types::SymmetricCryptoParameters::aes>(
                                            std::move(cppAesCipherMode));
                            break;
                        case ndk_hwcrypto::types::AesCipherMode::ctr:
                            cipherModeParameters = convertSymmetricModeParameters(
                                    ndkOperationParameters
                                            .get<ndk_hwcrypto::OperationParameters::
                                                         symmetricCrypto>()
                                            .parameters
                                            .get<ndk_hwcrypto::types::SymmetricCryptoParameters::
                                                         aes>()
                                            .get<ndk_hwcrypto::types::AesCipherMode::ctr>());
                            cppAesCipherMode.set<cpp_hwcrypto::types::AesCipherMode::ctr>(
                                    std::move(cipherModeParameters));
                            cppSymmetricOperationParameters.parameters
                                    .set<cpp_hwcrypto::types::SymmetricCryptoParameters::aes>(
                                            std::move(cppAesCipherMode));
                            break;
                        default:
                            LOG(ERROR) << "received invalid aes parameters";
                            return std::nullopt;
                    }
                    break;
                default:
                    LOG(ERROR) << "received invalid symmetric crypto parameters";
                    return std::nullopt;
            }
            operationParameters.set<cpp_hwcrypto::OperationParameters::symmetricCrypto>(
                    std::move(cppSymmetricOperationParameters));
            break;
        case ndk_hwcrypto::OperationParameters::hmac:
            opaqueKey = retrieveCppBinder<cpp_hwcrypto::IOpaqueKey, ndk_hwcrypto::IOpaqueKey,
                                          keyMapping>(
                    ndkOperationParameters.get<ndk_hwcrypto::OperationParameters::hmac>().key);
            if (!opaqueKey) {
                LOG(ERROR) << "couldn't get hmac key";
                return std::nullopt;
            }
            hmacParameters.key = opaqueKey;
            operationParameters.set<cpp_hwcrypto::OperationParameters::hmac>(
                    std::move(hmacParameters));
            break;
        default:
            LOG(ERROR) << "received invalid operation parameters";
            return std::nullopt;
    }
    return operationParameters;
}

class HwCryptoOperationsNdk : public ndk_hwcrypto::BnHwCryptoOperations {
  private:
    sp<cpp_hwcrypto::IHwCryptoOperations> mHwCryptoOperations;

  public:
    HwCryptoOperationsNdk(sp<cpp_hwcrypto::IHwCryptoOperations> operations)
        : mHwCryptoOperations(std::move(operations)) {}

    static std::shared_ptr<HwCryptoOperationsNdk> Create(
            sp<cpp_hwcrypto::IHwCryptoOperations> operations) {
        if (operations == nullptr) {
            return nullptr;
        }
        std::shared_ptr<HwCryptoOperationsNdk> operationsNdk =
                ndk::SharedRefBase::make<HwCryptoOperationsNdk>(std::move(operations));

        if (!operationsNdk) {
            LOG(ERROR) << "failed to allocate HwCryptoOperations";
            return nullptr;
        }
        return operationsNdk;
    }

    ndk::ScopedAStatus processCommandList(
            std::vector<ndk_hwcrypto::CryptoOperationSet>* operationSets,
            std::vector<ndk_hwcrypto::CryptoOperationResult>* aidl_return) {
        Status status = Status::fromExceptionCode(Status::EX_ILLEGAL_ARGUMENT);
        if (operationSets == nullptr) {
            LOG(ERROR) << "received a null operation set";
            return convertStatus(status);
        }
        if (aidl_return == nullptr) {
            LOG(ERROR) << "received a null CryptoOperationResult set";
            return convertStatus(status);
        }
        std::vector<cpp_hwcrypto::CryptoOperationResult> binderResult;
        std::vector<cpp_hwcrypto::CryptoOperationSet> cppOperationSets;
        for (ndk_hwcrypto::CryptoOperationSet& operationSet : *operationSets) {
            cpp_hwcrypto::CryptoOperationSet cppSingleOperation =
                    cpp_hwcrypto::CryptoOperationSet();
            cppSingleOperation.context =
                    retrieveCppBinder<cpp_hwcrypto::ICryptoOperationContext,
                                      ndk_hwcrypto::ICryptoOperationContext, contextMapping>(
                            operationSet.context);
            for (ndk_hwcrypto::CryptoOperation& operation : operationSet.operations) {
                cpp_hwcrypto::CryptoOperation cppOperation;
                cpp_hwcrypto::types::Void voidObj;
                std::optional<cpp_hwcrypto::types::OperationData> cppOperationData;
                std::optional<cpp_hwcrypto::PatternParameters> cppPatternParameters;
                std::optional<cpp_hwcrypto::OperationParameters> cppOperationParameters;
                std::optional<cpp_hwcrypto::MemoryBufferParameter> cppMemBuffParams;
                switch (operation.getTag()) {
                    case ndk_hwcrypto::CryptoOperation::setMemoryBuffer:
                        cppMemBuffParams = convertMemoryBufferParameters(
                                operation.get<ndk_hwcrypto::CryptoOperation::setMemoryBuffer>());
                        if (cppMemBuffParams.has_value()) {
                            cppOperation.set<cpp_hwcrypto::CryptoOperation::setMemoryBuffer>(
                                    std::move(cppMemBuffParams.value()));
                        } else {
                            return convertStatus(status);
                        }
                        break;
                    case ndk_hwcrypto::CryptoOperation::setOperationParameters:
                        cppOperationParameters = convertOperationParameters(
                                operation.get<
                                        ndk_hwcrypto::CryptoOperation::setOperationParameters>());
                        if (cppOperationParameters.has_value()) {
                            cppOperation.set<cpp_hwcrypto::CryptoOperation::setOperationParameters>(
                                    std::move(cppOperationParameters.value()));
                        } else {
                            LOG(ERROR) << "couldn't convert operation parameters";
                            return convertStatus(status);
                        }
                        break;
                    case ndk_hwcrypto::CryptoOperation::setPattern:
                        cppPatternParameters = convertPatternParameters(
                                operation.get<ndk_hwcrypto::CryptoOperation::setPattern>());
                        if (cppPatternParameters.has_value()) {
                            cppOperation.set<cpp_hwcrypto::CryptoOperation::setPattern>(
                                    std::move(cppPatternParameters.value()));
                        } else {
                            LOG(ERROR) << "couldn't convert pattern parameters";
                            return convertStatus(status);
                        }
                        break;
                    case ndk_hwcrypto::CryptoOperation::copyData:
                        cppOperationData = convertOperationData(
                                operation.get<ndk_hwcrypto::CryptoOperation::copyData>());
                        if (cppOperationData.has_value()) {
                            cppOperation.set<cpp_hwcrypto::CryptoOperation::copyData>(
                                    std::move(cppOperationData.value()));
                        } else {
                            LOG(ERROR) << "couldn't convert CryptoOperation::copyData";
                            return convertStatus(status);
                        }
                        break;
                    case ndk_hwcrypto::CryptoOperation::aadInput:
                        cppOperationData = convertOperationData(
                                operation.get<ndk_hwcrypto::CryptoOperation::aadInput>());
                        if (cppOperationData.has_value()) {
                            cppOperation.set<cpp_hwcrypto::CryptoOperation::aadInput>(
                                    std::move(cppOperationData.value()));
                        } else {
                            LOG(ERROR) << "couldn't convert CryptoOperation::aadInput";
                            return convertStatus(status);
                        }
                        break;
                    case ndk_hwcrypto::CryptoOperation::dataInput:
                        cppOperationData = convertOperationData(
                                operation.get<ndk_hwcrypto::CryptoOperation::dataInput>());
                        if (cppOperationData.has_value()) {
                            cppOperation.set<cpp_hwcrypto::CryptoOperation::dataInput>(
                                    std::move(cppOperationData.value()));
                        } else {
                            LOG(ERROR) << "couldn't convert CryptoOperation::dataInput";
                            return convertStatus(status);
                        }
                        break;
                    case ndk_hwcrypto::CryptoOperation::dataOutput:
                        cppOperationData = convertOperationData(
                                operation.get<ndk_hwcrypto::CryptoOperation::dataOutput>());
                        if (cppOperationData.has_value()) {
                            cppOperation.set<cpp_hwcrypto::CryptoOperation::dataOutput>(
                                    std::move(cppOperationData.value()));
                        } else {
                            LOG(ERROR) << "couldn't convert CryptoOperation::dataOutput";
                            return convertStatus(status);
                        }
                        break;
                    case ndk_hwcrypto::CryptoOperation::destroyContext:
                        cppOperation.set<cpp_hwcrypto::CryptoOperation::destroyContext>(
                                std::move(voidObj));
                        break;
                    case ndk_hwcrypto::CryptoOperation::finish:
                        cppOperation.set<cpp_hwcrypto::CryptoOperation::finish>(std::move(voidObj));
                        break;
                    default:
                        // This shouldn't happen
                        LOG(ERROR) << "received unknown crypto operation";
                        return convertStatus(status);
                }
                cppSingleOperation.operations.push_back(std::move(cppOperation));
            }
            cppOperationSets.push_back(std::move(cppSingleOperation));
        }
        status = mHwCryptoOperations->processCommandList(&cppOperationSets, &binderResult);
        if (status.isOk()) {
            *aidl_return = std::vector<ndk_hwcrypto::CryptoOperationResult>();
            for (cpp_hwcrypto::CryptoOperationResult& result : binderResult) {
                ndk_hwcrypto::CryptoOperationResult ndkResult =
                        ndk_hwcrypto::CryptoOperationResult();
                if (result.context != nullptr) {
                    insertBinderMapping<cpp_hwcrypto::ICryptoOperationContext,
                                        ndk_hwcrypto::ICryptoOperationContext,
                                        HwCryptoOperationContextNdk, contextMapping>(
                            result.context, &ndkResult.context);
                } else {
                    ndkResult.context = nullptr;
                }
                aidl_return->push_back(std::move(ndkResult));
            }
        } else {
            // No reason to copy back the data output vectors if this failed
            LOG(ERROR) << "couldn't process command list";
            return convertStatus(status);
        }
        // We need to copy the vectors from the cpp operations back to the ndk one
        if (cppOperationSets.size() != operationSets->size()) {
            LOG(ERROR) << "ndk and cpp operation sets had a different number of elements";
            return convertStatus(Status::fromExceptionCode(Status::EX_ILLEGAL_ARGUMENT));
        }
        for (unsigned setIdx = 0; setIdx < cppOperationSets.size(); ++setIdx) {
            // We will get rid of all inputs on the operation set parameter and then copy the return
            // results. This will contain only the operations that contain generated outputs from
            // the service.
            (*operationSets)[setIdx].operations.clear();
            int operationIdx = 0;
            for (auto& cppOperation : cppOperationSets[setIdx].operations) {
                if (cppOperation.getTag() != cpp_hwcrypto::CryptoOperation::dataOutput) {
                    LOG(ERROR) << "cpp operations on set " << setIdx << " and operation "
                               << operationIdx << " is not CryptoOperation::dataOutput";
                    return convertStatus(Status::fromExceptionCode(Status::EX_ILLEGAL_ARGUMENT));
                }
                auto cppDataOperation =
                        cppOperation.get<cpp_hwcrypto::CryptoOperation::dataOutput>();
                if (cppDataOperation.getTag() != cpp_hwcrypto::types::OperationData::dataBuffer) {
                    LOG(ERROR) << "cpp operations on set " << setIdx << " and operation "
                               << operationIdx
                               << " is not CryptoOperation::dataOutput(OperationData::dataBuffer)";
                    return convertStatus(Status::fromExceptionCode(Status::EX_ILLEGAL_ARGUMENT));
                }
                ndk_hwcrypto::types::OperationData ndkOperationData;
                ndkOperationData.set<ndk_hwcrypto::types::OperationData::dataBuffer>(
                        cppDataOperation.get<cpp_hwcrypto::types::OperationData::dataBuffer>());
                ndk_hwcrypto::CryptoOperation ndkOperation;
                ndkOperation.set<ndk_hwcrypto::CryptoOperation::dataOutput>(
                        std::move(ndkOperationData));
                (*operationSets)[setIdx].operations.push_back(std::move(ndkOperation));
                ++operationIdx;
            }
        }
        return convertStatus(status);
    }
};

class OpaqueKeyNdk : public ndk_hwcrypto::BnOpaqueKey {
  private:
    sp<cpp_hwcrypto::IOpaqueKey> mOpaqueKey;
    std::weak_ptr<ndk_hwcrypto::IOpaqueKey> self;

  public:
    OpaqueKeyNdk(sp<cpp_hwcrypto::IOpaqueKey> opaqueKey) : mOpaqueKey(std::move(opaqueKey)) {}

    ~OpaqueKeyNdk() { keyMapping.erase(self); }

    static std::shared_ptr<OpaqueKeyNdk> Create(sp<cpp_hwcrypto::IOpaqueKey> opaqueKey) {
        if (opaqueKey == nullptr) {
            return nullptr;
        }
        std::shared_ptr<OpaqueKeyNdk> opaqueKeyNdk =
                ndk::SharedRefBase::make<OpaqueKeyNdk>(std::move(opaqueKey));

        if (!opaqueKeyNdk) {
            LOG(ERROR) << "failed to allocate HwCryptoKey";
            return nullptr;
        }
        opaqueKeyNdk->self = opaqueKeyNdk;
        return opaqueKeyNdk;
    }

    ndk::ScopedAStatus exportWrappedKey(
            const std::shared_ptr<ndk_hwcrypto::IOpaqueKey>& wrappingKey,
            ::std::vector<uint8_t>* aidl_return) {
        Status status = Status::fromExceptionCode(Status::EX_ILLEGAL_ARGUMENT);
        auto wrappingKeyNdk =
                retrieveCppBinder<cpp_hwcrypto::IOpaqueKey, ndk_hwcrypto::IOpaqueKey, keyMapping>(
                        wrappingKey);
        if (wrappingKeyNdk == nullptr) {
            LOG(ERROR) << "couldn't get wrapped key";
            return convertStatus(status);
        }
        status = mOpaqueKey->exportWrappedKey(wrappingKeyNdk, aidl_return);
        return convertStatus(status);
    }

    ndk::ScopedAStatus getKeyPolicy(ndk_hwcrypto::KeyPolicy* aidl_return) {
        Status status = Status::fromExceptionCode(Status::EX_ILLEGAL_ARGUMENT);
        if (aidl_return == nullptr) {
            LOG(ERROR) << "return value passed to getKeyPolicy is nullptr";
            return convertStatus(status);
        }
        cpp_hwcrypto::KeyPolicy cppPolicy = cpp_hwcrypto::KeyPolicy();

        status = mOpaqueKey->getKeyPolicy(&cppPolicy);
        if (status.isOk()) {
            auto ndkPolicy =
                    convertKeyPolicy<ndk_hwcrypto::KeyPolicy, cpp_hwcrypto::KeyPolicy>(cppPolicy);
            *aidl_return = std::move(ndkPolicy);
        }
        return convertStatus(status);
    }

    ndk::ScopedAStatus getPublicKey(::std::vector<uint8_t>* aidl_return) {
        auto status = mOpaqueKey->getPublicKey(aidl_return);
        return convertStatus(status);
    }

    ndk::ScopedAStatus getShareableToken(const ::std::vector<uint8_t>& sealingDicePolicy,
                                         ndk_hwcrypto::types::OpaqueKeyToken* aidl_return) {
        Status status = Status::fromExceptionCode(Status::EX_ILLEGAL_ARGUMENT);
        if (aidl_return == nullptr) {
            LOG(ERROR) << "return value passed to getShareableToken is nullptr";
            return convertStatus(status);
        }
        cpp_hwcrypto::types::OpaqueKeyToken binder_return;
        status = mOpaqueKey->getShareableToken(sealingDicePolicy, &binder_return);
        if (status.isOk()) {
            aidl_return->keyToken = std::move(binder_return.keyToken);
        }
        return convertStatus(status);
    }

    ndk::ScopedAStatus setProtectionId(
            const ndk_hwcrypto::types::ProtectionId /*protectionId*/,
            const ::std::vector<ndk_hwcrypto::types::OperationType>& /*allowedOperations*/) {
        return ndk::ScopedAStatus::fromServiceSpecificErrorWithMessage(
                ndk_hwcrypto::types::HalErrorCode::UNAUTHORIZED,
                "android is not authorized to call setProtectionId");
    }
};

static void create_key_mapping_if_success(Status status,
                                          sp<cpp_hwcrypto::IOpaqueKey>& binder_return,
                                          std::shared_ptr<ndk_hwcrypto::IOpaqueKey>* aidl_return) {
    if (status.isOk()) {
        if ((binder_return != nullptr)) {
            insertBinderMapping<cpp_hwcrypto::IOpaqueKey, ndk_hwcrypto::IOpaqueKey, OpaqueKeyNdk,
                                keyMapping>(binder_return, aidl_return);
        } else {
            *aidl_return = nullptr;
        }
    }
}

Result<void> HwCryptoKey::connectToTrusty(const char* tipcDev) {
    assert(!mSession);
    auto session_initializer = [](sp<RpcSession>& session) {
        session->setFileDescriptorTransportMode(RpcSession::FileDescriptorTransportMode::TRUSTY);
    };
    mSession =
            RpcTrustyConnectWithSessionInitializer(tipcDev, HWCRYPTO_KEY_PORT, session_initializer);
    if (!mSession) {
        return ErrnoError() << "failed to connect to hwcrypto";
    }
    mRoot = mSession->getRootObject();
    mHwCryptoServer = cpp_hwcrypto::IHwCryptoKey::asInterface(mRoot);
    return {};
}

HwCryptoKey::HwCryptoKey() {}

std::shared_ptr<HwCryptoKey> HwCryptoKey::Create(const char* tipcDev) {
    std::shared_ptr<HwCryptoKey> hwCrypto = ndk::SharedRefBase::make<HwCryptoKey>();

    if (!hwCrypto) {
        LOG(ERROR) << "failed to allocate HwCryptoKey";
        return nullptr;
    }

    auto ret = hwCrypto->connectToTrusty(tipcDev);
    if (!ret.ok()) {
        LOG(ERROR) << "failed to connect HwCryptoKey to Trusty: " << ret.error();
        return nullptr;
    }

    return hwCrypto;
}

ndk::ScopedAStatus HwCryptoKey::deriveCurrentDicePolicyBoundKey(
        const ndk_hwcrypto::IHwCryptoKey::DiceBoundDerivationKey& /*derivationKey*/,
        ndk_hwcrypto::IHwCryptoKey::DiceCurrentBoundKeyResult* /*aidl_return*/) {
    return ndk::ScopedAStatus::fromServiceSpecificErrorWithMessage(
            ndk_hwcrypto::types::HalErrorCode::UNAUTHORIZED,
            "android is not authorized to call deriveCurrentDicePolicyBoundKey");
}

ndk::ScopedAStatus HwCryptoKey::deriveDicePolicyBoundKey(
        const ndk_hwcrypto::IHwCryptoKey::DiceBoundDerivationKey& /*derivationKey*/,
        const ::std::vector<uint8_t>& /*dicePolicyForKeyVersion*/,
        ndk_hwcrypto::IHwCryptoKey::DiceBoundKeyResult* /*aidl_return*/) {
    return ndk::ScopedAStatus::fromServiceSpecificErrorWithMessage(
            ndk_hwcrypto::types::HalErrorCode::UNAUTHORIZED,
            "android is not authorized to call deriveDicePolicyBoundKey");
}

ndk::ScopedAStatus HwCryptoKey::deriveKey(
        const ndk_hwcrypto::IHwCryptoKey::DerivedKeyParameters& /*parameters*/,
        ndk_hwcrypto::IHwCryptoKey::DerivedKey* /*aidl_return*/) {
    return ndk::ScopedAStatus::fromServiceSpecificErrorWithMessage(
            ndk_hwcrypto::types::HalErrorCode::UNAUTHORIZED,
            "android is not authorized to call deriveKey");
}

ndk::ScopedAStatus HwCryptoKey::getHwCryptoOperations(
        std::shared_ptr<ndk_hwcrypto::IHwCryptoOperations>* aidl_return) {
    Status status = Status::fromExceptionCode(Status::EX_ILLEGAL_ARGUMENT);
    if (aidl_return == nullptr) {
        LOG(ERROR) << "return value passed to getHwCryptoOperations is nullptr";
        return convertStatus(status);
    }
    sp<cpp_hwcrypto::IHwCryptoOperations> binder_return;
    status = mHwCryptoServer->getHwCryptoOperations(&binder_return);
    if (status.isOk()) {
        std::shared_ptr<ndk_hwcrypto::IHwCryptoOperations> operations =
                HwCryptoOperationsNdk::Create(binder_return);
        *aidl_return = operations;
    }
    return convertStatus(status);
}

ndk::ScopedAStatus HwCryptoKey::importClearKey(
        const ndk_hwcrypto::types::ExplicitKeyMaterial& keyMaterial,
        const ndk_hwcrypto::KeyPolicy& newKeyPolicy,
        std::shared_ptr<ndk_hwcrypto::IOpaqueKey>* aidl_return) {
    sp<cpp_hwcrypto::IOpaqueKey> binder_return = nullptr;
    Status status = Status::fromExceptionCode(Status::EX_ILLEGAL_ARGUMENT);
    if (aidl_return == nullptr) {
        LOG(ERROR) << "return value passed to importClearKey is nullptr";
        return convertStatus(status);
    }
    auto cppKeyPolicy =
            convertKeyPolicy<cpp_hwcrypto::KeyPolicy, ndk_hwcrypto::KeyPolicy>(newKeyPolicy);
    auto explicitKeyCpp = convertExplicitKeyMaterial(keyMaterial);
    if (!explicitKeyCpp.has_value()) {
        LOG(ERROR) << "couldn't convert key material";
        return convertStatus(status);
    }
    status = mHwCryptoServer->importClearKey(explicitKeyCpp.value(), cppKeyPolicy, &binder_return);
    create_key_mapping_if_success(status, binder_return, aidl_return);
    return convertStatus(status);
}

ndk::ScopedAStatus HwCryptoKey::getCurrentDicePolicy(std::vector<uint8_t>* aidl_return) {
    auto status = mHwCryptoServer->getCurrentDicePolicy(aidl_return);
    return convertStatus(status);
}

ndk::ScopedAStatus HwCryptoKey::keyTokenImport(
        const ndk_hwcrypto::types::OpaqueKeyToken& requestedKey,
        const ::std::vector<uint8_t>& sealingDicePolicy,
        std::shared_ptr<ndk_hwcrypto::IOpaqueKey>* aidl_return) {
    Status status = Status::fromExceptionCode(Status::EX_ILLEGAL_ARGUMENT);
    if (aidl_return == nullptr) {
        LOG(ERROR) << "return value passed to keyTokenImport is nullptr";
        return convertStatus(status);
    }
    sp<cpp_hwcrypto::IOpaqueKey> binder_return;
    cpp_hwcrypto::types::OpaqueKeyToken requestedKeyCpp;
    // trying first a shallow copy of the vector
    requestedKeyCpp.keyToken = requestedKey.keyToken;
    status = mHwCryptoServer->keyTokenImport(requestedKeyCpp, sealingDicePolicy, &binder_return);
    create_key_mapping_if_success(status, binder_return, aidl_return);
    return convertStatus(status);
}

ndk::ScopedAStatus HwCryptoKey::getKeyslotData(
        ndk_hwcrypto::IHwCryptoKey::KeySlot /*slotId*/,
        std::shared_ptr<ndk_hwcrypto::IOpaqueKey>* /*aidl_return*/) {
    return ndk::ScopedAStatus::fromServiceSpecificError(
            ndk_hwcrypto::types::HalErrorCode::UNAUTHORIZED);
}

}  // namespace hwcryptohalservice
}  // namespace trusty
}  // namespace android
