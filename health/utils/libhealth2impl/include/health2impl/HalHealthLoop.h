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

#include <optional>

#include <android/hardware/health/2.1/IHealth.h>
#include <health/HealthLoop.h>

namespace android {
namespace hardware {
namespace health {
namespace V2_1 {
namespace implementation {

// An implementation of HealthLoop for using a given health HAL. This is useful
// for services that opens the passthrough implementation and starts the HealthLoop
// to periodically poll data from the implementation.
class HalHealthLoop : public HealthLoop {
  public:
    HalHealthLoop(const std::string& name, const sp<IHealth>& service)
        : instance_name_(name), service_(service) {}

  protected:
    virtual void Init(struct healthd_config* config) override;
    virtual void Heartbeat() override;
    virtual int PrepareToWait() override;
    virtual void ScheduleBatteryUpdate() override;

    // HealthLoop periodically calls ScheduleBatteryUpdate, which calls
    // OnHealthInfoChanged callback. A client can override this function to
    // broadcast the health_info to interested listeners. By default, this
    // adjust uevents / wakealarm periods.
    virtual void OnHealthInfoChanged(const HealthInfo& health_info);

    const std::string& instance_name() const { return instance_name_; }
    const sp<IHealth>& service() const { return service_; }
    bool charger_online() const { return charger_online_; }

    // Helpers for subclasses to implement OnHealthInfoChanged.
    void set_charger_online(const HealthInfo& health_info);

  private:
    std::string instance_name_;
    sp<IHealth> service_;
    bool charger_online_ = false;
};

}  // namespace implementation
}  // namespace V2_1
}  // namespace health
}  // namespace hardware
}  // namespace android
