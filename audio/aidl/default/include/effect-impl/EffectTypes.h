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

namespace aidl::android::hardware::audio::effect {

enum class RetCode {
    SUCCESS,
    ERROR_ILLEGAL_PARAMETER, /* Illegal parameter */
    ERROR_THREAD,            /* Effect thread error */
    ERROR_NULL_POINTER,      /* NULL pointer */
    ERROR_ALIGNMENT_ERROR,   /* Memory alignment error */
    ERROR_BLOCK_SIZE_EXCEED  /* Maximum block size exceeded */
};

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
    }

    return out << "EnumError: " << code;
}

}  // namespace aidl::android::hardware::audio::effect
