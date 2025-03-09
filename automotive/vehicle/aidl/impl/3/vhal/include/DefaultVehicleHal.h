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

#ifndef android_hardware_automotive_vehicle_aidl_impl_vhal_include_DefaultVehicleHal_H_
#define android_hardware_automotive_vehicle_aidl_impl_vhal_include_DefaultVehicleHal_H_

#include <ConnectedClient.h>
#include <ParcelableUtils.h>
#include <PendingRequestPool.h>
#include <RecurrentTimer.h>
#include <SubscriptionManager.h>

#include <ConcurrentQueue.h>
#include <IVehicleHardware.h>
#include <VehicleUtils.h>
#include <aidl/android/hardware/automotive/vehicle/BnVehicle.h>
#include <android-base/expected.h>
#include <android-base/thread_annotations.h>
#include <android/binder_auto_utils.h>

#include <functional>
#include <memory>
#include <mutex>
#include <shared_mutex>
#include <unordered_map>
#include <vector>

namespace android {
namespace hardware {
namespace automotive {
namespace vehicle {

namespace aidlvhal = ::aidl::android::hardware::automotive::vehicle;

class DefaultVehicleHal final : public aidlvhal::BnVehicle {
  public:
    using CallbackType = std::shared_ptr<aidlvhal::IVehicleCallback>;

    explicit DefaultVehicleHal(std::unique_ptr<IVehicleHardware> hardware);

    // Test-only
    DefaultVehicleHal(std::unique_ptr<IVehicleHardware> hardware, int32_t testInterfaceVersion);

    ~DefaultVehicleHal();

    ndk::ScopedAStatus getAllPropConfigs(aidlvhal::VehiclePropConfigs* returnConfigs) override;
    ndk::ScopedAStatus getValues(const CallbackType& callback,
                                 const aidlvhal::GetValueRequests& requests) override;
    ndk::ScopedAStatus setValues(const CallbackType& callback,
                                 const aidlvhal::SetValueRequests& requests) override;
    ndk::ScopedAStatus getPropConfigs(const std::vector<int32_t>& props,
                                      aidlvhal::VehiclePropConfigs* returnConfigs) override;
    ndk::ScopedAStatus subscribe(const CallbackType& callback,
                                 const std::vector<aidlvhal::SubscribeOptions>& options,
                                 int32_t maxSharedMemoryFileCount) override;
    ndk::ScopedAStatus unsubscribe(const CallbackType& callback,
                                   const std::vector<int32_t>& propIds) override;
    ndk::ScopedAStatus returnSharedMemory(const CallbackType& callback,
                                          int64_t sharedMemoryId) override;
    binder_status_t dump(int fd, const char** args, uint32_t numArgs) override;

    IVehicleHardware* getHardware();

  private:
    // friend class for unit testing.
    friend class DefaultVehicleHalTest;

    using GetValuesClient = GetSetValuesClient<aidlvhal::GetValueResult, aidlvhal::GetValueResults>;
    using SetValuesClient = GetSetValuesClient<aidlvhal::SetValueResult, aidlvhal::SetValueResults>;

    // A wrapper for binder lifecycle operations to enable stubbing for test.
    class BinderLifecycleInterface {
      public:
        virtual ~BinderLifecycleInterface() = default;

        virtual binder_status_t linkToDeath(AIBinder* binder, AIBinder_DeathRecipient* recipient,
                                            void* cookie) = 0;

        virtual bool isAlive(const AIBinder* binder) = 0;
    };

    // A real implementation for BinderLifecycleInterface.
    class BinderLifecycleHandler final : public BinderLifecycleInterface {
      public:
        binder_status_t linkToDeath(AIBinder* binder, AIBinder_DeathRecipient* recipient,
                                    void* cookie) override;

        bool isAlive(const AIBinder* binder) override;
    };

    // OnBinderDiedContext is a type used as a cookie passed deathRecipient. The deathRecipient's
    // onBinderDied function takes only a cookie as input and we have to store all the contexts
    // as the cookie.
    struct OnBinderDiedContext {
        DefaultVehicleHal* vhal;
        const AIBinder* clientId;
    };

