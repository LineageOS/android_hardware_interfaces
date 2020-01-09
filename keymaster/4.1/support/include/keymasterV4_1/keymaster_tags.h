/*
 * Copyright (C) 2019 The Android Open Source Project
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

#ifndef HARDWARE_INTERFACES_KEYMASTER_V4_1_SUPPORT_INCLUDE_KEYMASTER_TAGS_H_
#define HARDWARE_INTERFACES_KEYMASTER_V4_1_SUPPORT_INCLUDE_KEYMASTER_TAGS_H_

#include <android/hardware/keymaster/4.1/types.h>

#include <keymasterV4_0/keymaster_tags.h>

namespace android::hardware::keymaster::V4_1 {

using V4_0::Algorithm;
using V4_0::BlockMode;
using V4_0::Digest;
using V4_0::EcCurve;
using V4_0::HardwareAuthenticatorType;
using V4_0::HardwareAuthToken;
using V4_0::HmacSharingParameters;
using V4_0::KeyBlobUsageRequirements;
using V4_0::KeyCharacteristics;
using V4_0::KeyFormat;
using V4_0::KeyOrigin;
using V4_0::KeyParameter;
using V4_0::KeyPurpose;
using V4_0::OperationHandle;
using V4_0::PaddingMode;
using V4_0::SecurityLevel;
using V4_0::TagType;
using V4_0::VerificationToken;

using V4_0::NullOr;
using V4_0::TypedTag;

using V4_0::TAG_ACTIVE_DATETIME;
using V4_0::TAG_ALGORITHM;
using V4_0::TAG_ALLOW_WHILE_ON_BODY;
using V4_0::TAG_APPLICATION_DATA;
using V4_0::TAG_APPLICATION_ID;
using V4_0::TAG_ASSOCIATED_DATA;
using V4_0::TAG_ATTESTATION_APPLICATION_ID;
using V4_0::TAG_ATTESTATION_CHALLENGE;
using V4_0::TAG_AUTH_TIMEOUT;
using V4_0::TAG_BLOB_USAGE_REQUIREMENTS;
using V4_0::TAG_BLOCK_MODE;
using V4_0::TAG_BOOT_PATCHLEVEL;
using V4_0::TAG_BOOTLOADER_ONLY;
using V4_0::TAG_CALLER_NONCE;
using V4_0::TAG_CONFIRMATION_TOKEN;
using V4_0::TAG_CREATION_DATETIME;
using V4_0::TAG_DIGEST;
using V4_0::TAG_EC_CURVE;
using V4_0::TAG_HARDWARE_TYPE;
using V4_0::TAG_INCLUDE_UNIQUE_ID;
using V4_0::TAG_INVALID;
using V4_0::TAG_KEY_SIZE;
using V4_0::TAG_MAC_LENGTH;
using V4_0::TAG_MAX_USES_PER_BOOT;
using V4_0::TAG_MIN_MAC_LENGTH;
using V4_0::TAG_MIN_SECONDS_BETWEEN_OPS;
using V4_0::TAG_NO_AUTH_REQUIRED;
using V4_0::TAG_NONCE;
using V4_0::TAG_ORIGIN;
using V4_0::TAG_ORIGINATION_EXPIRE_DATETIME;
using V4_0::TAG_OS_PATCHLEVEL;
using V4_0::TAG_OS_VERSION;
using V4_0::TAG_PADDING;
using V4_0::TAG_PURPOSE;
using V4_0::TAG_RESET_SINCE_ID_ROTATION;
using V4_0::TAG_ROLLBACK_RESISTANCE;
using V4_0::TAG_ROOT_OF_TRUST;
using V4_0::TAG_RSA_PUBLIC_EXPONENT;
using V4_0::TAG_TRUSTED_CONFIRMATION_REQUIRED;
using V4_0::TAG_TRUSTED_USER_PRESENCE_REQUIRED;
using V4_0::TAG_UNIQUE_ID;
using V4_0::TAG_UNLOCKED_DEVICE_REQUIRED;
using V4_0::TAG_USAGE_EXPIRE_DATETIME;
using V4_0::TAG_USER_AUTH_TYPE;
using V4_0::TAG_USER_ID;
using V4_0::TAG_USER_SECURE_ID;
using V4_0::TAG_VENDOR_PATCHLEVEL;

#define DECLARE_KM_4_1_TYPED_TAG(name)                                                   \
    typedef typename V4_0::Tag2TypedTag<(static_cast<V4_0::Tag>(V4_1::Tag::name))>::type \
            TAG_##name##_t;                                                              \
    static TAG_##name##_t TAG_##name;

DECLARE_KM_4_1_TYPED_TAG(EARLY_BOOT_ONLY);
DECLARE_KM_4_1_TYPED_TAG(DEVICE_UNIQUE_ATTESTATION);
DECLARE_KM_4_1_TYPED_TAG(STORAGE_KEY);
DECLARE_KM_4_1_TYPED_TAG(IDENTITY_CREDENTIAL_KEY);

}  // namespace android::hardware::keymaster::V4_1

#endif  // HARDWARE_INTERFACES_KEYMASTER_V4_1_SUPPORT_INCLUDE_KEYMASTER_TAGS_H_
