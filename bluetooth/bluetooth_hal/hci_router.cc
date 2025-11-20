/*
 * Copyright 2024 The Android Open Source Project
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

#define LOG_TAG "bthal.hci_router"

#include "bluetooth_hal/hci_router.h"

#include <stddef.h>

#include <atomic>
#include <functional>
#include <future>
#include <memory>
#include <mutex>
#include <unordered_map>
#include <unordered_set>

#include "bluetooth_hal/chip/async_chip_provisioner.h"
#include "bluetooth_hal/config/hal_config_loader.h"
#include "bluetooth_hal/debug/debug_central.h"
#include "bluetooth_hal/debug/vnd_snoop_logger.h"
#include "bluetooth_hal/extensions/thread/thread_handler.h"
#include "bluetooth_hal/hal_packet.h"
#include "bluetooth_hal/hal_types.h"
#include "bluetooth_hal/hci_monitor.h"
#include "bluetooth_hal/hci_router_callback.h"
#include "bluetooth_hal/hci_router_client_agent.h"
#include "bluetooth_hal/transport/transport_interface.h"
#include "bluetooth_hal/util/power/wakelock.h"
#include "bluetooth_hal/util/worker.h"

namespace bluetooth_hal {
namespace hci {

using ::bluetooth_hal::HalState;
using ::bluetooth_hal::chip::AsyncChipProvisioner;
using ::bluetooth_hal::config::HalConfigLoader;
using ::bluetooth_hal::debug::VndSnoopLogger;
using ::bluetooth_hal::hci::HciPacketType;
using ::bluetooth_hal::thread::ThreadHandler;
using ::bluetooth_hal::transport::TransportInterface;
using ::bluetooth_hal::transport::TransportInterfaceCallback;
using ::bluetooth_hal::util::power::ScopedWakelock;
using ::bluetooth_hal::util::power::Wakelock;
using ::bluetooth_hal::util::power::WakeSource;

class TxTask {
 public:
  enum class TxTaskType : int {
    kSendOrQueueCommand = 1,
    kGetCommandCallback = 2,
    kOnCommandCallbackCompleted = 3,
    kSendToTransport = 4,
  };

  static TxTask SendOrQueueCommand(
      HalPacket packet, const std::shared_ptr<HalPacketCallback> callback) {
    return TxTask{TxTaskType::kSendOrQueueCommand, std::move(packet), callback,
                  std::promise<std::shared_ptr<HalPacketCallback>>()};
  }

  static TxTask GetCommandCallback(
      HalPacket packet,
      std::promise<std::shared_ptr<HalPacketCallback>>&& promise) {
    return TxTask{TxTaskType::kGetCommandCallback, std::move(packet), nullptr,
                  std::move(promise)};
  }

  static TxTask OnCommandCallbackCompleted() {
    return TxTask{TxTaskType::kOnCommandCallbackCompleted, HalPacket(), nullptr,
                  std::promise<std::shared_ptr<HalPacketCallback>>()};
  }

  static TxTask SendToTransport(HalPacket packet) {
    return TxTask{TxTaskType::kSendToTransport, std::move(packet), nullptr,
                  std::promise<std::shared_ptr<HalPacketCallback>>()};
  }

  TxTaskType type;
  HalPacket packet;
  std::shared_ptr<HalPacketCallback> callback;
  std::promise<std::shared_ptr<HalPacketCallback>>&& promise;

 private:
  TxTask(TxTaskType type, HalPacket&& packet,
         std::shared_ptr<HalPacketCallback> callback,
         std::promise<std::shared_ptr<HalPacketCallback>>&& promise)
      : type(type),
        packet(std::move(packet)),
        callback(callback),
        promise(std::move(promise)) {};
};

class TxHandler {
 public:
  TxHandler() {
    tx_thread_ = std::make_unique<util::Worker<TxTask>>(
        std::bind_front(&TxHandler::PacketDispatcher, this));
  }

  ~TxHandler() { SetBusy(false); }

  void Post(TxTask task) { tx_thread_->Post(std::move(task)); }

 private:
  struct QueuedHciCommand {
   public:
    HalPacket command;
    std::shared_ptr<HalPacketCallback> callback;
  };

  void PacketDispatcher(TxTask task) {
    switch (task.type) {
      case TxTask::TxTaskType::kSendOrQueueCommand:
        SendOrQueueCommand(task.packet, task.callback);
        break;
      case TxTask::TxTaskType::kGetCommandCallback:
        GetCommandCallback(task.packet, std::move(task.promise));
        break;
      case TxTask::TxTaskType::kOnCommandCallbackCompleted:
        OnCommandCallbackCompleted();
        break;
      case TxTask::TxTaskType::kSendToTransport:
        SendToTransport(task.packet);
        break;
      default:
        LOG(ERROR) << "Unknown TxTask type: " << static_cast<int>(task.type);
        break;
    }
  }

  bool SendOrQueueCommand(const HalPacket& packet,
                          const std::shared_ptr<HalPacketCallback> callback) {
    bool is_queue_busy = !hci_cmd_queue_.empty();
    hci_cmd_queue_.emplace(QueuedHciCommand(packet, callback));

    if (is_queue_busy) {
      // Queue the current command and wait for the previous command to be
      // completed.
      LOG(INFO) << "command queued: " << packet.ToString();
      return true;
    }

    SetBusy(true);

    SendToTransport(packet);
    return true;
  }

  void GetCommandCallback(
      const HalPacket& event,
      std::promise<std::shared_ptr<HalPacketCallback>>&& promise) {
    uint16_t opcode = event.GetCommandOpcodeFromGeneratedEvent();
    if (hci_cmd_queue_.empty() ||
        hci_cmd_queue_.front().command.GetCommandOpcode() != opcode) {
      // TODO: b/387255243 - Check if this error requires an abort().
      LOG(ERROR)
          << "Unexpected command complete or command status event! opcode="
          << opcode;
      promise.set_value(nullptr);
      return;
    }
    std::shared_ptr<HalPacketCallback> callback =
        hci_cmd_queue_.front().callback;
    promise.set_value(callback);
  }

  void OnCommandCallbackCompleted() {
    if (hci_cmd_queue_.empty()) {
      LOG(ERROR) << "Unexpected callback completed! "
                 << "No command callback found in queue.";
      return;
    }
    hci_cmd_queue_.pop();

    bool has_queued_command = !hci_cmd_queue_.empty();
    SetBusy(has_queued_command);
    if (has_queued_command) {
      HalPacket queued_command = hci_cmd_queue_.front().command;
      SendToTransport(queued_command);
    }
  }

  bool SendToTransport(const HalPacket& packet) {
    ScopedWakelock wakelock(WakeSource::kTx);
    if (!TransportInterface::GetTransport().IsTransportActive()) {
      LOG(ERROR) << "Transport not active! packet: " << packet.ToString();
      return false;
    }

    VndSnoopLogger::GetLogger().Capture(packet,
                                        VndSnoopLogger::Direction::kOutgoing);

    return TransportInterface::GetTransport().Send(packet);
  }

  void SetBusy(bool busy) {
    if (busy) {
      Wakelock::GetWakelock().Acquire(WakeSource::kHciBusy);
    } else {
      Wakelock::GetWakelock().Release(WakeSource::kHciBusy);
    }

    is_busy_ = busy;
    TransportInterface::GetTransport().SetHciRouterBusy(busy);
  }

  std::queue<QueuedHciCommand> hci_cmd_queue_;
  std::unique_ptr<util::Worker<TxTask>> tx_thread_;
  std::atomic<bool> is_busy_;
};

class HciRouterImpl : virtual public HciRouter,
                      virtual public TransportInterfaceCallback {
 public:
  HciRouterImpl();
  bool Initialize(const std::shared_ptr<HciRouterCallback>& callback) override;
  void Cleanup() override;
  bool Send(const HalPacket& packet) override;
  bool SendCommand(const HalPacket& packet,
                   const HalPacketCallback& callback) override;
  bool SendCommandNoAck(const HalPacket& packet) override;
  HalState GetHalState() override;
  void UpdateHalState(HalState state) override;
  void SendPacketToStack(const HalPacket& packet) override;

  void OnTransportClosed() override;
  void OnTransportPacketReady(const HalPacket& packet) override;

 protected:
  bool InitializeModules();
  bool SendToTransport(const HalPacket& packet);
  void HandleCommandCompleteOrCommandStatusEvent(const HalPacket& event);
  bool InitializeTransport();
  bool IsHalStateValid(HalState new_state);
  void HandleReceivedPacket(const HalPacket& packet);

  // callback for the stack.
  std::shared_ptr<HciRouterCallback> hci_callback_;
  HalState hal_state_ = HalState::kShutdown;
  std::unique_ptr<TxHandler> tx_handler_;
  std::recursive_mutex mutex_;

  static const std::unordered_map<HalState, std::unordered_set<HalState>>
      kHalStateMachine;
};

/*
 * kHalStateMachine contains the sequence of the HciRouter state machine.
 * The Shutdown state, BtChipReady state and Running state are static states.
 * The state machine stays in the Shutdown state if the Bluetooth chip is
 * powered off. The state machine stays in the BtChipReady state if the
 * controller is fully ready, including Bluetooth is off when the "Accelerate BT
 * ON" feature is enabled. The state machine stays in the Running state after
 * the Bluetooth stack sends the first HCI_RESET command, indicating the
 * Bluetooth process is ready.
 *
 * All states can switch to the Shutdown state for error handling.
 *
 *                         ╔═══╗
 *                         ║   v
 *          ╔═══════════ kShutdown <═══════════╦══════════════════╗
 *          ║               ^                  ║                  ║
 *          v               ║                  ║                  ║
 *        kInit ════════════╣             kBtChipReady <════> kRunning
 *          ║               ║                  ^
 *          v               ║                  ║
 *  kFirmwareDownloading════╬══════════ kFirmwareReady
 *          ║               ║                  ^
 *          ║               ║                  ║
 *          ╚══> kFirmwareDownloadCompleted ═══╝
 *
 * Format of the map: {CurrentState, {ValidNextState1, ValidNextState2, ...}}
 */
