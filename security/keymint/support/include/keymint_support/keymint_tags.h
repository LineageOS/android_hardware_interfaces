/*
 * Copyright (C) 2020 The Android Open Source Project
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

#pragma once

#include <aidl/android/hardware/security/keymint/Algorithm.h>
#include <aidl/android/hardware/security/keymint/BlockMode.h>
#include <aidl/android/hardware/security/keymint/Digest.h>
#include <aidl/android/hardware/security/keymint/EcCurve.h>
#include <aidl/android/hardware/security/keymint/HardwareAuthenticatorType.h>
#include <aidl/android/hardware/security/keymint/KeyOrigin.h>
#include <aidl/android/hardware/security/keymint/KeyParameter.h>
#include <aidl/android/hardware/security/keymint/KeyPurpose.h>
#include <aidl/android/hardware/security/keymint/PaddingMode.h>
#include <aidl/android/hardware/security/keymint/SecurityLevel.h>
#include <aidl/android/hardware/security/keymint/Tag.h>
#include <aidl/android/hardware/security/keymint/TagType.h>

namespace aidl::android::hardware::security::keymint {

constexpr TagType typeFromTag(Tag tag) {
    return static_cast<TagType>(static_cast<uint32_t>(tag) & static_cast<uint32_t>(0xf0000000));
}

/**
 * TypedTag is a templatized version of Tag, which provides compile-time checking of KeyMint tag
 * types.  Instances are convertible to Tag, so they can be used wherever Tag is expected, and
 * because they encode the tag type it's possible to create function overloads that only operate on
 * tags with a particular type.
 */
template <TagType tag_type, Tag tag>
struct TypedTag {
    inline TypedTag() {
        // Ensure that it's impossible to create a TypedTag instance whose 'tag' doesn't have type
        // 'tag_type'.  Attempting to instantiate a tag with the wrong type will result in a compile
        // error (no match for template specialization StaticAssert<false>), with no run-time cost.
        static_assert(typeFromTag(tag) == tag_type, "mismatch between tag and tag_type");
    }
    operator Tag() const { return tag; }
    int32_t maskedTag() { return static_cast<uint32_t>(tag) & 0x0FFFFFFF; }
};

template <Tag tag>
struct Tag2TypedTag {
    typedef TypedTag<typeFromTag(tag), tag> type;
};

#ifdef DECLARE_TYPED_TAG
#undef DECLARE_TYPED_TAG
#endif

#define DECLARE_TYPED_TAG(name)                                    \
    typedef typename Tag2TypedTag<Tag::name>::type TAG_##name##_t; \
    static TAG_##name##_t TAG_##name;

