/*
 * Copyright (C) 2022 The Android Open Source Project
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

#ifndef WIFI_CHIP_H_
#define WIFI_CHIP_H_

#include <aidl/android/hardware/wifi/BnWifiChip.h>
#include <aidl/android/hardware/wifi/IWifiRttController.h>
#include <aidl/android/hardware/wifi/common/OuiKeyedData.h>
#include <android-base/macros.h>

#include <list>
#include <map>
#include <mutex>

#include "aidl_callback_util.h"
#include "ringbuffer.h"
#include "wifi_ap_iface.h"
#include "wifi_feature_flags.h"
#include "wifi_legacy_hal.h"
#include "wifi_mode_controller.h"
#include "wifi_nan_iface.h"
#include "wifi_p2p_iface.h"
#include "wifi_rtt_controller.h"
#include "wifi_sta_iface.h"

namespace aidl {
namespace android {
namespace hardware {
namespace wifi {

/**
 * AIDL interface object used to control a Wifi HAL chip instance.
 * Since there is only a single chip instance used today, there is no
 * identifying handle information stored here.
 */
class WifiChip : public BnWifiChip {
  public:
    WifiChip(int32_t chip_id, bool is_primary,
             const std::weak_ptr<legacy_hal::WifiLegacyHal> legacy_hal,
             const std::weak_ptr<mode_controller::WifiModeController> mode_controller,
             const std::shared_ptr<iface_util::WifiIfaceUtil> iface_util,
             const std::weak_ptr<feature_flags::WifiFeatureFlags> feature_flags,
             const std::function<void(const std::string&)>& subsystemCallbackHandler,
             bool using_dynamic_iface_combination);

    // Factory method - use instead of default constructor.
    static std::shared_ptr<WifiChip> create(
            int32_t chip_id, bool is_primary,
            const std::weak_ptr<legacy_hal::WifiLegacyHal> legacy_hal,
            const std::weak_ptr<mode_controller::WifiModeController> mode_controller,
            const std::shared_ptr<iface_util::WifiIfaceUtil> iface_util,
            const std::weak_ptr<feature_flags::WifiFeatureFlags> feature_flags,
            const std::function<void(const std::string&)>& subsystemCallbackHandler,
            bool using_dynamic_iface_combination);

    // AIDL does not provide a built-in mechanism to let the server invalidate
    // an AIDL interface object after creation. If any client process holds onto
    // a reference to the object in their context, any method calls on that
    // reference will continue to be directed to the server.
    //
    // However Wifi HAL needs to control the lifetime of these objects. So, add
    // a public |invalidate| method to |WifiChip| and its child objects. This
    // will be used to mark an object invalid when either:
    // a) Wifi HAL is stopped, or
    // b) Wifi Chip is reconfigured.
    //
    // All AIDL method implementations should check if the object is still
    // marked valid before processing them.
    void invalidate();
    bool isValid();
    std::set<std::shared_ptr<IWifiChipEventCallback>> getEventCallbacks();

