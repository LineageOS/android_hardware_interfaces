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

constexpr const char kCarMake[] = "Default Car";
constexpr VehicleProperty kVehicleProp[] = {VehicleProperty::INVALID,
                                            VehicleProperty::HVAC_FAN_SPEED,
                                            VehicleProperty::INFO_MAKE,
                                            VehicleProperty::DISPLAY_BRIGHTNESS,
                                            VehicleProperty::INFO_FUEL_CAPACITY,
                                            VehicleProperty::HVAC_SEAT_TEMPERATURE};
constexpr DiagnosticIntegerSensorIndex kDiagnosticIntIndex[] = {
        DiagnosticIntegerSensorIndex::FUEL_SYSTEM_STATUS,
        DiagnosticIntegerSensorIndex::MALFUNCTION_INDICATOR_LIGHT_ON,
        DiagnosticIntegerSensorIndex::NUM_OXYGEN_SENSORS_PRESENT,
        DiagnosticIntegerSensorIndex::FUEL_TYPE};
constexpr DiagnosticFloatSensorIndex kDiagnosticFloatIndex[] = {
        DiagnosticFloatSensorIndex::CALCULATED_ENGINE_LOAD,
        DiagnosticFloatSensorIndex::SHORT_TERM_FUEL_TRIM_BANK1,
        DiagnosticFloatSensorIndex::LONG_TERM_FUEL_TRIM_BANK1,
        DiagnosticFloatSensorIndex::THROTTLE_POSITION};
constexpr size_t kVehiclePropArrayLength = std::size(kVehicleProp);
constexpr size_t kIntSensorArrayLength = std::size(kDiagnosticIntIndex);
constexpr size_t kFloatSensorArrayLength = std::size(kDiagnosticFloatIndex);
constexpr VmsMessageType kAvailabilityMessageType[] = {VmsMessageType::AVAILABILITY_CHANGE,
                                                       VmsMessageType::AVAILABILITY_RESPONSE};