    // BinderDiedUnlinkedEvent represents either an onBinderDied or an onBinderUnlinked event.
    struct BinderDiedUnlinkedEvent {
        // true for onBinderDied, false for onBinderUnlinked.
        bool forOnBinderDied;
        const AIBinder* clientId;
    };

    // The default timeout of get or set value requests is 30s.
    // TODO(b/214605968): define TIMEOUT_IN_NANO in IVehicle and allow getValues/setValues/subscribe
    // to specify custom timeouts.
    static constexpr int64_t TIMEOUT_IN_NANO = 30'000'000'000;
    // heart beat event interval: 3s
    static constexpr int64_t HEART_BEAT_INTERVAL_IN_NANO = 3'000'000'000;
    bool mShouldRefreshPropertyConfigs;
    std::unique_ptr<IVehicleHardware> mVehicleHardware;

    // PendingRequestPool is thread-safe.
    std::shared_ptr<PendingRequestPool> mPendingRequestPool;
    // SubscriptionManager is thread-safe.
    std::shared_ptr<SubscriptionManager> mSubscriptionManager;
    // ConcurrentQueue is thread-safe.
    std::shared_ptr<ConcurrentQueue<aidlvhal::VehiclePropValue>> mBatchedEventQueue;
    // BatchingConsumer is thread-safe.
    std::shared_ptr<BatchingConsumer<aidlvhal::VehiclePropValue>>
            mPropertyChangeEventsBatchingConsumer;
    // Only set once during initialization.
    std::chrono::nanoseconds mEventBatchingWindow;
    // Only used for testing.
    int32_t mTestInterfaceVersion = 0;

    mutable std::atomic<bool> mConfigInit = false;
    mutable std::shared_timed_mutex mConfigLock;
    mutable std::unordered_map<int32_t, aidlvhal::VehiclePropConfig> mConfigsByPropId
            GUARDED_BY(mConfigLock);
    mutable std::unique_ptr<ndk::ScopedFileDescriptor> mConfigFile GUARDED_BY(mConfigLock);

    std::mutex mLock;
    std::unordered_map<const AIBinder*, std::unique_ptr<OnBinderDiedContext>> mOnBinderDiedContexts
            GUARDED_BY(mLock);
    std::unordered_map<const AIBinder*, std::shared_ptr<GetValuesClient>> mGetValuesClients
            GUARDED_BY(mLock);
    std::unordered_map<const AIBinder*, std::shared_ptr<SetValuesClient>> mSetValuesClients
            GUARDED_BY(mLock);
    // mBinderLifecycleHandler is only going to be changed in test.
    std::unique_ptr<BinderLifecycleInterface> mBinderLifecycleHandler;

    // Only initialized once.
    std::shared_ptr<std::function<void()>> mRecurrentAction;
    // RecurrentTimer is thread-safe.
    RecurrentTimer mRecurrentTimer;

    ndk::ScopedAIBinder_DeathRecipient mDeathRecipient;

    // ConcurrentQueue is thread-safe.
    ConcurrentQueue<BinderDiedUnlinkedEvent> mBinderEvents;

    // A thread to handle onBinderDied or onBinderUnlinked event.
    std::thread mOnBinderDiedUnlinkedHandlerThread;

    android::base::Result<void> checkProperty(const aidlvhal::VehiclePropValue& propValue);

    android::base::Result<std::vector<int64_t>> checkDuplicateRequests(
            const std::vector<aidlvhal::GetValueRequest>& requests);

    android::base::Result<std::vector<int64_t>> checkDuplicateRequests(
            const std::vector<aidlvhal::SetValueRequest>& requests);
    VhalResult<void> checkSubscribeOptions(
            const std::vector<aidlvhal::SubscribeOptions>& options,
            const std::unordered_map<int32_t, aidlvhal::VehiclePropConfig>& configsByPropId)
            REQUIRES_SHARED(mConfigLock);

