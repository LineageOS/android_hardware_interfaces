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
#pragma once

#include <android/hardware/health/2.1/IHealth.h>
#include <android/hardware/health/2.1/IHealthInfoCallback.h>

using ::android::sp;
using ::android::hardware::Return;
using ::android::hidl::base::V1_0::IBase;

namespace android {
namespace hardware {
namespace health {
namespace V2_1 {
namespace implementation {

// Wraps an IHealthInfoCallback.
class Callback {
  public:
    virtual ~Callback() {}
    virtual Return<void> Notify(const HealthInfo&) = 0;
    virtual sp<IBase> Get() = 0;
};

class Callback_2_0 : public Callback {
  public:
    Callback_2_0(const sp<V2_0::IHealthInfoCallback>& callback) : callback_(callback) {}
    Return<void> Notify(const HealthInfo& info) override {
        return callback_->healthInfoChanged(info.legacy);
    }
    sp<IBase> Get() override { return callback_; }

  private:
    sp<V2_0::IHealthInfoCallback> callback_;
};

class Callback_2_1 : public Callback {
  public:
    Callback_2_1(const sp<IHealthInfoCallback>& callback) : callback_(callback) {}
    Return<void> Notify(const HealthInfo& info) override {
        return callback_->healthInfoChanged_2_1(info);
    }
    sp<IBase> Get() override { return callback_; }

  private:
    sp<IHealthInfoCallback> callback_;
};

inline std::unique_ptr<Callback> Wrap(const sp<V2_0::IHealthInfoCallback>& callback_2_0) {
    auto callback_2_1 = IHealthInfoCallback::castFrom(callback_2_0).withDefault(nullptr);
    if (callback_2_1) return std::make_unique<Callback_2_1>(callback_2_1);
    return std::make_unique<Callback_2_0>(callback_2_0);
}

}  // namespace implementation
}  // namespace V2_1
}  // namespace health
}  // namespace hardware
}  // namespace android
