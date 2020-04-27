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

#ifndef android_hardware_automotive_vehicle_V2_0_impl_EmulatedUserHal_H_
#define android_hardware_automotive_vehicle_V2_0_impl_EmulatedUserHal_H_

#include <android-base/result.h>

#include <android/hardware/automotive/vehicle/2.0/types.h>

namespace android {
namespace hardware {
namespace automotive {
namespace vehicle {
namespace V2_0 {

namespace impl {

constexpr char kUserHalDumpOption[] = "--user-hal";

/**
 * Class used to emulate User HAL behavior through lshal debug requests.
 */
class EmulatedUserHal {
  public:
    EmulatedUserHal() {}

    ~EmulatedUserHal() = default;

    /**
     * Checks if the emulator can handle the property.
     */
    bool isSupported(int32_t prop);

    /**
     * Lets the emulator handle the property.
     *
     * @return updated property and StatusCode
     */
    android::base::Result<std::unique_ptr<VehiclePropValue>> onSetProperty(
            const VehiclePropValue& value);

    /**
     * Shows the User HAL emulation help.
     */
    void showDumpHelp(int fd);

    /**
     * Dump its contents.
     */
    void dump(int fd, std::string indent);

  private:
    /**
     * INITIAL_USER_INFO is called by Android when it starts, and it's expecting a property change
     * indicating what the initial user should be.
     *
     * During normal circumstances, the emulator will reply right away, passing a response if
     * InitialUserInfoResponseAction::DEFAULT (so Android could use its own logic to decide which
     * user to boot).
     *
     * But during development / testing, the behavior can be changed using lshal dump, which must
     * use the areaId to indicate what should happen next.
     *
     * So, the behavior of set(INITIAL_USER_INFO) is:
     *
     * - if it has an areaId, store the property into mInitialUserResponseFromCmd (as it was called
     * by lshal).
     * - else if mInitialUserResponseFromCmd is not set, return a response with the same request id
     * and InitialUserInfoResponseAction::DEFAULT
     * - else the behavior is defined by the areaId on mInitialUserResponseFromCmd:
     * - if it's 1, reply with mInitialUserResponseFromCmd and the right request id
     * - if it's 2, reply with mInitialUserResponseFromCmd but a wrong request id (so Android can
     * test this error scenario)
     * - if it's 3, then don't send a property change (so Android can emulate a timeout)
     *
     */
    android::base::Result<std::unique_ptr<VehiclePropValue>> onSetInitialUserInfoResponse(
            const VehiclePropValue& value);

    /**
     * Used to emulate SWITCH_USER - see onSetInitialUserInfoResponse() for usage.
     */
    android::base::Result<std::unique_ptr<VehiclePropValue>> onSetSwitchUserResponse(
            const VehiclePropValue& value);

    android::base::Result<std::unique_ptr<VehiclePropValue>> sendUserHalResponse(
            std::unique_ptr<VehiclePropValue> response, int32_t requestId);

    std::unique_ptr<VehiclePropValue> mInitialUserResponseFromCmd;
    std::unique_ptr<VehiclePropValue> mSwitchUserResponseFromCmd;
};

}  // namespace impl

}  // namespace V2_0
}  // namespace vehicle
}  // namespace automotive
}  // namespace hardware
}  // namespace android

#endif  // android_hardware_automotive_vehicle_V2_0_impl_EmulatedUserHal_H_
