/*
 * Copyright (C) 2018 The Android Open Source Project
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

#ifndef ANDROID_HARDWARE_VERSION_MACRO_H
#define ANDROID_HARDWARE_VERSION_MACRO_H

#if !defined(MAJOR_VERSION) || !defined(MINOR_VERSION)
#error "MAJOR_VERSION and MINOR_VERSION must be defined"
#endif

/** Allows macro expansion for x and add surrounding `<>`.
 * Is intended to be used for version dependant includes as
 * `#include` do not macro expand if starting with < or "
 * Example usage:
 *      #include PATH(path/to/FILE_VERSION/file)
 * @note: uses the implementation-define "Computed Includes" feature.
 */
#define PATH(x) <x>

#define CONCAT_3(a, b, c) a##b##c
#define EXPAND_CONCAT_3(a, b, c) CONCAT_3(a, b, c)
/** The directory name of the version: <major>.<minor> */
#define FILE_VERSION EXPAND_CONCAT_3(MAJOR_VERSION, ., MINOR_VERSION)

#define CONCAT_4(a, b, c, d) a##b##c##d
#define EXPAND_CONCAT_4(a, b, c, d) CONCAT_4(a, b, c, d)
/** The c++ namespace of the version: V<major>_<minor> */
#define CPP_VERSION EXPAND_CONCAT_4(V, MAJOR_VERSION, _, MINOR_VERSION)

#endif  // ANDROID_HARDWARE_VERSION_MACRO_H
