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

#pragma once

#include <android/hardware/health/storage/1.0/types.h>
#include <string>

namespace android::hardware::health::storage {

// Run debug on fd
void DebugDump(int fd);

// Run garbage collection on GetGarbageCollectPath(). Blocks until garbage
// collect finishes or |timeout_seconds| has reached.
V1_0::Result GarbageCollect(uint64_t timeout_seconds);

}  // namespace android::hardware::health::storage
