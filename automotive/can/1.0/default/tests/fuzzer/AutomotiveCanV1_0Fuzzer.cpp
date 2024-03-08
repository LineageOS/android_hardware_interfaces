/*
 * Copyright (C) 2022 The Android Open Source Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at:
 *
 * http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 *
 */
#include "AutomotiveCanV1_0Fuzzer.h"

namespace android::hardware::automotive::can::V1_0::implementation::fuzzer {

constexpr CanController::InterfaceType kInterfaceType[] = {
        CanController::InterfaceType::VIRTUAL, CanController::InterfaceType::SOCKETCAN,
        CanController::InterfaceType::SLCAN, CanController::InterfaceType::INDEXED};
constexpr FilterFlag kFilterFlag[] = {FilterFlag::DONT_CARE, FilterFlag::SET, FilterFlag::NOT_SET};
constexpr size_t kInterfaceTypeLength = std::size(kInterfaceType);
constexpr size_t kFilterFlagLength = std::size(kFilterFlag);
constexpr size_t kMaxCharacters = 30;
constexpr size_t kMaxPayloadBytes = 64;
constexpr size_t kMaxFilters = 20;
constexpr size_t kMaxSerialNumber = 1000;
constexpr size_t kMaxBuses = 100;
constexpr size_t kMaxRepeat = 100;

Bus CanFuzzer::makeBus() {
    ICanController::BusConfig config = {};
    if (mBusNames.size() > 0 && mLastInterface < mBusNames.size()) {
        config.name = mBusNames[mLastInterface++];
    } else {
        config.name = mFuzzedDataProvider->ConsumeRandomLengthString(kMaxCharacters);
    }
    config.interfaceId.virtualif({mFuzzedDataProvider->ConsumeRandomLengthString(kMaxCharacters)});
    return Bus(mCanController, config);
}

void CanFuzzer::getSupportedInterfaceTypes() {
    hidl_vec<CanController::InterfaceType> iftypesResult;
    mCanController->getSupportedInterfaceTypes(hidl_utils::fill(&iftypesResult));
}

hidl_vec<hidl_string> CanFuzzer::getBusNames() {
    hidl_vec<hidl_string> services = {};
    if (auto manager = hidl::manager::V1_2::IServiceManager::getService(); manager) {
        manager->listManifestByInterface(ICanBus::descriptor, hidl_utils::fill(&services));
    }
    return services;
}

void CanFuzzer::invokeUpInterface() {
    CanController::InterfaceType controller;
    if (mFuzzedDataProvider->ConsumeBool()) {
        controller = (CanController::InterfaceType)mFuzzedDataProvider->ConsumeIntegral<uint8_t>();
    } else {
        controller = kInterfaceType[mFuzzedDataProvider->ConsumeIntegralInRange<size_t>(
                0, kInterfaceTypeLength - 1)];
    }
    std::string configName;

    if (const bool shouldInvokeValidBus = mFuzzedDataProvider->ConsumeBool();
        (shouldInvokeValidBus) && (mBusNames.size() > 0)) {
        const size_t busNameIndex =
                mFuzzedDataProvider->ConsumeIntegralInRange<size_t>(0, mBusNames.size() - 1);
        configName = mBusNames[busNameIndex];
    } else {
        configName = mFuzzedDataProvider->ConsumeRandomLengthString(kMaxCharacters);
    }
    const std::string ifname = mFuzzedDataProvider->ConsumeRandomLengthString(kMaxCharacters);

    ICanController::BusConfig config = {.name = configName};

    if (controller == CanController::InterfaceType::SOCKETCAN) {
        CanController::BusConfig::InterfaceId::Socketcan socketcan = {};
        if (const bool shouldPassSerialSocket = mFuzzedDataProvider->ConsumeBool();
            shouldPassSerialSocket) {
            socketcan.serialno(
                    {mFuzzedDataProvider->ConsumeIntegralInRange<uint32_t>(0, kMaxSerialNumber)});
        } else {
            socketcan.ifname(ifname);
        }
        config.interfaceId.socketcan(socketcan);
    } else if (controller == CanController::InterfaceType::SLCAN) {
        CanController::BusConfig::InterfaceId::Slcan slcan = {};
        if (const bool shouldPassSerialSlcan = mFuzzedDataProvider->ConsumeBool();
            shouldPassSerialSlcan) {
            slcan.serialno(
                    {mFuzzedDataProvider->ConsumeIntegralInRange<uint32_t>(0, kMaxSerialNumber)});
        } else {
            slcan.ttyname(ifname);
        }
        config.interfaceId.slcan(slcan);
    } else if (controller == CanController::InterfaceType::VIRTUAL) {
        config.interfaceId.virtualif({ifname});
    } else if (controller == CanController::InterfaceType::INDEXED) {
        CanController::BusConfig::InterfaceId::Indexed indexed;
        indexed.index = mFuzzedDataProvider->ConsumeIntegral<uint8_t>();
        config.interfaceId.indexed(indexed);
    }

    const size_t numInvocations =
            mFuzzedDataProvider->ConsumeIntegralInRange<size_t>(0, kMaxRepeat);
    for (size_t i = 0; i < numInvocations; ++i) {
        mCanController->upInterface(config);
    }
}

void CanFuzzer::invokeDownInterface() {
    hidl_string configName;
    if (const bool shouldInvokeValidBus = mFuzzedDataProvider->ConsumeBool();
        (shouldInvokeValidBus) && (mBusNames.size() > 0)) {
        size_t busNameIndex;
        if (mBusNames.size() == 1) {
            busNameIndex = 0;
        } else {
            busNameIndex =
                    mFuzzedDataProvider->ConsumeIntegralInRange<size_t>(0, mBusNames.size() - 1);
        }
        configName = mBusNames[busNameIndex];
    } else {
        configName = mFuzzedDataProvider->ConsumeRandomLengthString(kMaxCharacters);
    }

    const size_t numInvocations =
            mFuzzedDataProvider->ConsumeIntegralInRange<size_t>(0, kMaxRepeat);
    for (size_t i = 0; i < numInvocations; ++i) {
        mCanController->downInterface(configName);
    }
}

void CanFuzzer::invokeBus() {
    const size_t numBuses = mFuzzedDataProvider->ConsumeIntegralInRange<size_t>(1, kMaxBuses);
    for (size_t i = 0; i < numBuses; ++i) {
        if (const bool shouldSendMessage = mFuzzedDataProvider->ConsumeBool(); shouldSendMessage) {
            auto sendingBus = makeBus();
            CanMessage msg = {.id = mFuzzedDataProvider->ConsumeIntegral<uint32_t>()};
            uint32_t numPayloadBytes =
                    mFuzzedDataProvider->ConsumeIntegralInRange<uint32_t>(0, kMaxPayloadBytes);
            hidl_vec<uint8_t> payload(numPayloadBytes);
            for (uint32_t j = 0; j < numPayloadBytes; ++j) {
                payload[j] = mFuzzedDataProvider->ConsumeIntegral<uint32_t>();
            }
            msg.payload = payload;
            msg.remoteTransmissionRequest = mFuzzedDataProvider->ConsumeBool();
            msg.isExtendedId = mFuzzedDataProvider->ConsumeBool();
            sendingBus.send(msg);
        } else {
            auto listeningBus = makeBus();
            uint32_t numFilters =
                    mFuzzedDataProvider->ConsumeIntegralInRange<uint32_t>(1, kMaxFilters);
            hidl_vec<CanMessageFilter> filterVector(numFilters);
            for (uint32_t k = 0; k < numFilters; ++k) {
                filterVector[k].id = mFuzzedDataProvider->ConsumeIntegral<uint32_t>();
                filterVector[k].mask = mFuzzedDataProvider->ConsumeIntegral<uint32_t>();
                if (mFuzzedDataProvider->ConsumeBool()) {
                    filterVector[k].rtr =
                            (FilterFlag)mFuzzedDataProvider->ConsumeIntegral<uint8_t>();
                } else {
                    filterVector[k].rtr =
                            kFilterFlag[mFuzzedDataProvider->ConsumeIntegralInRange<size_t>(
                                    0, kFilterFlagLength - 1)];
                }
                if (mFuzzedDataProvider->ConsumeBool()) {
                    filterVector[k].extendedFormat =
                            (FilterFlag)mFuzzedDataProvider->ConsumeIntegral<uint8_t>();
                } else {
                    filterVector[k].extendedFormat =
                            kFilterFlag[mFuzzedDataProvider->ConsumeIntegralInRange<size_t>(
                                    0, kFilterFlagLength - 1)];
                }
                filterVector[k].exclude = mFuzzedDataProvider->ConsumeBool();
            }
            auto listener = listeningBus.listen(filterVector);
        }
    }
}

void CanFuzzer::deInit() {
    mCanController.clear();
    if (mFuzzedDataProvider) {
        delete mFuzzedDataProvider;
    }
    mBusNames = {};
}

void CanFuzzer::process(const uint8_t* data, size_t size) {
    mFuzzedDataProvider = new FuzzedDataProvider(data, size);
    while (mFuzzedDataProvider->remaining_bytes()) {
        auto CanFuzzerFunction =
                mFuzzedDataProvider->PickValueInArray<const std::function<void()>>({
                        [&]() { getSupportedInterfaceTypes(); },
                        [&]() { invokeUpInterface(); },
                        [&]() { invokeDownInterface(); },
                        [&]() { invokeBus(); },
                });
        CanFuzzerFunction();
    }
}

bool CanFuzzer::init() {
    mCanController = sp<CanController>::make();
    if (!mCanController) {
        return false;
    }
    mBusNames = getBusNames();
    return true;
}

extern "C" int LLVMFuzzerTestOneInput(const uint8_t* data, size_t size) {
    if (size < 1) {
        return 0;
    }
    CanFuzzer canFuzzer;
    if (canFuzzer.init()) {
        canFuzzer.process(data, size);
    }
    return 0;
}

}  // namespace android::hardware::automotive::can::V1_0::implementation::fuzzer