const std::unordered_map<HalState, std::unordered_set<HalState>>
    HciRouterImpl::kHalStateMachine = {
        {HalState::kShutdown, {HalState::kShutdown, HalState::kInit}},
        {HalState::kInit,
         {HalState::kShutdown, HalState::kFirmwareDownloading}},
        {HalState::kFirmwareDownloading,
         {HalState::kShutdown, HalState::kFirmwareDownloadCompleted}},
        {HalState::kFirmwareDownloadCompleted,
         {HalState::kShutdown, HalState::kFirmwareReady}},
        {HalState::kFirmwareReady,
         {HalState::kShutdown, HalState::kBtChipReady}},
        {HalState::kBtChipReady,
         {HalState::kShutdown, HalState::kBtChipReady, HalState::kRunning}},
        {HalState::kRunning, {HalState::kShutdown, HalState::kBtChipReady}},
};

HciRouterImpl::HciRouterImpl() {
  if (HalConfigLoader::GetLoader().IsAcceleratedBtOnSupported()) {
    // Power ON Bluetooth chip and download firmware if Accelerated BT ON
    // feature is supported.
    LOG(INFO) << "Powering ON Bluetooth chip for Accelerated BT ON.";
    InitializeModules();
  }
}

bool HciRouterImpl::Initialize(
    const std::shared_ptr<HciRouterCallback>& callback) {
  DURATION_TRACKER(AnchorType::BTHAL_PERFORM_INIT, __func__);
  std::scoped_lock<std::recursive_mutex> lock(mutex_);
  LOG(INFO) << "Initializing Bluetooth HCI Router.";
  hci_callback_ = callback;
  return InitializeModules();
}