constexpr VmsMessageType kSubscriptionMessageType[] = {VmsMessageType::SUBSCRIPTIONS_CHANGE,
                                                       VmsMessageType::SUBSCRIPTIONS_RESPONSE};

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
            pValue = getValuePool()->obtainString(kCarMake);
            break;
        case VehicleProperty::INFO_FUEL_CAPACITY:
            if (mFuelCapacityAttemptsLeft-- > 0) {
                *outStatus = StatusCode::TRY_AGAIN;
            } else {
                pValue = getValuePool()->obtainFloat(42.42);
            }
            break;
        default:
            if (requestedPropValue.prop == kCustomComplexProperty) {
                pValue = getValuePool()->obtainComplex();
                pValue->value.int32Values = hidl_vec<int32_t>{10, 20};
                pValue->value.int64Values = hidl_vec<int64_t>{30, 40};
                pValue->value.floatValues = hidl_vec<float_t>{1.1, 2.2};
                pValue->value.bytes = hidl_vec<uint8_t>{1, 2, 3};
                pValue->value.stringValue = kCarMake;
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

void VehicleHalManagerFuzzer::process(const uint8_t* data, size_t size) {
    mFuzzedDataProvider = new FuzzedDataProvider(data, size);
    invokeDebug();
    invokePropConfigs();
    invokeSubscribe();
    invokeSetAndGetValues();
    invokeObd2SensorStore();
    invokeVmsUtils();
    invokeVehiclePropStore();
    invokeWatchDogClient();
}

void VehicleHalManagerFuzzer::invokeDebug() {
    hidl_string debugOption = mFuzzedDataProvider->PickValueInArray(
            {"--help", "--list", "--get", "--set", "", "invalid"});
    hidl_handle fd = {};
    fd.setTo(native_handle_create(/*numFds=*/1, /*numInts=*/0), /*shouldOwn=*/true);

    mManager->debug(fd, {});
    mManager->debug(fd, {debugOption});
}

void VehicleHalManagerFuzzer::invokePropConfigs() {
    int32_t vehicleProp1 = mFuzzedDataProvider->ConsumeIntegral<int32_t>();
    int32_t vehicleProp2 = mFuzzedDataProvider->ConsumeIntegral<int32_t>();

    hidl_vec<int32_t> properties = {vehicleProp1, vehicleProp2};

    mManager->getPropConfigs(properties,
                             []([[maybe_unused]] StatusCode status,
                                [[maybe_unused]] const hidl_vec<VehiclePropConfig>& c) {});

    mManager->getPropConfigs({toInt(kVehicleProp[abs(vehicleProp1) % kVehiclePropArrayLength])},
                             []([[maybe_unused]] StatusCode status,
                                [[maybe_unused]] const hidl_vec<VehiclePropConfig>& c) {});

    mManager->getAllPropConfigs(
            []([[maybe_unused]] const hidl_vec<VehiclePropConfig>& propConfigs) {});
}

void VehicleHalManagerFuzzer::invokeSubscribe() {
    int32_t vehicleProp1 = mFuzzedDataProvider->ConsumeIntegral<int32_t>();
    int32_t vehicleProp2 = mFuzzedDataProvider->ConsumeIntegral<int32_t>();
    int32_t vehicleProp3 = mFuzzedDataProvider->ConsumeIntegral<int32_t>();

    const auto prop1 = toInt(kVehicleProp[abs(vehicleProp1) % kVehiclePropArrayLength]);
    sp<MockedVehicleCallback> cb = new MockedVehicleCallback();

    hidl_vec<SubscribeOptions> options = {
            SubscribeOptions{.propId = prop1, .flags = SubscribeFlags::EVENTS_FROM_CAR}};

    mManager->subscribe(cb, options);

    auto unsubscribedValue = mObjectPool->obtain(VehiclePropertyType::INT32);
    unsubscribedValue->prop = toInt(kVehicleProp[abs(vehicleProp2) % kVehiclePropArrayLength]);

    mHal->sendPropEvent(std::move(unsubscribedValue));
    cb->getReceivedEvents();
    cb->waitForExpectedEvents(0);

    auto subscribedValue = mObjectPool->obtain(VehiclePropertyType::INT32);
    subscribedValue->prop = toInt(kVehicleProp[abs(vehicleProp2) % kVehiclePropArrayLength]);
    subscribedValue->value.int32Values[0] = INT32_MAX;

    cb->reset();
    VehiclePropValue actualValue(*subscribedValue.get());
    mHal->sendPropEvent(std::move(subscribedValue));
    cb->waitForExpectedEvents(1);
    mManager->unsubscribe(cb, prop1);

    sp<MockedVehicleCallback> cb2 = new MockedVehicleCallback();

    hidl_vec<SubscribeOptions> options2 = {
            SubscribeOptions{
                    .propId = toInt(kVehicleProp[abs(vehicleProp3) % kVehiclePropArrayLength]),
                    .flags = SubscribeFlags::EVENTS_FROM_CAR},
    };

    mManager->subscribe(cb2, options2);

    mHal->sendHalError(StatusCode::TRY_AGAIN,
                       toInt(kVehicleProp[abs(vehicleProp3) % kVehiclePropArrayLength]),
                       /*areaId=*/0);
}

void VehicleHalManagerFuzzer::invokeSetAndGetValues() {
    uint32_t vehicleProp1 =
            mFuzzedDataProvider->ConsumeIntegralInRange<uint32_t>(0, kVehiclePropArrayLength - 1);
    uint32_t vehicleProp2 =
            mFuzzedDataProvider->ConsumeIntegralInRange<uint32_t>(0, kVehiclePropArrayLength - 1);
    uint32_t vehicleProp3 =
            mFuzzedDataProvider->ConsumeIntegralInRange<uint32_t>(0, kVehiclePropArrayLength - 1);

    invokeGet(kCustomComplexProperty, 0);
    invokeGet(toInt(kVehicleProp[vehicleProp2]), 0);
    invokeGet(toInt(kVehicleProp[vehicleProp1]), 0);

    auto expectedValue = mObjectPool->obtainInt32(mFuzzedDataProvider->ConsumeIntegral<int32_t>());
    mObjectPool->obtainInt64(mFuzzedDataProvider->ConsumeIntegral<int64_t>());
    mObjectPool->obtainFloat(mFuzzedDataProvider->ConsumeFloatingPoint<float>());
    mObjectPool->obtainBoolean(mFuzzedDataProvider->ConsumeBool());
    expectedValue->prop = toInt(kVehicleProp[vehicleProp2]);
    expectedValue->areaId = 0;

    mManager->set(*expectedValue.get());
    invokeGet(toInt(kVehicleProp[vehicleProp2]), 0);
    expectedValue->prop = toInt(kVehicleProp[vehicleProp3]);
    mManager->set(*expectedValue.get());
    expectedValue->prop = toInt(VehicleProperty::INVALID);
    mManager->set(*expectedValue.get());
}

void VehicleHalManagerFuzzer::invokeObd2SensorStore() {
    uint32_t diagnosticIntIndex =
            mFuzzedDataProvider->ConsumeIntegralInRange<uint32_t>(0, kIntSensorArrayLength - 1);
    int32_t diagnosticIntValue = mFuzzedDataProvider->ConsumeIntegral<int32_t>();
    uint32_t diagnosticFloatIndex =
            mFuzzedDataProvider->ConsumeIntegralInRange<uint32_t>(0, kFloatSensorArrayLength - 1);
    float diagnosticFloatValue = mFuzzedDataProvider->ConsumeFloatingPoint<float>();

    std::unique_ptr<Obd2SensorStore> sensorStore(
            new Obd2SensorStore(kIntSensorArrayLength, kFloatSensorArrayLength));
    if (sensorStore) {
        sensorStore->setIntegerSensor(kDiagnosticIntIndex[diagnosticIntIndex], diagnosticIntValue);
        sensorStore->setFloatSensor(kDiagnosticFloatIndex[diagnosticFloatIndex],
                                    diagnosticFloatValue);
        sensorStore->getIntegerSensors();
        sensorStore->getFloatSensors();
        sensorStore->getSensorsBitmask();
        static std::vector<std::string> sampleDtcs = {"P0070",
                                                      "P0102"
                                                      "P0123"};
        for (auto&& dtc : sampleDtcs) {
            auto freezeFrame = createVehiclePropValue(VehiclePropertyType::MIXED, 0);
            sensorStore->fillPropValue(dtc, freezeFrame.get());
            freezeFrame->prop = static_cast<int>(VehicleProperty::OBD2_FREEZE_FRAME);
        }
    }
}

void VehicleHalManagerFuzzer::invokeVmsUtils() {
    bool availabilityMsgType = mFuzzedDataProvider->ConsumeBool();
    bool subscriptionMsgType = mFuzzedDataProvider->ConsumeBool();
    int32_t intValue = mFuzzedDataProvider->ConsumeIntegral<int32_t>();

    VmsLayer layer(1, 0, 2);
    auto message = createSubscribeMessage(layer);
    isValidVmsMessage(*message);
    message = createUnsubscribeMessage(layer);

    VmsOffers offers = {intValue, {VmsLayerOffering(VmsLayer(1, 0, 2))}};
    message = createOfferingMessage(offers);
    std::vector<VmsLayer> dependencies = {VmsLayer(2, 0, 2), VmsLayer(3, 0, 3)};
    std::vector<VmsLayerOffering> offering = {VmsLayerOffering(layer, dependencies)};
    offers = {intValue, offering};
    message = createOfferingMessage(offers);

    message = createAvailabilityRequest();
    message = createSubscriptionsRequest();

    std::string bytes = "placeholder";
    const VmsLayerAndPublisher layer_and_publisher(VmsLayer(2, 0, 1), intValue);
    message = createDataMessageWithLayerPublisherInfo(layer_and_publisher, bytes);
    parseData(*message);
    createSubscribeToPublisherMessage(layer_and_publisher);
    createUnsubscribeToPublisherMessage(layer_and_publisher);

    std::string pub_bytes = "pub_id";
    message = createPublisherIdRequest(pub_bytes);
    message = createBaseVmsMessage(2);
    message->value.int32Values =
            hidl_vec<int32_t>{toInt(VmsMessageType::PUBLISHER_ID_RESPONSE), intValue};
    parsePublisherIdResponse(*message);

    message->value.int32Values =
            hidl_vec<int32_t>{toInt(kSubscriptionMessageType[subscriptionMsgType]), intValue};
    getSequenceNumberForSubscriptionsState(*message);

    message->value.int32Values = hidl_vec<int32_t>{toInt(kSubscriptionMessageType[0]), intValue};
    isSequenceNumberNewer(*message, intValue + 1);
    invokeGetSubscribedLayers(kSubscriptionMessageType[subscriptionMsgType]);

    message->value.int32Values =
            hidl_vec<int32_t>{toInt(kAvailabilityMessageType[availabilityMsgType]), 0};
    hasServiceNewlyStarted(*message);
    message = createStartSessionMessage(intValue, intValue + 1);
    parseMessageType(*message);

    message->value.int32Values =
            hidl_vec<int32_t>{toInt(kAvailabilityMessageType[availabilityMsgType]), intValue};
    isAvailabilitySequenceNumberNewer(*message, intValue + 1);

    message->value.int32Values =
            hidl_vec<int32_t>{toInt(kAvailabilityMessageType[availabilityMsgType]), intValue};
    getSequenceNumberForAvailabilityState(*message);
    message = createBaseVmsMessage(3);
    int new_service_id;
    message->value.int32Values = hidl_vec<int32_t>{toInt(VmsMessageType::START_SESSION), 0, -1};
    parseStartSessionMessage(*message, -1, 0, &new_service_id);
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

void VehicleHalManagerFuzzer::invokeGetSubscribedLayers(VmsMessageType type) {
    VmsOffers offers = {123,
                        {VmsLayerOffering(VmsLayer(1, 0, 1), {VmsLayer(4, 1, 1)}),
                         VmsLayerOffering(VmsLayer(2, 0, 1))}};
    auto message = createBaseVmsMessage(16);
    message->value.int32Values = hidl_vec<int32_t>{toInt(type),
                                                   1234,  // sequence number
                                                   2,     // number of layers
                                                   1,     // number of associated layers
                                                   1,     // layer 1
                                                   0,           1,
                                                   4,  // layer 2
                                                   1,           1,
                                                   2,  // associated layer
                                                   0,           1,
                                                   2,    // number of publisher IDs
                                                   111,  // publisher IDs
                                                   123};
    isValidVmsMessage(*message);
    getSubscribedLayers(*message, offers);
    getAvailableLayers(*message);
}

void VehicleHalManagerFuzzer::invokeVehiclePropStore() {
    bool shouldWriteStatus = mFuzzedDataProvider->ConsumeBool();
    int32_t vehicleProp = mFuzzedDataProvider->ConsumeIntegral<int32_t>();
    auto store = std::make_unique<VehiclePropertyStore>();
    VehiclePropConfig config{
            .prop = vehicleProp,
            .access = VehiclePropertyAccess::READ,
            .changeMode = VehiclePropertyChangeMode::STATIC,
            .areaConfigs = {VehicleAreaConfig{.areaId = (0)}},
    };
    store->registerProperty(config);
    VehiclePropValue propValue{};
    propValue.prop = vehicleProp;
    propValue.areaId = 0;
    store->writeValue(propValue, shouldWriteStatus);
    store->readAllValues();
    store->getAllConfigs();
    store->getConfigOrNull(vehicleProp);
    store->readValuesForProperty(vehicleProp);
    store->readValueOrNull(propValue);
    store->readValueOrNull(propValue.prop, propValue.areaId, 0);
    store->removeValuesForProperty(vehicleProp);
    store->removeValue(propValue);
    store->getConfigOrDie(vehicleProp);
}

void VehicleHalManagerFuzzer::invokeWatchDogClient() {
    auto service = new VehicleHalManager(mHal.get());
    sp<Looper> looper(Looper::prepare(/*opts=*/mFuzzedDataProvider->ConsumeBool()));
    if (auto watchdogClient = ndk::SharedRefBase::make<WatchdogClient>(looper, service);
        watchdogClient->initialize()) {
        watchdogClient->checkIfAlive(-1, TimeoutLength::TIMEOUT_NORMAL);
        watchdogClient->prepareProcessTermination();
    }
    delete service;
}

extern "C" int LLVMFuzzerTestOneInput(const uint8_t* data, size_t size) {
    VehicleHalManagerFuzzer vmFuzzer;
    vmFuzzer.process(data, size);
    return 0;
}

}  // namespace android::hardware::automotive::vehicle::V2_0::fuzzer
