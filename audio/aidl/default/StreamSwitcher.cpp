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

#include <limits>

#define LOG_TAG "AHAL_StreamSwitcher"

#include <Utils.h>
#include <android-base/logging.h>
#include <error/expected_utils.h>

#include "core-impl/StreamStub.h"
#include "core-impl/StreamSwitcher.h"

using aidl::android::hardware::audio::effect::IEffect;
using aidl::android::media::audio::common::AudioDevice;

namespace aidl::android::hardware::audio::core {

StreamSwitcher::StreamSwitcher(StreamContext* context, const Metadata& metadata)
    : mContext(context),
      mMetadata(metadata),
      mStream(new InnerStreamWrapper<StreamStub>(context, mMetadata)) {}

ndk::ScopedAStatus StreamSwitcher::closeCurrentStream(bool validateStreamState) {
    if (!mStream) return ndk::ScopedAStatus::ok();
    RETURN_STATUS_IF_ERROR(mStream->prepareToClose());
    RETURN_STATUS_IF_ERROR(mStream->close());
    if (validateStreamState && !isValidClosingStreamState(mStream->getStatePriorToClosing())) {
        return ndk::ScopedAStatus::fromExceptionCode(EX_ILLEGAL_STATE);
    }
    mStream.reset();
    return ndk::ScopedAStatus::ok();
}

ndk::ScopedAStatus StreamSwitcher::close() {
    if (mStream != nullptr) {
        auto status = closeCurrentStream(false /*validateStreamState*/);
        // The actual state is irrelevant since only StreamSwitcher cares about it.
        onClose(StreamDescriptor::State::STANDBY);
        return status;
    }
    LOG(ERROR) << __func__ << ": stream was already closed";
    return ndk::ScopedAStatus::fromExceptionCode(EX_ILLEGAL_STATE);
}

ndk::ScopedAStatus StreamSwitcher::prepareToClose() {
    if (mStream != nullptr) {
        return mStream->prepareToClose();
    }
    LOG(ERROR) << __func__ << ": stream was closed";
    return ndk::ScopedAStatus::fromExceptionCode(EX_ILLEGAL_STATE);
}

ndk::ScopedAStatus StreamSwitcher::updateHwAvSyncId(int32_t in_hwAvSyncId) {
    if (mStream == nullptr) {
        LOG(ERROR) << __func__ << ": stream was closed";
        return ndk::ScopedAStatus::fromExceptionCode(EX_ILLEGAL_STATE);
    }
    RETURN_STATUS_IF_ERROR(mStream->updateHwAvSyncId(in_hwAvSyncId));
    mHwAvSyncId = in_hwAvSyncId;
    return ndk::ScopedAStatus::ok();
}

ndk::ScopedAStatus StreamSwitcher::getVendorParameters(const std::vector<std::string>& in_ids,
                                                       std::vector<VendorParameter>* _aidl_return) {
    if (mStream == nullptr) {
        LOG(ERROR) << __func__ << ": stream was closed";
        return ndk::ScopedAStatus::fromExceptionCode(EX_ILLEGAL_STATE);
    }
    if (mIsStubStream) {
        LOG(ERROR) << __func__ << ": the stream is not connected";
        return ndk::ScopedAStatus::fromExceptionCode(EX_ILLEGAL_STATE);
    }
    return mStream->getVendorParameters(in_ids, _aidl_return);
}

ndk::ScopedAStatus StreamSwitcher::setVendorParameters(
        const std::vector<VendorParameter>& in_parameters, bool in_async) {
    if (mStream == nullptr) {
        LOG(ERROR) << __func__ << ": stream was closed";
        return ndk::ScopedAStatus::fromExceptionCode(EX_ILLEGAL_STATE);
    }
    if (mIsStubStream) {
        mMissedParameters.emplace_back(in_parameters, in_async);
        return ndk::ScopedAStatus::ok();
    }
    return mStream->setVendorParameters(in_parameters, in_async);
}

ndk::ScopedAStatus StreamSwitcher::addEffect(const std::shared_ptr<IEffect>& in_effect) {
    if (in_effect == nullptr) {
        LOG(DEBUG) << __func__ << ": null effect";
        return ndk::ScopedAStatus::fromExceptionCode(EX_ILLEGAL_ARGUMENT);
    }
    if (mStream == nullptr) {
        LOG(ERROR) << __func__ << ": stream was closed";
        return ndk::ScopedAStatus::fromExceptionCode(EX_ILLEGAL_STATE);
    }
    if (!mIsStubStream) {
        RETURN_STATUS_IF_ERROR(mStream->addEffect(in_effect));
    }
    mEffects.push_back(in_effect);
    return ndk::ScopedAStatus::ok();
}

ndk::ScopedAStatus StreamSwitcher::removeEffect(const std::shared_ptr<IEffect>& in_effect) {
    if (in_effect == nullptr) {
        LOG(DEBUG) << __func__ << ": null effect";
        return ndk::ScopedAStatus::fromExceptionCode(EX_ILLEGAL_ARGUMENT);
    }
    if (mStream == nullptr) {
        LOG(ERROR) << __func__ << ": stream was closed";
        return ndk::ScopedAStatus::fromExceptionCode(EX_ILLEGAL_STATE);
    }
    for (auto it = mEffects.begin(); it != mEffects.end(); ++it) {
        if ((*it)->asBinder() == in_effect->asBinder()) {
            mEffects.erase(it);
            break;
        }
    }
    return !mIsStubStream ? mStream->removeEffect(in_effect) : ndk::ScopedAStatus::ok();
}

ndk::ScopedAStatus StreamSwitcher::getStreamCommonCommon(
        std::shared_ptr<IStreamCommon>* _aidl_return) {
    if (!mCommon) {
        LOG(FATAL) << __func__ << ": the common interface was not created";
    }
    *_aidl_return = mCommon.getInstance();
    LOG(DEBUG) << __func__ << ": returning " << _aidl_return->get()->asBinder().get();
    return ndk::ScopedAStatus::ok();
}

ndk::ScopedAStatus StreamSwitcher::updateMetadataCommon(const Metadata& metadata) {
    if (mStream == nullptr) {
        LOG(ERROR) << __func__ << ": stream was closed";
        return ndk::ScopedAStatus::fromExceptionCode(EX_ILLEGAL_STATE);
    }
    mMetadata = metadata;
    return !mIsStubStream ? mStream->updateMetadataCommon(metadata) : ndk::ScopedAStatus::ok();
}

ndk::ScopedAStatus StreamSwitcher::initInstance(
        const std::shared_ptr<StreamCommonInterface>& delegate) {
    mCommon = ndk::SharedRefBase::make<StreamCommonDelegator>(delegate);
    // The delegate is null because StreamSwitcher handles IStreamCommon methods by itself.
    return mStream->initInstance(nullptr);
}

const StreamContext& StreamSwitcher::getContext() const {
    return *mContext;
}

bool StreamSwitcher::isClosed() const {
    return mStream == nullptr || mStream->isClosed();
}

const StreamCommonInterface::ConnectedDevices& StreamSwitcher::getConnectedDevices() const {
    return mStream->getConnectedDevices();
}

ndk::ScopedAStatus StreamSwitcher::setConnectedDevices(const std::vector<AudioDevice>& devices) {
    LOG(DEBUG) << __func__ << ": " << ::android::internal::ToString(devices);
    if (mStream->getConnectedDevices() == devices) return ndk::ScopedAStatus::ok();
    const DeviceSwitchBehavior behavior = switchCurrentStream(devices);
    if (behavior == DeviceSwitchBehavior::UNSUPPORTED_DEVICES) {
        return ndk::ScopedAStatus::fromExceptionCode(EX_UNSUPPORTED_OPERATION);
    } else if (behavior == DeviceSwitchBehavior::SWITCH_TO_STUB_STREAM && !devices.empty()) {
        // This is an error in the extending class.
        LOG(FATAL) << __func__
                   << ": switching to stub stream with connected devices is not allowed";
    }
    if (behavior == USE_CURRENT_STREAM) {
        mIsStubStream = false;
    } else {
        LOG(DEBUG) << __func__ << ": connected devices changed, switching stream";
        // Two streams can't be opened for the same context, thus we always need to close
        // the current one before creating a new one.
        RETURN_STATUS_IF_ERROR(closeCurrentStream(true /*validateStreamState*/));
        if (behavior == CREATE_NEW_STREAM) {
            mStream = createNewStream(devices, mContext, mMetadata);
            mIsStubStream = false;
        } else {  // SWITCH_TO_STUB_STREAM
            mStream.reset(new InnerStreamWrapper<StreamStub>(mContext, mMetadata));
            mIsStubStream = true;
        }
        // The delegate is null because StreamSwitcher handles IStreamCommon methods by itself.
        if (ndk::ScopedAStatus status = mStream->initInstance(nullptr); !status.isOk()) {
            if (mIsStubStream) {
                LOG(FATAL) << __func__
                           << ": failed to initialize stub stream: " << status.getDescription();
            }
            // Need to close the current failed stream, and report an error.
            // Since we can't operate without a stream implementation, put a stub in.
            RETURN_STATUS_IF_ERROR(closeCurrentStream(false /*validateStreamState*/));
            mStream.reset(new InnerStreamWrapper<StreamStub>(mContext, mMetadata));
            (void)mStream->initInstance(nullptr);
            (void)mStream->setConnectedDevices(devices);
            return status;
        }
    }
    RETURN_STATUS_IF_ERROR(mStream->setConnectedDevices(devices));
    if (behavior == CREATE_NEW_STREAM) {
        // These updates are less critical, only log warning on failure.
        if (mHwAvSyncId.has_value()) {
            if (auto status = mStream->updateHwAvSyncId(*mHwAvSyncId); !status.isOk()) {
                LOG(WARNING) << __func__ << ": could not update HW AV Sync for a new stream: "
                             << status.getDescription();
            }
        }
        for (const auto& vndParam : mMissedParameters) {
            if (auto status = mStream->setVendorParameters(vndParam.first, vndParam.second);
                !status.isOk()) {
                LOG(WARNING) << __func__ << ": error while setting parameters for a new stream: "
                             << status.getDescription();
            }
        }
        mMissedParameters.clear();
        for (const auto& effect : mEffects) {
            if (auto status = mStream->addEffect(effect); !status.isOk()) {
                LOG(WARNING) << __func__ << ": error while adding effect for a new stream: "
                             << status.getDescription();
            }
        }
        if (mBluetoothParametersUpdated) {
            if (auto status = mStream->bluetoothParametersUpdated(); !status.isOk()) {
                LOG(WARNING) << __func__
                             << ": error while updating BT parameters for a new stream: "
                             << status.getDescription();
            }
        }
        mBluetoothParametersUpdated = false;
    }
    return ndk::ScopedAStatus::ok();
}

ndk::ScopedAStatus StreamSwitcher::bluetoothParametersUpdated() {
    if (mStream == nullptr) {
        LOG(ERROR) << __func__ << ": stream was closed";
        return ndk::ScopedAStatus::fromExceptionCode(EX_ILLEGAL_STATE);
    }
    if (mIsStubStream) {
        mBluetoothParametersUpdated = true;
        return ndk::ScopedAStatus::ok();
    }
    return mStream->bluetoothParametersUpdated();
}

}  // namespace aidl::android::hardware::audio::core
