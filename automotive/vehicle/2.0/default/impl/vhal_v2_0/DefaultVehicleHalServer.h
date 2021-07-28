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

#include <vhal_v2_0/VehicleObjectPool.h>
#include <vhal_v2_0/VehiclePropertyStore.h>
#include <vhal_v2_0/VehicleServer.h>

#include "DefaultConfig.h"
#include "GeneratorHub.h"

namespace android {
namespace hardware {
namespace automotive {
namespace vehicle {
namespace V2_0 {

namespace impl {

// This contains the server operation for VHAL running in emulator.
class DefaultVehicleHalServer : public IVehicleServer {
  public:
    DefaultVehicleHalServer();

    // Send all the property values to client.
    void sendAllValuesToClient();

    // Methods from IVehicleServer

    std::vector<VehiclePropConfig> onGetAllPropertyConfig() const override;

    StatusCode onSetProperty(const VehiclePropValue& value, bool updateStatus) override;

    // Dump/debug the Default VHAL server. If options is empty, the internal information for the
    // server would be dumped. Otherwise, the options would be treated as debug commands and sent
    // to debug function to handle the commands.
    DumpResult onDump(const std::vector<std::string>& options) override;

    // Set the Property Value Pool used in this server
    void setValuePool(VehiclePropValuePool* valuePool);

  protected:
    using VehiclePropValuePtr = recyclable_ptr<VehiclePropValue>;
    GeneratorHub* getGeneratorHub();

    VehiclePropValuePool* getValuePool() const;

    void onFakeValueGenerated(const VehiclePropValue& value);

    StatusCode handleGenerateFakeDataRequest(const VehiclePropValue& request);

    VehiclePropValuePtr createApPowerStateReq(VehicleApPowerStateReq req, int32_t param);

    VehiclePropValuePtr createHwInputKeyProp(VehicleHwKeyInputAction action, int32_t keyCode,
                                             int32_t targetDisplay);

    void storePropInitialValue(const ConfigDeclaration& config);

    // Handles debug commands. The first option must be "--debughal" otherwise the command would be
    // ignored. The second option specifies the operations to execute. Different operations require
    // different input options, for detail, see the helpInfo printed by getHelpInfo().
    DumpResult debugCommand(const std::vector<std::string>& options);

    // Gets help info. Contains the usage for different debug commands.
    std::string getHelpInfo();

    // If "persist.vendor.vhal_init_value_override" is true, try to override the properties default
    // values according to JSON files in 'overrideDir'. Would be called in constructor using
    // VENDOR_OVERRIDE_DIR as overrideDir.
    void maybeOverrideProperties(const char* overrideDir);

    // Handles "--genfakedata" debug command.
    DumpResult genFakeDataCommand(const std::vector<std::string>& options);

    // Handles "--setint" or "--setfloat" or "--setbool" debug command.
    DumpResult setValueCommand(const std::vector<std::string>& options);

  protected:
    GeneratorHub mGeneratorHub{
            [this](const VehiclePropValue& value) { return onFakeValueGenerated(value); }};

    VehiclePropValuePool* mValuePool{nullptr};
    VehiclePropertyStore mServerSidePropStore;

  private:
    // Expose protected methods to unit test.
    friend class DefaultVhalImplTestHelper;
    // Override the properties using config files in 'overrideDir'.
    void overrideProperties(const char* overrideDir);
};

}  // namespace impl

}  // namespace V2_0
}  // namespace vehicle
}  // namespace automotive
}  // namespace hardware
}  // namespace android