DECLARE_TYPED_TAG(ACTIVE_DATETIME);
DECLARE_TYPED_TAG(ALGORITHM);
DECLARE_TYPED_TAG(ALLOW_WHILE_ON_BODY);
DECLARE_TYPED_TAG(APPLICATION_DATA);
DECLARE_TYPED_TAG(APPLICATION_ID);
DECLARE_TYPED_TAG(ASSOCIATED_DATA);
DECLARE_TYPED_TAG(ATTESTATION_APPLICATION_ID);
DECLARE_TYPED_TAG(ATTESTATION_CHALLENGE);
DECLARE_TYPED_TAG(ATTESTATION_ID_BRAND);
DECLARE_TYPED_TAG(ATTESTATION_ID_DEVICE);
DECLARE_TYPED_TAG(ATTESTATION_ID_IMEI);
DECLARE_TYPED_TAG(ATTESTATION_ID_MANUFACTURER);
DECLARE_TYPED_TAG(ATTESTATION_ID_MEID);
DECLARE_TYPED_TAG(ATTESTATION_ID_PRODUCT);
DECLARE_TYPED_TAG(ATTESTATION_ID_MODEL);
DECLARE_TYPED_TAG(ATTESTATION_ID_SERIAL);
DECLARE_TYPED_TAG(AUTH_TIMEOUT);
DECLARE_TYPED_TAG(BLOCK_MODE);
DECLARE_TYPED_TAG(BOOTLOADER_ONLY);
DECLARE_TYPED_TAG(BOOT_PATCHLEVEL);
DECLARE_TYPED_TAG(CALLER_NONCE);
DECLARE_TYPED_TAG(CONFIRMATION_TOKEN);
DECLARE_TYPED_TAG(CREATION_DATETIME);
DECLARE_TYPED_TAG(DEVICE_UNIQUE_ATTESTATION);
DECLARE_TYPED_TAG(DIGEST);
DECLARE_TYPED_TAG(EARLY_BOOT_ONLY);
DECLARE_TYPED_TAG(EC_CURVE);
DECLARE_TYPED_TAG(HARDWARE_TYPE);
DECLARE_TYPED_TAG(IDENTITY_CREDENTIAL_KEY);
DECLARE_TYPED_TAG(INCLUDE_UNIQUE_ID);
DECLARE_TYPED_TAG(INVALID);
DECLARE_TYPED_TAG(KEY_SIZE);
DECLARE_TYPED_TAG(MAC_LENGTH);
DECLARE_TYPED_TAG(MAX_USES_PER_BOOT);
DECLARE_TYPED_TAG(MIN_MAC_LENGTH);
DECLARE_TYPED_TAG(MIN_SECONDS_BETWEEN_OPS);
DECLARE_TYPED_TAG(NONCE);
DECLARE_TYPED_TAG(NO_AUTH_REQUIRED);
DECLARE_TYPED_TAG(ORIGIN);
DECLARE_TYPED_TAG(ORIGINATION_EXPIRE_DATETIME);
DECLARE_TYPED_TAG(OS_PATCHLEVEL);
DECLARE_TYPED_TAG(OS_VERSION);
DECLARE_TYPED_TAG(PADDING);
DECLARE_TYPED_TAG(PURPOSE);
DECLARE_TYPED_TAG(RESET_SINCE_ID_ROTATION);
DECLARE_TYPED_TAG(ROLLBACK_RESISTANCE);
DECLARE_TYPED_TAG(ROOT_OF_TRUST);
DECLARE_TYPED_TAG(RSA_PUBLIC_EXPONENT);
DECLARE_TYPED_TAG(STORAGE_KEY);
DECLARE_TYPED_TAG(TRUSTED_CONFIRMATION_REQUIRED);
DECLARE_TYPED_TAG(TRUSTED_USER_PRESENCE_REQUIRED);
DECLARE_TYPED_TAG(UNIQUE_ID);
DECLARE_TYPED_TAG(UNLOCKED_DEVICE_REQUIRED);
DECLARE_TYPED_TAG(USAGE_COUNT_LIMIT);
DECLARE_TYPED_TAG(USAGE_EXPIRE_DATETIME);
DECLARE_TYPED_TAG(USER_AUTH_TYPE);
DECLARE_TYPED_TAG(USER_ID);
DECLARE_TYPED_TAG(USER_SECURE_ID);
DECLARE_TYPED_TAG(VENDOR_PATCHLEVEL);
DECLARE_TYPED_TAG(RSA_OAEP_MGF_DIGEST);
DECLARE_TYPED_TAG(CERTIFICATE_SERIAL);
DECLARE_TYPED_TAG(CERTIFICATE_SUBJECT);
DECLARE_TYPED_TAG(CERTIFICATE_NOT_BEFORE);
DECLARE_TYPED_TAG(CERTIFICATE_NOT_AFTER);
DECLARE_TYPED_TAG(MAX_BOOT_LEVEL);

#undef DECLARE_TYPED_TAG

template <typename... Elems>
struct MetaList {};

