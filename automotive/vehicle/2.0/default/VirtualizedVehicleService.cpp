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

#include <android-base/logging.h>
#include <cutils/properties.h>
#include <hidl/HidlTransportSupport.h>

#include <vhal_v2_0/EmulatedVehicleConnector.h>
#include <vhal_v2_0/EmulatedVehicleHal.h>
#include <vhal_v2_0/VehicleHalManager.h>
#include <vhal_v2_0/virtualization/GrpcVehicleClient.h>
#include <vhal_v2_0/virtualization/Utils.h>

using namespace android;
using namespace android::hardware;
using namespace android::hardware::automotive::vehicle::V2_0;

int main(int argc, char* argv[]) {
    constexpr const char* VHAL_SERVER_CID_PROPERTY_KEY = "ro.vendor.vehiclehal.server.cid";
    constexpr const char* VHAL_SERVER_PORT_PROPERTY_KEY = "ro.vendor.vehiclehal.server.port";

    auto property_get_uint = [](const char* key, unsigned int default_value) {
        auto value = property_get_int64(key, default_value);
        if (value < 0 || value > UINT_MAX) {
            LOG(DEBUG) << key << ": " << value << " is out of bound, using default value '"
                       << default_value << "' instead";
            return default_value;
        }
        return static_cast<unsigned int>(value);
    };

    impl::VsockServerInfo serverInfo{property_get_uint(VHAL_SERVER_CID_PROPERTY_KEY, 0),
                                     property_get_uint(VHAL_SERVER_PORT_PROPERTY_KEY, 0)};

    if (serverInfo.serverCid == 0 || serverInfo.serverPort == 0) {
        LOG(FATAL) << "Invalid server information, CID: " << serverInfo.serverCid
                   << "; port: " << serverInfo.serverPort;
        // Will abort after logging
    }

    auto store = std::make_unique<VehiclePropertyStore>();
    auto connector = impl::makeGrpcVehicleClient(impl::getVsockUri(serverInfo));
    auto hal = std::make_unique<impl::EmulatedVehicleHal>(store.get(), connector.get());
    auto emulator = std::make_unique<impl::VehicleEmulator>(hal.get());
    auto service = std::make_unique<VehicleHalManager>(hal.get());

    configureRpcThreadpool(4, true /* callerWillJoin */);

    LOG(INFO) << "Registering as service...";
    status_t status = service->registerAsService();

    if (status != OK) {
        LOG(ERROR) << "Unable to register vehicle service (" << status << ")";
        return 1;
    }

    LOG(INFO) << "Ready";
    joinRpcThreadpool();

    return 1;
}