    // AIDL methods exposed.
    ndk::ScopedAStatus getId(int32_t* _aidl_return) override;
    ndk::ScopedAStatus registerEventCallback(
            const std::shared_ptr<IWifiChipEventCallback>& in_callback) override;
    ndk::ScopedAStatus getFeatureSet(int32_t* _aidl_return) override;
    ndk::ScopedAStatus getAvailableModes(std::vector<IWifiChip::ChipMode>* _aidl_return) override;
    ndk::ScopedAStatus configureChip(int32_t in_modeId) override;
    ndk::ScopedAStatus getMode(int32_t* _aidl_return) override;
    ndk::ScopedAStatus requestChipDebugInfo(IWifiChip::ChipDebugInfo* _aidl_return) override;
    ndk::ScopedAStatus requestDriverDebugDump(std::vector<uint8_t>* _aidl_return) override;
    ndk::ScopedAStatus requestFirmwareDebugDump(std::vector<uint8_t>* _aidl_return) override;
    ndk::ScopedAStatus createApIface(std::shared_ptr<IWifiApIface>* _aidl_return) override;
    ndk::ScopedAStatus createBridgedApIface(std::shared_ptr<IWifiApIface>* _aidl_return) override;
    ndk::ScopedAStatus createApOrBridgedApIface(
            IfaceConcurrencyType in_ifaceType,
            const std::vector<common::OuiKeyedData>& in_vendorData,
            std::shared_ptr<IWifiApIface>* _aidl_return) override;
    ndk::ScopedAStatus getApIfaceNames(std::vector<std::string>* _aidl_return) override;
    ndk::ScopedAStatus getApIface(const std::string& in_ifname,
                                  std::shared_ptr<IWifiApIface>* _aidl_return) override;
    ndk::ScopedAStatus removeApIface(const std::string& in_ifname) override;
    ndk::ScopedAStatus removeIfaceInstanceFromBridgedApIface(
            const std::string& in_brIfaceName, const std::string& in_ifaceInstanceName) override;
    ndk::ScopedAStatus createNanIface(std::shared_ptr<IWifiNanIface>* _aidl_return) override;
    ndk::ScopedAStatus getNanIfaceNames(std::vector<std::string>* _aidl_return) override;
    ndk::ScopedAStatus getNanIface(const std::string& in_ifname,
                                   std::shared_ptr<IWifiNanIface>* _aidl_return) override;
    ndk::ScopedAStatus removeNanIface(const std::string& in_ifname) override;
    ndk::ScopedAStatus createP2pIface(std::shared_ptr<IWifiP2pIface>* _aidl_return) override;
    ndk::ScopedAStatus getP2pIfaceNames(std::vector<std::string>* _aidl_return) override;
    ndk::ScopedAStatus getP2pIface(const std::string& in_ifname,
                                   std::shared_ptr<IWifiP2pIface>* _aidl_return) override;
    ndk::ScopedAStatus removeP2pIface(const std::string& in_ifname) override;
    ndk::ScopedAStatus createStaIface(std::shared_ptr<IWifiStaIface>* _aidl_return) override;
    ndk::ScopedAStatus getStaIfaceNames(std::vector<std::string>* _aidl_return) override;
    ndk::ScopedAStatus getStaIface(const std::string& in_ifname,
                                   std::shared_ptr<IWifiStaIface>* _aidl_return) override;
    ndk::ScopedAStatus removeStaIface(const std::string& in_ifname) override;
    ndk::ScopedAStatus createRttController(
            const std::shared_ptr<IWifiStaIface>& in_boundIface,
            std::shared_ptr<IWifiRttController>* _aidl_return) override;
    ndk::ScopedAStatus getDebugRingBuffersStatus(
            std::vector<WifiDebugRingBufferStatus>* _aidl_return) override;
    ndk::ScopedAStatus startLoggingToDebugRingBuffer(
            const std::string& in_ringName, WifiDebugRingBufferVerboseLevel in_verboseLevel,
            int32_t in_maxIntervalInSec, int32_t in_minDataSizeInBytes) override;
    ndk::ScopedAStatus forceDumpToDebugRingBuffer(const std::string& in_ringName) override;
    ndk::ScopedAStatus flushRingBufferToFile() override;
    ndk::ScopedAStatus stopLoggingToDebugRingBuffer() override;
    ndk::ScopedAStatus getDebugHostWakeReasonStats(
            WifiDebugHostWakeReasonStats* _aidl_return) override;
    ndk::ScopedAStatus enableDebugErrorAlerts(bool in_enable) override;
    ndk::ScopedAStatus selectTxPowerScenario(IWifiChip::TxPowerScenario in_scenario) override;
    ndk::ScopedAStatus resetTxPowerScenario() override;
    ndk::ScopedAStatus setLatencyMode(IWifiChip::LatencyMode in_mode) override;
    ndk::ScopedAStatus setMultiStaPrimaryConnection(const std::string& in_ifName) override;
    ndk::ScopedAStatus setMultiStaUseCase(IWifiChip::MultiStaUseCase in_useCase) override;
    ndk::ScopedAStatus setCoexUnsafeChannels(
            const std::vector<IWifiChip::CoexUnsafeChannel>& in_unsafeChannels,
            int32_t in_restrictions) override;
    ndk::ScopedAStatus setCountryCode(const std::array<uint8_t, 2>& in_code) override;
    ndk::ScopedAStatus getUsableChannels(WifiBand in_band, int32_t in_ifaceModeMask,
                                         int32_t in_filterMask,
                                         std::vector<WifiUsableChannel>* _aidl_return) override;
    ndk::ScopedAStatus setAfcChannelAllowance(
            const AfcChannelAllowance& afcChannelAllowance) override;
    ndk::ScopedAStatus triggerSubsystemRestart() override;
    ndk::ScopedAStatus getSupportedRadioCombinations(
            std::vector<WifiRadioCombination>* _aidl_return) override;
    ndk::ScopedAStatus getWifiChipCapabilities(WifiChipCapabilities* _aidl_return) override;
    ndk::ScopedAStatus enableStaChannelForPeerNetwork(
            int32_t in_channelCategoryEnableFlag) override;
    binder_status_t dump(int fd, const char** args, uint32_t numArgs) override;
    ndk::ScopedAStatus setMloMode(const ChipMloMode in_mode) override;
    ndk::ScopedAStatus setVoipMode(const VoipMode in_mode) override;