using all_tags_t = MetaList<
        TAG_INVALID_t, TAG_KEY_SIZE_t, TAG_MAC_LENGTH_t, TAG_CALLER_NONCE_t, TAG_MIN_MAC_LENGTH_t,
        TAG_RSA_PUBLIC_EXPONENT_t, TAG_INCLUDE_UNIQUE_ID_t, TAG_ACTIVE_DATETIME_t,
        TAG_ORIGINATION_EXPIRE_DATETIME_t, TAG_USAGE_EXPIRE_DATETIME_t,
        TAG_MIN_SECONDS_BETWEEN_OPS_t, TAG_MAX_USES_PER_BOOT_t, TAG_USAGE_COUNT_LIMIT_t,
        TAG_USER_ID_t, TAG_USER_SECURE_ID_t, TAG_NO_AUTH_REQUIRED_t, TAG_AUTH_TIMEOUT_t,
        TAG_ALLOW_WHILE_ON_BODY_t, TAG_UNLOCKED_DEVICE_REQUIRED_t, TAG_APPLICATION_ID_t,
        TAG_APPLICATION_DATA_t, TAG_CREATION_DATETIME_t, TAG_ROLLBACK_RESISTANCE_t,
        TAG_HARDWARE_TYPE_t, TAG_ROOT_OF_TRUST_t, TAG_ASSOCIATED_DATA_t, TAG_NONCE_t,
        TAG_BOOTLOADER_ONLY_t, TAG_OS_VERSION_t, TAG_OS_PATCHLEVEL_t, TAG_UNIQUE_ID_t,
        TAG_ATTESTATION_CHALLENGE_t, TAG_ATTESTATION_APPLICATION_ID_t, TAG_ATTESTATION_ID_BRAND_t,
        TAG_ATTESTATION_ID_DEVICE_t, TAG_ATTESTATION_ID_PRODUCT_t,
        TAG_ATTESTATION_ID_MANUFACTURER_t, TAG_ATTESTATION_ID_MODEL_t,
        TAG_RESET_SINCE_ID_ROTATION_t, TAG_PURPOSE_t, TAG_ALGORITHM_t, TAG_BLOCK_MODE_t,
        TAG_DIGEST_t, TAG_PADDING_t, TAG_ORIGIN_t, TAG_USER_AUTH_TYPE_t, TAG_EC_CURVE_t,
        TAG_BOOT_PATCHLEVEL_t, TAG_VENDOR_PATCHLEVEL_t, TAG_TRUSTED_CONFIRMATION_REQUIRED_t,
        TAG_TRUSTED_USER_PRESENCE_REQUIRED_t, TAG_CERTIFICATE_SERIAL_t, TAG_CERTIFICATE_SUBJECT_t>;

template <typename TypedTagType>
struct TypedTag2ValueType;

#ifdef MAKE_TAG_VALUE_ACCESSOR
#undef MAKE_TAG_VALUE_ACCESSOR
#endif

#define MAKE_TAG_VALUE_ACCESSOR(tag_type, field_name)                                           \
    template <Tag tag>                                                                          \
    struct TypedTag2ValueType<TypedTag<tag_type, tag>> {                                        \
        using type = std::remove_reference<                                                     \
                decltype(static_cast<KeyParameterValue*>(nullptr)                               \
                                 ->get<KeyParameterValue::field_name>())>::type;                \
        static constexpr KeyParameterValue::Tag unionTag = KeyParameterValue::field_name;       \
    };                                                                                          \
    template <Tag tag>                                                                          \
    inline std::optional<std::reference_wrapper<                                                \
            const typename TypedTag2ValueType<TypedTag<tag_type, tag>>::type>>                  \
    accessTagValue(TypedTag<tag_type, tag>, const KeyParameter& param) {                        \
        if (param.value.getTag() == KeyParameterValue::field_name) {                            \
            return std::optional(                                                               \
                    std::reference_wrapper(param.value.get<KeyParameterValue::field_name>()));  \
        } else {                                                                                \
            return std::nullopt;                                                                \
        }                                                                                       \
    }                                                                                           \
    template <Tag tag>                                                                          \
    inline std::optional<                                                                       \
            std::reference_wrapper<typename TypedTag2ValueType<TypedTag<tag_type, tag>>::type>> \
    accessTagValue(TypedTag<tag_type, tag>, KeyParameter& param) {                              \
        if (param.value.getTag() == KeyParameterValue::field_name) {                            \
            return std::optional(                                                               \
                    std::reference_wrapper(param.value.get<KeyParameterValue::field_name>()));  \
        } else {                                                                                \
            return std::nullopt;                                                                \
        }                                                                                       \
    }