    VhalResult<void> checkPermissionHelper(const aidlvhal::VehiclePropValue& value,
                                           aidlvhal::VehiclePropertyAccess accessToTest) const;

    VhalResult<void> checkReadPermission(const aidlvhal::VehiclePropValue& value) const;

    VhalResult<void> checkWritePermission(const aidlvhal::VehiclePropValue& value) const;

    android::base::Result<aidlvhal::VehiclePropConfig> getConfig(int32_t propId) const;

    void onBinderDiedWithContext(const AIBinder* clientId);

    void onBinderUnlinkedWithContext(const AIBinder* clientId);

    // Registers a onBinderDied callback for the client if not already registered.
    // Returns true if the client Binder is alive, false otherwise.
    bool monitorBinderLifeCycleLocked(const AIBinder* clientId) REQUIRES(mLock);

    bool checkDumpPermission();

    bool isConfigSupportedForCurrentVhalVersion(const aidlvhal::VehiclePropConfig& config) const;

    bool getAllPropConfigsFromHardwareLocked() const EXCLUDES(mConfigLock);

    // The looping handler function to process all onBinderDied or onBinderUnlinked events in
    // mBinderEvents.
    void onBinderDiedUnlinkedHandler();

    size_t countSubscribeClients();

    // Handles the property change events in batch.
    void handleBatchedPropertyEvents(std::vector<aidlvhal::VehiclePropValue>&& batchedEvents);

    int32_t getVhalInterfaceVersion() const;

    // Gets mConfigsByPropId, lazy init it if necessary. Note that the reference is only valid in
    // the scope of the callback and it is guaranteed that read lock is obtained during the
    // callback.
    void getConfigsByPropId(
            std::function<void(const std::unordered_map<int32_t, aidlvhal::VehiclePropConfig>&)>
                    callback) const EXCLUDES(mConfigLock);

    // Puts the property change events into a queue so that they can handled in batch.
    static void batchPropertyChangeEvent(
            const std::weak_ptr<ConcurrentQueue<aidlvhal::VehiclePropValue>>& batchedEventQueue,
            std::vector<aidlvhal::VehiclePropValue>&& updatedValues);

    // Gets or creates a {@code T} object for the client to or from {@code clients}.
    template <class T>
    static std::shared_ptr<T> getOrCreateClient(
            std::unordered_map<const AIBinder*, std::shared_ptr<T>>* clients,
            const CallbackType& callback, std::shared_ptr<PendingRequestPool> pendingRequestPool);

    static void onPropertyChangeEvent(const std::weak_ptr<SubscriptionManager>& subscriptionManager,
                                      std::vector<aidlvhal::VehiclePropValue>&& updatedValues);

    static void onPropertySetErrorEvent(
            const std::weak_ptr<SubscriptionManager>& subscriptionManager,
            const std::vector<SetValueErrorEvent>& errorEvents);

    static void checkHealth(IVehicleHardware* hardware,
                            std::weak_ptr<SubscriptionManager> subscriptionManager);

    static void onBinderDied(void* cookie);

    static void onBinderUnlinked(void* cookie);

    static void parseSubscribeOptions(
            const std::vector<aidlvhal::SubscribeOptions>& options,
            const std::unordered_map<int32_t, aidlvhal::VehiclePropConfig>& configsByPropId,
            std::vector<aidlvhal::SubscribeOptions>& onChangeSubscriptions,
            std::vector<aidlvhal::SubscribeOptions>& continuousSubscriptions);

    // Test-only
    // Set the default timeout for pending requests.
    void setTimeout(int64_t timeoutInNano);

    // Test-only
    void setBinderLifecycleHandler(std::unique_ptr<BinderLifecycleInterface> impl);
};

}  // namespace vehicle
}  // namespace automotive
}  // namespace hardware
}  // namespace android

#endif  // android_hardware_automotive_vehicle_aidl_impl_vhal_include_DefaultVehicleHal_H_