  private:
    void invalidateAndRemoveAllIfaces();
    // When a STA iface is removed any dependent NAN-ifaces/RTT-controllers are
    // invalidated & removed.
    void invalidateAndRemoveDependencies(const std::string& removed_iface_name);

    // Corresponding worker functions for the AIDL methods.
    std::pair<int32_t, ndk::ScopedAStatus> getIdInternal();
    ndk::ScopedAStatus registerEventCallbackInternal(
            const std::shared_ptr<IWifiChipEventCallback>& event_callback);
    std::pair<int32_t, ndk::ScopedAStatus> getFeatureSetInternal();
    std::pair<std::vector<IWifiChip::ChipMode>, ndk::ScopedAStatus> getAvailableModesInternal();
    ndk::ScopedAStatus configureChipInternal(std::unique_lock<std::recursive_mutex>* lock,
                                             int32_t mode_id);
    std::pair<int32_t, ndk::ScopedAStatus> getModeInternal();
    std::pair<IWifiChip::ChipDebugInfo, ndk::ScopedAStatus> requestChipDebugInfoInternal();
    std::pair<std::vector<uint8_t>, ndk::ScopedAStatus> requestDriverDebugDumpInternal();
    std::pair<std::vector<uint8_t>, ndk::ScopedAStatus> requestFirmwareDebugDumpInternal();
    std::shared_ptr<WifiApIface> newWifiApIface(std::string& ifname);
    ndk::ScopedAStatus createVirtualApInterface(const std::string& apVirtIf);
    std::pair<std::shared_ptr<IWifiApIface>, ndk::ScopedAStatus> createApIfaceInternal();
    std::pair<std::shared_ptr<IWifiApIface>, ndk::ScopedAStatus> createBridgedApIfaceInternal();
    std::pair<std::shared_ptr<IWifiApIface>, ndk::ScopedAStatus> createApOrBridgedApIfaceInternal(
            IfaceConcurrencyType ifaceType, const std::vector<common::OuiKeyedData>& vendorData);
    std::pair<std::vector<std::string>, ndk::ScopedAStatus> getApIfaceNamesInternal();
    std::pair<std::shared_ptr<IWifiApIface>, ndk::ScopedAStatus> getApIfaceInternal(
            const std::string& ifname);
    ndk::ScopedAStatus removeApIfaceInternal(const std::string& ifname);
    ndk::ScopedAStatus removeIfaceInstanceFromBridgedApIfaceInternal(
            const std::string& brIfaceName, const std::string& ifInstanceName);
    std::pair<std::shared_ptr<IWifiNanIface>, ndk::ScopedAStatus> createNanIfaceInternal();
    std::pair<std::vector<std::string>, ndk::ScopedAStatus> getNanIfaceNamesInternal();
    std::pair<std::shared_ptr<IWifiNanIface>, ndk::ScopedAStatus> getNanIfaceInternal(
            const std::string& ifname);
    ndk::ScopedAStatus removeNanIfaceInternal(const std::string& ifname);
    std::pair<std::shared_ptr<IWifiP2pIface>, ndk::ScopedAStatus> createP2pIfaceInternal();
    std::pair<std::vector<std::string>, ndk::ScopedAStatus> getP2pIfaceNamesInternal();
    std::pair<std::shared_ptr<IWifiP2pIface>, ndk::ScopedAStatus> getP2pIfaceInternal(
            const std::string& ifname);
    ndk::ScopedAStatus removeP2pIfaceInternal(const std::string& ifname);
    std::pair<std::shared_ptr<IWifiStaIface>, ndk::ScopedAStatus> createStaIfaceInternal();
    std::pair<std::vector<std::string>, ndk::ScopedAStatus> getStaIfaceNamesInternal();
    std::pair<std::shared_ptr<IWifiStaIface>, ndk::ScopedAStatus> getStaIfaceInternal(
            const std::string& ifname);
    ndk::ScopedAStatus removeStaIfaceInternal(const std::string& ifname);
    std::pair<std::shared_ptr<IWifiRttController>, ndk::ScopedAStatus> createRttControllerInternal(
            const std::shared_ptr<IWifiStaIface>& bound_iface);
    std::pair<std::vector<WifiDebugRingBufferStatus>, ndk::ScopedAStatus>
    getDebugRingBuffersStatusInternal();
    ndk::ScopedAStatus startLoggingToDebugRingBufferInternal(
            const std::string& ring_name, WifiDebugRingBufferVerboseLevel verbose_level,
            uint32_t max_interval_in_sec, uint32_t min_data_size_in_bytes);
    ndk::ScopedAStatus forceDumpToDebugRingBufferInternal(const std::string& ring_name);
    ndk::ScopedAStatus flushRingBufferToFileInternal();
    ndk::ScopedAStatus stopLoggingToDebugRingBufferInternal();
    std::pair<WifiDebugHostWakeReasonStats, ndk::ScopedAStatus>
    getDebugHostWakeReasonStatsInternal();
    ndk::ScopedAStatus enableDebugErrorAlertsInternal(bool enable);
    ndk::ScopedAStatus selectTxPowerScenarioInternal(IWifiChip::TxPowerScenario scenario);
    ndk::ScopedAStatus resetTxPowerScenarioInternal();
    ndk::ScopedAStatus setLatencyModeInternal(IWifiChip::LatencyMode mode);
    ndk::ScopedAStatus setMultiStaPrimaryConnectionInternal(const std::string& ifname);
    ndk::ScopedAStatus setMultiStaUseCaseInternal(IWifiChip::MultiStaUseCase use_case);
    ndk::ScopedAStatus setCoexUnsafeChannelsInternal(
            std::vector<IWifiChip::CoexUnsafeChannel> unsafe_channels, int32_t restrictions);
    ndk::ScopedAStatus setCountryCodeInternal(const std::array<uint8_t, 2>& in_code);
    std::pair<std::vector<WifiUsableChannel>, ndk::ScopedAStatus> getUsableChannelsInternal(
            WifiBand band, int32_t ifaceModeMask, int32_t filterMask);
    ndk::ScopedAStatus enableStaChannelForPeerNetworkInternal(int32_t channelCategoryEnableFlag);
    ndk::ScopedAStatus setAfcChannelAllowanceInternal(
            const AfcChannelAllowance& afcChannelAllowance);
    ndk::ScopedAStatus handleChipConfiguration(std::unique_lock<std::recursive_mutex>* lock,
                                               int32_t mode_id);
    ndk::ScopedAStatus registerDebugRingBufferCallback();
    ndk::ScopedAStatus registerRadioModeChangeCallback();

