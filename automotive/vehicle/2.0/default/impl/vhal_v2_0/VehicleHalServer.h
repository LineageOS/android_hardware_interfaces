/*
 * Copyright (C) 2020 The Android Open Source Project
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

#include <vhal_v2_0/VehicleObjectPool.h>
#include <vhal_v2_0/VehicleServer.h>

#include "GeneratorHub.h"

namespace android::hardware::automotive::vehicle::V2_0::impl {

// This contains the common server operations that will be used by
// both native and virtualized VHAL server. Notice that in the virtualized
// scenario, the server may be run on a different OS than Android.
class VehicleHalServer : public IVehicleServer {
  public:
    // Methods from IVehicleServer

    std::vector<VehiclePropConfig> onGetAllPropertyConfig() const override;

    StatusCode onSetProperty(const VehiclePropValue& value, bool updateStatus) override;

    // Set the Property Value Pool used in this server
    void setValuePool(VehiclePropValuePool* valuePool);

  private:
    using VehiclePropValuePtr = recyclable_ptr<VehiclePropValue>;

    GeneratorHub* getGenerator();

    VehiclePropValuePool* getValuePool() const;

    void onFakeValueGenerated(const VehiclePropValue& value);

    StatusCode handleGenerateFakeDataRequest(const VehiclePropValue& request);

    VehiclePropValuePtr createApPowerStateReq(VehicleApPowerStateReq req, int32_t param);

    VehiclePropValuePtr createHwInputKeyProp(VehicleHwKeyInputAction action, int32_t keyCode,
                                             int32_t targetDisplay);

    StatusCode onSetInitialUserInfoResponse(const VehiclePropValue& value, bool updateStatus);
    StatusCode onSetSwitchUserResponse(const VehiclePropValue& value, bool updateStatus);

    // data members

  protected:
    // TODO(b/146207078): it might be clearer to move members below to an EmulatedUserHal class
    std::unique_ptr<VehiclePropValue> mInitialUserResponseFromCmd;
    std::unique_ptr<VehiclePropValue> mSwitchUserResponseFromCmd;

  private:
    GeneratorHub mGeneratorHub{
            std::bind(&VehicleHalServer::onFakeValueGenerated, this, std::placeholders::_1)};

    VehiclePropValuePool* mValuePool{nullptr};
};

}  // namespace android::hardware::automotive::vehicle::V2_0::impl
