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

#define LOG_TAG "automotive.vehicle@2.0-connector"

#include <fstream>

#include <android-base/logging.h>
#include <utils/SystemClock.h>

#include "DefaultConfig.h"
#include "EmulatedVehicleConnector.h"
#include "JsonFakeValueGenerator.h"
#include "LinearFakeValueGenerator.h"
#include "Obd2SensorStore.h"

namespace android {
namespace hardware {
namespace automotive {
namespace vehicle {
namespace V2_0 {

namespace impl {

class EmulatedPassthroughConnector : public PassthroughConnector {
  public:
    bool onDump(const hidl_handle& fd, const hidl_vec<hidl_string>& options) override;
};

bool EmulatedPassthroughConnector::onDump(const hidl_handle& handle,
                                          const hidl_vec<hidl_string>& options) {
    int fd = handle->data[0];

    if (options.size() > 0) {
        if (options[0] == "--help") {
            dprintf(fd, "Emulator-specific usage:\n");
            mEmulatedUserHal.showDumpHelp(fd);
            dprintf(fd, "\n");
            // Include caller's help options
            return true;
        } else if (options[0] == kUserHalDumpOption) {
            mEmulatedUserHal.dump(fd, "");
            return false;

        } else {
            // Let caller handle the options...
            return true;
        }
    }

    dprintf(fd, "Emulator-specific state:\n");
    mEmulatedUserHal.dump(fd, "  ");
    dprintf(fd, "\n");

    return true;
}

PassthroughConnectorPtr makeEmulatedPassthroughConnector() {
    return std::make_unique<EmulatedPassthroughConnector>();
}

}  // namespace impl

}  // namespace V2_0
}  // namespace vehicle
}  // namespace automotive
}  // namespace hardware
}  // namespace android