    std::vector<ChipConcurrencyCombination> getCurrentModeConcurrencyCombinations();
    std::map<IfaceConcurrencyType, size_t> getCurrentConcurrencyCombination();
    std::vector<std::map<IfaceConcurrencyType, size_t>> expandConcurrencyCombinations(
            const ChipConcurrencyCombination& combination);
    bool canExpandedConcurrencyComboSupportConcurrencyTypeWithCurrentTypes(
            const std::map<IfaceConcurrencyType, size_t>& expanded_combo,
            IfaceConcurrencyType requested_type);
    bool canCurrentModeSupportConcurrencyTypeWithCurrentTypes(IfaceConcurrencyType requested_type);
    bool canExpandedConcurrencyComboSupportConcurrencyCombo(
            const std::map<IfaceConcurrencyType, size_t>& expanded_combo,
            const std::map<IfaceConcurrencyType, size_t>& req_combo);
    bool canCurrentModeSupportConcurrencyCombo(
            const std::map<IfaceConcurrencyType, size_t>& req_combo);
    bool canCurrentModeSupportConcurrencyType(IfaceConcurrencyType requested_type);

    bool isValidModeId(int32_t mode_id);
    bool isStaApConcurrencyAllowedInCurrentMode();
    bool isDualStaConcurrencyAllowedInCurrentMode();
    uint32_t startIdxOfApIface();
    std::string getFirstActiveWlanIfaceName();
    std::string allocateApOrStaIfaceName(IfaceType type, uint32_t start_idx);
    std::string allocateApIfaceName();
    std::vector<std::string> allocateBridgedApInstanceNames();
    std::string allocateStaIfaceName();
    bool writeRingbufferFilesInternal();
    std::string getWlanIfaceNameWithType(IfaceType type, unsigned idx);
    void invalidateAndClearBridgedApAll();
    void deleteApIface(const std::string& if_name);
    bool findUsingNameFromBridgedApInstances(const std::string& name);
    ndk::ScopedAStatus triggerSubsystemRestartInternal();
    std::pair<std::vector<WifiRadioCombination>, ndk::ScopedAStatus>
    getSupportedRadioCombinationsInternal();
    std::pair<WifiChipCapabilities, ndk::ScopedAStatus> getWifiChipCapabilitiesInternal();
    ndk::ScopedAStatus setMloModeInternal(const ChipMloMode in_mode);
    ndk::ScopedAStatus setVoipModeInternal(const VoipMode in_mode);
    void retrieveDynamicIfaceCombination();
    void setWeakPtr(std::weak_ptr<WifiChip> ptr);

