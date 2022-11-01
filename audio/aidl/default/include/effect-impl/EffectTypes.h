/*
 * Copyright (C) 2022 The Android Open Source Project
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
#include <ostream>
#include <string>

#include <aidl/android/hardware/audio/effect/BnEffect.h>

typedef binder_exception_t (*EffectCreateFunctor)(
        const ::aidl::android::media::audio::common::AudioUuid*,
        std::shared_ptr<aidl::android::hardware::audio::effect::IEffect>*);
typedef binder_exception_t (*EffectDestroyFunctor)(
        const std::shared_ptr<aidl::android::hardware::audio::effect::IEffect>&);

struct effect_dl_interface_s {
    EffectCreateFunctor createEffectFunc;
    EffectDestroyFunctor destroyEffectFunc;
};

namespace aidl::android::hardware::audio::effect {

enum class RetCode {
    SUCCESS,
    ERROR_ILLEGAL_PARAMETER, /* Illegal parameter */
    ERROR_THREAD,            /* Effect thread error */
    ERROR_NULL_POINTER,      /* NULL pointer */
    ERROR_ALIGNMENT_ERROR,   /* Memory alignment error */
    ERROR_BLOCK_SIZE_EXCEED, /* Maximum block size exceeded */
    ERROR_EFFECT_LIB_ERROR
};

static const int INVALID_AUDIO_SESSION_ID = -1;

inline std::ostream& operator<<(std::ostream& out, const RetCode& code) {
    switch (code) {
        case RetCode::SUCCESS:
            return out << "SUCCESS";
        case RetCode::ERROR_ILLEGAL_PARAMETER:
            return out << "ERROR_ILLEGAL_PARAMETER";
        case RetCode::ERROR_THREAD:
            return out << "ERROR_THREAD";
        case RetCode::ERROR_NULL_POINTER:
            return out << "ERROR_NULL_POINTER";
        case RetCode::ERROR_ALIGNMENT_ERROR:
            return out << "ERROR_ALIGNMENT_ERROR";
        case RetCode::ERROR_BLOCK_SIZE_EXCEED:
            return out << "ERROR_BLOCK_SIZE_EXCEED";
        case RetCode::ERROR_EFFECT_LIB_ERROR:
            return out << "ERROR_EFFECT_LIB_ERROR";
    }

    return out << "EnumError: " << code;
}

#define RETURN_IF_ASTATUS_NOT_OK(status, message)                                              \
    do {                                                                                       \
        const ::ndk::ScopedAStatus curr_status = (status);                                     \
        if (!curr_status.isOk()) {                                                             \
            LOG(ERROR) << __func__ << ":" << __LINE__                                          \
                       << "return with status: " << curr_status.getDescription() << (message); \
            return ndk::ScopedAStatus::fromExceptionCodeWithMessage(                           \
                    curr_status.getExceptionCode(), (message));                                \
        }                                                                                      \
    } while (0)

#define RETURN_IF(expr, exception, message)                                                  \
    do {                                                                                     \
        if (expr) {                                                                          \
            LOG(ERROR) << __func__ << ":" << __LINE__ << " return with expr " << #expr;      \
            return ndk::ScopedAStatus::fromExceptionCodeWithMessage((exception), (message)); \
        }                                                                                    \
    } while (0)

#define RETURN_OK_IF(expr)                                                             \
    do {                                                                               \
        if (expr) {                                                                    \
            LOG(INFO) << __func__ << ":" << __LINE__ << " return with expr " << #expr; \
            return ndk::ScopedAStatus::ok();                                           \
        }                                                                              \
    } while (0)

#define RETURN_VALUE_IF(expr, ret, log)                                                          \
    do {                                                                                         \
        if (expr) {                                                                              \
            LOG(ERROR) << __func__ << ":" << __LINE__ << " return with expr " << #expr << (log); \
            return ret;                                                                          \
        }                                                                                        \
    } while (0)

}  // namespace aidl::android::hardware::audio::effect