MAKE_TAG_VALUE_ACCESSOR(TagType::ULONG, longInteger)
MAKE_TAG_VALUE_ACCESSOR(TagType::ULONG_REP, longInteger)
MAKE_TAG_VALUE_ACCESSOR(TagType::DATE, dateTime)
MAKE_TAG_VALUE_ACCESSOR(TagType::UINT, integer)
MAKE_TAG_VALUE_ACCESSOR(TagType::UINT_REP, integer)
MAKE_TAG_VALUE_ACCESSOR(TagType::BOOL, boolValue)
MAKE_TAG_VALUE_ACCESSOR(TagType::BYTES, blob)
MAKE_TAG_VALUE_ACCESSOR(TagType::BIGNUM, blob)

#undef MAKE_TAG_VALUE_ACCESSOR

#ifdef MAKE_TAG_ENUM_VALUE_ACCESSOR
#undef MAKE_TAG_ENUM_VALUE_ACCESSOR
#endif

#define MAKE_TAG_ENUM_VALUE_ACCESSOR(typed_tag, field_name)                                       \
    template <>                                                                                   \
    struct TypedTag2ValueType<decltype(typed_tag)> {                                              \
        using type = std::remove_reference<                                                       \
                decltype(static_cast<KeyParameterValue*>(nullptr)                                 \
                                 ->get<KeyParameterValue::field_name>())>::type;                  \
        static constexpr KeyParameterValue::Tag unionTag = KeyParameterValue::field_name;         \
    };                                                                                            \
    inline std::optional<                                                                         \
            std::reference_wrapper<const typename TypedTag2ValueType<decltype(typed_tag)>::type>> \
    accessTagValue(decltype(typed_tag), const KeyParameter& param) {                              \
        if (param.value.getTag() == KeyParameterValue::field_name) {                              \
            return std::optional(                                                                 \
                    std::reference_wrapper(param.value.get<KeyParameterValue::field_name>()));    \
        } else {                                                                                  \
            return std::nullopt;                                                                  \
        }                                                                                         \
    }                                                                                             \
    inline std::optional<                                                                         \
            std::reference_wrapper<typename TypedTag2ValueType<decltype(typed_tag)>::type>>       \
    accessTagValue(decltype(typed_tag), KeyParameter& param) {                                    \
        if (param.value.getTag() == KeyParameterValue::field_name) {                              \
            return std::optional(                                                                 \
                    std::reference_wrapper(param.value.get<KeyParameterValue::field_name>()));    \
        } else {                                                                                  \
            return std::nullopt;                                                                  \
        }                                                                                         \
    }

MAKE_TAG_ENUM_VALUE_ACCESSOR(TAG_ALGORITHM, algorithm)
MAKE_TAG_ENUM_VALUE_ACCESSOR(TAG_BLOCK_MODE, blockMode)
MAKE_TAG_ENUM_VALUE_ACCESSOR(TAG_DIGEST, digest)
MAKE_TAG_ENUM_VALUE_ACCESSOR(TAG_EC_CURVE, ecCurve)
MAKE_TAG_ENUM_VALUE_ACCESSOR(TAG_ORIGIN, origin)
MAKE_TAG_ENUM_VALUE_ACCESSOR(TAG_PADDING, paddingMode)
MAKE_TAG_ENUM_VALUE_ACCESSOR(TAG_PURPOSE, keyPurpose)
MAKE_TAG_ENUM_VALUE_ACCESSOR(TAG_USER_AUTH_TYPE, hardwareAuthenticatorType)
MAKE_TAG_ENUM_VALUE_ACCESSOR(TAG_HARDWARE_TYPE, securityLevel)
MAKE_TAG_ENUM_VALUE_ACCESSOR(TAG_RSA_OAEP_MGF_DIGEST, digest)

