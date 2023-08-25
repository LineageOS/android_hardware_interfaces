/*
 * Copyright 2022 The Android Open Source Project
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

#include <aidl/android/hardware/tv/input/TvStreamConfig.h>
#include <aidlcommonsupport/NativeHandle.h>

using namespace std;

namespace aidl {
namespace android {
namespace hardware {
namespace tv {
namespace input {

class TvStreamConfigWrapper {
  public:
    TvStreamConfigWrapper() {}
    TvStreamConfigWrapper(int32_t streamId_, int32_t maxVideoWidth_, int32_t maxVideoHeight_,
                          bool isOpen_) {
        streamConfig.streamId = streamId_;
        streamConfig.maxVideoWidth = maxVideoWidth_;
        streamConfig.maxVideoHeight = maxVideoHeight_;
        isOpen = isOpen_;
        handle = nullptr;
    }

    TvStreamConfig streamConfig;
    bool isOpen;
    native_handle_t* handle;
};
}  // namespace input
}  // namespace tv
}  // namespace hardware
}  // namespace android
}  // namespace aidl
