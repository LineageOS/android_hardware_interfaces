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

#define LOG_TAG "DefaultVehicleHal"

#include <DefaultVehicleHal.h>

#include <LargeParcelableBase.h>
#include <VehicleHalTypes.h>
#include <VehicleUtils.h>

#include <android-base/result.h>
#include <android-base/stringprintf.h>
#include <utils/Log.h>
#include <utils/SystemClock.h>

#include <inttypes.h>
#include <set>
#include <unordered_set>

namespace android {
namespace hardware {
namespace automotive {
namespace vehicle {

namespace {

using ::aidl::android::hardware::automotive::vehicle::GetValueRequest;
using ::aidl::android::hardware::automotive::vehicle::GetValueRequests;
using ::aidl::android::hardware::automotive::vehicle::GetValueResult;
using ::aidl::android::hardware::automotive::vehicle::GetValueResults;
using ::aidl::android::hardware::automotive::vehicle::IVehicleCallback;
using ::aidl::android::hardware::automotive::vehicle::SetValueRequest;
using ::aidl::android::hardware::automotive::vehicle::SetValueRequests;
using ::aidl::android::hardware::automotive::vehicle::SetValueResult;
using ::aidl::android::hardware::automotive::vehicle::SetValueResults;
using ::aidl::android::hardware::automotive::vehicle::StatusCode;
using ::aidl::android::hardware::automotive::vehicle::SubscribeOptions;
using ::aidl::android::hardware::automotive::vehicle::VehicleAreaConfig;
using ::aidl::android::hardware::automotive::vehicle::VehiclePropConfig;
using ::aidl::android::hardware::automotive::vehicle::VehiclePropConfigs;
using ::aidl::android::hardware::automotive::vehicle::VehicleProperty;
using ::aidl::android::hardware::automotive::vehicle::VehiclePropertyAccess;
using ::aidl::android::hardware::automotive::vehicle::VehiclePropertyChangeMode;
using ::aidl::android::hardware::automotive::vehicle::VehiclePropertyStatus;
using ::aidl::android::hardware::automotive::vehicle::VehiclePropValue;
using ::android::automotive::car_binder_lib::LargeParcelableBase;
using ::android::base::Error;
using ::android::base::expected;
using ::android::base::Result;
using ::android::base::StringPrintf;
using ::ndk::ScopedAStatus;

std::string toString(const std::unordered_set<int64_t>& values) {
    std::string str = "";
    for (auto it = values.begin(); it != values.end(); it++) {
        str += std::to_string(*it);
        if (std::next(it, 1) != values.end()) {
            str += ", ";
        }
    }
    return str;
}

}  // namespace

std::shared_ptr<SubscriptionClient> DefaultVehicleHal::SubscriptionClients::getClient(
        const CallbackType& callback) {
    std::scoped_lock<std::mutex> lockGuard(mLock);
    return getOrCreateClient(&mClients, callback, mPendingRequestPool);
}

int64_t DefaultVehicleHal::SubscribeIdByClient::getId(const CallbackType& callback) {
    std::scoped_lock<std::mutex> lockGuard(mLock);
    // This would be initialized to 0 if callback does not exist in the map.
    int64_t subscribeId = (mIds[callback])++;
    return subscribeId;
}

size_t DefaultVehicleHal::SubscriptionClients::countClients() {
    std::scoped_lock<std::mutex> lockGuard(mLock);
    return mClients.size();
}

DefaultVehicleHal::DefaultVehicleHal(std::unique_ptr<IVehicleHardware> hardware)
    : mVehicleHardware(std::move(hardware)),
      mPendingRequestPool(std::make_shared<PendingRequestPool>(TIMEOUT_IN_NANO)) {
    auto configs = mVehicleHardware->getAllPropertyConfigs();
    for (auto& config : configs) {
        mConfigsByPropId[config.prop] = config;
    }
    VehiclePropConfigs vehiclePropConfigs;
    vehiclePropConfigs.payloads = std::move(configs);
    auto result = LargeParcelableBase::parcelableToStableLargeParcelable(vehiclePropConfigs);
    if (!result.ok()) {
        ALOGE("failed to convert configs to shared memory file, error: %s, code: %d",
              getErrorMsg(result).c_str(), getIntErrorCode(result));
        return;
    }

    if (result.value() != nullptr) {
        mConfigFile = std::move(result.value());
    }

    mSubscriptionClients = std::make_shared<SubscriptionClients>(mPendingRequestPool);

    auto subscribeIdByClient = std::make_shared<SubscribeIdByClient>();
    // Make a weak copy of IVehicleHardware because subscriptionManager uses IVehicleHardware and
    // IVehicleHardware uses subscriptionManager. We want to avoid cyclic reference.
    std::weak_ptr<IVehicleHardware> hardwareCopy = mVehicleHardware;
    SubscriptionManager::GetValueFunc getValueFunc = std::bind(
            &DefaultVehicleHal::getValueFromHardwareCallCallback, hardwareCopy, subscribeIdByClient,
            mSubscriptionClients, std::placeholders::_1, std::placeholders::_2);
    mSubscriptionManager = std::make_shared<SubscriptionManager>(std::move(getValueFunc));

    std::weak_ptr<SubscriptionManager> subscriptionManagerCopy = mSubscriptionManager;
    mVehicleHardware->registerOnPropertyChangeEvent(
            std::make_unique<IVehicleHardware::PropertyChangeCallback>(
                    [subscriptionManagerCopy](std::vector<VehiclePropValue> updatedValues) {
                        onPropertyChangeEvent(subscriptionManagerCopy, updatedValues);
                    }));

    // Register heartbeat event.
    mRecurrentTimer.registerTimerCallback(
            HEART_BEAT_INTERVAL_IN_NANO,
            std::make_shared<std::function<void()>>([hardwareCopy, subscriptionManagerCopy]() {
                checkHealth(hardwareCopy, subscriptionManagerCopy);
            }));
}

void DefaultVehicleHal::onPropertyChangeEvent(
        std::weak_ptr<SubscriptionManager> subscriptionManager,
        const std::vector<VehiclePropValue>& updatedValues) {
    auto manager = subscriptionManager.lock();
    if (manager == nullptr) {
        ALOGW("the SubscriptionManager is destroyed, DefaultVehicleHal is ending");
        return;
    }
    auto updatedValuesByClients = manager->getSubscribedClients(updatedValues);
    for (const auto& [callback, valuePtrs] : updatedValuesByClients) {
        std::vector<VehiclePropValue> values;
        for (const VehiclePropValue* valuePtr : valuePtrs) {
            values.push_back(*valuePtr);
        }
        SubscriptionClient::sendUpdatedValues(callback, std::move(values));
    }
}

template <class T>
std::shared_ptr<T> DefaultVehicleHal::getOrCreateClient(
        std::unordered_map<CallbackType, std::shared_ptr<T>>* clients, const CallbackType& callback,
        std::shared_ptr<PendingRequestPool> pendingRequestPool) {
    if (clients->find(callback) == clients->end()) {
        // TODO(b/204943359): Remove client from clients when linkToDeath is implemented.
        (*clients)[callback] = std::make_shared<T>(pendingRequestPool, callback);
    }
    return (*clients)[callback];
}

template std::shared_ptr<DefaultVehicleHal::GetValuesClient>
DefaultVehicleHal::getOrCreateClient<DefaultVehicleHal::GetValuesClient>(
        std::unordered_map<CallbackType, std::shared_ptr<GetValuesClient>>* clients,
        const CallbackType& callback, std::shared_ptr<PendingRequestPool> pendingRequestPool);
template std::shared_ptr<DefaultVehicleHal::SetValuesClient>
DefaultVehicleHal::getOrCreateClient<DefaultVehicleHal::SetValuesClient>(
        std::unordered_map<CallbackType, std::shared_ptr<SetValuesClient>>* clients,
        const CallbackType& callback, std::shared_ptr<PendingRequestPool> pendingRequestPool);
template std::shared_ptr<SubscriptionClient>
DefaultVehicleHal::getOrCreateClient<SubscriptionClient>(
        std::unordered_map<CallbackType, std::shared_ptr<SubscriptionClient>>* clients,
        const CallbackType& callback, std::shared_ptr<PendingRequestPool> pendingRequestPool);

void DefaultVehicleHal::getValueFromHardwareCallCallback(
        std::weak_ptr<IVehicleHardware> vehicleHardware,
        std::shared_ptr<SubscribeIdByClient> subscribeIdByClient,
        std::shared_ptr<SubscriptionClients> subscriptionClients, const CallbackType& callback,
        const VehiclePropValue& value) {
    int64_t subscribeId = subscribeIdByClient->getId(callback);
    auto client = subscriptionClients->getClient(callback);
    if (auto addRequestResult = client->addRequests({subscribeId}); !addRequestResult.ok()) {
        ALOGE("subscribe[%" PRId64 "]: too many pending requests, ignore the getValue request",
              subscribeId);
        return;
    }

    std::vector<GetValueRequest> hardwareRequests = {{
            .requestId = subscribeId,
            .prop = value,
    }};

    std::shared_ptr<IVehicleHardware> hardware = vehicleHardware.lock();
    if (hardware == nullptr) {
        ALOGW("the IVehicleHardware is destroyed, DefaultVehicleHal is ending");
        return;
    }
    if (StatusCode status = hardware->getValues(client->getResultCallback(), hardwareRequests);
        status != StatusCode::OK) {
        // If the hardware returns error, finish all the pending requests for this request because
        // we never expect hardware to call callback for these requests.
        client->tryFinishRequests({subscribeId});
        ALOGE("subscribe[%" PRId64 "]: failed to get value from VehicleHardware, code: %d",
              subscribeId, toInt(status));
    }
}

void DefaultVehicleHal::setTimeout(int64_t timeoutInNano) {
    mPendingRequestPool = std::make_unique<PendingRequestPool>(timeoutInNano);
}

ScopedAStatus DefaultVehicleHal::getAllPropConfigs(VehiclePropConfigs* output) {
    if (mConfigFile != nullptr) {
        output->payloads.clear();
        output->sharedMemoryFd.set(dup(mConfigFile->get()));
        return ScopedAStatus::ok();
    }
    output->payloads.reserve(mConfigsByPropId.size());
    for (const auto& [_, config] : mConfigsByPropId) {
        output->payloads.push_back(config);
    }
    return ScopedAStatus::ok();
}

Result<const VehiclePropConfig*> DefaultVehicleHal::getConfig(int32_t propId) const {
    auto it = mConfigsByPropId.find(propId);
    if (it == mConfigsByPropId.end()) {
        return Error() << "no config for property, ID: " << propId;
    }
    return &(it->second);
}

Result<void> DefaultVehicleHal::checkProperty(const VehiclePropValue& propValue) {
    int32_t propId = propValue.prop;
    auto result = getConfig(propId);
    if (!result.ok()) {
        return result.error();
    }
    const VehiclePropConfig* config = result.value();
    const VehicleAreaConfig* areaConfig = getAreaConfig(propValue, *config);
    if (!isGlobalProp(propId) && areaConfig == nullptr) {
        // Ignore areaId for global property. For non global property, check whether areaId is
        // allowed. areaId must appear in areaConfig.
        return Error() << "invalid area ID: " << propValue.areaId << " for prop ID: " << propId
                       << ", not listed in config";
    }
    if (auto result = checkPropValue(propValue, config); !result.ok()) {
        return Error() << "invalid property value: " << propValue.toString()
                       << ", error: " << getErrorMsg(result);
    }
    if (auto result = checkValueRange(propValue, areaConfig); !result.ok()) {
        return Error() << "property value out of range: " << propValue.toString()
                       << ", error: " << getErrorMsg(result);
    }
    return {};
}

ScopedAStatus DefaultVehicleHal::getValues(const CallbackType& callback,
                                           const GetValueRequests& requests) {
    expected<LargeParcelableBase::BorrowedOwnedObject<GetValueRequests>, ScopedAStatus>
            deserializedResults = fromStableLargeParcelable(requests);
    if (!deserializedResults.ok()) {
        ALOGE("getValues: failed to parse getValues requests");
        return std::move(deserializedResults.error());
    }
    const std::vector<GetValueRequest>& getValueRequests =
            deserializedResults.value().getObject()->payloads;

    auto maybeRequestIds = checkDuplicateRequests(getValueRequests);
    if (!maybeRequestIds.ok()) {
        ALOGE("getValues: duplicate request ID");
        return toScopedAStatus(maybeRequestIds, StatusCode::INVALID_ARG);
    }

    // A list of failed result we already know before sending to hardware.
    std::vector<GetValueResult> failedResults;
    // The list of requests that we would send to hardware.
    std::vector<GetValueRequest> hardwareRequests;

    for (const auto& request : getValueRequests) {
        if (auto result = checkReadPermission(request.prop); !result.ok()) {
            ALOGW("property does not support reading: %s", getErrorMsg(result).c_str());
            failedResults.push_back(GetValueResult{
                    .requestId = request.requestId,
                    .status = getErrorCode(result),
                    .prop = {},
            });
        } else {
            hardwareRequests.push_back(request);
        }
    }

    // The set of request Ids that we would send to hardware.
    std::unordered_set<int64_t> hardwareRequestIds;
    for (const auto& request : hardwareRequests) {
        hardwareRequestIds.insert(request.requestId);
    }

    std::shared_ptr<GetValuesClient> client;
    {
        std::scoped_lock<std::mutex> lockGuard(mLock);
        client = getOrCreateClient(&mGetValuesClients, callback, mPendingRequestPool);
    }
    // Register the pending hardware requests and also check for duplicate request Ids.
    if (auto addRequestResult = client->addRequests(hardwareRequestIds); !addRequestResult.ok()) {
        ALOGE("getValues[%s]: failed to add pending requests, error: %s",
              toString(hardwareRequestIds).c_str(), getErrorMsg(addRequestResult).c_str());
        return toScopedAStatus(addRequestResult);
    }

    if (!failedResults.empty()) {
        // First send the failed results we already know back to the client.
        client->sendResults(failedResults);
    }

    if (hardwareRequests.empty()) {
        return ScopedAStatus::ok();
    }

    if (StatusCode status =
                mVehicleHardware->getValues(client->getResultCallback(), hardwareRequests);
        status != StatusCode::OK) {
        // If the hardware returns error, finish all the pending requests for this request because
        // we never expect hardware to call callback for these requests.
        client->tryFinishRequests(hardwareRequestIds);
        ALOGE("getValues[%s]: failed to get value from VehicleHardware, status: %d",
              toString(hardwareRequestIds).c_str(), toInt(status));
        return ScopedAStatus::fromServiceSpecificErrorWithMessage(
                toInt(status), "failed to get value from VehicleHardware");
    }
    return ScopedAStatus::ok();
}

ScopedAStatus DefaultVehicleHal::setValues(const CallbackType& callback,
                                           const SetValueRequests& requests) {
    expected<LargeParcelableBase::BorrowedOwnedObject<SetValueRequests>, ScopedAStatus>
            deserializedResults = fromStableLargeParcelable(requests);
    if (!deserializedResults.ok()) {
        ALOGE("setValues: failed to parse setValues requests");
        return std::move(deserializedResults.error());
    }
    const std::vector<SetValueRequest>& setValueRequests =
            deserializedResults.value().getObject()->payloads;

    // A list of failed result we already know before sending to hardware.
    std::vector<SetValueResult> failedResults;
    // The list of requests that we would send to hardware.
    std::vector<SetValueRequest> hardwareRequests;

    auto maybeRequestIds = checkDuplicateRequests(setValueRequests);
    if (!maybeRequestIds.ok()) {
        ALOGE("setValues: duplicate request ID");
        return toScopedAStatus(maybeRequestIds, StatusCode::INVALID_ARG);
    }

    for (auto& request : setValueRequests) {
        int64_t requestId = request.requestId;
        if (auto result = checkWritePermission(request.value); !result.ok()) {
            ALOGW("property does not support writing: %s", getErrorMsg(result).c_str());
            failedResults.push_back(SetValueResult{
                    .requestId = requestId,
                    .status = getErrorCode(result),
            });
            continue;
        }
        if (auto result = checkProperty(request.value); !result.ok()) {
            ALOGW("setValues[%" PRId64 "]: property is not valid: %s", requestId,
                  getErrorMsg(result).c_str());
            failedResults.push_back(SetValueResult{
                    .requestId = requestId,
                    .status = StatusCode::INVALID_ARG,
            });
            continue;
        }

        hardwareRequests.push_back(request);
    }

    // The set of request Ids that we would send to hardware.
    std::unordered_set<int64_t> hardwareRequestIds;
    for (const auto& request : hardwareRequests) {
        hardwareRequestIds.insert(request.requestId);
    }

    std::shared_ptr<SetValuesClient> client;
    {
        std::scoped_lock<std::mutex> lockGuard(mLock);
        client = getOrCreateClient(&mSetValuesClients, callback, mPendingRequestPool);
    }

    // Register the pending hardware requests and also check for duplicate request Ids.
    if (auto addRequestResult = client->addRequests(hardwareRequestIds); !addRequestResult.ok()) {
        ALOGE("setValues[%s], failed to add pending requests, error: %s",
              toString(hardwareRequestIds).c_str(), getErrorMsg(addRequestResult).c_str());
        return toScopedAStatus(addRequestResult, StatusCode::INVALID_ARG);
    }

    if (!failedResults.empty()) {
        // First send the failed results we already know back to the client.
        client->sendResults(failedResults);
    }

    if (hardwareRequests.empty()) {
        return ScopedAStatus::ok();
    }

    if (StatusCode status =
                mVehicleHardware->setValues(client->getResultCallback(), hardwareRequests);
        status != StatusCode::OK) {
        // If the hardware returns error, finish all the pending requests for this request because
        // we never expect hardware to call callback for these requests.
        client->tryFinishRequests(hardwareRequestIds);
        ALOGE("setValues[%s], failed to set value to VehicleHardware, status: %d",
              toString(hardwareRequestIds).c_str(), toInt(status));
        return ScopedAStatus::fromServiceSpecificErrorWithMessage(
                toInt(status), "failed to set value to VehicleHardware");
    }

    return ScopedAStatus::ok();
}

#define CHECK_DUPLICATE_REQUESTS(PROP_NAME)                                                      \
    do {                                                                                         \
        std::vector<int64_t> requestIds;                                                         \
        std::set<::aidl::android::hardware::automotive::vehicle::VehiclePropValue> requestProps; \
        for (const auto& request : requests) {                                                   \
            const auto& prop = request.PROP_NAME;                                                \
            if (requestProps.count(prop) != 0) {                                                 \
                return ::android::base::Error()                                                  \
                       << "duplicate request for property: " << prop.toString();                 \
            }                                                                                    \
            requestProps.insert(prop);                                                           \
            requestIds.push_back(request.requestId);                                             \
        }                                                                                        \
        return requestIds;                                                                       \
    } while (0);

::android::base::Result<std::vector<int64_t>> DefaultVehicleHal::checkDuplicateRequests(
        const std::vector<GetValueRequest>& requests) {
    CHECK_DUPLICATE_REQUESTS(prop);
}

::android::base::Result<std::vector<int64_t>> DefaultVehicleHal::checkDuplicateRequests(
        const std::vector<SetValueRequest>& requests) {
    CHECK_DUPLICATE_REQUESTS(value);
}

#undef CHECK_DUPLICATE_REQUESTS

ScopedAStatus DefaultVehicleHal::getPropConfigs(const std::vector<int32_t>& props,
                                                VehiclePropConfigs* output) {
    std::vector<VehiclePropConfig> configs;
    for (int32_t prop : props) {
        if (mConfigsByPropId.find(prop) != mConfigsByPropId.end()) {
            configs.push_back(mConfigsByPropId[prop]);
        }
    }
    return vectorToStableLargeParcelable(std::move(configs), output);
}

Result<void> DefaultVehicleHal::checkSubscribeOptions(
        const std::vector<SubscribeOptions>& options) {
    for (const auto& option : options) {
        int32_t propId = option.propId;
        if (mConfigsByPropId.find(propId) == mConfigsByPropId.end()) {
            return Error(toInt(StatusCode::INVALID_ARG))
                   << StringPrintf("no config for property, ID: %" PRId32, propId);
        }
        const VehiclePropConfig& config = mConfigsByPropId[propId];

        if (config.changeMode != VehiclePropertyChangeMode::ON_CHANGE &&
            config.changeMode != VehiclePropertyChangeMode::CONTINUOUS) {
            return Error(toInt(StatusCode::INVALID_ARG))
                   << "only support subscribing to ON_CHANGE or CONTINUOUS property";
        }

        if (config.access != VehiclePropertyAccess::READ &&
            config.access != VehiclePropertyAccess::READ_WRITE) {
            return Error(toInt(StatusCode::ACCESS_DENIED))
                   << StringPrintf("Property %" PRId32 " has no read access", propId);
        }

        if (config.changeMode == VehiclePropertyChangeMode::CONTINUOUS) {
            float sampleRate = option.sampleRate;
            float minSampleRate = config.minSampleRate;
            float maxSampleRate = config.maxSampleRate;
            if (sampleRate < minSampleRate || sampleRate > maxSampleRate) {
                return Error(toInt(StatusCode::INVALID_ARG))
                       << StringPrintf("sample rate: %f out of range, must be within %f and %f",
                                       sampleRate, minSampleRate, maxSampleRate);
            }
            if (!SubscriptionManager::checkSampleRate(sampleRate)) {
                return Error(toInt(StatusCode::INVALID_ARG))
                       << "invalid sample rate: " << sampleRate;
            }
        }

        if (isGlobalProp(propId)) {
            continue;
        }

        // Non-global property.
        for (int32_t areaId : option.areaIds) {
            if (auto areaConfig = getAreaConfig(propId, areaId, config); areaConfig == nullptr) {
                return Error(toInt(StatusCode::INVALID_ARG))
                       << StringPrintf("invalid area ID: %" PRId32 " for prop ID: %" PRId32
                                       ", not listed in config",
                                       areaId, propId);
            }
        }
    }
    return {};
}

ScopedAStatus DefaultVehicleHal::subscribe(const CallbackType& callback,
                                           const std::vector<SubscribeOptions>& options,
                                           [[maybe_unused]] int32_t maxSharedMemoryFileCount) {
    // TODO(b/205189110): Use shared memory file count.
    if (auto result = checkSubscribeOptions(options); !result.ok()) {
        ALOGE("subscribe: invalid subscribe options: %s", getErrorMsg(result).c_str());
        return toScopedAStatus(result);
    }

    std::vector<SubscribeOptions> onChangeSubscriptions;
    std::vector<SubscribeOptions> continuousSubscriptions;
    for (const auto& option : options) {
        int32_t propId = option.propId;
        // We have already validate config exists.
        const VehiclePropConfig& config = mConfigsByPropId[propId];

        SubscribeOptions optionCopy = option;
        // If areaIds is empty, subscribe to all areas.
        if (optionCopy.areaIds.empty() && !isGlobalProp(propId)) {
            for (const auto& areaConfig : config.areaConfigs) {
                optionCopy.areaIds.push_back(areaConfig.areaId);
            }
        }

        if (isGlobalProp(propId)) {
            optionCopy.areaIds = {0};
        }

        if (config.changeMode == VehiclePropertyChangeMode::CONTINUOUS) {
            continuousSubscriptions.push_back(std::move(optionCopy));
        } else {
            onChangeSubscriptions.push_back(std::move(optionCopy));
        }
    }
    // Since we have already check the sample rates, the following functions must succeed.
    if (!onChangeSubscriptions.empty()) {
        mSubscriptionManager->subscribe(callback, onChangeSubscriptions,
                                        /*isContinuousProperty=*/false);
    }
    if (!continuousSubscriptions.empty()) {
        mSubscriptionManager->subscribe(callback, continuousSubscriptions,
                                        /*isContinuousProperty=*/true);
    }
    return ScopedAStatus::ok();
}

ScopedAStatus DefaultVehicleHal::unsubscribe(const CallbackType& callback,
                                             const std::vector<int32_t>& propIds) {
    return toScopedAStatus(mSubscriptionManager->unsubscribe(callback, propIds),
                           StatusCode::INVALID_ARG);
}

ScopedAStatus DefaultVehicleHal::returnSharedMemory(const CallbackType&, int64_t) {
    // TODO(b/200737967): implement this.
    return ScopedAStatus::ok();
}

IVehicleHardware* DefaultVehicleHal::getHardware() {
    return mVehicleHardware.get();
}

Result<void> DefaultVehicleHal::checkWritePermission(const VehiclePropValue& value) const {
    int32_t propId = value.prop;
    auto result = getConfig(propId);
    if (!result.ok()) {
        return Error(toInt(StatusCode::INVALID_ARG)) << getErrorMsg(result);
    }
    const VehiclePropConfig* config = result.value();

    if (config->access != VehiclePropertyAccess::WRITE &&
        config->access != VehiclePropertyAccess::READ_WRITE) {
        return Error(toInt(StatusCode::ACCESS_DENIED))
               << StringPrintf("Property %" PRId32 " has no write access", propId);
    }
    return {};
}

Result<void> DefaultVehicleHal::checkReadPermission(const VehiclePropValue& value) const {
    int32_t propId = value.prop;
    auto result = getConfig(propId);
    if (!result.ok()) {
        return Error(toInt(StatusCode::INVALID_ARG)) << getErrorMsg(result);
    }
    const VehiclePropConfig* config = result.value();

    if (config->access != VehiclePropertyAccess::READ &&
        config->access != VehiclePropertyAccess::READ_WRITE) {
        return Error(toInt(StatusCode::ACCESS_DENIED))
               << StringPrintf("Property %" PRId32 " has no read access", propId);
    }
    return {};
}

void DefaultVehicleHal::checkHealth(std::weak_ptr<IVehicleHardware> hardware,
                                    std::weak_ptr<SubscriptionManager> subscriptionManager) {
    auto hardwarePtr = hardware.lock();
    if (hardwarePtr == nullptr) {
        ALOGW("the VehicleHardware is destroyed, DefaultVehicleHal is ending");
        return;
    }

    StatusCode status = hardwarePtr->checkHealth();
    if (status != StatusCode::OK) {
        ALOGE("VHAL check health returns non-okay status");
        return;
    }
    std::vector<VehiclePropValue> values = {{
            .prop = toInt(VehicleProperty::VHAL_HEARTBEAT),
            .areaId = 0,
            .status = VehiclePropertyStatus::AVAILABLE,
            .value.int64Values = {uptimeMillis()},
    }};
    onPropertyChangeEvent(subscriptionManager, values);
    return;
}

}  // namespace vehicle
}  // namespace automotive
}  // namespace hardware
}  // namespace android