bool HciRouterImpl::InitializeModules() {
  std::scoped_lock<std::recursive_mutex> lock(mutex_);
  switch (hal_state_) {
    case HalState::kRunning:
      LOG(WARNING) << "HciRouter has already initialized!";
      return false;
    case HalState::kShutdown:
      // Exit from the switch-case and continue the initialization process.
      break;
    case HalState::kBtChipReady:
      if (HalConfigLoader::GetLoader().IsAcceleratedBtOnSupported()) {
#ifndef UNIT_TEST
        AsyncChipProvisioner::GetProvisioner().PostResetFirmware();
#endif
        return true;
      }
      [[fallthrough]];
    default:
      LOG(WARNING) << "HciRouter is initializing!";
      return true;
  }

  UpdateHalState(HalState::kInit);

  // Initialize TX Handler to process TX packets.
  tx_handler_ = std::make_unique<TxHandler>();

  // Initialize transport.
  if (!InitializeTransport()) {
    LOG(ERROR) << "Failed to initialize transport!";
    Cleanup();
    return false;
  }

  LOG(INFO) << "Start downloading Bluetooth firmware.";
#ifndef UNIT_TEST
  AsyncChipProvisioner::GetProvisioner().PostInitialize(
      std::bind_front(&HciRouterImpl::UpdateHalState, this));
  AsyncChipProvisioner::GetProvisioner().PostDownloadFirmware();
#endif

  return true;
}

