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

#include <cstdlib>

namespace aidl::android::hardware::audio::effect {

// Visualizer implementation UUID.
static const ::aidl::android::media::audio::common::AudioUuid VisualizerUUID = {
        static_cast<int32_t>(0x1d4033c0),
        0x8557,
        0x11df,
        0x9f2d,
        {0x00, 0x02, 0xa5, 0xd5, 0xc5, 0x1b}};

}  // namespace aidl::android::hardware::audio::effect