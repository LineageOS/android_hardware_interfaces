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
#define ATRACE_TAG ATRACE_TAG_HAL

#include <DefaultVehicleHal.h>

#include <LargeParcelableBase.h>
#include <VehicleHalTypes.h>
#include <VehicleUtils.h>

#include <android-base/result.h>
#include <android-base/stringprintf.h>
#include <android/binder_ibinder.h>
#include <private/android_filesystem_config.h>
#include <utils/Log.h>
#include <utils/SystemClock.h>
#include <utils/Trace.h>

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

using ::ndk::ScopedAIBinder_DeathRecipient;
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

float getDefaultSampleRateHz(float sampleRateHz, float minSampleRateHz, float maxSampleRateHz) {
    if (sampleRateHz < minSampleRateHz) {
        return minSampleRateHz;
    }
    if (sampleRateHz > maxSampleRateHz) {
        return maxSampleRateHz;
    }
    return sampleRateHz;
}

}  // namespace

std::shared_ptr<SubscriptionClient> DefaultVehicleHal::SubscriptionClients::maybeAddClient(
        const CallbackType& callback) {
    std::scoped_lock<std::mutex> lockGuard(mLock);
    return getOrCreateClient(&mClients, callback, mPendingRequestPool);
}

std::shared_ptr<SubscriptionClient> DefaultVehicleHal::SubscriptionClients::getClient(
        const CallbackType& callback) {
    std::scoped_lock<std::mutex> lockGuard(mLock);
    const AIBinder* clientId = callback->asBinder().get();
    if (mClients.find(clientId) == mClients.end()) {
        return nullptr;
    }
    return mClients[clientId];
}

int64_t DefaultVehicleHal::SubscribeIdByClient::getId(const CallbackType& callback) {
    std::scoped_lock<std::mutex> lockGuard(mLock);
    // This would be initialized to 0 if callback does not exist in the map.
    int64_t subscribeId = (mIds[callback->asBinder().get()])++;
    return subscribeId;
}

void DefaultVehicleHal::SubscriptionClients::removeClient(const AIBinder* clientId) {
    std::scoped_lock<std::mutex> lockGuard(mLock);
    mClients.erase(clientId);
}

size_t DefaultVehicleHal::SubscriptionClients::countClients() {
    std::scoped_lock<std::mutex> lockGuard(mLock);
    return mClients.size();
}

DefaultVehicleHal::DefaultVehicleHal(std::unique_ptr<IVehicleHardware> vehicleHardware)
    : mVehicleHardware(std::move(vehicleHardware)),
      mPendingRequestPool(std::make_shared<PendingRequestPool>(TIMEOUT_IN_NANO)) {
    if (!getAllPropConfigsFromHardware()) {
        return;
    }

    mSubscriptionClients = std::make_shared<SubscriptionClients>(mPendingRequestPool);

    auto subscribeIdByClient = std::make_shared<SubscribeIdByClient>();
    IVehicleHardware* vehicleHardwarePtr = mVehicleHardware.get();
    mSubscriptionManager = std::make_shared<SubscriptionManager>(vehicleHardwarePtr);

    std::weak_ptr<SubscriptionManager> subscriptionManagerCopy = mSubscriptionManager;
    mVehicleHardware->registerOnPropertyChangeEvent(
            std::make_unique<IVehicleHardware::PropertyChangeCallback>(
                    [subscriptionManagerCopy](std::vector<VehiclePropValue> updatedValues) {
                        onPropertyChangeEvent(subscriptionManagerCopy, updatedValues);
                    }));
    mVehicleHardware->registerOnPropertySetErrorEvent(
            std::make_unique<IVehicleHardware::PropertySetErrorCallback>(
                    [subscriptionManagerCopy](std::vector<SetValueErrorEvent> errorEvents) {
                        onPropertySetErrorEvent(subscriptionManagerCopy, errorEvents);
                    }));

    // Register heartbeat event.
    mRecurrentAction = std::make_shared<std::function<void()>>(
            [vehicleHardwarePtr, subscriptionManagerCopy]() {
                checkHealth(vehicleHardwarePtr, subscriptionManagerCopy);
            });
    mRecurrentTimer.registerTimerCallback(HEART_BEAT_INTERVAL_IN_NANO, mRecurrentAction);

    mBinderLifecycleHandler = std::make_unique<BinderLifecycleHandler>();
    mOnBinderDiedUnlinkedHandlerThread = std::thread([this] { onBinderDiedUnlinkedHandler(); });
    mDeathRecipient = ScopedAIBinder_DeathRecipient(
            AIBinder_DeathRecipient_new(&DefaultVehicleHal::onBinderDied));
    AIBinder_DeathRecipient_setOnUnlinked(mDeathRecipient.get(),
                                          &DefaultVehicleHal::onBinderUnlinked);
}

