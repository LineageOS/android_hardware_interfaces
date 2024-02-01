/*
 * Copyright (C) 2023 The Android Open Source Project
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

//! Test helper functions.

pub mod dice_sample;

// Constants for DICE map keys.

/// Map key for authority hash.
pub const AUTHORITY_HASH: i64 = -4670549;
/// Map key for config descriptor.
pub const CONFIG_DESC: i64 = -4670548;
/// Map key for component name.
pub const COMPONENT_NAME: i64 = -70002;
/// Map key for component version.
pub const COMPONENT_VERSION: i64 = -70003;
/// Map key for Resettable.
pub const COMPONENT_RESETTABLE: i64 = -70004;
/// Map key for security version.
pub const SECURITY_VERSION: i64 = -70005;
/// Map key for mode.
pub const MODE: i64 = -4670551;
/// Map key for SubcomponentDescriptor.
pub const SUBCOMPONENT_DESCRIPTORS: i64 = -71002;
/// Map key for name of subcomponent.
pub const SUBCOMPONENT_NAME: i64 = 1;
/// Map key for Security Version of subcomponent.
pub const SUBCOMPONENT_SECURITY_VERSION: i64 = 2;
/// Map key for Code hash of subcomponent.
pub const SUBCOMPONENT_CODE_HASH: i64 = 3;
/// Map key for Authority Hash of subcomponent.
pub const SUBCOMPONENT_AUTHORITY_HASH: i64 = 4;
