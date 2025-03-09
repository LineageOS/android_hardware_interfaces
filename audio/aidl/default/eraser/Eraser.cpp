/*
 * Copyright (C) 2024 The Android Open Source Project
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

#define LOG_TAG "AHAL_Eraser"

#include "Eraser.h"

#include <android-base/logging.h>
#include <system/audio_effects/effect_uuid.h>

#include <optional>

using aidl::android::hardware::audio::common::getChannelCount;
using aidl::android::hardware::audio::effect::Descriptor;
using aidl::android::hardware::audio::effect::EraserSw;
using aidl::android::hardware::audio::effect::getEffectImplUuidEraserSw;
using aidl::android::hardware::audio::effect::getEffectTypeUuidEraser;
using aidl::android::hardware::audio::effect::IEffect;
using aidl::android::hardware::audio::effect::State;
using aidl::android::media::audio::common::AudioChannelLayout;
using aidl::android::media::audio::common::AudioUuid;

extern "C" binder_exception_t createEffect(const AudioUuid* in_impl_uuid,
                                           std::shared_ptr<IEffect>* instanceSpp) {
    if (!in_impl_uuid || *in_impl_uuid != getEffectImplUuidEraserSw()) {
        LOG(ERROR) << __func__ << "uuid not supported";
        return EX_ILLEGAL_ARGUMENT;
    }
    if (!instanceSpp) {
        LOG(ERROR) << __func__ << " invalid input parameter!";
        return EX_ILLEGAL_ARGUMENT;
    }

    *instanceSpp = ndk::SharedRefBase::make<EraserSw>();
    LOG(DEBUG) << __func__ << " instance " << instanceSpp->get() << " created";
    return EX_NONE;
}

extern "C" binder_exception_t queryEffect(const AudioUuid* in_impl_uuid, Descriptor* _aidl_return) {
    if (!in_impl_uuid || *in_impl_uuid != getEffectImplUuidEraserSw()) {
        LOG(ERROR) << __func__ << "uuid not supported";
        return EX_ILLEGAL_ARGUMENT;
    }
    *_aidl_return = EraserSw::kDescriptor;
    return EX_NONE;
}

namespace aidl::android::hardware::audio::effect {

const std::string EraserSw::kEffectName = "EraserSw";
const Descriptor EraserSw::kDescriptor = {
        .common = {.id = {.type = getEffectTypeUuidEraser(), .uuid = getEffectImplUuidEraserSw()},
                   .flags = {.type = Flags::Type::INSERT,
                             .insert = Flags::Insert::FIRST,
                             .hwAcceleratorMode = Flags::HardwareAccelerator::NONE},
                   .name = EraserSw::kEffectName,
                   .implementor = "The Android Open Source Project"}};

ndk::ScopedAStatus EraserSw::getDescriptor(Descriptor* _aidl_return) {
    LOG(DEBUG) << __func__ << kDescriptor.toString();
    *_aidl_return = kDescriptor;
    return ndk::ScopedAStatus::ok();
}

ndk::ScopedAStatus EraserSw::setParameterSpecific(const Parameter::Specific& specific) {
    RETURN_IF(Parameter::Specific::eraser != specific.getTag(), EX_ILLEGAL_ARGUMENT,
              "EffectNotSupported");
    RETURN_IF(!mContext, EX_NULL_POINTER, "nullContext");

    auto& param = specific.get<Parameter::Specific::eraser>();
    return mContext->setParam(param.getTag(), param);
}

ndk::ScopedAStatus EraserSw::getParameterSpecific(const Parameter::Id& id,
                                                  Parameter::Specific* specific) {
    RETURN_IF(!mContext, EX_NULL_POINTER, "nullContext");

    auto tag = id.getTag();
    RETURN_IF(Parameter::Id::eraserTag != tag, EX_ILLEGAL_ARGUMENT, "wrongIdTag");
    auto eraserId = id.get<Parameter::Id::eraserTag>();
    auto eraserTag = eraserId.getTag();
    switch (eraserTag) {
        case Eraser::Id::commonTag: {
            auto specificTag = eraserId.get<Eraser::Id::commonTag>();
            std::optional<Eraser> param = mContext->getParam(specificTag);
            if (!param.has_value()) {
                return ndk::ScopedAStatus::fromExceptionCodeWithMessage(EX_ILLEGAL_ARGUMENT,
                                                                        "EraserTagNotSupported");
            }
            specific->set<Parameter::Specific::eraser>(param.value());
            break;
        }
        default: {
            LOG(ERROR) << __func__ << " unsupported tag: " << toString(tag);
            return ndk::ScopedAStatus::fromExceptionCodeWithMessage(EX_ILLEGAL_ARGUMENT,
                                                                    "EraserTagNotSupported");
        }
    }
    return ndk::ScopedAStatus::ok();
}

std::shared_ptr<EffectContext> EraserSw::createContext(const Parameter::Common& common) {
    if (mContext) {
        LOG(DEBUG) << __func__ << " context already exist";
    } else {
        mContext = std::make_shared<EraserSwContext>(1 /* statusFmqDepth */, common);
    }
    return mContext;
}

RetCode EraserSw::releaseContext() {
    if (mContext) {
        mContext.reset();
    }
    return RetCode::SUCCESS;
}

EraserSw::~EraserSw() {
    cleanUp();
    LOG(DEBUG) << __func__;
}