DefaultVehicleHal::~DefaultVehicleHal() {
    // Delete the deathRecipient so that onBinderDied would not be called to reference 'this'.
    mDeathRecipient = ScopedAIBinder_DeathRecipient();
    mBinderEvents.deactivate();
    if (mOnBinderDiedUnlinkedHandlerThread.joinable()) {
        mOnBinderDiedUnlinkedHandlerThread.join();
    }
    // mRecurrentAction uses pointer to mVehicleHardware, so it has to be unregistered before
    // mVehicleHardware.
    mRecurrentTimer.unregisterTimerCallback(mRecurrentAction);
    // mSubscriptionManager uses pointer to mVehicleHardware, so it has to be destroyed before
    // mVehicleHardware.
    mSubscriptionManager.reset();
    mVehicleHardware.reset();
}

void DefaultVehicleHal::onPropertyChangeEvent(
        const std::weak_ptr<SubscriptionManager>& subscriptionManager,
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

void DefaultVehicleHal::onPropertySetErrorEvent(
        const std::weak_ptr<SubscriptionManager>& subscriptionManager,
        const std::vector<SetValueErrorEvent>& errorEvents) {
    auto manager = subscriptionManager.lock();
    if (manager == nullptr) {
        ALOGW("the SubscriptionManager is destroyed, DefaultVehicleHal is ending");
        return;
    }
    auto vehiclePropErrorsByClient = manager->getSubscribedClientsForErrorEvents(errorEvents);
    for (auto& [callback, vehiclePropErrors] : vehiclePropErrorsByClient) {
        SubscriptionClient::sendPropertySetErrors(callback, std::move(vehiclePropErrors));
    }
}

template <class T>
std::shared_ptr<T> DefaultVehicleHal::getOrCreateClient(
        std::unordered_map<const AIBinder*, std::shared_ptr<T>>* clients,
        const CallbackType& callback, std::shared_ptr<PendingRequestPool> pendingRequestPool) {
    const AIBinder* clientId = callback->asBinder().get();
    if (clients->find(clientId) == clients->end()) {
        (*clients)[clientId] = std::make_shared<T>(pendingRequestPool, callback);
    }
    return (*clients)[clientId];
}

bool DefaultVehicleHal::monitorBinderLifeCycleLocked(const AIBinder* clientId) {
    OnBinderDiedContext* contextPtr = nullptr;
    if (mOnBinderDiedContexts.find(clientId) != mOnBinderDiedContexts.end()) {
        return mBinderLifecycleHandler->isAlive(clientId);
    } else {
        std::unique_ptr<OnBinderDiedContext> context = std::make_unique<OnBinderDiedContext>(
                OnBinderDiedContext{.vhal = this, .clientId = clientId});
        // We know context must be alive when we use contextPtr because context would only
        // be removed in OnBinderUnlinked, which must be called after OnBinderDied.
        contextPtr = context.get();
        // Insert into a map to keep the context object alive.
        mOnBinderDiedContexts[clientId] = std::move(context);
    }

    // If this function fails, onBinderUnlinked would be called to remove the added context.
    binder_status_t status = mBinderLifecycleHandler->linkToDeath(
            const_cast<AIBinder*>(clientId), mDeathRecipient.get(), static_cast<void*>(contextPtr));
    if (status == STATUS_OK) {
        return true;
    }
    ALOGE("failed to call linkToDeath on client binder, client may already died, status: %d",
          static_cast<int>(status));
    return false;
}

void DefaultVehicleHal::onBinderDied(void* cookie) {
    OnBinderDiedContext* context = reinterpret_cast<OnBinderDiedContext*>(cookie);
    // To be handled in mOnBinderDiedUnlinkedHandlerThread. We cannot handle the event in the same
    // thread because we might be holding the mLock the handler requires.
    context->vhal->mBinderEvents.push(
            BinderDiedUnlinkedEvent{/*forOnBinderDied=*/true, context->clientId});
}

void DefaultVehicleHal::onBinderDiedWithContext(const AIBinder* clientId) {
    std::scoped_lock<std::mutex> lockGuard(mLock);
    ALOGD("binder died, client ID: %p", clientId);
    mSetValuesClients.erase(clientId);
    mGetValuesClients.erase(clientId);
    mSubscriptionClients->removeClient(clientId);
    mSubscriptionManager->unsubscribe(clientId);
}

void DefaultVehicleHal::onBinderUnlinked(void* cookie) {
    OnBinderDiedContext* context = reinterpret_cast<OnBinderDiedContext*>(cookie);
    // To be handled in mOnBinderDiedUnlinkedHandlerThread. We cannot handle the event in the same
    // thread because we might be holding the mLock the handler requires.
    context->vhal->mBinderEvents.push(
            BinderDiedUnlinkedEvent{/*forOnBinderDied=*/false, context->clientId});
}

void DefaultVehicleHal::onBinderUnlinkedWithContext(const AIBinder* clientId) {
    ALOGD("binder unlinked");
    std::scoped_lock<std::mutex> lockGuard(mLock);
    // Delete the context associated with this cookie.
    mOnBinderDiedContexts.erase(clientId);
}

void DefaultVehicleHal::onBinderDiedUnlinkedHandler() {
    while (mBinderEvents.waitForItems()) {
        for (BinderDiedUnlinkedEvent& event : mBinderEvents.flush()) {
            if (event.forOnBinderDied) {
                onBinderDiedWithContext(event.clientId);
            } else {
                onBinderUnlinkedWithContext(event.clientId);
            }
        }
    }
}

template std::shared_ptr<DefaultVehicleHal::GetValuesClient>
DefaultVehicleHal::getOrCreateClient<DefaultVehicleHal::GetValuesClient>(
        std::unordered_map<const AIBinder*, std::shared_ptr<GetValuesClient>>* clients,
        const CallbackType& callback, std::shared_ptr<PendingRequestPool> pendingRequestPool);
template std::shared_ptr<DefaultVehicleHal::SetValuesClient>
DefaultVehicleHal::getOrCreateClient<DefaultVehicleHal::SetValuesClient>(
        std::unordered_map<const AIBinder*, std::shared_ptr<SetValuesClient>>* clients,
        const CallbackType& callback, std::shared_ptr<PendingRequestPool> pendingRequestPool);
template std::shared_ptr<SubscriptionClient>
DefaultVehicleHal::getOrCreateClient<SubscriptionClient>(
        std::unordered_map<const AIBinder*, std::shared_ptr<SubscriptionClient>>* clients,
        const CallbackType& callback, std::shared_ptr<PendingRequestPool> pendingRequestPool);

void DefaultVehicleHal::setTimeout(int64_t timeoutInNano) {
    mPendingRequestPool = std::make_unique<PendingRequestPool>(timeoutInNano);
}

bool DefaultVehicleHal::getAllPropConfigsFromHardware() {
    auto configs = mVehicleHardware->getAllPropertyConfigs();
    for (auto& config : configs) {
        mConfigsByPropId[config.prop] = config;
    }
    VehiclePropConfigs vehiclePropConfigs;
    vehiclePropConfigs.payloads = std::move(configs);
    auto result = LargeParcelableBase::parcelableToStableLargeParcelable(vehiclePropConfigs);
    if (!result.ok()) {
        ALOGE("failed to convert configs to shared memory file, error: %s, code: %d",
              result.error().message().c_str(), static_cast<int>(result.error().code()));
        mConfigFile = nullptr;
        return false;
    }

    if (result.value() != nullptr) {
        mConfigFile = std::move(result.value());
    }
    return true;
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
    ATRACE_CALL();
    if (callback == nullptr) {
        return ScopedAStatus::fromExceptionCode(EX_NULL_POINTER);
    }
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
        // Lock to make sure onBinderDied would not be called concurrently.
        std::scoped_lock lockGuard(mLock);
        if (!monitorBinderLifeCycleLocked(callback->asBinder().get())) {
            return ScopedAStatus::fromExceptionCodeWithMessage(EX_TRANSACTION_FAILED,
                                                               "client died");
        }

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
        client->sendResults(std::move(failedResults));
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
    ATRACE_CALL();
    if (callback == nullptr) {
        return ScopedAStatus::fromExceptionCode(EX_NULL_POINTER);
    }
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
        // Lock to make sure onBinderDied would not be called concurrently.
        std::scoped_lock lockGuard(mLock);
        if (!monitorBinderLifeCycleLocked(callback->asBinder().get())) {
            return ScopedAStatus::fromExceptionCodeWithMessage(EX_TRANSACTION_FAILED,
                                                               "client died");
        }
        client = getOrCreateClient(&mSetValuesClients, callback, mPendingRequestPool);
    }

    // Register the pending hardware requests and also check for duplicate request Ids.
    if (auto addRequestResult = client->addRequests(hardwareRequestIds); !addRequestResult.ok()) {
        ALOGE("setValues[%s], failed to add pending requests, error: %s",
              toString(hardwareRequestIds).c_str(), getErrorMsg(addRequestResult).c_str());
        return toScopedAStatus(addRequestResult);
    }

    if (!failedResults.empty()) {
        // First send the failed results we already know back to the client.
        client->sendResults(std::move(failedResults));
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
        } else {
            return ScopedAStatus::fromServiceSpecificErrorWithMessage(
                    toInt(StatusCode::INVALID_ARG),
                    StringPrintf("no config for property, ID: %" PRId32, prop).c_str());
        }
    }
    return vectorToStableLargeParcelable(std::move(configs), output);
}