#undef MAKE_TAG_ENUM_VALUE_ACCESSOR

template <TagType tag_type, Tag tag, typename ValueT>
inline KeyParameter makeKeyParameter(TypedTag<tag_type, tag> ttag, ValueT&& value) {
    KeyParameter retval;
    retval.tag = tag;
    retval.value = KeyParameterValue::make<TypedTag2ValueType<decltype(ttag)>::unionTag>(
            std::forward<ValueT>(value));
    return retval;
}

// the boolean case
template <Tag tag>
inline KeyParameter makeKeyParameter(TypedTag<TagType::BOOL, tag>) {
    KeyParameter retval;
    retval.tag = tag;
    retval.value = KeyParameterValue::make<KeyParameterValue::boolValue>(true);
    return retval;
}

// the invalid case
inline KeyParameter makeKeyParameter(TypedTag<TagType::INVALID, Tag::INVALID>) {
    KeyParameter retval;
    retval.tag = Tag::INVALID;
    retval.value = KeyParameterValue::make<KeyParameterValue::invalid>(0);
    return retval;
}

template <typename... Pack>
struct FirstOrNoneHelper;
template <typename First>
struct FirstOrNoneHelper<First> {
    typedef First type;
};
template <>
struct FirstOrNoneHelper<> {
    struct type {};
};

template <typename... Pack>
using FirstOrNone = typename FirstOrNoneHelper<Pack...>::type;

template <TagType tag_type, Tag tag, typename... Args>
inline KeyParameter Authorization(TypedTag<tag_type, tag> ttag, Args&&... args) {
    static_assert(tag_type != TagType::BOOL || (sizeof...(args) == 0),
                  "TagType::BOOL Authorizations do not take parameters. Presence is truth.");
    static_assert(tag_type == TagType::BOOL || (sizeof...(args) == 1),
                  "Authorization other then TagType::BOOL take exactly one parameter.");
    static_assert(
            tag_type == TagType::BOOL ||
                    std::is_convertible<
                            std::remove_cv_t<std::remove_reference_t<FirstOrNone<Args...>>>,
                            typename TypedTag2ValueType<TypedTag<tag_type, tag>>::type>::value,
            "Invalid argument type for given tag.");

    return makeKeyParameter(ttag, std::forward<Args>(args)...);
}

template <typename T>
std::remove_reference_t<T> NullOrOr(T&& v) {
    if (v) return v;
    return {};
}

template <typename Head, typename... Tail>
std::remove_reference_t<Head> NullOrOr(Head&& head, Tail&&... tail) {
    if (head) return head;
    return NullOrOr(std::forward<Tail>(tail)...);
}

template <typename Default, typename Wrapped>
std::remove_reference_t<Wrapped> defaultOr(std::optional<Wrapped>&& optional, Default&& def) {
    static_assert(std::is_convertible<std::remove_reference_t<Default>,
                                      std::remove_reference_t<Wrapped>>::value,
                  "Type of default value must match the type wrapped by std::optional");
    if (optional) return *optional;
    return def;
}

template <TagType tag_type, Tag tag>
inline std::optional<
        std::reference_wrapper<const typename TypedTag2ValueType<TypedTag<tag_type, tag>>::type>>
authorizationValue(TypedTag<tag_type, tag> ttag, const KeyParameter& param) {
    // We only check if the parameter has the correct tag here; accessTagValue checks if the correct
    // union field was initialized.
    if (tag != param.tag) return {};
    return accessTagValue(ttag, param);
}

}  // namespace aidl::android::hardware::security::keymint