void HciRouterImpl::Cleanup() {
  std::scoped_lock<std::recursive_mutex> lock(mutex_);
  if (hal_state_ == HalState::kRunning &&
      HalConfigLoader::GetLoader().IsAcceleratedBtOnSupported()) {
    // Keep the Bluetooth chip powered on and only reset firmware if the
    // Accelerated BT On feature is supported.
#ifndef UNIT_TEST
    AsyncChipProvisioner::GetProvisioner().PostResetFirmware();
#endif
    return;
  }

  if (tx_handler_) {
    tx_handler_.reset();
  }

  // Cleanup Thread manager.
  if (ThreadHandler::IsHandlerRunning()) {
    ThreadHandler::Cleanup();
  }

  // Cleanup transport.
  if (TransportInterface::GetTransport().IsTransportActive()) {
    TransportInterface::GetTransport().Cleanup();
  }

  // Set HAL state back to the default state (kShutdown).
  UpdateHalState(HalState::kShutdown);
  hci_callback_ = nullptr;
}

bool HciRouterImpl::Send(const HalPacket& packet) {
  if (packet.GetType() == HciPacketType::kCommand) {
    // HCI commands require separate handling to manage command flow control.
    // The events for the commands sent over Send() will be received by the
    // stack through hci_callback_.
    return SendCommand(
        packet,
        std::bind_front(&HciRouterCallback::OnCommandCallback, hci_callback_));
  }
  tx_handler_->Post(TxTask::SendToTransport(packet));
  return true;
}

bool HciRouterImpl::SendCommand(const HalPacket& packet,
                                const HalPacketCallback& callback) {
  tx_handler_->Post(TxTask::SendOrQueueCommand(
      packet, std::make_shared<HalPacketCallback>(callback)));
  return true;
}

bool HciRouterImpl::SendCommandNoAck(const HalPacket& packet) {
  tx_handler_->Post(TxTask::SendToTransport(packet));
  return true;
}

HalState HciRouterImpl::GetHalState() {
  std::unique_lock<std::recursive_mutex> lock(mutex_);
  return hal_state_;
}

void HciRouterImpl::SendPacketToStack(const HalPacket& packet) {
  HandleReceivedPacket(packet);
}

bool HciRouterImpl::InitializeTransport() {
  LOG(INFO) << "Initializing Bluetooth transport.";
  return TransportInterface::GetTransport().Initialize(this);
}

