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

#define LOG_TAG "AHAL_EffectImpl"
#include "effect-impl/EffectImpl.h"
#include "effect-impl/EffectTypes.h"
#include "include/effect-impl/EffectTypes.h"

using aidl::android::hardware::audio::effect::IEffect;
using aidl::android::hardware::audio::effect::State;
using aidl::android::media::audio::common::PcmType;

extern "C" binder_exception_t destroyEffect(const std::shared_ptr<IEffect>& instanceSp) {
    State state;
    ndk::ScopedAStatus status = instanceSp->getState(&state);
    if (!status.isOk() || State::INIT != state) {
        LOG(ERROR) << __func__ << " instance " << instanceSp.get()
                   << " in state: " << toString(state) << ", status: " << status.getDescription();
        return EX_ILLEGAL_STATE;
    }
    LOG(DEBUG) << __func__ << " instance " << instanceSp.get() << " destroyed";
    return EX_NONE;
}

namespace aidl::android::hardware::audio::effect {

ndk::ScopedAStatus EffectImpl::open(const Parameter::Common& common,
                                    const std::optional<Parameter::Specific>& specific,
                                    OpenEffectReturn* ret) {
    LOG(DEBUG) << getEffectName() << __func__;
    // effect only support 32bits float
    RETURN_IF(common.input.base.format.pcm != common.output.base.format.pcm ||
                      common.input.base.format.pcm != PcmType::FLOAT_32_BIT,
              EX_ILLEGAL_ARGUMENT, "dataMustBe32BitsFloat");
    RETURN_OK_IF(mState != State::INIT);
    auto context = createContext(common);
    RETURN_IF(!context, EX_NULL_POINTER, "createContextFailed");

    if (specific.has_value()) {
        RETURN_IF_ASTATUS_NOT_OK(setParameterSpecific(specific.value()), "setSpecParamErr");
    }

    mState = State::IDLE;
    context->dupeFmq(ret);
    RETURN_IF(createThread(context, getEffectName()) != RetCode::SUCCESS, EX_UNSUPPORTED_OPERATION,
              "FailedToCreateWorker");
    return ndk::ScopedAStatus::ok();
}

ndk::ScopedAStatus EffectImpl::close() {
    RETURN_OK_IF(mState == State::INIT);
    RETURN_IF(mState == State::PROCESSING, EX_ILLEGAL_STATE, "closeAtProcessing");

    // stop the worker thread, ignore the return code
    RETURN_IF(destroyThread() != RetCode::SUCCESS, EX_UNSUPPORTED_OPERATION,
              "FailedToDestroyWorker");
    mState = State::INIT;
    RETURN_IF(releaseContext() != RetCode::SUCCESS, EX_UNSUPPORTED_OPERATION,
              "FailedToCreateWorker");

    LOG(DEBUG) << getEffectName() << __func__;
    return ndk::ScopedAStatus::ok();
}

ndk::ScopedAStatus EffectImpl::setParameter(const Parameter& param) {
    LOG(VERBOSE) << getEffectName() << __func__ << " with: " << param.toString();

    const auto tag = param.getTag();
    switch (tag) {
        case Parameter::common:
        case Parameter::deviceDescription:
        case Parameter::mode:
        case Parameter::source:
            FALLTHROUGH_INTENDED;
        case Parameter::volumeStereo:
            return setParameterCommon(param);
        case Parameter::specific: {
            return setParameterSpecific(param.get<Parameter::specific>());
        }
        default: {
            LOG(ERROR) << getEffectName() << __func__ << " unsupportedParameterTag "
                       << toString(tag);
            return ndk::ScopedAStatus::fromExceptionCodeWithMessage(EX_ILLEGAL_ARGUMENT,
                                                                    "ParameterNotSupported");
        }
    }
}

ndk::ScopedAStatus EffectImpl::getParameter(const Parameter::Id& id, Parameter* param) {
    auto tag = id.getTag();
    switch (tag) {
        case Parameter::Id::commonTag: {
            RETURN_IF_ASTATUS_NOT_OK(getParameterCommon(id.get<Parameter::Id::commonTag>(), param),
                                     "CommonParamNotSupported");
            break;
        }
        case Parameter::Id::vendorEffectTag:
            FALLTHROUGH_INTENDED;
        default: {
            Parameter::Specific specific;
            RETURN_IF_ASTATUS_NOT_OK(getParameterSpecific(id, &specific), "SpecParamNotSupported");
            param->set<Parameter::specific>(specific);
            break;
        }
    }
    LOG(VERBOSE) << getEffectName() << __func__ << id.toString() << param->toString();
    return ndk::ScopedAStatus::ok();
}

ndk::ScopedAStatus EffectImpl::setParameterCommon(const Parameter& param) {
    auto context = getContext();
    RETURN_IF(!context, EX_NULL_POINTER, "nullContext");

    auto tag = param.getTag();
    switch (tag) {
        case Parameter::common:
            RETURN_IF(context->setCommon(param.get<Parameter::common>()) != RetCode::SUCCESS,
                      EX_ILLEGAL_ARGUMENT, "setCommFailed");
            break;
        case Parameter::deviceDescription:
            RETURN_IF(context->setOutputDevice(param.get<Parameter::deviceDescription>()) !=
                              RetCode::SUCCESS,
                      EX_ILLEGAL_ARGUMENT, "setDeviceFailed");
            break;
        case Parameter::mode:
            RETURN_IF(context->setAudioMode(param.get<Parameter::mode>()) != RetCode::SUCCESS,
                      EX_ILLEGAL_ARGUMENT, "setModeFailed");
            break;
        case Parameter::source:
            RETURN_IF(context->setAudioSource(param.get<Parameter::source>()) != RetCode::SUCCESS,
                      EX_ILLEGAL_ARGUMENT, "setSourceFailed");
            break;
        case Parameter::volumeStereo:
            RETURN_IF(context->setVolumeStereo(param.get<Parameter::volumeStereo>()) !=
                              RetCode::SUCCESS,
                      EX_ILLEGAL_ARGUMENT, "setVolumeStereoFailed");
            break;
        default: {
            LOG(ERROR) << getEffectName() << __func__ << " unsupportedParameterTag "
                       << toString(tag);
            return ndk::ScopedAStatus::fromExceptionCodeWithMessage(EX_ILLEGAL_ARGUMENT,
                                                                    "commonParamNotSupported");
        }
    }
    return ndk::ScopedAStatus::ok();
}

ndk::ScopedAStatus EffectImpl::getParameterCommon(const Parameter::Tag& tag, Parameter* param) {
    auto context = getContext();
    RETURN_IF(!context, EX_NULL_POINTER, "nullContext");

    switch (tag) {
        case Parameter::common: {
            param->set<Parameter::common>(context->getCommon());
            break;
        }
        case Parameter::deviceDescription: {
            param->set<Parameter::deviceDescription>(context->getOutputDevice());
            break;
        }
        case Parameter::mode: {
            param->set<Parameter::mode>(context->getAudioMode());
            break;
        }
        case Parameter::source: {
            param->set<Parameter::source>(context->getAudioSource());
            break;
        }
        case Parameter::volumeStereo: {
            param->set<Parameter::volumeStereo>(context->getVolumeStereo());
            break;
        }
        default: {
            LOG(DEBUG) << getEffectName() << __func__ << " unsupported tag " << toString(tag);
            return ndk::ScopedAStatus::fromExceptionCodeWithMessage(EX_ILLEGAL_ARGUMENT,
                                                                    "tagNotSupported");
        }
    }
    return ndk::ScopedAStatus::ok();
}

ndk::ScopedAStatus EffectImpl::getState(State* state) {
    *state = mState;
    return ndk::ScopedAStatus::ok();
}

ndk::ScopedAStatus EffectImpl::command(CommandId command) {
    RETURN_IF(mState == State::INIT, EX_ILLEGAL_STATE, "CommandStateError");
    LOG(DEBUG) << getEffectName() << __func__ << ": receive command: " << toString(command)
               << " at state " << toString(mState);

    switch (command) {
        case CommandId::START:
            RETURN_IF(mState == State::INIT, EX_ILLEGAL_STATE, "instanceNotOpen");
            RETURN_OK_IF(mState == State::PROCESSING);
            RETURN_IF_ASTATUS_NOT_OK(commandImpl(command), "commandImplFailed");
            startThread();
            mState = State::PROCESSING;
            break;
        case CommandId::STOP:
        case CommandId::RESET:
            RETURN_OK_IF(mState == State::IDLE);
            stopThread();
            RETURN_IF_ASTATUS_NOT_OK(commandImpl(command), "commandImplFailed");
            mState = State::IDLE;
            break;
        default:
            LOG(ERROR) << getEffectName() << __func__ << " instance still processing";
            return ndk::ScopedAStatus::fromExceptionCodeWithMessage(EX_ILLEGAL_ARGUMENT,
                                                                    "CommandIdNotSupported");
    }
    LOG(DEBUG) << getEffectName() << __func__ << " transfer to state: " << toString(mState);
    return ndk::ScopedAStatus::ok();
}

ndk::ScopedAStatus EffectImpl::commandImpl(CommandId command) {
    auto context = getContext();
    RETURN_IF(!context, EX_NULL_POINTER, "nullContext");
    if (command == CommandId::RESET) {
        context->resetBuffer();
    }
    return ndk::ScopedAStatus::ok();
}

void EffectImpl::cleanUp() {
    command(CommandId::STOP);
    close();
}

IEffect::Status EffectImpl::status(binder_status_t status, size_t consumed, size_t produced) {
    IEffect::Status ret;
    ret.status = status;
    ret.fmqConsumed = consumed;
    ret.fmqProduced = produced;
    return ret;
}

// A placeholder processing implementation to copy samples from input to output
IEffect::Status EffectImpl::effectProcessImpl(float* in, float* out, int samples) {
    for (int i = 0; i < samples; i++) {
        *out++ = *in++;
    }
    LOG(VERBOSE) << getEffectName() << __func__ << " done processing " << samples << " samples";
    return {STATUS_OK, samples, samples};
}

}  // namespace aidl::android::hardware::audio::effect
