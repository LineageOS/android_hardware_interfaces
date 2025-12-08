/*
 * Copyright (C) 2023 The Android Open Source Project
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

#include <libminradio/modem/RadioModem.h>

#include <libminradio/debug.h>
#include <libminradio/response.h>
#include <chrono>
#include <thread>

#define RADIO_MODULE "Modem"

namespace android::hardware::radio::minimal {

using namespace ::android::hardware::radio::minimal::binder_printing;
using ::aidl::android::hardware::radio::RadioIndicationType;
using ::ndk::ScopedAStatus;
namespace aidl = ::aidl::android::hardware::radio::modem;
namespace aidlRadio = ::aidl::android::hardware::radio;
constexpr auto ok = &ScopedAStatus::ok;

RadioModem::RadioModem(std::shared_ptr<SlotContext> context,
                       std::vector<aidlRadio::RadioTechnology> rats)
    : RadioSlotBase(context) {
    int32_t ratBitmap = 0;
    for (auto rat : rats) {
        CHECK(rat > aidlRadio::RadioTechnology::UNKNOWN) << "Invalid RadioTechnology: " << rat;
        CHECK(rat <= aidlRadio::RadioTechnology::NR)
                << ": " << rat << " not supported yet: "
                << "please verify if RadioAccessFamily for this RadioTechnology is a bit-shifted 1";
        ratBitmap |= 1 << static_cast<int32_t>(rat);
    }
    mRatBitmap = ratBitmap;
}

std::string RadioModem::getModemUuid() const {
    // Assumes one modem per slot.
    return std::format("com.android.minradio.modem{}", mContext->getSlotIndex());
}

std::string RadioModem::getSimUuid() const {
    // Assumes one SIM per slot.
    return std::format("com.android.minradio.sim{}", mContext->getSlotIndex());
}

ScopedAStatus RadioModem::enableModem(int32_t serial, bool on) {
    LOG_NOT_SUPPORTED << on;
    return ScopedAStatus::fromExceptionCode(EX_UNSUPPORTED_OPERATION);
}

ScopedAStatus RadioModem::getBasebandVersion(int32_t serial) {
    LOG_CALL;
    respond()->getBasebandVersionResponse(  //
            noError(serial), std::format("libminradio V{}", IRadioModem::version));
    return ok();
}

ScopedAStatus RadioModem::getDeviceIdentity(int32_t serial) {
    LOG_AND_RETURN_DEPRECATED();
}

ScopedAStatus RadioModem::getHardwareConfig(int32_t serial) {
    LOG_CALL;

    aidl::HardwareConfig modem1Config{
            .type = aidl::HardwareConfig::TYPE_MODEM,
            .uuid = getModemUuid(),
            .state = aidl::HardwareConfig::STATE_ENABLED,
            .modem = {{
                    .rilModel = 0,  // 0=single (one-to-one relationship for hw and ril daemon)
                    .rat = static_cast<aidlRadio::RadioTechnology>(mRatBitmap),
                    .maxVoiceCalls = 0,
                    .maxDataCalls = 1,
                    .maxStandby = 1,
            }},
    };

    aidl::HardwareConfig sim1Config{
            .type = aidl::HardwareConfig::TYPE_SIM,
            .uuid = getSimUuid(),
            .state = aidl::HardwareConfig::STATE_ENABLED,
            .sim = {{
                    .modemUuid = getModemUuid(),
            }},
    };

    respond()->getHardwareConfigResponse(noError(serial), {modem1Config, sim1Config});
    return ok();
}

ScopedAStatus RadioModem::getModemActivityInfo(int32_t serial) {
    LOG_CALL_IGNORED;
    const aidl::ActivityStatsTechSpecificInfo generalActivityStats{
            .txmModetimeMs = {0, 0, 0, 0, 0},
    };
    const aidl::ActivityStatsInfo info{
            // idleModeTimeMs doesn't make sense for external modem, but the framework
            // doesn't allow for ModemActivityInfo.isEmpty
            .idleModeTimeMs = 1,
            .techSpecificInfo = {generalActivityStats},
    };
    respond()->getModemActivityInfoResponse(noError(serial), info);
    return ok();
}

ScopedAStatus RadioModem::getModemStackStatus(int32_t serial) {
    LOG_CALL;
    respond()->getModemStackStatusResponse(noError(serial), true);
    return ok();
}

ScopedAStatus RadioModem::getRadioCapability(int32_t serial) {
    LOG_CALL;
    aidl::RadioCapability cap{
            .session = 0,
            .phase = aidl::RadioCapability::PHASE_FINISH,
            .raf = mRatBitmap,  // rafs are nothing else than rat masks
            .logicalModemUuid = getModemUuid(),
            .status = aidl::RadioCapability::STATUS_SUCCESS,
    };
    respond()->getRadioCapabilityResponse(noError(serial), cap);
    return ok();
}

ScopedAStatus RadioModem::nvReadItem(int32_t serial, aidl::NvItem) {
    LOG_AND_RETURN_DEPRECATED();
}

ScopedAStatus RadioModem::nvResetConfig(int32_t serial, aidl::ResetNvType resetType) {
    LOG_CALL << resetType;  // RELOAD is the only non-deprecated argument
    if (resetType == aidl::ResetNvType::RELOAD) {
        respond()->nvResetConfigResponse(noError(serial));

        // Broadcasting radio unavailable and then on a different thread to simulate reboot.
        std::thread([this] {
            using namespace std::chrono_literals;
            std::this_thread::sleep_for(1s);
            indicate()->radioStateChanged(RadioIndicationType::UNSOLICITED,
                                          aidl::RadioState::UNAVAILABLE);
            std::this_thread::sleep_for(1s);
            indicate()->radioStateChanged(RadioIndicationType::UNSOLICITED, aidl::RadioState::ON);
        }).detach();
    } else {
        respond()->nvResetConfigResponse(notSupported(serial));
    }
    return ok();
}

ScopedAStatus RadioModem::nvWriteCdmaPrl(int32_t serial, const std::vector<uint8_t>&) {
    LOG_AND_RETURN_DEPRECATED();
}

ScopedAStatus RadioModem::nvWriteItem(int32_t serial, const aidl::NvWriteItem&) {
    LOG_AND_RETURN_DEPRECATED();
}

ScopedAStatus RadioModem::requestShutdown(int32_t serial) {
    LOG_NOT_SUPPORTED;
    return ScopedAStatus::fromExceptionCode(EX_UNSUPPORTED_OPERATION);
}

ScopedAStatus RadioModem::responseAcknowledgement() {
    LOG_CALL_NOSERIAL;
    return ok();
}

ScopedAStatus RadioModem::sendDeviceState(int32_t serial, aidl::DeviceStateType type, bool state) {
    LOG_CALL_IGNORED << type << ' ' << state;
    respond()->sendDeviceStateResponse(noError(serial));
    return ok();
}

ScopedAStatus RadioModem::setRadioCapability(int32_t serial, const aidl::RadioCapability& rc) {
    LOG_NOT_SUPPORTED << rc;
    respond()->setRadioCapabilityResponse(notSupported(serial), {});
    return ok();
}

ScopedAStatus RadioModem::setRadioPower(int32_t serial, bool powerOn, bool forEmergencyCall,
                                        bool preferredForEmergencyCall) {
    LOG_CALL_IGNORED << powerOn << " " << forEmergencyCall << " " << preferredForEmergencyCall;
    respond()->setRadioPowerResponse(noError(serial));
    indicate()->radioStateChanged(RadioIndicationType::UNSOLICITED,
                                  powerOn ? aidl::RadioState::ON : aidl::RadioState::OFF);
    return ok();
}

ScopedAStatus RadioModem::setResponseFunctions(
        const std::shared_ptr<aidl::IRadioModemResponse>& response,
        const std::shared_ptr<aidl::IRadioModemIndication>& indication) {
    LOG_CALL_NOSERIAL << response << ' ' << indication;
    CHECK(response);
    CHECK(indication);
    respond = response;
    indicate = indication;
    setResponseFunctionsBase();
    return ok();
}

void RadioModem::onUpdatedResponseFunctions() {
    indicate()->rilConnected(RadioIndicationType::UNSOLICITED);
    indicate()->radioStateChanged(RadioIndicationType::UNSOLICITED, aidl::RadioState::ON);
}

}  // namespace android::hardware::radio::minimal