void HciRouterImpl::UpdateHalState(HalState state) {
  std::scoped_lock<std::recursive_mutex> lock(mutex_);
  LOG(INFO) << "Bluetooth HAL state changed: " << static_cast<int>(hal_state_)
            << " -> " << static_cast<int>(state);
  if (!IsHalStateValid(state)) {
    LOG(FATAL) << "Invalid Bluetooth HAL state changed! "
               << static_cast<int>(hal_state_) << " -> "
               << static_cast<int>(state);
  }
  HalState old_state = hal_state_;
  hal_state_ = state;

  std::shared_ptr<void> defer_task;

  switch (state) {
    case HalState::kShutdown:
      VndSnoopLogger::GetLogger().StopRecording();
      break;
    case HalState::kInit:
      // New recording for BT OFF.
      VndSnoopLogger::GetLogger().StartNewRecording();
      break;
    case HalState::kFirmwareDownloading:
    case HalState::kFirmwareDownloadCompleted:
    case HalState::kFirmwareReady:
      break;
    case HalState::kBtChipReady:
      if (HalConfigLoader::GetLoader().IsAcceleratedBtOnSupported()) {
        if (old_state == HalState::kRunning) {
          // Bluetooth turned OFF with Accelerated BT ON enabled.
          // New recording for BT OFF.
          VndSnoopLogger::GetLogger().StartNewRecording();
        } else if (old_state == HalState::kFirmwareReady) {
          if (HalConfigLoader::GetLoader().IsThreadDispatcherEnabled()) {
            LOG(INFO) << "Initialize Thread handler.";
            ThreadHandler::Initialize();
          }
        }
      }
      if (old_state == HalState::kFirmwareReady && hci_callback_ != nullptr) {
        // Once HAL changes to chip ready, it will automatically update to the
        // running state if the stack had called Initialize.
        defer_task = std::shared_ptr<void>(
            nullptr, [this](void*) { UpdateHalState(HalState::kRunning); });
      }
      break;
    case HalState::kRunning:
      VndSnoopLogger::GetLogger().StartNewRecording();
      if (HalConfigLoader::GetLoader().IsThreadDispatcherEnabled() &&
          !HalConfigLoader::GetLoader().IsAcceleratedBtOnSupported()) {
        LOG(INFO) << "Initialize Thread handler.";
        ThreadHandler::Initialize();
      }
      break;
    default:
      break;
  }

  // The Bluetooth stack needs to be the first to know about the state change to
  // avoid edge cases.
  if (hci_callback_ != nullptr) {
    hci_callback_->OnHalStateChanged(state, old_state);
  }
  HciRouterClientAgent::GetAgent().NotifyHalStateChange(state, old_state);

  TransportInterface::GetTransport().NotifyHalStateChange(state);
}

bool HciRouterImpl::IsHalStateValid(HalState new_state) {
  std::scoped_lock<std::recursive_mutex> lock(mutex_);
  return kHalStateMachine.at(hal_state_).count(new_state) > 0;
}

void HciRouterImpl::HandleReceivedPacket(const HalPacket& packet) {
  std::scoped_lock<std::recursive_mutex> lock(mutex_);
  if (packet.IsCommandCompleteStatusEvent()) {
    HandleCommandCompleteOrCommandStatusEvent(packet);
    return;
  }
  if (HciRouterClientAgent::GetAgent().DispatchPacketToClients(packet) !=
          MonitorMode::kIntercept &&
      hci_callback_ != nullptr) {
    hci_callback_->OnPacketCallback(packet);
  }
}

void HciRouterImpl::HandleCommandCompleteOrCommandStatusEvent(
    const HalPacket& event) {
  std::scoped_lock<std::recursive_mutex> lock(mutex_);
  uint16_t opcode = event.GetCommandOpcodeFromGeneratedEvent();

  std::promise<std::shared_ptr<HalPacketCallback>> promise;
  std::future<std::shared_ptr<HalPacketCallback>> future = promise.get_future();
  tx_handler_->Post(TxTask::GetCommandCallback(event, std::move(promise)));

  std::shared_ptr<HalPacketCallback> callback = future.get();
  if (callback == nullptr || (*callback) == nullptr) {
    LOG(ERROR) << "Command callback is null!";
    if (hci_callback_ != nullptr) {
      hci_callback_->OnPacketCallback(event);
    }
    return;
  }

  if (HciRouterClientAgent::GetAgent().DispatchPacketToClients(event) !=
      MonitorMode::kIntercept) {
    (*callback)(event);
  }

  tx_handler_->Post(TxTask::OnCommandCallbackCompleted());
}

void HciRouterImpl::OnTransportPacketReady(const HalPacket& packet) {
  ScopedWakelock wakelock(WakeSource::kRx);

  std::scoped_lock<std::recursive_mutex> lock(mutex_);
  if (hal_state_ == HalState::kShutdown) {
    LOG(WARNING) << __func__ << ": Hal is not ready to receive packets.";
    return;
  }

  VndSnoopLogger::GetLogger().Capture(packet,
                                      VndSnoopLogger::Direction::kIncoming);

  HandleReceivedPacket(packet);
}

void HciRouterImpl::OnTransportClosed() {
  LOG(INFO) << __func__ << ": Current transport is closed.";
}

HciRouter& HciRouter::GetRouter() {
  static HciRouterImpl router;
  return router;
}

}  // namespace hci
}  // namespace bluetooth_hal
