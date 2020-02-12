/*
 ** Copyright 2020, The Android Open Source Project
 **
 ** Licensed under the Apache License, Version 2.0 (the "License");
 ** you may not use this file except in compliance with the License.
 ** You may obtain a copy of the License at
 **
 **     http://www.apache.org/licenses/LICENSE-2.0
 **
 ** Unless required by applicable law or agreed to in writing, software
 ** distributed under the License is distributed on an "AS IS" BASIS,
 ** WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 ** See the License for the specific language governing permissions and
 ** limitations under the License.
 */

#pragma once

#include <android/hardware/keymaster/4.1/IOperation.h>

#include <keymasterV4_1/keymaster_tags.h>

namespace android::hardware::keymaster::V4_1::support {

class Operation : public IOperation {
  public:
    Operation(OperationHandle handle) : handle_(handle) {}

    Return<void> getOperationChallenge(getOperationChallenge_cb _hidl_cb) override {
        _hidl_cb(V4_1::ErrorCode::OK, handle_);
        return Void();
    }

  private:
    OperationHandle handle_;
};

}  // namespace android::hardware::keymaster::V4_1::support
