/******************************************************************************
 *
 * Copyright (C) 2021 The Android Open Source Project
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
 *****************************************************************************
 * Originally developed and contributed by Ittiam Systems Pvt. Ltd, Bangalore
 */

#include "VehicleManager_fuzzer.h"
#include <utils/SystemClock.h>
#include <vhal_v2_0/Obd2SensorStore.h>
#include <vhal_v2_0/WatchdogClient.h>

namespace android::hardware::automotive::vehicle::V2_0::fuzzer {

using ::aidl::android::automotive::watchdog::TimeoutLength;
using ::android::elapsedRealtimeNano;
using ::android::Looper;
using ::android::sp;
using ::android::hardware::hidl_handle;
using ::android::hardware::hidl_string;
using ::android::hardware::hidl_vec;
using ::android::hardware::automotive::vehicle::V2_0::DiagnosticFloatSensorIndex;
using ::android::hardware::automotive::vehicle::V2_0::DiagnosticIntegerSensorIndex;
using ::android::hardware::automotive::vehicle::V2_0::kCustomComplexProperty;
using ::android::hardware::automotive::vehicle::V2_0::kVehicleProperties;
using ::android::hardware::automotive::vehicle::V2_0::MockedVehicleCallback;
using ::android::hardware::automotive::vehicle::V2_0::Obd2SensorStore;
using ::android::hardware::automotive::vehicle::V2_0::recyclable_ptr;
using ::android::hardware::automotive::vehicle::V2_0::StatusCode;
using ::android::hardware::automotive::vehicle::V2_0::SubscribeFlags;
using ::android::hardware::automotive::vehicle::V2_0::SubscribeOptions;
using ::android::hardware::automotive::vehicle::V2_0::VehicleAreaConfig;
using ::android::hardware::automotive::vehicle::V2_0::VehicleHal;
using ::android::hardware::automotive::vehicle::V2_0::VehicleHalManager;
using ::android::hardware::automotive::vehicle::V2_0::VehiclePropConfig;
using ::android::hardware::automotive::vehicle::V2_0::VehicleProperty;
using ::android::hardware::automotive::vehicle::V2_0::VehiclePropertyAccess;
using ::android::hardware::automotive::vehicle::V2_0::VehiclePropertyChangeMode;
using ::android::hardware::automotive::vehicle::V2_0::VehiclePropertyStore;
using ::android::hardware::automotive::vehicle::V2_0::VehiclePropertyType;
using ::android::hardware::automotive::vehicle::V2_0::VehiclePropValue;
using ::android::hardware::automotive::vehicle::V2_0::VehiclePropValuePool;
using ::android::hardware::automotive::vehicle::V2_0::VmsMessageType;
using ::android::hardware::automotive::vehicle::V2_0::WatchdogClient;
using ::android::hardware::automotive::vehicle::V2_0::vms::createAvailabilityRequest;
using ::android::hardware::automotive::vehicle::V2_0::vms::createBaseVmsMessage;
using ::android::hardware::automotive::vehicle::V2_0::vms::createPublisherIdRequest;
using ::android::hardware::automotive::vehicle::V2_0::vms::createStartSessionMessage;
using ::android::hardware::automotive::vehicle::V2_0::vms::createSubscriptionsRequest;
using ::android::hardware::automotive::vehicle::V2_0::vms::getAvailableLayers;
using ::android::hardware::automotive::vehicle::V2_0::vms::getSequenceNumberForAvailabilityState;
using ::android::hardware::automotive::vehicle::V2_0::vms::getSequenceNumberForSubscriptionsState;
using ::android::hardware::automotive::vehicle::V2_0::vms::hasServiceNewlyStarted;
using ::android::hardware::automotive::vehicle::V2_0::vms::isAvailabilitySequenceNumberNewer;
using ::android::hardware::automotive::vehicle::V2_0::vms::isSequenceNumberNewer;
using ::android::hardware::automotive::vehicle::V2_0::vms::isValidVmsMessage;
using ::android::hardware::automotive::vehicle::V2_0::vms::parseData;
using ::android::hardware::automotive::vehicle::V2_0::vms::parseMessageType;
using ::android::hardware::automotive::vehicle::V2_0::vms::parsePublisherIdResponse;
using ::android::hardware::automotive::vehicle::V2_0::vms::parseStartSessionMessage;
using ::android::hardware::automotive::vehicle::V2_0::vms::VmsLayer;
using ::android::hardware::automotive::vehicle::V2_0::vms::VmsLayerAndPublisher;
using ::android::hardware::automotive::vehicle::V2_0::vms::VmsLayerOffering;
using ::android::hardware::automotive::vehicle::V2_0::vms::VmsOffers;

std::string kCarMake;
constexpr int32_t kMaxCaseMessage = 8;
constexpr int32_t kMaxRuns = 20;
constexpr int32_t kMaxSize = 1000;
constexpr int32_t kMinSize = 0;
constexpr int32_t kMaxFileSize = 100;
float kFloatValue;
std::vector<int32_t> kVec32;
std::vector<int64_t> kVec64;
std::vector<uint8_t> kVec8;
std::vector<float> kVecFloat;
static const std::vector<std::string> kSampleDtcs = {"P0070",
                                                     "P0102"
                                                     "P0123"};

MockedVehicleHal::VehiclePropValuePtr MockedVehicleHal::get(
        const VehiclePropValue& requestedPropValue, StatusCode* outStatus) {
    VehiclePropValuePtr pValue = nullptr;
    if (outStatus == nullptr) {
        return pValue;
    }
    auto property = static_cast<VehicleProperty>(requestedPropValue.prop);
    int32_t areaId = requestedPropValue.areaId;
    *outStatus = StatusCode::OK;

    switch (property) {
        case VehicleProperty::INFO_MAKE:
            pValue = getValuePool()->obtainString(kCarMake.c_str());
            break;
        case VehicleProperty::INFO_FUEL_CAPACITY:
            if (mFuelCapacityAttemptsLeft-- > 0) {
                *outStatus = StatusCode::TRY_AGAIN;
            } else {
                pValue = getValuePool()->obtainFloat(kFloatValue);
            }
            break;
        default:
            if (requestedPropValue.prop == kCustomComplexProperty) {
                pValue = getValuePool()->obtainComplex();
                pValue->value.int32Values = hidl_vec<int32_t>{kVec32};
                pValue->value.int64Values = hidl_vec<int64_t>{kVec64};
                pValue->value.floatValues = hidl_vec<float_t>{kVecFloat};
                pValue->value.bytes = hidl_vec<uint8_t>{kVec8};
                pValue->value.stringValue = kCarMake.c_str();
                break;
            }
            auto key = makeKey(toInt(property), areaId);
            pValue = getValuePool()->obtain(mValues[key]);
    }

    if (*outStatus == StatusCode::OK && pValue.get() != nullptr) {
        pValue->prop = toInt(property);
        pValue->areaId = areaId;
        pValue->timestamp = elapsedRealtimeNano();
    }

    return pValue;
}

void VehicleHalManagerFuzzer::initValue() {
    kCarMake = mFuzzedDataProvider->ConsumeRandomLengthString(kMaxFileSize);
    kFloatValue = mFuzzedDataProvider->ConsumeFloatingPoint<float>();
    fillParameter<int32_t>(mFuzzedDataProvider->ConsumeIntegralInRange<size_t>(kMinSize, kMaxSize),
                           kVec32);
    fillParameter<int64_t>(mFuzzedDataProvider->ConsumeIntegralInRange<size_t>(kMinSize, kMaxSize),
                           kVec64);
    fillParameter<uint8_t>(mFuzzedDataProvider->ConsumeIntegralInRange<size_t>(kMinSize, kMaxSize),
                           kVec8);
    size_t size = mFuzzedDataProvider->ConsumeIntegralInRange<size_t>(kMinSize, kMaxSize);
    for (size_t i = 0; i < size; ++i) {
        kVecFloat.push_back(mFuzzedDataProvider->ConsumeFloatingPoint<float>());
    }
}

void VehicleHalManagerFuzzer::process(const uint8_t* data, size_t size) {
    mFuzzedDataProvider = new FuzzedDataProvider(data, size);
    initValue();
    /* Limited while loop runs to prevent timeouts caused
     * by repeated calls to high-execution-time APIs.
     */
    size_t maxRuns = mFuzzedDataProvider->ConsumeIntegralInRange<size_t>(kMinSize, kMaxRuns);
    size_t itr = 0;
    while (mFuzzedDataProvider->remaining_bytes() && ++itr <= maxRuns) {
        auto invokeVehicleHalManagerFuzzer =
                mFuzzedDataProvider->PickValueInArray<const std::function<void()>>({
                        [&]() { invokeDebug(); },
                        [&]() { invokePropConfigs(); },
                        [&]() { invokeSubscribe(); },
                        [&]() { invokeSetAndGetValues(); },
                        [&]() { invokeObd2SensorStore(); },
                        [&]() { invokeVmsUtils(); },
                        [&]() { invokeVehiclePropStore(); },
                        [&]() { invokeWatchDogClient(); },
                });
        invokeVehicleHalManagerFuzzer();
    }
}

void VehicleHalManagerFuzzer::invokeDebug() {
    hidl_handle fd = {};

    native_handle_t* rawHandle = native_handle_create(/*numFds=*/1, /*numInts=*/0);
    fd.setTo(native_handle_clone(rawHandle), /*shouldOwn=*/true);
    int32_t size = mFuzzedDataProvider->ConsumeIntegralInRange<int32_t>(kMinSize, kMaxFileSize);
    hidl_vec<hidl_string> options(size);

    for (int32_t idx = 0; idx < size; ++idx) {
        if (idx == 0 && mFuzzedDataProvider->ConsumeBool()) {
            options[idx] = mFuzzedDataProvider->PickValueInArray(
                    {"--help", "--list", "--get", "--set", "", "invalid"});
        } else if (idx == 2 && mFuzzedDataProvider->ConsumeBool()) {
            options[idx] =
                    mFuzzedDataProvider->PickValueInArray({"-i", "-i64", "-f", "-s", "-b", "-a"});
        } else if (mFuzzedDataProvider->ConsumeBool()) {
            options[idx] = mFuzzedDataProvider->ConsumeRandomLengthString(kMaxSize);
        } else {
            options[idx] = std::to_string(mFuzzedDataProvider->ConsumeIntegral<int32_t>());
        }
    }

    if (mFuzzedDataProvider->ConsumeBool()) {
        mManager->debug(fd, {});
    } else {
        mManager->debug(fd, options);
    }
    native_handle_delete(rawHandle);
}

void VehicleHalManagerFuzzer::invokePropConfigs() {
    int32_t vehicleProp1 = mFuzzedDataProvider->ConsumeIntegral<int32_t>();
    int32_t vehicleProp2 = mFuzzedDataProvider->ConsumeIntegral<int32_t>();

    hidl_vec<int32_t> properties = {vehicleProp1, vehicleProp2};
    auto invokePropConfigsAPI = mFuzzedDataProvider->PickValueInArray<const std::function<void()>>({
            [&]() {
                mManager->getPropConfigs(
                        properties, []([[maybe_unused]] StatusCode status,
                                       [[maybe_unused]] const hidl_vec<VehiclePropConfig>& c) {});
            },
            [&]() {
                mManager->getPropConfigs(
                        {mFuzzedDataProvider->ConsumeIntegral<int32_t>()},
                        []([[maybe_unused]] StatusCode status,
                           [[maybe_unused]] const hidl_vec<VehiclePropConfig>& c) {});
            },
            [&]() {
                mManager->getAllPropConfigs(
                        []([[maybe_unused]] const hidl_vec<VehiclePropConfig>& propConfigs) {});
            },

    });
    invokePropConfigsAPI();
}

void VehicleHalManagerFuzzer::invokeSubscribe() {
    int32_t vehicleProp2 = mFuzzedDataProvider->ConsumeIntegral<int32_t>();
    int32_t vehicleProp3 = mFuzzedDataProvider->ConsumeIntegral<int32_t>();

    sp<MockedVehicleCallback> cb = new MockedVehicleCallback();
    VehiclePropertyType type =
            static_cast<VehiclePropertyType>(mFuzzedDataProvider->ConsumeIntegral<int32_t>());

    auto invokeSubscribeAPI = mFuzzedDataProvider->PickValueInArray<const std::function<void()>>({
            [&]() {
                size_t size =
                        mFuzzedDataProvider->ConsumeIntegralInRange<size_t>(kMinSize, kMaxSize);
                hidl_vec<SubscribeOptions> options(size);
                for (size_t idx = 0; idx < size; ++idx) {
                    options[idx] = {SubscribeOptions{
                            .propId = mFuzzedDataProvider->ConsumeIntegral<int32_t>(),
                            .flags = static_cast<SubscribeFlags>(
                                    mFuzzedDataProvider->ConsumeIntegral<int32_t>())}};
                }
                mManager->subscribe(cb, options);
            },
            [&]() {
                auto unsubscribedValue = mObjectPool->obtain(type);
                if (!unsubscribedValue) {
                    return;
                }
                unsubscribedValue->prop = vehicleProp2;
                unsubscribedValue->value.int32Values[0] = INT32_MAX;
                mHal->sendPropEvent(std::move(unsubscribedValue));
                cb->waitForExpectedEvents(mFuzzedDataProvider->ConsumeIntegral<size_t>());
            },
            [&]() {
                const auto prop1 = mFuzzedDataProvider->ConsumeIntegral<int32_t>();
                mManager->unsubscribe(cb, prop1);
            },
            [&]() {
                mHal->sendHalError(StatusCode::TRY_AGAIN, vehicleProp3,
                                   mFuzzedDataProvider->ConsumeIntegral<int32_t>() /*areaId=*/);
            },

    });
    invokeSubscribeAPI();
}

void VehicleHalManagerFuzzer::invokeSetAndGetValues() {
    auto invokeSetAndGetAPI = mFuzzedDataProvider->PickValueInArray<const std::function<void()>>({
            [&]() {
                invokeGet(mFuzzedDataProvider->ConsumeIntegral<int32_t>(),
                          mFuzzedDataProvider->ConsumeIntegral<int32_t>());
            },
            [&]() { mObjectPool->obtainInt64(mFuzzedDataProvider->ConsumeIntegral<int64_t>()); },
            [&]() { mObjectPool->obtainFloat(mFuzzedDataProvider->ConsumeFloatingPoint<float>()); },
            [&]() { mObjectPool->obtainBoolean(mFuzzedDataProvider->ConsumeBool()); },
            [&]() {
                int32_t vehicleProp2 = mFuzzedDataProvider->ConsumeIntegral<int32_t>();
                auto expectedValue =
                        mObjectPool->obtainInt32(mFuzzedDataProvider->ConsumeIntegral<int32_t>());
                expectedValue->prop = vehicleProp2;
                expectedValue->areaId = mFuzzedDataProvider->ConsumeIntegral<int32_t>();
                mManager->set(*expectedValue.get());
            },
    });
    invokeSetAndGetAPI();
}

void VehicleHalManagerFuzzer::invokeObd2SensorStore() {
    size_t diagnosticInt = mFuzzedDataProvider->ConsumeIntegralInRange<size_t>(kMinSize, kMaxSize);
    size_t diagnosticFloat =
            mFuzzedDataProvider->ConsumeIntegralInRange<size_t>(kMinSize, kMaxSize);

    std::unique_ptr<Obd2SensorStore> sensorStore(
            new Obd2SensorStore(diagnosticInt, diagnosticFloat));

    if (!sensorStore.get()) {
        return;
    }

    auto invokeObd2SensorStoreAPI =
            mFuzzedDataProvider->PickValueInArray<const std::function<void()>>({
                    [&]() {
                        int32_t diagnosticIntValue =
                                mFuzzedDataProvider->ConsumeIntegral<int32_t>();
                        int32_t diagnosticIntIndex =
                                mFuzzedDataProvider->ConsumeIntegralInRange<int32_t>(
                                        kMinSize,
                                        toInt(DiagnosticIntegerSensorIndex::LAST_SYSTEM_INDEX) +
                                                diagnosticInt);
                        sensorStore->setIntegerSensor(
                                static_cast<DiagnosticIntegerSensorIndex>(diagnosticIntIndex),
                                diagnosticIntValue);
                    },
                    [&]() {
                        float diagnosticFloatValue =
                                mFuzzedDataProvider->ConsumeFloatingPoint<float>();
                        int32_t diagnosticFloatIndex =
                                mFuzzedDataProvider->ConsumeIntegralInRange<int32_t>(
                                        kMinSize,
                                        toInt(DiagnosticFloatSensorIndex::LAST_SYSTEM_INDEX) +
                                                diagnosticFloat);
                        sensorStore->setFloatSensor(
                                static_cast<DiagnosticFloatSensorIndex>(diagnosticFloatIndex),
                                diagnosticFloatValue);
                    },
                    [&]() { sensorStore->getIntegerSensors(); },
                    [&]() { sensorStore->getFloatSensors(); },
                    [&]() { sensorStore->getSensorsBitmask(); },
                    [&]() {
                        for (auto&& dtc : kSampleDtcs) {
                            VehiclePropertyType type = static_cast<VehiclePropertyType>(
                                    mFuzzedDataProvider->ConsumeIntegral<int32_t>());
                            auto freezeFrame = createVehiclePropValue(
                                    type, mFuzzedDataProvider->ConsumeIntegralInRange<int32_t>(
                                                  kMinSize, kMaxSize));
                            if (!freezeFrame.get()) {
                                return;
                            }
                            freezeFrame->prop = mFuzzedDataProvider->ConsumeIntegral<int32_t>();
                            sensorStore->fillPropValue(dtc, freezeFrame.get());
                        }
                    },
            });
    invokeObd2SensorStoreAPI();
}

void VehicleHalManagerFuzzer::invokeVmsUtils() {
    std::unique_ptr<VehiclePropValue> message;
    int32_t intValue = mFuzzedDataProvider->ConsumeIntegral<int32_t>();
    VmsLayer layer(mFuzzedDataProvider->ConsumeIntegral<int32_t>(),
                   mFuzzedDataProvider->ConsumeIntegral<int32_t>(),
                   mFuzzedDataProvider->ConsumeIntegral<int32_t>());
    VmsOffers offers = {
            intValue,
            {VmsLayerOffering(VmsLayer(mFuzzedDataProvider->ConsumeIntegral<int32_t>(),
                                       mFuzzedDataProvider->ConsumeIntegral<int32_t>(),
                                       mFuzzedDataProvider->ConsumeIntegral<int32_t>()))}};
    const VmsLayerAndPublisher layer_and_publisher(
            VmsLayer(mFuzzedDataProvider->ConsumeIntegral<int32_t>(),
                     mFuzzedDataProvider->ConsumeIntegral<int32_t>(),
                     mFuzzedDataProvider->ConsumeIntegral<int32_t>()),
            intValue);

    switch (mFuzzedDataProvider->ConsumeIntegralInRange<int32_t>(kMinSize, kMaxCaseMessage)) {
        case 0: {
            message = createSubscribeMessage(layer);
            break;
        }
        case 1: {
            message = createUnsubscribeMessage(layer);
            break;
        }
        case 2: {
            message = createSubscriptionsRequest();
            break;
        }
        case 3: {
            message = createOfferingMessage(offers);
            break;
        }
        case 4: {
            message = createAvailabilityRequest();
            break;
        }
        case 5: {
            std::string pub_bytes;
            if (mFuzzedDataProvider->ConsumeBool()) {
                pub_bytes = "pub_id";
            } else {
                pub_bytes = mFuzzedDataProvider->ConsumeRandomLengthString(kMaxFileSize);
            }
            message = createPublisherIdRequest(pub_bytes);
            break;
        }
        case 6: {
            std::string bytes = "placeholder";
            if (mFuzzedDataProvider->ConsumeBool()) {
                bytes = "placeholder";
            } else {
                bytes = mFuzzedDataProvider->ConsumeRandomLengthString(kMaxFileSize);
            }
            message = createDataMessageWithLayerPublisherInfo(layer_and_publisher, bytes);
            break;
        }
        case 7: {
            message = createBaseVmsMessage(
                    mFuzzedDataProvider->ConsumeIntegralInRange<size_t>(kMinSize, kMaxSize));
            break;
        }
        case 8: {
            message = createStartSessionMessage(intValue, intValue + 1);
            break;
        }
    }

    isValidVmsMessage(*message);
    message->value.int32Values =
            hidl_vec<int32_t>{mFuzzedDataProvider->ConsumeIntegral<int32_t>(), intValue};

    auto invokeVmsUtilsAPI = mFuzzedDataProvider->PickValueInArray<const std::function<void()>>({
            [&]() { parseData(*message); },
            [&]() { createSubscribeToPublisherMessage(layer_and_publisher); },
            [&]() { createUnsubscribeToPublisherMessage(layer_and_publisher); },
            [&]() { parsePublisherIdResponse(*message); },
            [&]() { getSequenceNumberForSubscriptionsState(*message); },
            [&]() { isSequenceNumberNewer(*message, intValue + 1); },
            [&]() {
                invokeGetSubscribedLayers(
                        (VmsMessageType)mFuzzedDataProvider->ConsumeIntegral<int32_t>());
            },
            [&]() { hasServiceNewlyStarted(*message); },
            [&]() { parseMessageType(*message); },
            [&]() { isAvailabilitySequenceNumberNewer(*message, intValue + 1); },
            [&]() { getSequenceNumberForAvailabilityState(*message); },
            [&]() {
                int32_t new_service_id;
                parseStartSessionMessage(*message, -1, 0, &new_service_id);
            },
    });
    invokeVmsUtilsAPI();
}

void VehicleHalManagerFuzzer::invokeGet(int32_t property, int32_t areaId) {
    VehiclePropValue requestedValue{};
    requestedValue.prop = property;
    requestedValue.areaId = areaId;
    mActualValue = VehiclePropValue{};  // reset previous values

    StatusCode refStatus;
    VehiclePropValue refValue;
    mManager->get(requestedValue,
                  [&refStatus, &refValue](StatusCode status, const VehiclePropValue& value) {
                      refStatus = status;
                      refValue = value;
                  });

    mActualValue = refValue;
    mActualStatusCode = refStatus;
}

void VehicleHalManagerFuzzer::invokeGetSubscribedLayers(VmsMessageType /*type*/) {
    int32_t intValue = mFuzzedDataProvider->ConsumeIntegral<int32_t>();
    VmsOffers offers = {
            intValue,
            {VmsLayerOffering(VmsLayer(mFuzzedDataProvider->ConsumeIntegral<int32_t>(),
                                       mFuzzedDataProvider->ConsumeIntegral<int32_t>(),
                                       mFuzzedDataProvider->ConsumeIntegral<int32_t>()))}};
    auto message = createBaseVmsMessage(
            mFuzzedDataProvider->ConsumeIntegralInRange<size_t>(kMinSize, kMaxFileSize));
    std::vector<int32_t> v;
    size_t size = mFuzzedDataProvider->ConsumeIntegralInRange<size_t>(kMinSize, kMaxSize);
    for (size_t i = 0; i < size; i++) {
        v.push_back(mFuzzedDataProvider->ConsumeIntegralInRange<int32_t>(kMinSize, kMaxSize));
    }

    message->value.int32Values = hidl_vec<int32_t>(v);
    if (!isValidVmsMessage(*message)) {
        return;
    }

    if (mFuzzedDataProvider->ConsumeBool()) {
        getSubscribedLayers(*message, offers);
    } else {
        getAvailableLayers(*message);
    }
}

void VehicleHalManagerFuzzer::invokeVehiclePropStore() {
    bool shouldWriteStatus = mFuzzedDataProvider->ConsumeBool();
    int32_t vehicleProp = mFuzzedDataProvider->ConsumeIntegral<int32_t>();
    auto store = std::make_unique<VehiclePropertyStore>();
    VehiclePropConfig config{
            .prop = vehicleProp,
            .access = VehiclePropertyAccess::READ,
            .changeMode = VehiclePropertyChangeMode::STATIC,
            .areaConfigs = {VehicleAreaConfig{
                    .areaId = (mFuzzedDataProvider->ConsumeIntegral<int32_t>())}},
    };
    VehiclePropValue propValue{};
    propValue.prop = vehicleProp;
    propValue.areaId = mFuzzedDataProvider->ConsumeIntegral<int32_t>();

    auto invokeVehiclePropStoreAPI =
            mFuzzedDataProvider->PickValueInArray<const std::function<void()>>({
                    [&]() { store->registerProperty(config); },
                    [&]() { store->writeValue(propValue, shouldWriteStatus); },
                    [&]() { store->readAllValues(); },
                    [&]() { store->getAllConfigs(); },
                    [&]() { store->getConfigOrNull(vehicleProp); },
                    [&]() { store->readValuesForProperty(vehicleProp); },
                    [&]() { store->readValueOrNull(propValue); },
                    [&]() {
                        store->readValueOrNull(propValue.prop, propValue.areaId,
                                               mFuzzedDataProvider->ConsumeIntegralInRange<int64_t>(
                                                       kMinSize, kMaxFileSize));
                    },
                    [&]() { store->removeValuesForProperty(vehicleProp); },
                    [&]() { store->removeValue(propValue); },
                    [&]() {
                        if (store->getConfigOrNull(vehicleProp)) {
                            store->getConfigOrDie(vehicleProp);
                        }
                    },
            });
    invokeVehiclePropStoreAPI();
}

void VehicleHalManagerFuzzer::invokeWatchDogClient() {
    sp<Looper> looper(Looper::prepare(/*opts=*/mFuzzedDataProvider->ConsumeBool()));
    if (auto watchdogClient = ndk::SharedRefBase::make<WatchdogClient>(looper, mManager.get());
        watchdogClient->initialize()) {
        if (mFuzzedDataProvider->ConsumeBool()) {
            watchdogClient->checkIfAlive(
                    mFuzzedDataProvider->ConsumeIntegral<int32_t>(),
                    (TimeoutLength)mFuzzedDataProvider->ConsumeIntegral<int32_t>());
        }
        watchdogClient->prepareProcessTermination();
    }
}

extern "C" int LLVMFuzzerTestOneInput(const uint8_t* data, size_t size) {
    VehicleHalManagerFuzzer vmFuzzer;
    vmFuzzer.process(data, size);
    return 0;
}

}  // namespace android::hardware::automotive::vehicle::V2_0::fuzzer