ndk::ScopedAStatus EraserSw::command(CommandId command) {
    std::lock_guard lg(mImplMutex);
    RETURN_IF(mState == State::INIT, EX_ILLEGAL_STATE, "instanceNotOpen");

    switch (command) {
        case CommandId::START:
            RETURN_OK_IF(mState == State::PROCESSING);
            mState = State::PROCESSING;
            mContext->enable();
            startThread();
            RETURN_IF(notifyEventFlag(mDataMqNotEmptyEf) != RetCode::SUCCESS, EX_ILLEGAL_STATE,
                      "notifyEventFlagNotEmptyFailed");
            break;
        case CommandId::STOP:
            RETURN_OK_IF(mState == State::IDLE || mState == State::DRAINING);
            if (mVersion < kDrainSupportedVersion) {
                mState = State::IDLE;
                stopThread();
                mContext->disable();
            } else {
                mState = State::DRAINING;
                startDraining();
                mContext->startDraining();
            }
            RETURN_IF(notifyEventFlag(mDataMqNotEmptyEf) != RetCode::SUCCESS, EX_ILLEGAL_STATE,
                      "notifyEventFlagNotEmptyFailed");
            break;
        case CommandId::RESET:
            mState = State::IDLE;
            RETURN_IF(notifyEventFlag(mDataMqNotEmptyEf) != RetCode::SUCCESS, EX_ILLEGAL_STATE,
                      "notifyEventFlagNotEmptyFailed");
            stopThread();
            mImplContext->disable();
            mImplContext->reset();
            mImplContext->resetBuffer();
            break;
        default:
            LOG(ERROR) << getEffectNameWithVersion() << __func__ << " instance still processing";
            return ndk::ScopedAStatus::fromExceptionCodeWithMessage(EX_ILLEGAL_ARGUMENT,
                                                                    "CommandIdNotSupported");
    }
    LOG(VERBOSE) << getEffectNameWithVersion() << __func__
                 << " transfer to state: " << toString(mState);
    return ndk::ScopedAStatus::ok();
}

// Processing method running in EffectWorker thread.
IEffect::Status EraserSw::effectProcessImpl(float* in, float* out, int samples) {
    RETURN_VALUE_IF(!mContext, (IEffect::Status{EX_NULL_POINTER, 0, 0}), "nullContext");
    IEffect::Status procStatus{STATUS_NOT_ENOUGH_DATA, 0, 0};
    procStatus = mContext->process(in, out, samples);
    if (mState == State::DRAINING && procStatus.status == STATUS_NOT_ENOUGH_DATA) {
        drainingComplete_l();
    }

    return procStatus;
}

void EraserSw::drainingComplete_l() {
    if (mState != State::DRAINING) return;

    LOG(DEBUG) << getEffectNameWithVersion() << __func__;
    finishDraining();
    mState = State::IDLE;
}

EraserSwContext::EraserSwContext(int statusDepth, const Parameter::Common& common)
    : EffectContext(statusDepth, common) {
    LOG(DEBUG) << __func__;
}

EraserSwContext::~EraserSwContext() {
    LOG(DEBUG) << __func__;
}

template <typename TAG>
std::optional<Eraser> EraserSwContext::getParam(TAG tag) {
    if (mParamsMap.find(tag) != mParamsMap.end()) {
        return mParamsMap.at(tag);
    }
    return std::nullopt;
}

template <typename TAG>
ndk::ScopedAStatus EraserSwContext::setParam(TAG tag, Eraser eraser) {
    mParamsMap[tag] = eraser;
    return ndk::ScopedAStatus::ok();
}

IEffect::Status EraserSwContext::process(float* in, float* out, int samples) {
    LOG(DEBUG) << __func__ << " in " << in << " out " << out << " samples " << samples;
    IEffect::Status procStatus = {EX_ILLEGAL_ARGUMENT, 0, 0};
    const auto inputChannelCount = getChannelCount(mCommon.input.base.channelMask);
    const auto outputChannelCount = getChannelCount(mCommon.output.base.channelMask);
    if (inputChannelCount < outputChannelCount) {
        LOG(ERROR) << __func__ << " invalid channel count, in: " << inputChannelCount
                   << " out: " << outputChannelCount;
        return procStatus;
    }

    if (samples <= 0 || 0 != samples % inputChannelCount) {
        LOG(ERROR) << __func__ << " invalid samples: " << samples;
        return procStatus;
    }

    const int iFrames = samples / inputChannelCount;
    const float gainPerSample = 1.f / iFrames;
    for (int i = 0; i < iFrames; i++) {
        if (isDraining()) {
            const float gain = (iFrames - i - 1) * gainPerSample;
            for (size_t c = 0; c < outputChannelCount; c++) {
                out[c] = in[c] * gain;
            }
        } else {
            std::memcpy(out, in, outputChannelCount * sizeof(float));
        }

        in += inputChannelCount;
        out += outputChannelCount;
    }

    // drain for one cycle
    if (isDraining()) {
        procStatus.status = STATUS_NOT_ENOUGH_DATA;
        finishDraining();
    } else {
        procStatus.status = STATUS_OK;
    }
    procStatus.fmqConsumed = static_cast<int32_t>(iFrames * inputChannelCount);
    procStatus.fmqProduced = static_cast<int32_t>(iFrames * outputChannelCount);

    return procStatus;
}

}  // namespace aidl::android::hardware::audio::effect
