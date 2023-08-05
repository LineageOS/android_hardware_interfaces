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

#pragma once

#include "Stream.h"

namespace aidl::android::hardware::audio::core {

// 'StreamSwitcher' is an implementation of 'StreamCommonInterface' which allows
// dynamically switching the underlying stream implementation based on currently
// connected devices. This is achieved by replacing inheritance from
// 'StreamCommonImpl' with owning an instance of it. StreamSwitcher must be
// extended in order to supply the logic for choosing the stream
// implementation. When there are no connected devices, for instance, upon the
// creation, the StreamSwitcher engages an instance of a stub stream in order to
// keep serving requests coming via 'StreamDescriptor'.
//
// StreamSwitcher implements the 'IStreamCommon' interface directly, with
// necessary delegation to the current stream implementation. While the stub
// stream is engaged, any requests made via 'IStreamCommon' (parameters, effects
// setting, etc) are postponed and only delivered on device connection change
// to the "real" stream implementation provided by the extending class. This is why
// the behavior of StreamSwitcher in the "stub" state is not identical to behavior
// of 'StreamStub'. It can become a full substitute for 'StreamStub' once
// device connection change event occurs and the extending class returns
// 'LEAVE_CURRENT_STREAM' from 'switchCurrentStream' method.
//
// There is a natural limitation that the current stream implementation may only
// be switched when the stream is in the 'STANDBY' state. Thus, when the event
// to switch the stream occurs, the current stream is stopped and joined, and
// its last state is validated. Since the change of the set of connected devices
// normally occurs on patch updates, if the stream was not in standby, this is
// reported to the caller of 'IModule.setAudioPatch' as the 'EX_ILLEGAL_STATE'
// error.
//
// The simplest use case, when the implementor just needs to emulate the legacy HAL API
// behavior of receiving the connected devices upon stream creation, the implementation
// of the extending class can look as follows. We assume that 'StreamLegacy' implementation
// is the one requiring to know connected devices on creation:
//
//   class StreamLegacy : public StreamCommonImpl {
//     public:
//       StreamLegacy(StreamContext* context, const Metadata& metadata,
//                    const std::vector<AudioDevice>& devices);
//   };
//
//   class StreamOutLegacy final : public StreamOut, public StreamSwitcher {
//     public:
//       StreamOutLegacy(StreamContext&& context, metatadata etc.)
//     private:
//       DeviceSwitchBehavior switchCurrentStream(const std::vector<AudioDevice>&) override {
//           // This implementation effectively postpones stream creation until
//           // receiving the first call to 'setConnectedDevices' with a non-empty list.
//           return isStubStream() ? DeviceSwitchBehavior::CREATE_NEW_STREAM :
//               DeviceSwitchBehavior::USE_CURRENT_STREAM;
//       }
//       std::unique_ptr<StreamCommonInterfaceEx> createNewStream(
//               const std::vector<AudioDevice>& devices,
//               StreamContext* context, const Metadata& metadata) override {
//           return std::unique_ptr<StreamCommonInterfaceEx>(new InnerStreamWrapper<StreamLegacy>(
//               context, metadata, devices));
//       }
//       void onClose(StreamDescriptor::State) override { defaultOnClose(); }
//   }
//

class StreamCommonInterfaceEx : virtual public StreamCommonInterface {
  public:
    virtual StreamDescriptor::State getStatePriorToClosing() const = 0;
};

template <typename T>
class InnerStreamWrapper : public T, public StreamCommonInterfaceEx {
  public:
    template <typename... Args>
    InnerStreamWrapper(Args&&... args) : T(std::forward<Args>(args)...) {}
    StreamDescriptor::State getStatePriorToClosing() const override { return mStatePriorToClosing; }

  private:
    // Do not need to do anything on close notification from the inner stream
    // because StreamSwitcher handles IStreamCommon::close by itself.
    void onClose(StreamDescriptor::State statePriorToClosing) override {
        mStatePriorToClosing = statePriorToClosing;
    }

    StreamDescriptor::State mStatePriorToClosing = StreamDescriptor::State::STANDBY;
};

class StreamSwitcher : virtual public StreamCommonInterface {
  public:
    StreamSwitcher(StreamContext* context, const Metadata& metadata);