VhalResult<void> DefaultVehicleHal::checkSubscribeOptions(
        const std::vector<SubscribeOptions>& options) {
    for (const auto& option : options) {
        int32_t propId = option.propId;
        if (mConfigsByPropId.find(propId) == mConfigsByPropId.end()) {
            return StatusError(StatusCode::INVALID_ARG)
                   << StringPrintf("no config for property, ID: %" PRId32, propId);
        }
        const VehiclePropConfig& config = mConfigsByPropId[propId];

        if (config.changeMode != VehiclePropertyChangeMode::ON_CHANGE &&
            config.changeMode != VehiclePropertyChangeMode::CONTINUOUS) {
            return StatusError(StatusCode::INVALID_ARG)
                   << "only support subscribing to ON_CHANGE or CONTINUOUS property";
        }

        if (config.access != VehiclePropertyAccess::READ &&
            config.access != VehiclePropertyAccess::READ_WRITE) {
            return StatusError(StatusCode::ACCESS_DENIED)
                   << StringPrintf("Property %" PRId32 " has no read access", propId);
        }

        if (config.changeMode == VehiclePropertyChangeMode::CONTINUOUS) {
            float sampleRateHz = option.sampleRate;
            float minSampleRateHz = config.minSampleRate;
            float maxSampleRateHz = config.maxSampleRate;
            float defaultRateHz =
                    getDefaultSampleRateHz(sampleRateHz, minSampleRateHz, maxSampleRateHz);
            if (sampleRateHz != defaultRateHz) {
                ALOGW("sample rate: %f HZ out of range, must be within %f HZ and %f HZ , set to %f "
                      "HZ",
                      sampleRateHz, minSampleRateHz, maxSampleRateHz, defaultRateHz);
                sampleRateHz = defaultRateHz;
            }
            if (!SubscriptionManager::checkSampleRateHz(sampleRateHz)) {
                return StatusError(StatusCode::INVALID_ARG)
                       << "invalid sample rate: " << sampleRateHz << " HZ";
            }
        }

        if (isGlobalProp(propId)) {
            continue;
        }

        // Non-global property.
        for (int32_t areaId : option.areaIds) {
            if (auto areaConfig = getAreaConfig(propId, areaId, config); areaConfig == nullptr) {
                return StatusError(StatusCode::INVALID_ARG)
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
    if (callback == nullptr) {
        return ScopedAStatus::fromExceptionCode(EX_NULL_POINTER);
    }
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
            optionCopy.sampleRate = getDefaultSampleRateHz(
                    optionCopy.sampleRate, config.minSampleRate, config.maxSampleRate);
            continuousSubscriptions.push_back(std::move(optionCopy));
        } else {
            onChangeSubscriptions.push_back(std::move(optionCopy));
        }
    }

    {
        // Lock to make sure onBinderDied would not be called concurrently.
        std::scoped_lock lockGuard(mLock);
        if (!monitorBinderLifeCycleLocked(callback->asBinder().get())) {
            return ScopedAStatus::fromExceptionCodeWithMessage(EX_TRANSACTION_FAILED,
                                                               "client died");
        }

        // Create a new SubscriptionClient if there isn't an existing one.
        mSubscriptionClients->maybeAddClient(callback);

        if (!onChangeSubscriptions.empty()) {
            auto result = mSubscriptionManager->subscribe(callback, onChangeSubscriptions,
                                                          /*isContinuousProperty=*/false);
            if (!result.ok()) {
                return toScopedAStatus(result);
            }
        }
        if (!continuousSubscriptions.empty()) {
            auto result = mSubscriptionManager->subscribe(callback, continuousSubscriptions,
                                                          /*isContinuousProperty=*/true);
            if (!result.ok()) {
                return toScopedAStatus(result);
            }
        }
    }
    return ScopedAStatus::ok();
}

ScopedAStatus DefaultVehicleHal::unsubscribe(const CallbackType& callback,
                                             const std::vector<int32_t>& propIds) {
    if (callback == nullptr) {
        return ScopedAStatus::fromExceptionCode(EX_NULL_POINTER);
    }
    return toScopedAStatus(mSubscriptionManager->unsubscribe(callback->asBinder().get(), propIds));
}

ScopedAStatus DefaultVehicleHal::returnSharedMemory(const CallbackType&, int64_t) {
    // TODO(b/200737967): implement this.
    return ScopedAStatus::ok();
}

IVehicleHardware* DefaultVehicleHal::getHardware() {
    return mVehicleHardware.get();
}

VhalResult<void> DefaultVehicleHal::checkWritePermission(const VehiclePropValue& value) const {
    int32_t propId = value.prop;
    auto result = getConfig(propId);
    if (!result.ok()) {
        return StatusError(StatusCode::INVALID_ARG) << getErrorMsg(result);
    }
    const VehiclePropConfig* config = result.value();

    if (config->access != VehiclePropertyAccess::WRITE &&
        config->access != VehiclePropertyAccess::READ_WRITE) {
        return StatusError(StatusCode::ACCESS_DENIED)
               << StringPrintf("Property %" PRId32 " has no write access", propId);
    }
    return {};
}

VhalResult<void> DefaultVehicleHal::checkReadPermission(const VehiclePropValue& value) const {
    int32_t propId = value.prop;
    auto result = getConfig(propId);
    if (!result.ok()) {
        return StatusError(StatusCode::INVALID_ARG) << getErrorMsg(result);
    }
    const VehiclePropConfig* config = result.value();

    if (config->access != VehiclePropertyAccess::READ &&
        config->access != VehiclePropertyAccess::READ_WRITE) {
        return StatusError(StatusCode::ACCESS_DENIED)
               << StringPrintf("Property %" PRId32 " has no read access", propId);
    }
    return {};
}

void DefaultVehicleHal::checkHealth(IVehicleHardware* vehicleHardware,
                                    std::weak_ptr<SubscriptionManager> subscriptionManager) {
    StatusCode status = vehicleHardware->checkHealth();
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

binder_status_t DefaultVehicleHal::BinderLifecycleHandler::linkToDeath(
        AIBinder* binder, AIBinder_DeathRecipient* recipient, void* cookie) {
    return AIBinder_linkToDeath(binder, recipient, cookie);
}

bool DefaultVehicleHal::BinderLifecycleHandler::isAlive(const AIBinder* binder) {
    return AIBinder_isAlive(binder);
}

void DefaultVehicleHal::setBinderLifecycleHandler(
        std::unique_ptr<BinderLifecycleInterface> handler) {
    mBinderLifecycleHandler = std::move(handler);
}

bool DefaultVehicleHal::checkDumpPermission() {
    uid_t uid = AIBinder_getCallingUid();
    return uid == AID_ROOT || uid == AID_SHELL || uid == AID_SYSTEM;
}

binder_status_t DefaultVehicleHal::dump(int fd, const char** args, uint32_t numArgs) {
    if (!checkDumpPermission()) {
        dprintf(fd, "Caller must be root, system or shell");
        return STATUS_PERMISSION_DENIED;
    }

    std::vector<std::string> options;
    for (uint32_t i = 0; i < numArgs; i++) {
        options.push_back(args[i]);
    }
    if (options.size() == 1 && options[0] == "-a") {
        // Ignore "-a" option. Bugreport will call with this option.
        options.clear();
    }
    DumpResult result = mVehicleHardware->dump(options);
    if (result.refreshPropertyConfigs) {
        getAllPropConfigsFromHardware();
    }
    dprintf(fd, "%s", (result.buffer + "\n").c_str());
    if (!result.callerShouldDumpState) {
        return STATUS_OK;
    }
    dprintf(fd, "Vehicle HAL State: \n");
    {
        std::scoped_lock<std::mutex> lockGuard(mLock);
        dprintf(fd, "Containing %zu property configs\n", mConfigsByPropId.size());
        dprintf(fd, "Currently have %zu getValues clients\n", mGetValuesClients.size());
        dprintf(fd, "Currently have %zu setValues clients\n", mSetValuesClients.size());
        dprintf(fd, "Currently have %zu subscription clients\n",
                mSubscriptionClients->countClients());
    }
    return STATUS_OK;
}

}  // namespace vehicle
}  // namespace automotive
}  // namespace hardware
}  // namespace android
