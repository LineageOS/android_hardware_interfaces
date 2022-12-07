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

#ifndef android_hardware_automotive_vehicle_aidl_impl_fake_impl_userhal_include_FakeUserHal_H_
#define android_hardware_automotive_vehicle_aidl_impl_fake_impl_userhal_include_FakeUserHal_H_

#include <android-base/format.h>
#include <android-base/result.h>
#include <android-base/thread_annotations.h>

#include <VehicleHalTypes.h>
#include <VehicleObjectPool.h>
#include <VehicleUtils.h>

#include <memory>
#include <mutex>

namespace android {
namespace hardware {
namespace automotive {
namespace vehicle {
namespace fake {

constexpr char kUserHalDumpOption[] = "--user-hal";

// Class used to emulate a real User HAL behavior through lshal debug requests.
class FakeUserHal final {
  public:
    using ValueResultType = VhalResult<VehiclePropValuePool::RecyclableType>;

    explicit FakeUserHal(std::shared_ptr<VehiclePropValuePool> valuePool) : mValuePool(valuePool) {}

    ~FakeUserHal() = default;

    // Checks if the emulator can handle the property.
    static bool isSupported(int32_t prop);

    // Lets the emulator set the property.
    //
    // @return updated property and StatusCode
    ValueResultType onSetProperty(
            const aidl::android::hardware::automotive::vehicle::VehiclePropValue& value);

    // Gets the property value from the emulator.
    //
    // @return property value and StatusCode
    ValueResultType onGetProperty(
            const aidl::android::hardware::automotive::vehicle::VehiclePropValue& value) const;

    // Shows the User HAL emulation help.
    std::string showDumpHelp() const;

    // Dump its contents.
    std::string dump() const;

  private:
    const std::shared_ptr<VehiclePropValuePool> mValuePool;
    mutable std::mutex mLock;
    VehiclePropValuePool::RecyclableType mInitialUserResponseFromCmd GUARDED_BY(mLock);
    VehiclePropValuePool::RecyclableType mSwitchUserResponseFromCmd GUARDED_BY(mLock);
    VehiclePropValuePool::RecyclableType mCreateUserResponseFromCmd GUARDED_BY(mLock);
    VehiclePropValuePool::RecyclableType mSetUserIdentificationAssociationResponseFromCmd
            GUARDED_BY(mLock);

    // INITIAL_USER_INFO is called by Android when it starts, and it's expecting a property change
    // indicating what the initial user should be.
    //
    // During normal circumstances, the emulator will reply right away, passing a response if
    // InitialUserInfoResponseAction::DEFAULT (so Android could use its own logic to decide which
    // user to boot).
    //
    // But during development / testing, the behavior can be changed using lshal dump, which must
    // use the areaId to indicate what should happen next.
    //
    // So, the behavior of set(INITIAL_USER_INFO) is:
    //
    // - if it has an areaId, store the property into mInitialUserResponseFromCmd (as it was called
    // by lshal).
    // - else if mInitialUserResponseFromCmd is not set, return a response with the same request id
    // and InitialUserInfoResponseAction::DEFAULT
    // - else the behavior is defined by the areaId on mInitialUserResponseFromCmd:
    // - if it's 1, reply with mInitialUserResponseFromCmd and the right request id
    // - if it's 2, reply with mInitialUserResponseFromCmd but a wrong request id (so Android can
    // test this error scenario)
    // - if it's 3, then don't send a property change (so Android can emulate a timeout)
    ValueResultType onSetInitialUserInfoResponse(
            const aidl::android::hardware::automotive::vehicle::VehiclePropValue& value);

    // Used to emulate SWITCH_USER - see onSetInitialUserInfoResponse() for usage.
    ValueResultType onSetSwitchUserResponse(
            const aidl::android::hardware::automotive::vehicle::VehiclePropValue& value);

    // Used to emulate CREATE_USER - see onSetInitialUserInfoResponse() for usage.
    ValueResultType onSetCreateUserResponse(
            const aidl::android::hardware::automotive::vehicle::VehiclePropValue& value);

    // Used to emulate set USER_IDENTIFICATION_ASSOCIATION - see onSetInitialUserInfoResponse() for
    // usage.
    ValueResultType onSetUserIdentificationAssociation(
            const aidl::android::hardware::automotive::vehicle::VehiclePropValue& value);

    // Used to emulate get USER_IDENTIFICATION_ASSOCIATION - see onSetInitialUserInfoResponse() for
    // usage.
    ValueResultType onGetUserIdentificationAssociation(
            const aidl::android::hardware::automotive::vehicle::VehiclePropValue& value) const;

    // Creates a default USER_IDENTIFICATION_ASSOCIATION when it was not set by lshal.
    static ValueResultType defaultUserIdentificationAssociation(
            const aidl::android::hardware::automotive::vehicle::VehiclePropValue& request);

    ValueResultType sendUserHalResponse(VehiclePropValuePool::RecyclableType response,
                                        int32_t requestId);
};

}  // namespace fake
}  // namespace vehicle
}  // namespace automotive
}  // namespace hardware
}  // namespace android

#endif  // android_hardware_automotive_vehicle_aidl_impl_fake_impl_userhal_include_FakeUserHal_H_