    ndk::ScopedAStatus close() override;
    ndk::ScopedAStatus prepareToClose() override;
    ndk::ScopedAStatus updateHwAvSyncId(int32_t in_hwAvSyncId) override;
    ndk::ScopedAStatus getVendorParameters(const std::vector<std::string>& in_ids,
                                           std::vector<VendorParameter>* _aidl_return) override;
    ndk::ScopedAStatus setVendorParameters(const std::vector<VendorParameter>& in_parameters,
                                           bool in_async) override;
    ndk::ScopedAStatus addEffect(
            const std::shared_ptr<::aidl::android::hardware::audio::effect::IEffect>& in_effect)
            override;
    ndk::ScopedAStatus removeEffect(
            const std::shared_ptr<::aidl::android::hardware::audio::effect::IEffect>& in_effect)
            override;

    ndk::ScopedAStatus getStreamCommonCommon(std::shared_ptr<IStreamCommon>* _aidl_return) override;
    ndk::ScopedAStatus updateMetadataCommon(const Metadata& metadata) override;

    ndk::ScopedAStatus initInstance(
            const std::shared_ptr<StreamCommonInterface>& delegate) override;
    const StreamContext& getContext() const override;
    bool isClosed() const override;
    const ConnectedDevices& getConnectedDevices() const override;
    ndk::ScopedAStatus setConnectedDevices(
            const std::vector<::aidl::android::media::audio::common::AudioDevice>& devices)
            override;
    ndk::ScopedAStatus bluetoothParametersUpdated() override;

  protected:
    // Since switching a stream requires closing down the current stream, StreamSwitcher
    // asks the extending class its intent on the connected devices change.
    enum DeviceSwitchBehavior {
        // Continue using the current stream implementation. If it's the stub implementation,
        // StreamSwitcher starts treating the stub stream as a "real" implementation,
        // without effectively closing it and starting again.
        USE_CURRENT_STREAM,
        // This is the normal case when the extending class provides a "real" implementation
        // which is not a stub implementation.
        CREATE_NEW_STREAM,
        // This is the case when the extending class wants to revert back to the initial
        // condition of using a stub stream provided by the StreamSwitcher. This behavior
        // is only allowed when the list of connected devices is empty.
        SWITCH_TO_STUB_STREAM,
        // Use when the set of devices is not supported by the extending class. This returns
        // 'EX_UNSUPPORTED_OPERATION' from 'setConnectedDevices'.
        UNSUPPORTED_DEVICES,
    };
    // StreamSwitcher will call these methods from 'setConnectedDevices'. If the switch behavior
    // is 'CREATE_NEW_STREAM', the 'createwNewStream' function will be called (with the same
    // device vector) for obtaining a new stream implementation, assuming that closing
    // the current stream was a success.
    virtual DeviceSwitchBehavior switchCurrentStream(
            const std::vector<::aidl::android::media::audio::common::AudioDevice>& devices) = 0;
    virtual std::unique_ptr<StreamCommonInterfaceEx> createNewStream(
            const std::vector<::aidl::android::media::audio::common::AudioDevice>& devices,
            StreamContext* context, const Metadata& metadata) = 0;
    virtual void onClose(StreamDescriptor::State streamPriorToClosing) = 0;

    bool isStubStream() const { return mIsStubStream; }
    StreamCommonInterfaceEx* getCurrentStream() const { return mStream.get(); }

  private:
    using VndParam = std::pair<std::vector<VendorParameter>, bool /*isAsync*/>;

    static constexpr bool isValidClosingStreamState(StreamDescriptor::State state) {
        return state == StreamDescriptor::State::STANDBY || state == StreamDescriptor::State::ERROR;
    }

    ndk::ScopedAStatus closeCurrentStream(bool validateStreamState);

    // StreamSwitcher does not own the context.
    StreamContext* mContext;
    Metadata mMetadata;
    ChildInterface<StreamCommonDelegator> mCommon;
    // The current stream.
    std::unique_ptr<StreamCommonInterfaceEx> mStream;
    // Indicates whether 'mCurrentStream' is a stub stream implementation
    // maintained by StreamSwitcher until the extending class provides a "real"
    // implementation. The invariant of this state is that there are no connected
    // devices.
    bool mIsStubStream = true;
    // Storage for the data from commands received via 'IStreamCommon'.
    std::optional<int32_t> mHwAvSyncId;
    std::vector<VndParam> mMissedParameters;
    std::vector<std::shared_ptr<::aidl::android::hardware::audio::effect::IEffect>> mEffects;
    bool mBluetoothParametersUpdated = false;
};

}  // namespace aidl::android::hardware::audio::core
