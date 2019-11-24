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

#ifndef HARDWARE_INTERFACES_KEYMASTER_V4_1_SUPPORT_INCLUDE_AUTHORIZATION_SET_H_
#define HARDWARE_INTERFACES_KEYMASTER_V4_1_SUPPORT_INCLUDE_AUTHORIZATION_SET_H_

#include <keymasterV4_0/authorization_set.h>

#include <keymasterV4_1/keymaster_tags.h>

namespace android::hardware::keymaster::V4_1 {

using V4_0::AuthorizationSet;
using V4_0::AuthorizationSetBuilder;
using V4_0::KeyParameter;

}  // namespace android::hardware::keymaster::V4_1

#endif  // HARDWARE_INTERFACES_KEYMASTER_V4_1_SUPPORT_INCLUDE_AUTHORIZATION_SET_H_