    int32_t chip_id_;
    std::weak_ptr<legacy_hal::WifiLegacyHal> legacy_hal_;
    std::weak_ptr<mode_controller::WifiModeController> mode_controller_;
    std::shared_ptr<iface_util::WifiIfaceUtil> iface_util_;
    std::vector<std::shared_ptr<WifiApIface>> ap_ifaces_;
    std::vector<std::shared_ptr<WifiNanIface>> nan_ifaces_;
    std::vector<std::shared_ptr<WifiP2pIface>> p2p_ifaces_;
    std::vector<std::shared_ptr<WifiStaIface>> sta_ifaces_;
    std::vector<std::shared_ptr<WifiRttController>> rtt_controllers_;
    std::map<std::string, Ringbuffer> ringbuffer_map_;
    bool is_valid_;
    // Members pertaining to chip configuration.
    int32_t current_mode_id_;
    std::mutex lock_t;
    std::vector<IWifiChip::ChipMode> modes_;
    // The legacy ring buffer callback API has only a global callback
    // registration mechanism. Use this to check if we have already
    // registered a callback.
    bool debug_ring_buffer_cb_registered_;
    bool using_dynamic_iface_combination_;
    aidl_callback_util::AidlCallbackHandler<IWifiChipEventCallback> event_cb_handler_;
    std::weak_ptr<WifiChip> weak_ptr_this_;

    const std::function<void(const std::string&)> subsystemCallbackHandler_;
    std::map<std::string, std::vector<std::string>> br_ifaces_ap_instances_;
    DISALLOW_COPY_AND_ASSIGN(WifiChip);
};

}  // namespace wifi
}  // namespace hardware
}  // namespace android
}  // namespace aidl

#endif  // WIFI_CHIP_H_
